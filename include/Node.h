#ifndef NODE_H
#define NODE_H

#include<iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>  // Include the JSON library
#include <bits/stdc++.h>

using json = nlohmann::json;

using namespace std;

// struct LogEntry {
//     int term;
//     string command;
// };

// const int TOTAL_NODES = 5;

// class Node {
// public:
//     string ip;
//     int port;
//     int sockfd;
//     struct sockaddr_in address;

//     bool isLeader;
//     int termNumber;
//     int role;           // 0 = follower, 1 = candidate
//     int votedFor;       // port of the candidate voted for (-1 if none)
//     set<int> votes;     // ports of nodes that voted for me
//     int totalNodes;
    
//     // Log replication fields:
//     vector<LogEntry> log;
//     int commitIndex;    // highest log entry known to be committed
//     int lastApplied;    // highest log entry applied to state machine
//     map<int,int> nextIndex;  // for each follower (by port) next log index to send
//     map<int,int> matchIndex; // for each follower, highest replicated index

//     Node(string ip_, int port_, bool isLeader_ = false) 
//       : ip(ip_), port(port_), isLeader(isLeader_), termNumber(0), role(0), 
//         votedFor(-1), totalNodes(TOTAL_NODES), commitIndex(0), lastApplied(0) {}

//     // toJson() should output the node's state for the shared file.
//     json toJson() {
//         json j;
//         j["ip"] = ip;
//         j["port"] = port;
//         j["isLeader"] = isLeader;
//         // We do not serialize votes and log for brevity.
//         return j;
//     }
// };

static string leaderIp = "127.0.0.1";
static int leaderPort = 8080;

class Node {
public:
    int totalNodes = 5;
    string ip;
    int port;
    int sockfd;
    struct sockaddr_in address;

    bool isLeader;
    string leaderIp = "127.0.0.1";
    int leaderPort = 8080;

    int termNumber = 1;
    set<int> votes;
    int votedFor = -1;
    int voteTimeout = 3;

    int role = 0;  // 0 - follower, 1 - candidate, 2 - leader
    
    json toJson() const {
        return {{"ip", ip}, {"port", port}, {"isLeader", isLeader}};
    }

    // Load Node data from its corresponding JSON file
    void loadFromJson() {
        std::string filename = "../data/" + std::to_string(port) + ".json";
        std::ifstream inFile(filename);

        if (!inFile) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        try {
            json j;
            inFile >> j; // Read JSON data into j

            totalNodes = j.value("totalNodes", totalNodes);
            ip = j.value("ip", ip);
            port = j.value("port", port);
            sockfd = j.value("sockfd", sockfd);
            isLeader = j.value("isLeader", isLeader);
            leaderIp = j.value("leaderIp", leaderIp);
            leaderPort = j.value("leaderPort", leaderPort);
            termNumber = j.value("termNumber", termNumber);
            votes = j.value("votes", std::set<int>{});
            votedFor = j.value("votedFor", votedFor);
            voteTimeout = j.value("voteTimeout", voteTimeout);
            role = j.value("role", role);
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
        }

        inFile.close();
    }

    // Save Node data to its corresponding JSON file
    void saveToJson() {
        std::string filename = "../data/" + std::to_string(port) + ".json";
        std::ofstream outFile(filename);

        if (!outFile) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        json j = {
            {"totalNodes", totalNodes},
            {"ip", ip},
            {"port", port},
            {"sockfd", sockfd},
            {"isLeader", isLeader},
            {"leaderIp", leaderIp},
            {"leaderPort", leaderPort},
            {"termNumber", termNumber},
            {"votes", votes},
            {"votedFor", votedFor},
            {"voteTimeout", voteTimeout},
            {"role", role}
        };

        outFile << j.dump(4); // Pretty print JSON with 4-space indentation
        outFile.close();
    }
    
public:
    Node(string ip, int port, bool isLeader = false) {
        this->ip = ip;
        this->port = port;
        this->isLeader = isLeader;
    }
};

#endif