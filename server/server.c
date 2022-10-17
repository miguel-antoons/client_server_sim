#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

bool sig_caught = false;

void signal_handler(int sig) { if (sig == SIGINT) sig_caught = true; }

// Function designed for chat between client and server.
void chat(int connfd) {
    unsigned int fileNumber;
    unsigned int matrixSize;
    char key[4] = {0};
    char response[4] = {0};

    while (1) {
        read(connfd, fileNumber, sizeof(fileNumber));
        read(connfd, matrixSize, sizeof(matrixSize));
        read(connfd, key, sizeof(key));

        printf("Received following key from client : [ ");

        for (int i = 0; i < 4; i++) {
            printf("%d ", key[i]);
            key[i] *= (char) 2;
        }

        printf("]\n\n");

        for (int i = 0; i < 4; i++) {
            printf("%d", key[i]);
        }

        printf("\n");
        sleep(1);

        write(connfd, key, sizeof(key));
    }

    printf("\nClosed connection.\n");
}

int main(int argc, char *argv[]) {
    int socketFD, connectionFD, len;
    struct sockaddr_in server = {0};
    struct sockaddr client;

    // create the socket and check if everything went well
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket successfully created.\n");
    }

    // configure server network parameters
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // below port is temporar and will become a program argument
    server.sin_port = htons(2241);

    // bind the socket to the correct addresses and ports of the server
    if ((bind(socketFD, (struct sockaddr*)&server, sizeof(server))) != 0) {
        printf("Socket bind failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket successfully binded.\n");
    }

    // passively listen to incoming connections
    if ((listen(socketFD, 10)) != 0) {
        printf("Socket listen call has failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket is listening for incoming connections...\n");
    }

    len = sizeof(client);
    while (1) {
        if ((connectionFD = accept(socketFD, (struct sockaddr *)&client, &len)) < 0) {
            printf("Unable to accept client connection.\nExiting...\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Connected to new client.\n");
        }

        chat(connectionFD);

        if (sig_caught) {
            printf("Cauxght exit signal.\nExiting...\n");
            close(connectionFD);
            return 0;
        }
    }
}
