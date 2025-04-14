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
int st = 0;



#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int semmtx_client;

void wait_sem(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void signal_sem(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop signal");
        exit(EXIT_FAILURE);
    }
}

void sendingMessages(Node &node, int sockfd, string &leaderIp, int &leaderPort) {
    while (1) {
        wait_sem(semmtx_client);
        cout<<"TYPE THE MESSAGE TO BE SENT: "<<endl;
        string msgType;
        cin >> msgType;

        string sendMsg;

        if (msgType == "CREATE") {
            sendMsg = "REQUEST CREATE";
        } 
        else if (msgType == "WRITE") {
            cout << "Enter ID: ";
            string id;
            cin >> id;
            cin.ignore();  // Clear input buffer
            cout << "Enter message to WRITE (less than 1024 characters): ";
            string msg;
            getline(cin, msg);
            sendMsg = "REQUEST WRITE " + id + " " + msg;
        } 
        else if (msgType == "READ") {
            cout << "Enter ID to READ: ";
            string id;
            cin >> id;
            sendMsg = "REQUEST READ " + id;
        } 
        else if (msgType == "APPEND") {
            cout << "Enter ID to APPEND to: ";
            string id;
            cin >> id;
            cin.ignore();
            cout << "Enter message to APPEND (less than 1024 characters): ";
            string msg;
            getline(cin, msg);
            sendMsg = "REQUEST APPEND " + id + " " + msg;
        } 
        else {
            cout << "Invalid message type. Please try again.\n";
            continue;
        }

        cout << "Sending: " << sendMsg << endl;
        cout << "Leader Address: " << leaderIp << " " << leaderPort << endl;

        struct sockaddr_in leaderAddr;
        leaderAddr.sin_family = AF_INET;
        leaderAddr.sin_port = htons(leaderPort);
        inet_pton(AF_INET, leaderIp.c_str(), &leaderAddr.sin_addr);

        sendto(sockfd, sendMsg.c_str(), sendMsg.length(), 0, (struct sockaddr*)&leaderAddr, sizeof(leaderAddr));
    }
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <IP> <Port>" << endl;
        return EXIT_FAILURE;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);

    Node clientNode(ip, port, false);

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

    semmtx_client = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semmtx_client == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(semmtx_client, 0, SETVAL, 0);

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
            if(strncmp(buffer, "LEADER", 6) == 0){
                st = 1;
                cout << "Received LEADER message from server." << endl;
                sscanf(buffer, "LEADER %d", &leaderPort);
                cout << "Leader port: " << leaderPort << endl;
                signal_sem(semmtx_client);
            }
            else if(strncmp(buffer, "CREATE REPLY ID", 15) == 0){
                int id;
                sscanf(buffer, "CREATE REPLY ID %d", &id);
                cout<<buffer<<endl;
                signal_sem(semmtx_client);
            }
            else if(strncmp(buffer, "WRITE REPLY", 11) == 0){
                cout<<buffer<<endl;
                signal_sem(semmtx_client);
            }
            else if(strncmp(buffer, "READ REPLY", 10) == 0){
                cout<<buffer<<endl;
                signal_sem(semmtx_client);
            }
            else if(strncmp(buffer, "APPEND REPLY", 12) == 0){
                cout<<buffer<<endl;
                signal_sem(semmtx_client);
            }
            else{
                cout<<"Unknown message: "<<buffer<<endl;
            }
        }



    }

    return 0;
}
