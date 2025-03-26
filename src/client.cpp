#include "../include/Node.h"
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <set>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <ctime>
#include <algorithm>
#include <nlohmann/json.hpp>

using namespace std;



void sendingMessages(Node &node, int sockfd, string &leaderIp, int &leaderPort) {
    while(1){
        string msg;
        cin>>msg;

        // send as REQUEST msg
        string sendMsg = "REQUEST " + msg;
        cout << "Sending: " << sendMsg << endl;
        cout<<leaderIp<<" "<<leaderPort<<endl;

        // send to leader 
        struct sockaddr_in leaderAddr;
        leaderAddr.sin_family = AF_INET;
        leaderAddr.sin_port = htons(leaderPort);
        inet_pton(AF_INET, leaderIp.c_str(), &leaderAddr.sin_addr);

        sendto(sockfd, sendMsg.c_str(), strlen(sendMsg.c_str()), 0, (struct sockaddr*)&leaderAddr, sizeof(leaderAddr));
    }
}

int main() {
    Node clientNode("127.0.0.1", 9090); 

    // Create a socket, udp and wait for messages

    clientNode.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientNode.sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(clientNode.ip.c_str());
    address.sin_port = htons(clientNode.port);

    if (bind(clientNode.sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(clientNode.sockfd);
        exit(EXIT_FAILURE);
    }

    cout << "Client listening on " << clientNode.ip << ":" << clientNode.port << endl;

    string leaderIp = "127.0.0.1";
    int leaderPort = 8080;

    // create a thread for sending messages whenever something is recieved on cout

    thread sendingThread(sendingMessages, ref(clientNode), clientNode.sockfd, ref(leaderIp), ref(leaderPort));
    sendingThread.detach();


    while (1) {
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int bytesReceived = recvfrom(clientNode.sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Received: " << buffer << endl;
            if(strncmp(buffer, "LEADER", 6) == 0){
                cout << "Received LEADER message from server." << endl;
                sscanf(buffer, "LEADER %d", &leaderPort);
                cout << "Leader port: " << leaderPort << endl;
            }
        }



    }

    return 0;
}
