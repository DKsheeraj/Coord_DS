#include "../include/Node.h"
using namespace std;

int main() {
    Node clientNode("127.0.0.1", 8081); 

    clientNode.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientNode.sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    clientNode.address.sin_family = AF_INET;
    clientNode.address.sin_port = htons(clientNode.port);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8079);
    if (inet_pton(AF_INET, clientNode.ip.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (inet_pton(AF_INET, clientNode.ip.c_str(), &clientNode.address.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(clientNode.sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    string message = "Hello from Client";
    send(clientNode.sockfd, message.c_str(), message.length(), 0);
    cout << "Message sent to server.\n";

    close(clientNode.sockfd);

    return 0;
}
