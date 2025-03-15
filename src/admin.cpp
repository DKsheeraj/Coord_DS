#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // JSON Library: Install with `sudo apt install nlohmann-json3-dev`
#include <vector>
#include <sys/stat.h> 

using json = nlohmann::json;
using namespace std;

const string DATA_FOLDER = "../data/";
const string FILE_PATH = DATA_FOLDER + "servers.json";

void ensureDataFolderExists() {
    struct stat st;
    if (stat(DATA_FOLDER.c_str(), &st) != 0) {
        mkdir(DATA_FOLDER.c_str(), 0777); 
    }
}

void addServer(string ip, int port) {
    json serverList;
    ifstream inFile(FILE_PATH);

    if (inFile) {
        inFile >> serverList;
    }
    inFile.close();

    serverList.push_back({{"ip", ip}, {"port", port}, {"isLeader", false}});

    ensureDataFolderExists();

    ofstream outFile(FILE_PATH);
    outFile << serverList.dump(4); 
    outFile.close();

    cout << "Server added: " << ip << ":" << port << endl;
}

int main() {
    string ip;
    int port;
    cout << "Enter Server IP: ";
    cin >> ip;
    cout << "Enter Server Port: ";
    cin >> port;

    addServer(ip, port);
    return 0;
}
