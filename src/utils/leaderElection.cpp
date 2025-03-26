#include "../../include/leaderElection.h"
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

// Global election timeout.
int electionTimeout; // seconds
int sendAppendEntriesTimeout;

// --- Semaphore helper functions ---
int semmtx, semlocal;

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

// --- Message Handlers ---

void breakCommand(const string &command, vector<string> &V){
    istringstream ss(command);
    string word;
    while(ss >> word){
        V.push_back(word);
    }
}

// Handles append requests from the client of the form : REQUEST <command>
void handleAppendEntries(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    cout<<"Received append request from Client with command: "<<msg<<endl;

    string command;
    vector<string> V;
    breakCommand(msg, V);

    command = V[1];

    wait(semlocal);
    bool st = node.isLeader;
    signal(semlocal);

    if(!st) return;

    wait(semlocal);
    LogEntry entry;
    entry.term = node.termNumber;
    entry.command = command;
    node.log.push_back(entry);

    node.saveToJson();

    cout<<"Current status of log: \n";

    for(auto u: node.log){
        cout<<u.term<<" "<<u.command<<endl;
    }

    signal(semlocal);

    cv3.notify_all();

}

void sendAppendEntries(Node &node, const struct sockaddr_in& clientAddr, const char *msg, int sockfd) {
    cout<<"Became leader - Starting send append Entries thread\n";
    
    wait(semlocal);
    bool st = node.isLeader;
    signal(semlocal);
    
    if(!st) return;

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
        wait(semlocal);
        bool st = node.isLeader;
        signal(semlocal);

        if(!st) break;
        unique_lock<mutex> lock(cv_mtx3);

        auto status = cv3.wait_for(lock, chrono::seconds(sendAppendEntriesTimeout));
        cout<<"Sending Append Entries to all follower nodes"<<endl;
        
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
            sendAppendEntries += " ";

            int sz = node.log.size();
            


            for(int i = (lastLogIndex - 1); i < sz; i++) {
                if(i == -1){
                    sendAppendEntries += to_string(-1);
                    sendAppendEntries += " ";
                    sendAppendEntries += "START ";
                    continue;
                }
                sendAppendEntries += to_string(node.log[i].term);
                sendAppendEntries += " ";
                sendAppendEntries += node.log[i].command;
                sendAppendEntries += " ";
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

void parseAppendRequest(vector<string> &V, int &term, int &prevLogIndex, vector<LogEntry> &tempLog, int &commitIndex) {
    
    cout<<"Parsing Append Request"<<endl;

    term = stoi(V[2]);

    int sz = V.size();

    for(int i = 4; (i+1) < sz; i+=2){
        LogEntry temp;
        temp.term = stoi(V[i]);
        temp.command = V[i+1];
        tempLog.push_back(temp);

    }
    
    commitIndex = stoi(V[sz - 1]);
    
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

    parseAppendRequest(V, term, prevLogIndex, tempLog, commitIndex);
    
    if(prevLogIndex == -1){
        wait(semlocal);
        node.log.clear();
        int sz = tempLog.size();
        for(int i = 1; i < sz; i++){
            node.log.push_back(tempLog[i]);
        }
        signal(semlocal);
    }
    else{
        wait(semlocal);
        int sz = tempLog.size();
        int szlog = node.log.size();

        for(int i = 0; i < szlog; i++){
            if(node.log[i].term == tempLog[0].term && node.log[i].command == tempLog[0].command){
                node.log.erase(node.log.begin() + i + 1, node.log.end());
                for(int j = 1; j < sz; j++){
                    node.log.push_back(tempLog[j]);
                }
                break;
            }
        }
        signal(semlocal);
    }
    wait(semlocal);
    index = node.log.size();
    index--;

    cout<<"Current status of log: \n";

    for(auto u: node.log){
        cout<<u.term<<" "<<u.command<<endl;
    }

    

    node.commitIndex = min(commitIndex, index);
    node.saveToJson();
    signal(semlocal);

    return index;

}

void handleAppendRequest(Node &node, const struct sockaddr_in& clientAddr, const char *msg, int sockfd) {
    int term, prevLogIndex;
    cout<<"Append request received as: "<<msg<<endl;
    sscanf(msg, "APPEND REQUEST %d %d", &term, &prevLogIndex);

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
        cout<<prevLogIndex<<" "<<node.log.size()<<" \n";
        // if(prevLogIndex >= 0) cout<<(node.log[prevLogIndex].term)<<" "<<term<<" \n";
        if(prevLogIndex == -1 || ((prevLogIndex < node.log.size()) && (node.log[prevLogIndex].term == term))){
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
    // sscanf(msg, "APPEND REPLY %d %s %d", &term, success, &index);
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
            cv3.notify_all();
        }
        signal(semlocal);
    }
    

}


// Handles a HEARTBEAT message. Sends ACK only if term >= current term and notifies waiting threads.
void handleHeartbeat(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    int leaderPort, term;
    sscanf(msg, "HEARTBEAT %d %d", &leaderPort, &term);
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
        const char *ackMsg = "ACK";
        sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    }
    else {
        signal(semlocal);
    }
}

// Handles a REQUEST VOTE message.
void handleRequestVote(Node &node, const struct sockaddr_in &clientAddr, const char *msg, int sockfd) {
    int requesterPort, term;
    sscanf(msg, "REQUEST VOTE %d %d", &requesterPort, &term);
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
    if(node.isLeader) cout << "Received ACK from " << inet_ntoa(clientAddr.sin_addr) << " : " << ntohs(clientAddr.sin_port) << endl;
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
                handleAppendEntries(node, clientAddr, buffer, sockfd);
            }
            else if (strncmp(buffer, "APPEND REQUEST", 14) == 0) {
                handleAppendRequest(node, clientAddr, buffer, sockfd);
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
            sprintf(requestVote, "REQUEST VOTE %d %d", node.port, node.termNumber);
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
    semmtx = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);
    semlocal = semget(ftok("/tmp", static_cast<int>(getpid() % 256)), 1, 0666 | IPC_CREAT);

    if (semmtx == -1 || semlocal == -1) {
        perror("semget");
        return;
    }

    semctl(semlocal, 0, SETVAL, 1);

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
            for (Node &follower : serverList) {
                if (follower.ip == node.ip && follower.port == node.port)
                    continue;
                struct sockaddr_in followerAddr;
                followerAddr.sin_family = AF_INET;
                followerAddr.sin_port = htons(follower.port);
                inet_pton(AF_INET, follower.ip.c_str(), &followerAddr.sin_addr);
                char heartbeatMsg[1024] = {0};
                sprintf(heartbeatMsg, "HEARTBEAT %d %d", node.port, node.termNumber);
                sendto(sockfd, heartbeatMsg, strlen(heartbeatMsg), 0,
                       (struct sockaddr*)&followerAddr, sizeof(followerAddr));
                cout << "Sent HEARTBEAT to " << follower.ip << " : " << follower.port << endl;
            }

            this_thread::sleep_for(chrono::seconds(deltaHeartBeat));
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
                    node.saveToJson();
                    signal(semlocal);

                    thread sendAppendEntriesThread(sendAppendEntries, ref(node), ref(address), "APPEND ENTRIES", sockfd);
                    sendAppendEntriesThread.detach();

                    // send client at port 9090 and ip = 127.0.0.1 about the current leader port
                    struct sockaddr_in clientAddr;
                    clientAddr.sin_family = AF_INET;
                    clientAddr.sin_port = htons(9090);
                    inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr);

                    char leaderInfo[1024] = {0};
                    sprintf(leaderInfo, "LEADER %d", node.port);
                    sendto(sockfd, leaderInfo, strlen(leaderInfo), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                    cout << "Sent LEADER info to client." << endl;


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