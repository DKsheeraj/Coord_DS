#include "../../include/leaderElection.h"

void sendHeartbeat(Node &node) {
    vector<Node> serverList;
    ifstream inFile("../data/servers.json");

    if (!inFile) {
        cerr << "Error opening servers.json" << endl;
        return;
    }

    json serverData;
    inFile >> serverData;
    inFile.close();

    for (auto& s : serverData) {
        if (s["ip"] != node.ip || s["port"] != node.port) { 
            serverList.emplace_back(s["ip"], s["port"]);
        }
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (sockfd < 0) {
        perror("UDP Socket creation failed");
        return;
    }

    while (node.isLeader) {  
        for (const Node &follower : serverList) {
            struct sockaddr_in followerAddr;
            followerAddr.sin_family = AF_INET;
            followerAddr.sin_port = htons(follower.port);
            inet_pton(AF_INET, follower.ip.c_str(), &followerAddr.sin_addr);

            const char *heartbeatMsg = "HEARTBEAT";
            sendto(sockfd, heartbeatMsg, strlen(heartbeatMsg), 0, 
                   (struct sockaddr*)&followerAddr, sizeof(followerAddr));

            cout << "Sent heartbeat to " << follower.ip << ":" << follower.port << endl;

            char buffer[1024];
            socklen_t addrLen = sizeof(followerAddr);
            struct timeval timeout;
            timeout.tv_sec = 2;  
            timeout.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&followerAddr, &addrLen);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                if (strcmp(buffer, "ACK") == 0) {
                    cout << "Follower " << follower.ip << ":" << follower.port << " is alive\n";
                }
            } else {
                cout << "No ACK from follower " << follower.ip << ":" << follower.port << endl;
            }
        }

        this_thread::sleep_for(chrono::seconds(5));
    }

    close(sockfd);
}


void receiveHeartbeat(Node &node){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP Socket creation failed");
        return;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(node.ip.c_str());
    address.sin_port = htons(node.port);

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return;
    }

    while (!node.isLeader) {
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Received: " << buffer << endl;

            const char *ackMsg = "ACK";
            sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        }
    }

    close(sockfd);
}