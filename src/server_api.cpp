#include "../include/Node.h"
#include "../include/coordination.h"
#include "crow.h"          // Include Crow’s single header
#include <thread>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace std;

// For logging, we use simple macros
#define LOG_INFO(msg)  cout << "[INFO] " << msg << endl
#define LOG_ERROR(msg) cerr << "[ERROR] " << msg << endl

const int numFiles = 5;
const int maxRead = 5;
const int portOffset = 2048; // Offset for API port numbers

// Function prototypes
void wait(int semid);
void signal(int semid);

void initFileLocks(int port, int numFiles, int maxRead);
void acquireRead(int fileIdx);
void releaseRead(int fileIdx);
void acquireWrite(int fileIdx);
void releaseWrite(int fileIdx);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Recursive helper function to convert nlohmann::json to crow::json::wvalue.
crow::json::wvalue convertToWvalue(const json& j) {
    crow::json::wvalue w;
    if(j.is_object()){
        // For JSON objects, iterate over all keys.
        for (auto it = j.begin(); it != j.end(); ++it) {
            w[it.key()] = convertToWvalue(it.value());
        }
    } else if(j.is_array()){
        // For JSON arrays, build a wvalue list.
        crow::json::wvalue array = crow::json::wvalue::list();
        int idx = 0;
        for (const auto& element : j) {
            array[idx++] = convertToWvalue(element);
        }
        w = std::move(array);
    } else if(j.is_string()){
        // For string values.
        w = j.get<std::string>();
    } else if(j.is_boolean()){
        w = j.get<bool>();
    } else if(j.is_number_integer()){
        w = j.get<int>();
    } else if(j.is_number_float()){
        w = j.get<double>();
    } else if(j.is_null()){
        // You can choose to assign an empty wvalue or some explicit null.
        w = crow::json::wvalue();
    }
    return w;
}

int createOrGetSemaphore(key_t key, int initialValue) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    
    if (semid != -1) {
        semun arg;
        arg.val = initialValue;
        semctl(semid, 0, SETVAL, arg);
        cout << "Setting semaphore value\n";
    } else {
        if (errno == EEXIST) {
            cout << "Already existing\n";
            semid = semget(key, 1, 0666);
        } else {
            // Handle other errors appropriately
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }

    return semid;
}

void initFileLocks(int port, int maxRead, std::vector<int> &readSemaphores, std::vector<int> &writeSemaphores) {
    for (int i = 0; i < numFiles; i++) {
        key_t readKey = ftok("/tmp", 100 + port * 10 + i);
        key_t writeKey = ftok("/tmp", 200 + port * 10 + i);

        int readSem = createOrGetSemaphore(readKey, maxRead);
        int writeSem = createOrGetSemaphore(writeKey, 1);

        readSemaphores.push_back(readSem);
        writeSemaphores.push_back(writeSem);

        std::cout << "[Semaphore] File " << i << " — ReadSem: " << readSem << ", WriteSem: " << writeSem << std::endl;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <IP> <Port>" << endl;
        return EXIT_FAILURE;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);

    Node serverNode(ip, port, false);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    key_t key = ftok("/tmp", 1);
    int semmtx = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semmtx != -1) {
        // This server successfully created the semaphore
        // Safe to initialize it here
        semctl(semmtx, 0, SETVAL, 1);
        cout << "Setting semaphore value\n";
    } else {
        if (errno == EEXIST) {
            cout << "Already existing\n";
            // The semaphore set already exists
            // Get the semaphore ID without creating it
            semmtx = semget(key, 1, 0666);
        } else {
            // Handle other errors appropriately
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }

    int semlocal = semget(ftok("/tmp", static_cast<int>(getpid() % 256)), 1, 0666 | IPC_CREAT);

    if (semmtx == -1 || semlocal == -1) {
        perror("semget");
        return 0;
    }

    semctl(semlocal, 0, SETVAL, 1);

    // create maxRead mutex for each file and one write mutex for each file in this port
    std::vector<int> readSemaphores;
    std::vector<int> writeSemaphores;
    initFileLocks(port, maxRead, readSemaphores, writeSemaphores);

    serverNode.loadFromJson();
    LOG_INFO("Server node loaded from JSON state.");

    serverNode.port = port;
    serverNode.ip = ip;
    serverNode.isLeader = false;
    serverNode.role = 0;

    serverNode.saveToJson();

    thread raftThread(assignType, ref(serverNode));
    raftThread.detach();
    LOG_INFO("Raft engine (leader election and replication) launched.");

    // Instead of the old TCP loop with fork, we now start an HTTP server to interact with clients.
    // We use Crow as an example HTTP framework.
    crow::SimpleApp app;

    // POST /append endpoint:
    // This endpoint accepts a JSON payload with a "command" field.
    // If this node is the leader, it appends the command to its log.
    // Otherwise, it returns an error along with the current leader's information.
    CROW_ROUTE(app, "/append").methods("POST"_method)([&](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            LOG_ERROR("Invalid JSON received at /append endpoint");
            return crow::response(400, "Invalid JSON");
        }
        string command = body["command"].s();
        LOG_INFO("API /append received command: " << command);

        wait(semlocal);
        serverNode.loadFromJson();
        if (!serverNode.isLeader) {
            crow::json::wvalue res;
            res["error"] = "Not the leader";
            res["leaderIp"] = serverNode.leaderIp;
            res["leaderPort"] = serverNode.leaderPort;
            LOG_INFO("API /append rejected: not the leader. Current leader: " << serverNode.leaderIp 
                << ":" << serverNode.leaderPort);
            signal(semlocal);
            return crow::response(503, res);
        }
        signal(semlocal);

        // Call your existing append handler (wrapped in a helper)
        // This function internally calls handleAppendEntries() which appends the command to your leader's log.
        // (Note: In your code, handleAppendEntries() is meant for client TCP requests.
        // Here we simulate its use by building a command message.)
        char msg[1024];
        memset(msg, 0, sizeof(msg));
        strcpy(msg, "REQUEST ");
        strcat(msg, command.c_str());
        
        json res;

        thread hAPIApEn(handleAPIAppendEntries, ref(serverNode), ref(msg), ref(res));
        hAPIApEn.join();

        // Check the response from handleAPIAppendEntries
        if (res["status"] == "success") {
            LOG_INFO("API /append success: " << res["message"]);
        } else {
            LOG_ERROR("API /append failed: " << res["message"]);
        }
        
        // Convert the JSON response to a crow::json::wvalue
        return crow::response{convertToWvalue(res)};
    });

    // GET /status endpoint:
    // Returns the server node's state, including IP, port, term, role, log entries, commitIndex, etc.
    CROW_ROUTE(app, "/status")([&]() {
        crow::json::wvalue res;
        wait(semlocal);
        serverNode.loadFromJson();
        res["ip"] = serverNode.ip;
        res["port"] = serverNode.port;
        res["term"] = serverNode.termNumber;
        res["isLeader"] = serverNode.isLeader;
        res["role"] = serverNode.role;
        res["leaderIp"] = serverNode.leaderIp;
        res["leaderPort"] = serverNode.leaderPort;

        // Create a list-type wvalue for the log entries.
        crow::json::wvalue logEntries = crow::json::wvalue::list();
        size_t idx = 0;
        for (auto &entry : serverNode.log) {
            // Construct a wvalue for each log entry.
            logEntries[idx++] = crow::json::wvalue{{"term", entry.term}, {"command", entry.command}};
        }
        // Use move semantics to assign the log list.
        res["log"] = std::move(logEntries);
        res["commitIndex"] = serverNode.commitIndex;
        res["lastApplied"] = serverNode.lastApplied;
        res["totalNodes"] = serverNode.totalNodes;
        res["votedFor"] = serverNode.votedFor;
        signal(semlocal);
        return crow::response{res};
    });

    // GET /leader endpoint:
    // Returns the current leader's IP and port.
    CROW_ROUTE(app, "/leader")([&]() {
        crow::json::wvalue res;
        wait(semlocal);
        serverNode.loadFromJson();
        res["leaderIp"] = serverNode.leaderIp;
        res["leaderPort"] = serverNode.leaderPort;
        res["leaderHTTPPort"] = serverNode.leaderPort + portOffset; // Adjust for the server's port offset.
        if (serverNode.isLeader) {
            res["isLeader"] = true;
        } else {
            res["isLeader"] = false;
        }
        signal(semlocal);
        return crow::response{res};
    });

    // Additional endpoints (for example, to trigger reconfiguration or health checks) can be added here.
    int apiPort = port + portOffset; // Example: API port is portOffset more than the server port.
    app.port(apiPort).multithreaded().run();
    LOG_INFO("HTTP API server started on port " << apiPort);
    // The server will run indefinitely, handling requests as they come in.
    // The main thread will block here until the server is stopped.

    return 0;
}
