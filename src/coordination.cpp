#include "../include/coordination.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <set>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <ctime>
#include <algorithm>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// Global synchronization for heartbeat notifications.
mutex cv_mtx1, cv_mtx2, cv_mtx3;
condition_variable cv1, cv2, cv3;

// Global deltas
int deltaHeartBeat = 5; // seconds
int deltaRequestVote = 5; // seconds
int deltaElection = 10; // seconds

int noOfACKs = 0; // Number of ACKs received
int ACK_SEQ = 0; // Sequence number for ACKs

// Global election timeout.
int electionTimeout; // seconds
int sendAppendEntriesTimeout;

// --- Semaphore helper functions ---
int semmtx, semlocal;

int reply = 0;

const int numFiles = 5;
const int maxRead = 5;

std::vector<int> readSemaphores;
std::vector<int> writeSemaphores;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


void printSemaphoreStatus(int readSemId, int fileNo) {
    // Get the current value of the read semaphore
    int semValue = semctl(readSemId, 0, GETVAL);
    
    if (semValue == -1) {
        perror("semctl");
        std::cerr << "Failed to get the semaphore value for file " << fileNo << std::endl;
    } else {
        std::cout << "Current read semaphore value for file " << fileNo << ": " << semValue << std::endl;
    }
}


int createOrGetSemaphore_leader(key_t key, int initialValue) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        throw std::runtime_error("Failed to create/get semaphore");
    }

    semun arg;
    arg.val = initialValue;
    semctl(semid, 0, SETVAL, arg);

    return semid;
}

void initFileLocks(int port, int numFiles, int maxRead) {
    for (int i = 0; i < numFiles; i++) {
        key_t readKey = ftok("/tmp", 100 + port * 10 + i);
        key_t writeKey = ftok("/tmp", 200 + port * 10 + i);

        int readSem = createOrGetSemaphore_leader(readKey, maxRead);
        int writeSem = createOrGetSemaphore_leader(writeKey, 1);

        readSemaphores.push_back(readSem);
        writeSemaphores.push_back(writeSem);

        std::cout << "[Semaphore] File " << i << " — ReadSem: " << readSem << ", WriteSem: " << writeSem << std::endl;
    }
}

void acquireRead(int fileIdx) {
    struct sembuf op = {0, -1, 0};
    int ac = semop(readSemaphores[fileIdx], &op, 1);
    cout<<ac<<endl;
    printSemaphoreStatus(readSemaphores[fileIdx], 1);
}

void releaseRead(int fileIdx) {
    struct sembuf op = {0, 1, 0};
    semop(readSemaphores[fileIdx], &op, 1);
    printSemaphoreStatus(readSemaphores[fileIdx], 1);
}

void acquireWrite(int fileIdx) {
    struct sembuf op = {0, -1, 0};
    semop(writeSemaphores[fileIdx], &op, 1);
    printSemaphoreStatus(writeSemaphores[fileIdx], 1);
}

void releaseWrite(int fileIdx) {
    struct sembuf op = {0, 1, 0};
    semop(writeSemaphores[fileIdx], &op, 1);
    printSemaphoreStatus(writeSemaphores[fileIdx], 1);
}

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

void initLeaderLocks(Node& node, int numFiles, int maxRead) {
    // Initialize semaphores for each file on this node/port
    for (int i = 0; i < numFiles; i++) {
        // Generate unique semaphore keys for the current port and file index
        key_t readKey = ftok("/tmp", 100 + node.port * 10 + i);
        key_t writeKey = ftok("/tmp", 200 + node.port * 10 + i);

        // Create or get the semaphores for read and write
        int readSem = createOrGetSemaphore_leader(readKey, maxRead);
        int writeSem = createOrGetSemaphore_leader(writeKey, 1);

        // Store the semaphores for future use
        readSemaphores.push_back(readSem);
        writeSemaphores.push_back(writeSem);

        std::cout << "[Leader] Initialized semaphores for file " << i 
                  << " on port " << node.port 
                  << " — ReadSem: " << readSem 
                  << ", WriteSem: " << writeSem << std::endl;
    }
}

// --- Message Handlers ---

void breakCommand(const string &command, vector<string> &V){
    istringstream ss(command);
    string word;
    while(ss >> word){
        V.push_back(word);
    }
}

// Handles append requests from the client of the form : REQUEST <command>
void handleAppendEntries(Node &node, const struct sockaddr_in clientAddr, const char *msg, int sockfd) {
    cout<<"Received append request from Client with command: "<<msg<<endl;

    vector<string> V;
    breakCommand(msg, V);

    string requestType = V[1];
    string fullCommand = msg;
    fullCommand = fullCommand.substr(8);

    wait(semlocal);
    if(!node.isLeader) {
        signal(semlocal);
        return;
    }
    signal(semlocal);

    wait(semlocal);
    LogEntry entry;
    entry.term = node.termNumber;
    entry.command = fullCommand;
    node.log.push_back(entry);
    cout<<(entry.term)<<" "<<endl;
    node.saveToJson();


    int port = node.port;
    signal(semlocal);
    cv3.notify_all();

    string response;

    if (requestType == "CREATE") {
        wait(semlocal);
        node.fileNo++;
        node.saveToJson();
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + to_string(node.fileNo) + ".txt";


        ofstream file(filename);
        if (!file) {
            cerr << "Error creating file: " << filename << endl;
            return;
        }
        file.close();

        response = "CREATE REPLY ID " + to_string(node.fileNo);
        if (!response.empty()) {
            sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }
        signal(semlocal);
    } 
    else if (requestType == "WRITE" && V.size() >= 4) {
        string id = V[2];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";


        ofstream file(filename, ios::trunc);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        response = "WRITE REPLY ID " + id;
        cout << "Sending response: " << response << endl;
        if (!response.empty()) {
            sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);        
    } 
    else if (requestType == "READ" && V.size() >= 3) {
        string id = V[2];
        int filenum = stoi(id);
        acquireRead(filenum - 1);
        printSemaphoreStatus(readSemaphores[filenum - 1], filenum);
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ifstream file(filename);
        if (!file) {
            response = "ERROR: File not found";
        } else {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            response = content;
            file.close();
        }
        response = "READ REPLY " + response;
        cout << "Sending response: " << response << endl;
        
        if (!response.empty()) {
            sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }
        sleep(30);
        releaseRead(filenum - 1);
    } 
    else if (requestType == "APPEND" && V.size() >= 4) {
        string id = V[2];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ofstream file(filename, ios::app);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        response = "APPEND REPLY ID " + id;
        cout << "Sending response: " << response << endl;
        if (!response.empty()) {
            sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);
    }

    

    

}

// --- JSON API Handler ---
// Handles append requests from the client of the form : REQUEST <command>
void handleAPIAppendEntries(Node &node, const char *msg, json &responseJson) {
    cout<<"Received append request from Client with command: "<<msg<<endl;

    vector<string> V;
    breakCommand(msg, V);

    string requestType = V[1];
    string fullCommand = msg;
    fullCommand = fullCommand.substr(8);

    wait(semlocal);
    if(!node.isLeader) {
        signal(semlocal);
        responseJson["status"] = "error";
        responseJson["message"] = "Not the leader";
        return;
    }
    signal(semlocal);

    wait(semlocal);
    LogEntry entry;
    entry.term = node.termNumber;
    entry.command = fullCommand;
    node.log.push_back(entry);
    cout<<(entry.term)<<" "<<endl;
    node.saveToJson();

    int port = node.port;
    signal(semlocal);
    cv3.notify_all();

    string folder = "../serverfiles";  // Folder name
    string subfolder = folder + "/" + to_string(port); // Subfolder name

    // Create the folder if it doesn't exist
    if(!filesystem::exists(folder)){
        filesystem::create_directory(folder);
    }
    // Create the subfolder if it doesn't exist
    if(!filesystem::exists(subfolder)){
        filesystem::create_directory(subfolder);
    }

    if (requestType == "CREATE") {
        wait(semlocal);
        node.fileNo++;
        node.saveToJson();
        
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + to_string(node.fileNo) + ".txt";

        ofstream file(filename);
        if (!file) {
            cerr << "Error creating file: " << filename << endl;
            responseJson["status"] = "error";
            responseJson["message"] = "File creation failed";
            signal(semlocal);
            return;
        }
        file.close();

        signal(semlocal);
        responseJson["status"] = "success";
        responseJson["message"] = "File created successfully";
        responseJson["fileId"] = to_string(node.fileNo);
        responseJson["filePath"] = filename;
        return;
    } 
    else if (requestType == "WRITE" && V.size() >= 4) {
        string id = V[2];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ofstream file(filename, ios::trunc);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            responseJson["status"] = "error";
            responseJson["message"] = "File not found";
            responseJson["fileId"] = id;
            responseJson["filePath"] = filename;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);
        
        responseJson["status"] = "success";
        responseJson["message"] = "File written successfully";
        responseJson["fileId"] = id;
        responseJson["filePath"] = filename;
        return;
    } 
    else if (requestType == "READ" && V.size() >= 3) {
        string id = V[2];
        int filenum = stoi(id);
        acquireRead(filenum - 1);
        printSemaphoreStatus(readSemaphores[filenum - 1], filenum);
        // string filename = to_string(port) + "_" + id + ".txt";
        
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ifstream file(filename);
        if (!file) {
            responseJson["status"] = "error";
            responseJson["message"] = "File not found";
        } else {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();
            responseJson["status"] = "success";
            responseJson["message"] = "File read successfully";
            responseJson["response"] = content;
        }

        responseJson["fileId"] = id;
        responseJson["filePath"] = filename;

        sleep(30); // Simulate delay

        releaseRead(filenum - 1);
        return;
    } 
    else if (requestType == "APPEND" && V.size() >= 4) {
        string id = V[2];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        // string filename = to_string(port) + "_" + id + ".txt";
        
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ofstream file(filename, ios::app);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            responseJson["status"] = "error";
            responseJson["message"] = "File not found";
            responseJson["fileId"] = id;
            responseJson["filePath"] = filename;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);
        
        responseJson["status"] = "success";
        responseJson["message"] = "File appended successfully";
        responseJson["fileId"] = id;
        responseJson["filePath"] = filename;
        return;
    }

    responseJson["status"] = "error";
    responseJson["message"] = "Invalid request type";
    return;
}

void sendAppendEntries(Node &node, const struct sockaddr_in& clientAddr, const char *msg, int sockfd) {
    cout<<"Became leader - Starting send append Entries thread\n";
    
    wait(semlocal);
    if(!node.isLeader) {
        signal(semlocal);
        return;
    }
    signal(semlocal);

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(1.0, 2.0);
    double randomFactor = dist(gen);
    electionTimeout = static_cast<int>(deltaElection * randomFactor);

    sendAppendEntriesTimeout = electionTimeout/2;

    cout << "Append Entries Timeout: " << sendAppendEntriesTimeout << " seconds." << endl;

    wait(semmtx);
    vector<Node> serverList;
    {
        ifstream inFile("../data/servers.json");
        if (!inFile) {
            cerr << "Error opening servers.json" << endl;
            signal(semmtx);
            return;
        }
        json serverData;
        inFile >> serverData;
        inFile.close();
        for (auto &s : serverData) {
            serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
        }
    }
    signal(semmtx); 



    while(1){
        unique_lock<mutex> lock(cv_mtx3);

        auto status = cv3.wait_for(lock, chrono::seconds(sendAppendEntriesTimeout));

        wait(semlocal);
        if(!node.isLeader) {
            signal(semlocal);
            return;
        }
        signal(semlocal);

        cout << "Sending Append Entries to all follower nodes" << endl;
        
        for(auto server: serverList) {
            if(server.ip == node.ip && server.port == node.port) continue;


            struct sockaddr_in followerAddr;
            followerAddr.sin_family = AF_INET;
            followerAddr.sin_port = htons(server.port);
            inet_pton(AF_INET, server.ip.c_str(), &followerAddr.sin_addr);

            wait(semlocal);
            int lastLogIndex = node.nextIndex[server.port];
            
            string sendAppendEntries = "APPEND REQUEST ";
            sendAppendEntries += to_string(node.termNumber);
            sendAppendEntries += " ";
            sendAppendEntries += to_string(lastLogIndex -1);
            sendAppendEntries += " | ";

            int sz = node.log.size();
            


            for(int i = (lastLogIndex - 1); i < sz; i++) {
                if(i == -1){
                    sendAppendEntries += to_string(-1);
                    sendAppendEntries += " ";
                    sendAppendEntries += "START | ";
                    continue;
                }
                sendAppendEntries += to_string(node.log[i].term);
                sendAppendEntries += " ";
                sendAppendEntries += node.log[i].command;
                sendAppendEntries += " | ";
            }
            // add commit index
            sendAppendEntries += to_string(node.commitIndex);
            cout<<"Sending: "<<sendAppendEntries;
            cout << " to port "<< ntohs(followerAddr.sin_port) << endl;
            signal(semlocal);

            sendto(sockfd, sendAppendEntries.c_str(), strlen(sendAppendEntries.c_str()), 0, (struct sockaddr*)&followerAddr, sizeof(followerAddr));

        }
    }

    return;
}

void parseAppendRequest(const char * msg, vector<string> &V, int &term, int &prevLogIndex, vector<LogEntry> &tempLog, int &commitIndex) {
    
    cout << "Parsing Append Request" << endl;

    stringstream ss(msg);
    string word;

    // Read first four values (fixed structure: "APPEND REQUEST term prevLogIndex")
    string commandType, requestType;
    ss >> commandType >> requestType >> term >> prevLogIndex;

    tempLog.clear();
    LogEntry temp;
    string logEntry;
    string ty = "0";
    
    // Read log entries, separated by "|"
    while (getline(ss, logEntry, '|')) {  
        stringstream entrySS(logEntry);
        string part;
        vector<string> entryParts;

        
        ty = "0";
        while (entrySS >> part) {
            if(ty == "0") ty = part;  
            entryParts.push_back(part);
        }

        if(ty == "-1") continue;

        if (entryParts.size() >= 2) {  
            temp.term = stoi(entryParts[0]);  
            temp.command = "";

            for (size_t i = 1; i < entryParts.size(); i++) {  
                if (!temp.command.empty()) temp.command += " ";  
                temp.command += entryParts[i];
            }

            tempLog.push_back(temp);
        }
        
    }

    int sz = V.size();

    commitIndex = stoi(V[sz-1]);
    
}



void processLogEntry(Node &node, const LogEntry &entry) {
    vector<string> V;
    breakCommand(entry.command, V);

    string requestType = V[0];

    string fullCommand = entry.command;
    // fullCommand = fullCommand.substr(8);

    string response;

    wait(semlocal);
    int port = node.port;
    signal(semlocal);



    if (requestType == "CREATE") {
        wait(semlocal);
        node.fileNo++;
        node.saveToJson();
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + to_string(node.fileNo) + ".txt";

        ofstream file(filename);
        if (!file) {
            cerr << "Error creating file: " << filename << endl;
            return;
        }
        file.close();
        signal(semlocal);
    } 
    else if (requestType == "WRITE") {
        string id = V[1];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ofstream file(filename, ios::trunc);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);
    } 
    else if (requestType == "READ") {
        string id = V[1];
        int filenum = stoi(id);
        acquireRead(filenum - 1);
        printSemaphoreStatus(readSemaphores[filenum - 1], 1);
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ifstream file(filename);
        if (!file) {
            response = "ERROR: File not found";
        } else {
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            response = content;
            file.close();
        }
        // sleep(10);
        releaseRead(filenum - 1);

    } 
    else if (requestType == "APPEND") {
        string id = V[1];
        int filenum = stoi(id);
        acquireWrite(filenum - 1);
        for(int i = 0; i < maxRead; i++){
            acquireRead(filenum - 1);
        }
        // string filename = to_string(port) + "_" + id + ".txt";
        string folder = "../serverfiles";  // Folder name
        string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + id + ".txt";

        ofstream file(filename, ios::app);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            return;
        }

        string message = fullCommand.substr(fullCommand.find(id) + id.length() + 1);
        file << message;
        file.close();
        for(int i = 0; i < maxRead; i++){
            releaseRead(filenum - 1);
        }
        releaseWrite(filenum - 1);
    }
}


int storeEntries(Node &node, const char *msg) {
    
    cout<<"Storing Entries after success: "<<"message received: ";
    cout<<msg<<" \n";
    int term, prevLogIndex;
    vector<string> V;
    breakCommand(msg, V);
    term = stoi(V[2]);
    prevLogIndex = stoi(V[3]);

    int index = prevLogIndex;

    vector<LogEntry> tempLog;
    int commitIndex;

    parseAppendRequest(msg, V, term, prevLogIndex, tempLog, commitIndex);

    if(prevLogIndex == -1){
        wait(semlocal);
        cout<<"Entries sent from the start\n";
        
        size_t j = 0;
        size_t i = 0;
        size_t sz1 = tempLog.size();
        size_t sz2 = node.log.size();
        while(j < sz1 && i < sz2){
            if(node.log[i].term == tempLog[j].term && node.log[i].command == tempLog[j].command){
                j++;
                i++;
            }
            else break;
        }
        
        while(j < sz1){
            signal(semlocal);
            processLogEntry(node, tempLog[j]); // Process file operation
            wait(semlocal);
            node.log.push_back(tempLog[j]);
            j++;
        }

        signal(semlocal);
    }
    else{
        wait(semlocal);
        int sz = tempLog.size();
        int szlog = node.log.size();

        for (int i = 0; i < szlog; i++) {
            if (node.log[i].term == tempLog[0].term && node.log[i].command == tempLog[0].command) {
                
                size_t j = 0;
                while(j < sz){
                    if(node.log[i].term == tempLog[j].term && node.log[i].command == tempLog[j].command){
                        i++;
                        j++;
                    }
                    else break;
                }

                while(j < sz){
                    signal(semlocal);
                    processLogEntry(node, tempLog[j]); // Process file operation
                    wait(semlocal);
                    node.log.push_back(tempLog[j]);
                    j++;
                }

                break;
            }
        }
        signal(semlocal);
    }


    wait(semlocal);
    index = node.log.size();
    index--;
  

    node.commitIndex = min(commitIndex, index);
    node.saveToJson();
    signal(semlocal);

    return index;

}

void handleAppendRequest(Node &node, const struct sockaddr_in& clientAddr, const char *msg, int sockfd) {
    int term, prevLogIndex;
    int prevterm;
    cout<<"Append request received as: "<<msg<<endl;
    sscanf(msg, "APPEND REQUEST %d %d | %d", &term, &prevLogIndex, &prevterm);

    wait(semlocal);
    if(term > node.termNumber){
        node.termNumber = term;
        node.role = 0;
        node.isLeader = false;
        node.votedFor = -1;
        node.votes.clear();
        node.saveToJson();
    }
    

    if(term < node.termNumber){
        // send APPEND REPLY currentTerm false

        string sendAppendReply = "APPEND REPLY ";
        sendAppendReply += to_string(node.termNumber);
        sendAppendReply += " ";
        sendAppendReply += "false";
        sendAppendReply += " ";
        sendAppendReply += to_string(prevLogIndex);
        signal(semlocal);
        cout<<"Failure because of term number\n";
        cout<<"Sending reply as: "<<sendAppendReply<<endl;
        sendto(sockfd, sendAppendReply.c_str(), strlen(sendAppendReply.c_str()), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        return;
    }
    else{
        int index = 0;
        bool success = false;

        if(prevLogIndex == -1 || ((prevLogIndex < node.log.size()) && (node.log[prevLogIndex].term == prevterm))){
            success = true; 
        }
        
        signal(semlocal);
        
        if(success){
            cout<<"Success!\n";
            int index = storeEntries(node, msg);
            
            string sendAppendReply = "APPEND REPLY ";
            wait(semlocal);
            sendAppendReply += to_string(node.termNumber);
            sendAppendReply += " ";
            sendAppendReply += "true";
            sendAppendReply += " ";
            sendAppendReply += to_string(index);
            cout<<"REPLYING WITH: "<<sendAppendReply<<" HI\n";
            signal(semlocal);
            cout<<"Sending reply as: "<<sendAppendReply<<endl;

            sendto(sockfd, sendAppendReply.c_str(), strlen(sendAppendReply.c_str()), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            return;
        }
        else{
            cout<<"Failure\n";

            string sendAppendReply = "APPEND REPLY ";
            wait(semlocal);
            sendAppendReply += to_string(node.termNumber);
            sendAppendReply += " ";
            sendAppendReply += "false";
            sendAppendReply += " ";
            sendAppendReply += to_string(index);
            signal(semlocal);
            cout<<"Sending reply as: "<<sendAppendReply<<endl;
            sendto(sockfd, sendAppendReply.c_str(), strlen(sendAppendReply.c_str()), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            return;
        }
    }
}


void handleAppendReply(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    cout<<"Handling Append Reply as: " << msg << endl;
    int term;
    string success;
    int index;
    vector<string> V;
    breakCommand(msg, V);
    term = stoi(V[2]);
    success = V[3];
    index = stoi(V[4]);


    wait(semlocal);
    if(term > node.termNumber){
        node.termNumber = term;
        node.role = 0;
        node.isLeader = false;
        node.votedFor = -1;
        node.votes.clear();
        node.saveToJson();
        signal(semlocal);
    }
    else if(term == node.termNumber && node.isLeader){
        reply++;
        if(success == "true"){
            cout<<"Updating next Index for port: "<<(ntohs(clientAddr.sin_port))<<endl;
            node.nextIndex[(ntohs(clientAddr.sin_port))] = index + 1;
            node.saveToJson();
        }
        else{
            node.nextIndex[ntohs(clientAddr.sin_port)] = max(0, node.nextIndex[ntohs(clientAddr.sin_port)] - 1);
            node.saveToJson();
        }

        if(node.nextIndex[ntohs(clientAddr.sin_port)] < node.log.size()){
            if(reply == (node.totalNodes - 1)) cv3.notify_all();
        }
        signal(semlocal);
    }
    

}


// Handles a HEARTBEAT message. Sends ACK only if term >= current term and notifies waiting threads.
void handleHeartbeat(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    int leaderPort, term, recv_ack_seq;
    sscanf(msg, "HEARTBEAT %d %d %d", &leaderPort, &term, &recv_ack_seq);
    cout << "Received HEARTBEAT from " << inet_ntoa(clientAddr.sin_addr)
         << " (leader port: " << leaderPort << ", term: " << term << ")" << endl;

    wait(semlocal);
    if (term >= node.termNumber) {
        node.leaderIp = inet_ntoa(clientAddr.sin_addr);
        node.leaderPort = leaderPort;
        node.termNumber = term;
        node.role = 0;          // follower
        node.isLeader = false;
        node.votedFor = -1;
        node.votes.clear();
        node.saveToJson();
        signal(semlocal);

        // Notify any waiting thread that a heartbeat arrived.
        cv1.notify_all();
        cv2.notify_all();
        // Send ACK only when term is sufficient.
        char ackMsg[1024] = {0};
        sprintf(ackMsg, "ACK %d", recv_ack_seq);
        sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    }
    else {
        signal(semlocal);
    }
}

// Handles a REQUEST VOTE message.
void handleRequestVote(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    int requesterPort, term, lastLogTermC, lastLogIndexC;
    sscanf(msg, "REQUEST VOTE %d %d %d %d", &requesterPort, &term, &lastLogTermC, &lastLogIndexC);
    cout << "Received REQUEST VOTE from " << inet_ntoa(clientAddr.sin_addr)
         << " (requester port: " << requesterPort << ", term: " << term << ")" << endl;
    char vote[1024] = {0};

    wait(semlocal);
    bool doo = 0;
    if (term > node.termNumber) {
        if(node.role == 1) doo = 1;
        node.termNumber = term;
        node.role = 0;
        node.isLeader = false;
        node.votedFor = -1;
        node.votes.clear();
        node.saveToJson();
    }
    signal(semlocal);

    wait(semlocal);
    if (term == node.termNumber && (node.votedFor == requesterPort || node.votedFor == -1)) {
        int lastLogTermV = (node.log.size()) ? node.log.back().term : 0;
        int lastLogIndexV = node.log.size() - 1;
        if (lastLogTermC < lastLogTermV || (lastLogTermC == lastLogTermV && lastLogIndexC < lastLogIndexV)) {
            cout << "Vote denied to " << requesterPort << endl;
            signal(semlocal);
            return;
        }

        node.votedFor = requesterPort;
        node.role = 0;
        node.isLeader = false;
        node.saveToJson();
        signal(semlocal);

        sprintf(vote, "VOTE %d %d", node.termNumber, node.port);
        sendto(sockfd, vote, strlen(vote), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

        cout << "Sent VOTE to " << inet_ntoa(clientAddr.sin_addr) << " : " << requesterPort << endl;

        if(doo) cv2.notify_all(); // notify any waiting thread
        cv1.notify_all(); // notify any waiting thread
    }
    else {
        signal(semlocal);
    }
}

// Handles a VOTE message.
void handleVote(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    int voterPort, term;
    sscanf(msg, "VOTE %d %d", &term, &voterPort);
    cout << "Received VOTE from " << inet_ntoa(clientAddr.sin_addr) << " : "  << voterPort << " (term: " << term << ")" << endl;

    wait(semlocal);
    if (term > node.termNumber) {
        node.termNumber = term;
        node.role = 0;
        node.isLeader = false;
        node.votedFor = -1;
        node.votes.clear();
        node.saveToJson();
        cv2.notify_all(); // notify any waiting thread
    }
    signal(semlocal);

    wait(semlocal);
    if (term == node.termNumber && node.role == 1) { // candidate mode
        node.votes.insert(voterPort);
        int numVotes = node.votes.size(); // include self vote
        signal(semlocal);

        if (2 * numVotes > node.totalNodes) {
            cout << "Got enough votes to win election." << endl;
            cv2.notify_all(); // notify any waiting thread
        }
    }
    else {
        signal(semlocal);
    }
}

// Handles an ACK message (typically in response to a heartbeat).
void handleAck(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    wait(semlocal);
    if(!node.isLeader) {
        signal(semlocal);
        return;
    }
    signal(semlocal);

    int ack_seq;
    sscanf(msg, "ACK %d", &ack_seq);
    if(ack_seq != ACK_SEQ) return;
    
    wait(semlocal);
    noOfACKs++;
    cout << "Received ACK from " << inet_ntoa(clientAddr.sin_addr) << " : " << ntohs(clientAddr.sin_port) << endl;
    signal(semlocal);
    // Update any heartbeat tracking if needed.
}

// --- Central Receive Thread ---
// This thread continuously listens on the given UDP socket and dispatches messages.
void receiveThreadFunction(Node &node, int sockfd) {
    char buffer[1024];
    
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                       (struct sockaddr*)&clientAddr, &addrLen);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            if (strncmp(buffer, "HEARTBEAT", 9) == 0) {
                handleHeartbeat(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "REQUEST VOTE", 12) == 0) {
                handleRequestVote(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "VOTE", 4) == 0) {
                handleVote(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "ACK", 3) == 0) {
                handleAck(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "REQUEST", 7) == 0) {
                //create a thread to do this
                thread hbThread1(handleAppendEntries, ref(node), ref(clientAddr), ref(buffer), ref(sockfd));
                hbThread1.detach();
                // handleAppendEntries(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "APPEND REQUEST", 14) == 0) {
                // handleAppendRequest(node, clientAddr, buffer, sockfd);
                thread hbThread2(handleAppendRequest, ref(node), ref(clientAddr), ref(buffer), ref(sockfd));
                hbThread2.detach();
            }
            else if (strncmp(buffer, "APPEND REPLY", 12) == 0) {
                handleAppendReply(node, clientAddr, buffer, sockfd);
            }
            // (Other message types can be added here.)
        }
        // Optionally, sleep briefly or check for exit conditions.
    }
}

// --- Leader Election Timeout and Start Election ---
// When a follower times out (i.e. does not receive a heartbeat within a dynamically computed timeout),
// it starts an election.
void startElection(Node &node, int &sockfd) {
    // Randomized election timeout: e.g., base deltaElection seconds scaled by a factor between 1 and 2.
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(1.0, 2.0);
    double randomFactor = dist(gen);
    electionTimeout = static_cast<int>(deltaElection * randomFactor);
    cout << "Election timeout: " << electionTimeout << " seconds." << endl;

    int remainingTime = electionTimeout;

    wait(semlocal);
    // Increment term, vote for self, and update state.
    node.termNumber++;
    node.votes.clear();
    node.votes.insert(node.port);
    node.role = 1;       // candidate
    node.votedFor = node.port;

    if(node.totalNodes == 1) {
        signal(semlocal);
        return;
    }

    node.saveToJson();
    cout << "Starting election with term " << node.termNumber << endl;
    signal(semlocal);

    // Read server list from shared file.
    vector<Node> serverList;
    wait(semmtx);
    {
        ifstream inFile("../data/servers.json");
        if (!inFile) {
            cerr << "Error opening servers.json" << endl;
            signal(semmtx);
            return;
        }
        json serverData;
        inFile >> serverData;
        inFile.close();
        for (auto &s : serverData) {
            serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
        }
    }
    signal(semmtx);

    wait(semlocal);
    while(node.role == 1) {
        signal(semlocal);
        // Send REQUEST VOTE to every other node.
        for (auto &server : serverList) {
            wait(semlocal);
            if (server.ip == node.ip && server.port == node.port) {
                signal(semlocal);
                continue;
            }
            else if (node.votes.find(server.port) != node.votes.end()) {
                signal(semlocal);
                continue;
            }
            else {
                signal(semlocal);
            }

            struct sockaddr_in followerAddr;
            followerAddr.sin_family = AF_INET;
            followerAddr.sin_port = htons(server.port);
            inet_pton(AF_INET, server.ip.c_str(), &followerAddr.sin_addr);

            char requestVote[1024] = {0};
            sprintf(requestVote, "REQUEST VOTE %d %d %d %d", node.port, node.termNumber, (node.log.size()) ? node.log.back().term : 0, (int)(node.log.size() - 1));
            sendto(sockfd, requestVote, strlen(requestVote), 0,
                (struct sockaddr*)&followerAddr, sizeof(followerAddr));
            cout << "Sent REQUEST VOTE to " << server.ip << " : " << server.port << endl;
        }

        // Instead of a fixed sleep, we wait on the condition variable.
        unique_lock<mutex> lock(cv_mtx2);
        // Wait up to electionTimeout seconds; if a heartbeat arrives, cv1.notify_all() will wake us early.
        auto status = cv2.wait_for(lock, chrono::seconds(min(deltaRequestVote, remainingTime)));

        remainingTime -= deltaRequestVote;

        if (status != cv_status::timeout || remainingTime <= 0) {
            cout << "Stopping sending REQUEST VOTE messages." << endl;
            break;
        }

        wait(semlocal);
    }
    signal(semlocal);
}

// --- assignType ---
// This function creates the UDP socket (binding it once) and spawns the central receive thread.
// Then it continuously acts based on node state:
// If leader, it periodically sends heartbeats; if follower, it waits with a timeout that
// can be interrupted if a heartbeat is received.
void assignType(Node &node) {
    initLeaderLocks(node, numFiles, maxRead);
    semmtx = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);
    semlocal = semget(ftok("/tmp", static_cast<int>(getpid() % 256)), 1, 0666 | IPC_CREAT);

    if (semmtx == -1 || semlocal == -1) {
        perror("semget");
        return;
    }

    // semctl(semlocal, 0, SETVAL, 1);

    // Create and bind UDP socket.
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        return;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(node.ip.c_str());
    address.sin_port = htons(node.port);
    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }
    
    // Spawn the central receive thread.
    thread recvThread(receiveThreadFunction, ref(node), sockfd);
    recvThread.detach();

    // Randomized election timeout: e.g., base deltaElection seconds scaled by a factor between 1 and 2.
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(1.0, 2.0);
    double randomFactor = dist(gen);
    electionTimeout = static_cast<int>(deltaElection * randomFactor);
    cout << "Election timeout: " << electionTimeout << " seconds." << endl;
    
    // Main loop: if leader, send periodic heartbeats; if follower, wait on the condition variable.
    while (true) {
        wait(semlocal);
        if (node.isLeader) {
            signal(semlocal);
            // Send heartbeats to all followers.
            vector<Node> serverList;
            {
                wait(semmtx);
                ifstream inFile("../data/servers.json");
                if (!inFile) {
                    cerr << "Error opening servers.json" << endl;
                    signal(semmtx);
                    break;
                }
                json serverData;
                inFile >> serverData;
                inFile.close();
                signal(semmtx);
                for (auto &s : serverData) {
                    serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
                }
            }

            // Reset ACK count for this heartbeat.
            noOfACKs = 0;
            ACK_SEQ++;

            // Send heartbeats to all followers.
            for (Node &follower : serverList) {
                if (follower.ip == node.ip && follower.port == node.port)
                    continue;
                struct sockaddr_in followerAddr;
                followerAddr.sin_family = AF_INET;
                followerAddr.sin_port = htons(follower.port);
                inet_pton(AF_INET, follower.ip.c_str(), &followerAddr.sin_addr);
                char heartbeatMsg[1024] = {0};
                sprintf(heartbeatMsg, "HEARTBEAT %d %d %d", node.port, node.termNumber, ACK_SEQ);
                sendto(sockfd, heartbeatMsg, strlen(heartbeatMsg), 0,
                       (struct sockaddr*)&followerAddr, sizeof(followerAddr));
                cout << "Sent HEARTBEAT to " << follower.ip << " : " << follower.port << endl;
            }

            this_thread::sleep_for(chrono::seconds(deltaHeartBeat));
            
            wait(semlocal);
            if(noOfACKs < node.totalNodes / 2) {
                cout << "Not enough ACKs received. Starting election." << endl;
                node.role = 1; // candidate
                node.isLeader = false;
                node.votedFor = -1;
                node.votes.clear();
                node.saveToJson();
            }
            signal(semlocal);
        }
        else {
            if(node.role == 0) {
                signal(semlocal);
                // Instead of a fixed sleep, we wait on the condition variable.
                unique_lock<mutex> lock(cv_mtx1);
                // Wait up to electionTimeout seconds; if a heartbeat arrives, cv1.notify_all() will wake us early.
                auto status = cv1.wait_for(lock, chrono::seconds(electionTimeout));
        
                if (status == cv_status::timeout) {
                    cout << "Didn't receive Heartbeat. Timed out." << endl;
                    wait(semlocal);

                    node.role = 1;
                    node.saveToJson();
                    signal(semlocal);
                    // or
                    // goto elect;
                }
            }
            else if(node.role == 1) {
                signal(semlocal);

            // elect:;
                startElection(node, sockfd);
                
                wait(semlocal);
                if(node.role == 0) {
                    signal(semlocal);
                    cout << "Received heartbeat. Exiting election." << endl;
                    continue;
                }
                else {
                    signal(semlocal);
                }

                wait(semlocal);
                // Check if majority was achieved.
                int votesReceived = node.votes.size(); // including self vote
                if (2 * votesReceived > node.totalNodes) {
                    cout << "Election won with " << votesReceived << " votes out of " << node.totalNodes << endl;
                    node.isLeader = true;
                    node.role = 2;
                    node.leaderIp = node.ip;
                    node.leaderPort = node.port;
                    node.saveToJson();
                    signal(semlocal);

                    thread sendAppendEntriesThread(sendAppendEntries, ref(node), ref(address), "APPEND ENTRIES", sockfd);
                    sendAppendEntriesThread.detach();

                    // send client at port 9090 and ip = 127.0.0.1 about the current leader port
                    for(int ipadd = 9090; ipadd <= 9096; ipadd++){
                        struct sockaddr_in clientAddr;
                        clientAddr.sin_family = AF_INET;
                        clientAddr.sin_port = htons(ipadd);
                        inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr);

                        char leaderInfo[1024] = {0};
                        sprintf(leaderInfo, "LEADER %d", node.port);
                        sendto(sockfd, leaderInfo, strlen(leaderInfo), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                        cout << "Sent LEADER info to client." << endl;
                    }


                    // Update shared state: mark self as leader.
                    wait(semmtx);
                    {
                        ifstream inFile("../data/servers.json");
                        if (inFile) {
                            json serverData;
                            inFile >> serverData;
                            inFile.close();
                            for (auto &s : serverData) {
                                if (s["ip"] == node.ip && s["port"] == node.port)
                                    s["isLeader"] = true;
                                else
                                    s["isLeader"] = false;
                            }
                            ofstream outFile("../data/servers.json", ios::trunc);
                            outFile << serverData.dump(4);
                            outFile.close();
                        }
                    }
                    signal(semmtx);
                }
                else {
                    cout << "Election failed. Not enough votes." << endl;
                    node.role = 1;
                    node.isLeader = false;
                    node.votedFor = -1;
                    node.votes.clear();
                    node.saveToJson();
                    signal(semlocal);
                }
            }
        }
    }
    
    close(sockfd);
}