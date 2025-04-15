#ifndef LOADBALANCER_H
#define LOADBALANCER_H

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

class LoadBalancer {
public:
    string ip;
    int port;
    int sockfd;
    struct sockaddr_in address;

    int sem;

    string leaderIp;
    int leaderPort;

    set<pair<string, int>> health;

    LoadBalancer(string ip_, int port_)
        : ip(ip_), port(port_) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            cerr << "Socket creation failed" << endl;
            exit(EXIT_FAILURE);
        }
    }
    ~LoadBalancer() {
        close(sockfd);
    }
    void init() {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ip.c_str());
        address.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cerr << "Bind failed" << endl;
            exit(EXIT_FAILURE);
        }
    }
};

#endif
