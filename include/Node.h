#ifndef NODE_H
#define NODE_H

#include<iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

class Node {
public:
    string ip;
    int port;
    int sockfd;
    struct sockaddr_in address;

    bool isLeader;

public:
    Node(string ip, int port, bool isLeader = false) {
        this->ip = ip;
        this->port = port;
        this->isLeader = isLeader;
    }
};

#endif
