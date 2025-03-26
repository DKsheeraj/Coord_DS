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

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

    key_t key = ftok("/tmp", 1);
    int semid1 = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid1 != -1) {
        // This server successfully created the semaphore
        // Safe to initialize it here
        semctl(semid1, 0, SETVAL, 1);
        cout<<"Setting semaphore value\n";
    } else {
        if (errno == EEXIST) {
            cout<<"Already existing\n";
            // The semaphore set already exists
            // Get the semaphore ID without creating it
            semid1 = semget(key, 1, 0666);
        } else {
            // Handle other errors appropriately
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }



    

    serverNode.loadFromJson();

    for(auto u: serverNode.log){
        cout<<u.term<<" "<<u.command<<" JJJJJJJJj\n";
    }

    thread hbThread(assignType, ref(serverNode));
    hbThread.detach();

    cout<<"HE\n";
    cout<<(serverNode.ip)<<" HERE\n";

    serverNode.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverNode.sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    

    serverNode.address.sin_family = AF_INET;
    serverNode.address.sin_addr.s_addr = inet_addr((serverNode.ip).c_str());
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
