#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "../../include/assistantUtils.h"

using namespace std;

using json = nlohmann::json;

vector<Node> loadServers() {
    vector<Node> serverList;
    ifstream inFile("../data/servers.json");

    if (!inFile) {
        cerr << "Error opening servers.json" << endl;
        return serverList;
    }

    json serverData;
    inFile >> serverData;
    inFile.close();

    for (auto& s : serverData) {
        serverList.emplace_back(s["ip"], s["port"]);
    }

    return serverList;
}