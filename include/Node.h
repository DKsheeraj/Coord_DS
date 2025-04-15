#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <bits/stdc++.h>
#include <mutex>

using json = nlohmann::json;
using namespace std;

// Global mutex to protect Node persistent state.
static std::mutex g_nodeDataMutex;

struct LogEntry {
    int term;
    string command;

    // Convert LogEntry to JSON.
    json toJson() const {
        return {{"term", term}, {"command", command}};
    }

    // Create LogEntry from JSON.
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
    set<pair<string, int>> votes;     // nodes that voted for me; pair of (ip, port)
    int voteTimeout;    // in seconds

    int totalNodes;
    
    // Log replication fields:
    vector<LogEntry> log;
    int commitIndex;    // highest log entry known to be committed
    int lastApplied;    // highest log entry applied to state machine
    // For each follower, we use a pair (ip, port) as the key.
    map<pair<string,int>,int> nextIndex;
    map<pair<string,int>,int> matchIndex;
    int fileNo = 0;

    Node(string ip_, int port_, bool isLeader_ = false)
      : ip(ip_), port(port_), isLeader(isLeader_), termNumber(0), role(0),
        votedFor(-1), totalNodes(TOTAL_NODES), commitIndex(0), lastApplied(0) {}

    // Minimal toJson (for basic fields only).
    json toJson() const {
        return {{"ip", ip}, {"port", port}, {"isLeader", isLeader}};
    }

    // Load Node data from its corresponding JSON file.
    void loadFromJson() {
        // Lock the mutex during load.
        lock_guard<mutex> lock(g_nodeDataMutex);

        string filename = "../data/" + to_string(port) + ".json";
        ifstream inFile(filename);
        if (!inFile) {
            cerr << "Error opening file: " << filename << endl;
            return;
        }
        
        json j;
        try {
            inFile >> j;
        } catch (const exception& e) {
            cerr << "JSON parsing error: " << e.what() << endl;
            inFile.close();
            return;
        }
        inFile.close();

        // Basic fields.
        totalNodes   = j.value("totalNodes", totalNodes);
        ip           = j.value("ip", ip);
        port         = j.value("port", port);
        sockfd       = j.value("sockfd", sockfd);
        isLeader     = j.value("isLeader", isLeader);
        leaderIp     = j.value("leaderIp", leaderIp);
        leaderPort   = j.value("leaderPort", leaderPort);
        termNumber   = j.value("termNumber", termNumber);
        votedFor     = j.value("votedFor", votedFor);
        voteTimeout  = j.value("voteTimeout", voteTimeout);
        role         = j.value("role", role);
        commitIndex  = j.value("commitIndex", commitIndex);
        lastApplied  = j.value("lastApplied", lastApplied);
        fileNo       = j.value("fileNo", fileNo);

        // nextIndex: keys stored as "ip:port"
        nextIndex.clear();
        if (j.contains("nextIndex") && j["nextIndex"].is_object()) {
            for (auto& [key, value] : j["nextIndex"].items()) {
                auto pos = key.find(':');
                if (pos != string::npos) {
                    string peerIp = key.substr(0, pos);
                    int peerPort = stoi(key.substr(pos + 1));
                    nextIndex[{peerIp, peerPort}] = value.get<int>();
                }
            }
        }

        // matchIndex: similar conversion.
        matchIndex.clear();
        if (j.contains("matchIndex") && j["matchIndex"].is_object()) {
            for (auto& [key, value] : j["matchIndex"].items()) {
                auto pos = key.find(':');
                if (pos != string::npos) {
                    string peerIp = key.substr(0, pos);
                    int peerPort = stoi(key.substr(pos + 1));
                    matchIndex[{peerIp, peerPort}] = value.get<int>();
                }
            }
        }

        // Log: an array of LogEntry objects.
        log.clear();
        if (j.contains("log") && j["log"].is_array()) {
            for (const auto &entry : j["log"]) {
                log.push_back(LogEntry::fromJson(entry));
            }
        }

        // Votes: stored as an array of arrays, each with two elements.
        votes.clear();
        if (j.contains("votes") && j["votes"].is_array()) {
            for (const auto &voteItem : j["votes"]) {
                if (voteItem.is_array() && voteItem.size() >= 2) {
                    string voterIp = voteItem[0].get<string>();
                    int voterPort = voteItem[1].get<int>();
                    votes.insert({voterIp, voterPort});
                }
            }
        }
        
        // For debugging, print some state.
    //     cout << "Loaded Node state from " << filename << ":" << endl;
    //     cout << "IP: " << ip << ", Port: " << port << ", isLeader: " << boolalpha << isLeader << endl;
    //     cout << "Term: " << termNumber << ", votedFor: " << votedFor << ", role: " << role << endl;
    //     cout << "Total Nodes: " << totalNodes << ", commitIndex: " << commitIndex << endl;
    //     cout << "Log entries: " << log.size() << endl;
    //     cout << "FileNo: " << fileNo << endl;
    }

    // Save Node data to its corresponding JSON file.
    void saveToJson() {
        // Lock while writing persistent state.
        lock_guard<mutex> lock(g_nodeDataMutex);

        vector<json> logJson;
        for (const auto &entry : log) {
            logJson.push_back(entry.toJson());
        }

        // Convert votes (set<pair<string,int>>) to a JSON array.
        json votesJson = json::array();
        for (const auto &vote : votes) {
            votesJson.push_back({vote.first, vote.second});
        }

        // Convert nextIndex map to a JSON object with keys "ip:port".
        json nextIndexJson = json::object();
        for (const auto &entry : nextIndex) {
            string key = entry.first.first + ":" + to_string(entry.first.second);
            nextIndexJson[key] = entry.second;
        }
        
        // Convert matchIndex map similarly.
        json matchIndexJson = json::object();
        for (const auto &entry : matchIndex) {
            string key = entry.first.first + ":" + to_string(entry.first.second);
            matchIndexJson[key] = entry.second;
        }

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
            {"votes", votesJson},
            {"votedFor", votedFor},
            {"voteTimeout", voteTimeout},
            {"role", role},
            {"log", logJson},
            {"commitIndex", commitIndex},
            {"lastApplied", lastApplied},
            {"fileNo", fileNo},
            {"nextIndex", nextIndexJson},
            {"matchIndex", matchIndexJson}
        };

        outFile << j.dump(4);
        outFile.close();

        // // For debugging, print some state.
        // cout << "Storing data to Node " << filename << ":" << endl;
        // cout << "IP: " << ip << ", Port: " << port << ", isLeader: " << boolalpha << isLeader << endl;
        // cout << "Term: " << termNumber << ", votedFor: " << votedFor << ", role: " << role << endl;
        // cout << "Total Nodes: " << totalNodes << ", commitIndex: " << commitIndex << endl;
        // cout << "Log entries: " << log.size() << endl;
        // cout << "FileNo: " << fileNo << endl;
    }
};

#endif
