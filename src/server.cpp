#include "../include/Node.h"
#include "../include/coordination.h"

using namespace std;

const int numFiles = 5;
const int maxRead = 5;

void initFileLocks(int port, int numFiles, int maxRead);
void acquireRead(int fileIdx);
void releaseRead(int fileIdx);
void acquireWrite(int fileIdx);
void releaseWrite(int fileIdx);



union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int createOrGetSemaphore(key_t key, int initialValue) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    
    if (semid != -1) {
        semun arg;
        arg.val = initialValue;
        semctl(semid, 0, SETVAL, arg);
        cout<<"Setting semaphore value\n";
    } else {
        if (errno == EEXIST) {
            cout<<"Already existing\n";
            semid = semget(key, 1, 0666);
        } else {
            // Handle other errors appropriately
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }

    return semid;
}

void initFileLocks(int port, int maxRead, std::vector<int> &readSemaphores, std::vector<int> &writeSemaphores) {
    for (int i = 0; i < numFiles; i++) {
        key_t readKey = ftok("/tmp", 100 + port * 10 + i);
        key_t writeKey = ftok("/tmp", 200 + port * 10 + i);

        int readSem = createOrGetSemaphore(readKey, maxRead);
        int writeSem = createOrGetSemaphore(writeKey, 1);

        readSemaphores.push_back(readSem);
        writeSemaphores.push_back(writeSem);

        std::cout << "[Semaphore] File " << i << " — ReadSem: " << readSem << ", WriteSem: " << writeSem << std::endl;
    }
}



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
    int semmtx = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semmtx != -1) {
        // This server successfully created the semaphore
        // Safe to initialize it here
        semctl(semmtx, 0, SETVAL, 1);
        cout<<"Setting semaphore value\n";
    } else {
        if (errno == EEXIST) {
            cout<<"Already existing\n";
            // The semaphore set already exists
            // Get the semaphore ID without creating it
            semmtx = semget(key, 1, 0666);
        } else {
            // Handle other errors appropriately
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }

    int semlocal = semget(ftok("/tmp", static_cast<int>(getpid() % 256)), 1, 0666 | IPC_CREAT);

    if (semmtx == -1 || semlocal == -1) {
        perror("semget");
        return 0;
    }
    semctl(semlocal, 0, SETVAL, 1);

    // create maxRead mutex for each file and one write mutex for each file in this port
    std::vector<int> readSemaphores;
    std::vector<int> writeSemaphores;
    initFileLocks(port, maxRead, readSemaphores, writeSemaphores);


    serverNode.loadFromJson();
    serverNode.port = port;
    serverNode.ip = ip;
    serverNode.saveToJson();

    thread hbThread(assignType, ref(serverNode));
    hbThread.detach();


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
