#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // Include the nlohmann/json library

using namespace std;
using json = nlohmann::json;

const int totalNodes = 5;
const int voteTimeout = 5;

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