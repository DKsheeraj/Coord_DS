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

public:
    Node(string ip, int port, bool isLeader = false) {
        this->ip = ip;
        this->port = port;
        this->isLeader = isLeader;
    }
};

#endif