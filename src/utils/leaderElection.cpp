#include "../../include/leaderElection.h"

void assignType(Node &node){
    while(1){
        if(node.isLeader){
            sendHeartbeat(node);
        }
        else{
            receiveHeartbeat(node);
        }
    }
}

void sendHeartbeat(Node &node) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;


    int semid1 = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);
    
    vector<Node> serverList;

    P(semid1);
    ifstream inFile("../data/servers.json");

    if (!inFile) {
        cerr << "Error opening servers.json" << endl;
        return;
    }

    json serverData;
    inFile >> serverData;
    inFile.close();
    V(semid1);

    for (auto& s : serverData) {
        serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
    }

    

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (sockfd < 0) {
        perror("UDP Socket creation failed");
        return;
    }

    int cnt = 0;

    while(1){
        cnt++;

        P(semid1);
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
            serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
        }
        for(Node &server : serverList){
            if(server.ip == node.ip && server.port == node.port){
                node.isLeader = server.isLeader;
            }
        }
        V(semid1);

    
        if(node.isLeader){
            for (Node &follower : serverList) {
                if(follower.ip == node.ip && follower.port == node.port){
                    continue;
                }
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

            if(cnt == 3){
                node.isLeader = false;
                // follower.isLeader = true;

                P(semid1);

                std::ofstream clearFile("../data/servers.json", std::ios::out | std::ios::trunc);
                
                clearFile.close();
                json serverUpdatedData;

                for (Node& server : serverList) {
                    if(server.ip == node.ip && server.port == node.port){
                        server.isLeader = false;
                    }
                    else if(server.port == 8081 || server.port == 8080){
                        server.isLeader = true;
                    }
                    serverUpdatedData.push_back(server.toJson());
                }
                std::ofstream outFile("../data/servers.json", std::ios::trunc);
                outFile << serverUpdatedData.dump(4);  // Pretty print with indentation
                outFile.flush();
                outFile.close();

                V(semid1);
            }

            this_thread::sleep_for(chrono::seconds(5));
        }
        else{
            close(sockfd);
            return;
        }
    }

    close(sockfd);
}


void receiveHeartbeat(Node &node){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;


    int semid1 = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);
    
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

    while (1) {
        P(semid1);
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
            serverList.emplace_back(s["ip"], s["port"], s["isLeader"]);
        }
        for(Node &server : serverList){
            if(server.ip == node.ip && server.port == node.port){
                node.isLeader = server.isLeader;
            }
        }
        V(semid1);

        if(!node.isLeader){
            char buffer[1024];
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            struct timeval timeout;
            timeout.tv_sec = 2;  
            timeout.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                cout << "Received: " << buffer << endl;

                const char *ackMsg = "ACK";
                sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            }
        }
        else{
            close(sockfd);
            return;
        }
    }

    close(sockfd);
}