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

struct LogEntry {
    int term;
    string command;
};

const int TOTAL_NODES = 5;

class Node {
public:
    string ip;
    int port;
    int sockfd;
    struct sockaddr_in address;

    bool isLeader;
    int role;           // 0 = follower, 1 = candidate, 2 = leader

    string leaderIp;
    int leaderPort = -1;

    int termNumber;
    int votedFor;       // port of the candidate voted for (-1 if none)
    set<int> votes;     // ports of nodes that voted for me
    int voteTimeout;    // in seconds

    int totalNodes;
    
    // Log replication fields:
    vector<LogEntry> log;
    int commitIndex;    // highest log entry known to be committed
    int lastApplied;    // highest log entry applied to state machine
    map<int,int> nextIndex;  // for each follower (by port) next log index to send
    map<int,int> matchIndex; // for each follower, highest replicated index

    Node(string ip_, int port_, bool isLeader_ = false) 
      : ip(ip_), port(port_), isLeader(isLeader_), termNumber(0), role(0), 
        votedFor(-1), totalNodes(TOTAL_NODES), commitIndex(0), lastApplied(0) {}

    // toJson() should output the node's state for the shared file.
    json toJson() const {
        return {{"ip", ip}, {"port", port}, {"isLeader", isLeader}};
    }

    // Load Node data from its corresponding JSON file
    void loadFromJson() {
        string filename = "../data/" + to_string(port) + ".json";
        ifstream inFile(filename);

        if (!inFile) {
            cerr << "Error opening file: " << filename << endl;
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
            votes = j.value("votes", set<int>{});
            votedFor = j.value("votedFor", votedFor);
            voteTimeout = j.value("voteTimeout", voteTimeout);
            role = j.value("role", role);
        } catch (const exception& e) {
            cerr << "JSON parsing error: " << e.what() << endl;
        }

        inFile.close();
    }

    // Save Node data to its corresponding JSON file
    void saveToJson() {
        string filename = "../data/" + to_string(port) + ".json";
        ofstream outFile(filename);

        if (!outFile) {
            cerr << "Error opening file: " << filename << endl;
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
};

#endif