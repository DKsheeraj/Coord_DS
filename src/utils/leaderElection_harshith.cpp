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
mutex cv_mtx1, cv_mtx2;
condition_variable cv1, cv2;

// Global deltas
int deltaHeartBeat = 5; // seconds
int deltaRequestVote = 5; // seconds
int deltaElection = 10; // seconds

// Global election timeout.
int electionTimeout; // seconds

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
