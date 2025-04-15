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

    // Convert LogEntry to JSON
    json toJson() const {
        return {{"term", term}, {"command", command}};
    }

    // Create LogEntry from JSON
    static LogEntry fromJson(const json &j) {
        return LogEntry{j.at("term").get<int>(), j.at("command").get<string>()};
    }
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
    set<pair<string, int>> votes;     // ports of nodes that voted for me
    int voteTimeout;    // in seconds

    int totalNodes;
    
    // Log replication fields:
    vector<LogEntry> log;
    int commitIndex;    // highest log entry known to be committed
    int lastApplied;    // highest log entry applied to state machine
    map<pair<string,int>,int> nextIndex;  // for each follower (by port) next log index to send
    map<pair<string,int>,int> matchIndex; // for each follower, highest replicated index
    int fileNo = 0;

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
            votes = j.value("votes", set<pair<string, int>>{});
            votedFor = j.value("votedFor", votedFor);
            voteTimeout = j.value("voteTimeout", voteTimeout);
            role = j.value("role", role);
            commitIndex = j.value("commitIndex", commitIndex);
            fileNo = j.value("fileNo", fileNo);
            nextIndex.clear();
            if (j.contains("nextIndex") && j["nextIndex"].is_object()) {
                for (auto& [key, value] : j["nextIndex"].items()) {
                    auto pos = key.find(':');
                    if (pos != string::npos) {
                        string ip = key.substr(0, pos);
                        int port = stoi(key.substr(pos + 1));
                        nextIndex[{ip, port}] = value.get<int>();  // Convert key back to int
                    }
                }
            }

            matchIndex.clear();
            if (j.contains("matchIndex") && j["matchIndex"].is_object()) {
                for (auto& [key, value] : j["matchIndex"].items()) {
                    auto pos = key.find(':');
                    if (pos != string::npos) {
                        string ip = key.substr(0, pos);
                        int port = stoi(key.substr(pos + 1));
                        matchIndex[{ip, port}] = value.get<int>();  // Convert key back to int
                    }
                }
            }


            log.clear(); // Ensure log is empty before inserting entries
            if (j.contains("log") && j["log"].is_array()) {
                for (const auto &entry : j["log"]) {
                    log.push_back(LogEntry::fromJson(entry));
                }
            }
            
        } catch (const exception& e) {
            cerr << "JSON parsing error: " << e.what() << endl;
        }
        

        inFile.close();
    }

    // Save Node data to its corresponding JSON file
    void saveToJson() {
        vector<json> logJson;
        for (const auto &entry : log) {
            logJson.push_back(entry.toJson());
        }

        string filename = "../data/" + to_string(port) + ".json";
        ofstream outFile(filename);

        json nextIndexJson = json::object();
        for (const auto &entry : nextIndex) {
            nextIndexJson[entry.first.first + ":" + to_string(entry.first.second)] = entry.second;
        }

        json matchIndexJson = json::object();
        for (const auto &entry : matchIndex) {
            matchIndexJson[entry.first.first + ":" + to_string(entry.first.second)] = entry.second;
        }


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
            {"role", role},
            {"log", logJson},  // Storing log entries as JSON array
            {"commitIndex", commitIndex},
            {"lastApplied", lastApplied},
            {"fileNo", fileNo},
            {"nextIndex", nextIndexJson},  // Store as object with string keys
            {"matchIndex", matchIndexJson}

            
        };

        outFile << j.dump(4); // Pretty print JSON with 4-space indentation
        outFile.close();
    }
};

#endif