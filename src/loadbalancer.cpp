#include "../include/Node.h"
#include "../include/loadbalancer.h"
#include "../include/coordination.h"
#include "crow.h"          // Include Crow’s single header

using namespace std;

// For logging, we use simple macros
#define LOG_INFO(msg)  cout << "[INFO] " << msg << endl
#define LOG_ERROR(msg) cerr << "[ERROR] " << msg << endl

int portOffset = 2048; // Offset for API port numbers

void wait(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void signal(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void receiveThreadFunction(LoadBalancer &loadBalancer) {
    char buffer[1024];
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    string ip, command;
    int port, healthyCount;

    while (true) {
        int n = recvfrom(loadBalancer.sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (n < 0) {
            cerr << "Receive failed" << endl;
            continue;
        }
        buffer[n] = '\0';
        cout << "Received message: " << buffer << endl;

        // Handle health check message
        stringstream ss(buffer);
        ss >> command; // Read the command

        if (strncmp(command.c_str(), "HEALTHCHECK", 12) == 0) {
            ss >> healthyCount;

            wait(loadBalancer.sem);
            cout << "Received health check from leader - # of Healthy nodes: " << healthyCount << endl;

            loadBalancer.leaderIp = inet_ntoa(clientAddr.sin_addr);
            loadBalancer.leaderPort = ntohs(clientAddr.sin_port);
            loadBalancer.health.clear(); // Clear previous health status

            cout << "Leader - " << loadBalancer.leaderIp << ":" << loadBalancer.leaderPort << endl;
            loadBalancer.health.insert({{loadBalancer.leaderIp, loadBalancer.leaderPort}});

            for (int i = 0; i < healthyCount; ++i) {
                ss >> ip >> port;
                cout << "Healthy node - " << ip << ":" << port << endl;
                loadBalancer.health.insert({{ip, port}});
            }

            cout << "Updated health status of nodes." << endl;
            signal(loadBalancer.sem);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <IP> <Port>" << endl;
        return EXIT_FAILURE;
    }

    // Initialize the load balancer
    string ip = argv[1];
    int port = stoi(argv[2]);

    LoadBalancer loadBalancer(ip, port);
    loadBalancer.init();

    // Get a unique key for the semaphore
    key_t localkey = ftok("/tmp", static_cast<int>(getpid() % 256));
    loadBalancer.sem = semget(localkey, 1, 0666 | IPC_CREAT);
    if (loadBalancer.sem == -1) {
        perror("semget");
        return EXIT_FAILURE;
    }

    // Initialize the semaphore
    semctl(loadBalancer.sem, 0, SETVAL, 1); // Initialize semaphore value to 1

    cout << "LoadBalancer listening on " << loadBalancer.ip << ":" << loadBalancer.port << endl;

    thread receiveThread([&]() { receiveThreadFunction(loadBalancer); });
    receiveThread.detach();

    // Initialize Crow for HTTP API
    crow::SimpleApp app;

    // GET /server endpoint:
    // Returns a random live server's IP and port.
    CROW_ROUTE(app, "/server")([&]() {
        crow::json::wvalue res;

        wait(loadBalancer.sem);
        cout << "Received request for server info." << endl;

        cout << "# of healthy servers: " << loadBalancer.health.size() << endl;

        if (loadBalancer.health.empty()) {
            res["error"] = "No healthy servers available.";
            signal(loadBalancer.sem);
            return crow::response{res};
        }
        
        auto randserver = loadBalancer.health.begin();
        int randIndex = rand() % loadBalancer.health.size();
        std::advance( randserver, randIndex );

        res["status"] = "success";
        res["ip"] = randserver->first;
        res["port"] = randserver->second;

        cout << "Returning server info: " << randserver->first << ":" << randserver->second << endl;

        signal(loadBalancer.sem);
        return crow::response{res};
    });

    // Additional endpoints (for example, to trigger reconfiguration or health checks) can be added here.
    int apiPort = port + portOffset; // Example: API port is portOffset more than the server port.
    app.port(apiPort).multithreaded().run();
    LOG_INFO("HTTP API server started on port " << apiPort);

    return 0;
}
