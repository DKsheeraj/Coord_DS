#include "../include/Node.h"
#include "../include/leaderElection.h"

using namespace std;


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <IP> <Port>" << endl;
        return EXIT_FAILURE;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);

    Node serverNode(ip, port, false);
    if(port == 8080) {
        serverNode.isLeader = true;
        serverNode.role = 2;
    }

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;


    int semid1 = semget(ftok("/tmp", 1), 1, 0666 | IPC_CREAT);
    if(serverNode.port == 8080) semctl(semid1, 0, SETVAL, 1); 

    thread hbThread(assignType, ref(serverNode));
    hbThread.detach();


    serverNode.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverNode.sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serverNode.address.sin_family = AF_INET;
    serverNode.address.sin_addr.s_addr = inet_addr(ip.c_str());
    serverNode.address.sin_port = htons(serverNode.port);

    if (bind(serverNode.sockfd, (struct sockaddr*)&serverNode.address, sizeof(serverNode.address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server listening on " << serverNode.ip << ":" << serverNode.port << endl;

    listen(serverNode.sockfd, 5);

    while (true) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverNode.sockfd, (struct sockaddr*)&clientAddress, &clientAddressLength);

        if (clientSocket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {  
            close(serverNode.sockfd);
            char buffer[1024];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                cout << "Received: " << buffer << endl;
            }
            close(clientSocket);
            exit(0);
        }

        close(clientSocket);
    }

    return 0;
}
