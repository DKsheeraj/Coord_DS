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

using json = nlohmann::json;

using namespace std;

static string leaderIp = "127.0.0.1";
static int leaderPort = 8080;

class Node {
public:
    string ip;
    int port;
    int sockfd;
    struct sockaddr_in address;

    bool isLeader;

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
