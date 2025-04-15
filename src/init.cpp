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
        
        json portData;

        // Read the JSON file
        ifstream portInputFile(portFilePath);
        if (!portInputFile.is_open()) {
            cerr << "Error: Could not open file " << portFilePath << endl;
            continue;
        }

        for(int id = 1; id <= 100; id++){
            string folder = "../serverfiles";  // Folder name
            string filename = folder + "/" + to_string(port) + "/" + to_string(port) + "_" + to_string(id) + ".txt";


            if (remove(filename.c_str()) == 0) {
                // cout << "File deleted successfully: " << filename << endl;
            } else {
                // perror("Error deleting file");
            }
        }

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

        portData["ip"] = "";
        portData["leaderIp"] = "";

        // Integer fields should be initialized to -1 or 0, not null
        portData["port"] = -1;
        portData["commitIndex"] = 0;
        portData["lastApplied"] = 0;
        portData["fileNo"] = 0;

        // Log storage should be an empty array
        portData["log"] = json::array();

        json nextIndexJson = json::object();
        json matchIndexJson = json::object();

        vector<pair<string, int>> servers;
        //read from servers.json into servers

        ifstream serverFile("../data/servers.json");
        if (!serverFile.is_open()) {
            cerr << "Error: Could not open file " << "../data/servers.json" << endl;
            continue;
        }
        json serverData;
        serverFile >> serverData;
        serverFile.close();
        for (const auto& server : serverData) {
            string ip = server["ip"];
            int port = server["port"];
            servers.push_back(make_pair(ip, port));
        }
        serverFile.close();
        // Initialize nextIndex and matchIndex for each server
        for (const auto& server : servers) {
            string ip = server.first;
            int port = server.second;
            //nextindex is pair<string,int>
            
            nextIndexJson[ip + ":" + to_string(port)] = 0; // Initialize to 0
            matchIndexJson[ip + ":" + to_string(port)] = 0; // Initialize to 0
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

    int ret = system("for id in $(ipcs -s | awk '{print $2}' | tail -n +3); do ipcrm -s $id; done");
    if (ret != 0) {
        cerr << "Error executing shell command." << endl;
    }

    for (int iport = 8080; iport <= 8084; iport++) {
        string command = "sudo kill -9 $(sudo lsof -t -i :" + to_string(iport) + ")";
        int ret_command = system(command.c_str());
        if (ret_command != 0) {
            cerr << "Error executing kill command on port " << iport << endl;
        }
    }
    return 0;
}