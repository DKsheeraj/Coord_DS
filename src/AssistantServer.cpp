#include <fstream>
#include <nlohmann/json.hpp>
#include "../include/assistantUtils.h"

using json = nlohmann::json;

using namespace std;


int main() {
    Node serverNode("127.0.0.1", 8079, true);
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

    if (listen(serverNode.sockfd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server listening on " << serverNode.ip << ":" << serverNode.port << endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(serverNode.sockfd, (struct sockaddr*)&client_addr, &addrlen);

        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        if(fork() == 0){
            vector<Node> serverList = loadServers();
            close(serverNode.sockfd);
            //send server list to client
        }
    }

    close(serverNode.sockfd);


    return 0;
}
