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

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(node.ip.c_str());
    address.sin_port = htons(node.port);

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
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

                char heartbeatMsg[50];
                sprintf(heartbeatMsg, "HEARTBEAT %d %d", node.port, node.termNumber);
                sendto(sockfd, heartbeatMsg, strlen(heartbeatMsg), 0, 
                    (struct sockaddr*)&followerAddr, sizeof(followerAddr));
                
                P(semid1);
                cout << "Sent heartbeat to " << follower.ip << " : " << follower.port << endl;
                V(semid1);

                char buffer[1024];
                socklen_t addrLen = sizeof(followerAddr);
                struct timeval timeout;
                timeout.tv_sec = 2;  
                timeout.tv_usec = 0;
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

                int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&followerAddr, &addrLen);
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';

                    P(semid1);
                    cout << "Received : " << buffer << endl;
                    V(semid1);

                    if (strcmp(buffer, "ACK") == 0) {
                        P(semid1);
                        cout << "Follower " << follower.ip << " : " << follower.port << " is alive\n";
                        V(semid1);
                    }
                    else if(strncmp(buffer, "REQUEST VOTE", 12) == 0){
                        int term;
                        int requesterPort;
                        sscanf(buffer, "REQUEST VOTE %d %d", &requesterPort, &term);
    
                        char vote[50];

                        if(term > node.termNumber){
                            node.termNumber = term;
                            node.role = 0;
                            node.isLeader = false;
                            node.votedFor = -1;
                            node.votes.clear();
                        }
                        else if(term == node.termNumber && (node.votedFor == requesterPort || node.votedFor == -1)){
                            node.votedFor = requesterPort;
                            sprintf(vote, "VOTE %d %d", node.termNumber, node.port);
    
                            sendto(sockfd, vote, strlen(vote), 0, (struct sockaddr*)&followerAddr, sizeof(followerAddr));
                        }
                        else{
                            
                        }
                    }
                } else {
                    P(semid1);
                    cout << "No ACK from follower " << follower.ip << " : " << follower.port << endl;
                    V(semid1);
                }
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

    struct timeval timeout;
    
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(1.0, 2.0); 

    double randomDecimal = dist(gen);

    timeout.tv_sec = 10*randomDecimal;

    P(semid1);
    cout << "Timeout : " << timeout.tv_sec << endl;
    V(semid1);

    timeout.tv_usec = 0;

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
            
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';

                P(semid1);
                cout << "Received : " << buffer << endl;
                V(semid1);

                if(strncmp(buffer, "HEARTBEAT", 9) == 0){
                    const char *ackMsg = "ACK";
                    sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                }
                else if(strncmp(buffer, "REQUEST VOTE", 12) == 0){
                    int requesterPort;
                    int term;
                    sscanf(buffer, "REQUEST VOTE %d %d", &requesterPort, &term);

                    char vote[50];

                    if(term > node.termNumber){
                        node.termNumber = term;
                        node.role = 0;
                        node.isLeader = false;
                        node.votedFor = -1;
                        node.votes.clear();
                    }
                    else if(term == node.termNumber && (node.votedFor == requesterPort || node.votedFor == -1)){
                        node.votedFor = requesterPort;
                        sprintf(vote, "VOTE %d %d", node.termNumber, node.port);

                        sendto(sockfd, vote, strlen(vote), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                    }
                    else{
                        
                    }
                }
            }
            else{
                startElection(node, sockfd);
                close(sockfd);
                return;
            }
        }
        else{
            close(sockfd);
            return;
        }
    }

    close(sockfd);
}

void receiveVotes(Node &node, int &sockfd){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1 ; vop.sem_op = 1 ;

    int semid1 = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);

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

    while(1){
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        if(node.role != 1) return;

        int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);

        P(semid1);
        cout << "Received : " << buffer << endl;
        V(semid1);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            if(strncmp(buffer, "REQUEST VOTE", 12) == 0){
                int requesterPort;
                int term;
                sscanf(buffer, "REQUEST VOTE %d %d", &requesterPort, &term);

                char vote[50];
                sprintf(vote, "VOTE %d %d", node.termNumber, node.port);

                if(term > node.termNumber){
                    node.termNumber = term;
                    node.role = 0;
                    node.votedFor = -1;
                    node.votes.clear();
                }
                else if(term == node.termNumber && (node.votedFor == requesterPort || node.votedFor == -1)){
                    node.votedFor = requesterPort;
                    node.role = 0;
                    sendto(sockfd, vote, strlen(vote), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                }
                else{

                }
                
            }
            else if(strncmp(buffer, "VOTE", 4) == 0){
                int voterPort;
                int term;
                sscanf(buffer, "VOTE %d %d", &term, &voterPort);

                P(semid1);
                cout << "Received vote from " << voterPort << " now have " << node.votes.size() << " / " << node.totalNodes << endl;
                V(semid1);

                if(term > node.termNumber){
                    node.termNumber = term;
                    node.role = 0;
                    node.votedFor = -1;
                    node.votes.clear();
                }
                if(term == node.termNumber && node.role == 1){
                    node.votes.insert(voterPort);
                    int numOfVotesRecieved = 0;
                    numOfVotesRecieved = (node.votes).size();
                    if(2*numOfVotesRecieved > node.totalNodes){
                        node.isLeader = true;
                        node.role = 0;
                        return;
                    }
                }

            }
            else if(strncmp(buffer, "HEARTBEAT", 9) == 0){
                int term;
                int leaderPort;
                sscanf(buffer, "HEARTBEAT %d %d", &leaderPort, &term);

                if(term >= node.termNumber){
                    node.termNumber = term;
                    node.role = 0;
                    node.isLeader = false;
                    node.votedFor = -1;
                    node.votes.clear();
                    return;
                }
            }
            
        }
    }
}

void startElection(Node &node, int& sockfd){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    int semid1 = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);

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

    node.termNumber++;
    node.votes.clear();
    node.votes.insert(node.port);

    multiset<pair<long long, pair<int,string>>> M;
    for(Node &server : serverList){
        if(server.ip != node.ip || server.port != node.port) M.insert({time(0), {server.port, server.ip}});
    }

    thread receiveThread(receiveVotes, ref(node), ref(sockfd));
    receiveThread.detach();
    node.role = 1;
    node.votedFor = node.port;

    while(1){
        int numOfVotesRecieved = 0;
        numOfVotesRecieved = (node.votes).size();

        if(2*numOfVotesRecieved > node.totalNodes){
            node.isLeader = true;
            break;
        }

        if(node.role != 1) break;

        multiset<pair<long long, pair<int,string>>> Mnew;

        for(auto u: M){
            long long time_rec = u.first;
            long long curtime = time(nullptr);
            if(curtime - time_rec < 3){
                if(node.votes.find(u.second.first) == node.votes.end()) Mnew.insert(u);
            }
            else{
                if(node.votes.find(u.second.first) != node.votes.end()) continue;
                struct sockaddr_in followerAddr;
                followerAddr.sin_family = AF_INET;
                followerAddr.sin_port = htons(u.second.first);
                inet_pton(AF_INET, u.second.second.c_str(), &followerAddr.sin_addr);

                char requestVote[50]; 
                sprintf(requestVote, "REQUEST VOTE %d %d", node.port, node.termNumber);

                sendto(sockfd, requestVote, strlen(requestVote), 0, 
                    (struct sockaddr*)&followerAddr, sizeof(followerAddr));
                
                P(semid1);
                cout << "Sent request vote to " << u.second.second << " : " << u.second.first << endl;
                V(semid1);

                Mnew.insert({curtime, u.second});
            }
        }

        M = Mnew;
    }

    if(node.isLeader){
        P(semid1);

        std::ofstream clearFile("../data/servers.json", std::ios::out | std::ios::trunc);
        
        clearFile.close();
        json serverUpdatedData;

        for (Node& server : serverList) {
            if(server.ip == node.ip && server.port == node.port){
                server.isLeader = true;
            }
            else{
                server.isLeader = false;
            }
            serverUpdatedData.push_back(server.toJson());
        }
        std::ofstream outFile("../data/servers.json", std::ios::trunc);
        outFile << serverUpdatedData.dump(4); 
        outFile.flush();
        outFile.close();

        V(semid1);
    }
}