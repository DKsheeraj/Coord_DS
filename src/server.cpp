#include "../include/Node.h"
using namespace std;

int main() {
    Node serverNode("127.0.0.1", 8080, true);

    serverNode.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverNode.sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serverNode.address.sin_family = AF_INET;
    serverNode.address.sin_addr.s_addr = INADDR_ANY;
    serverNode.address.sin_port = htons(serverNode.port);

    if (bind(serverNode.sockfd, (struct sockaddr*)&serverNode.address, sizeof(serverNode.address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }


    cout << "Server listening on " << serverNode.ip << ":" << serverNode.port << endl;

    listen(serverNode.sockfd, 5);

    while(1){
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverNode.sockfd, (struct sockaddr*)&clientAddress, &clientAddressLength);

        if (clientSocket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        if(fork() == 0){
            close(serverNode.sockfd);
            char buffer[1024];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                cout << "Received: " << buffer << endl;
            }
        }

        close(clientSocket);
    }

    return 0;
}
