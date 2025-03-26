#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // Include the nlohmann/json library

using namespace std;
using json = nlohmann::json;

const int totalNodes = 5;
const int voteTimeout = 5;
const int basePort = 8080;

int main() {
    // Path to the JSON file
    const string filePath = "../data/servers.json";

    // Read the JSON file
    ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file " << filePath << endl;
        return 1;
    }

    json servers;
    inputFile >> servers;
    inputFile.close();

    // Modify all "isLeader" fields to false
    for (auto& server : servers) {
        server["isLeader"] = false;
    }

    // Write the modified JSON back to the file
    ofstream outputFile(filePath);
    if (!outputFile.is_open()) {
        cerr << "Error: Could not write to file " << filePath << endl;
        return 1;
    }

    outputFile << servers.dump(4); // Pretty print with 4 spaces
    outputFile.close();

    cout << "Updated all 'isLeader' fields to false in " << filePath << endl;

    // Update JSON files for ports 8080 to 8084
    for (int port = 8080; port <= 8084; ++port) {
        string portFilePath = "../data/" + to_string(port) + ".json";
        

        // Read the JSON file
        ifstream portInputFile(portFilePath);
        if (!portInputFile.is_open()) {
            cerr << "Error: Could not open file " << portFilePath << endl;
            continue;
        }

        

        json portData;
        portInputFile >> portData;
        portInputFile.close();

        

        // Update the fields
        portData["isLeader"] = false;
        portData["leaderPort"] = -1;
        portData["role"] = 0;
        portData["sockfd"] = -1;
        portData["termNumber"] = 0;
        portData["totalNodes"] = totalNodes;
        portData["voteTimeout"] = voteTimeout;
        portData["votedFor"] = -1;
        portData["votes"] = json::array();

        // portData["ip"] = "";
        portData["leaderIp"] = "";

        // Integer fields should be initialized to -1 or 0, not null
        // portData["port"] = -1;
        portData["commitIndex"] = 0;
        portData["lastApplied"] = 0;

        // Log storage should be an empty array
        portData["log"] = json::array();

        json nextIndexJson = json::object();
        json matchIndexJson = json::object();

        // If total nodes are known, pre-fill default nextIndex and matchIndex
        for (int i = 0; i < totalNodes; i++) {
            int nodePort = basePort + i;  // Example way to derive ports
            nextIndexJson[to_string(nodePort)] = 0;  // Start index at 0
            matchIndexJson[to_string(nodePort)] = 0; // Start index at 0
        }

        portData["nextIndex"] = nextIndexJson;
        portData["matchIndex"] = matchIndexJson;


        // Write the modified JSON back to the file
        ofstream portOutputFile(portFilePath);
        if (!portOutputFile.is_open()) {
            cerr << "Error: Could not write to file " << portFilePath << endl;
            continue;
        }

        portOutputFile << portData.dump(4); // Pretty print with 4 spaces
        portOutputFile.close();

        cout << "Updated file: " << portFilePath << endl;
    }
    return 0;
}