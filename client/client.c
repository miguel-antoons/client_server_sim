#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>


void chat(int socketFD, int requests) {
    // temporar dummy values, these will change in the future
    char fileNumber = 0;
    char matrixSize = 2;
    char key[4] = {1, 2, 3, 4};
    unsigned char response[8 * 8] = {0};
    double interRequest = (1.0 / requests) * 1000000.0;
    struct timeval timeVal;
    double cpuTime, start[requests], end;
    struct pollfd pollFD[1];

    pollFD->fd = socketFD;
    pollFD->events = POLLIN;

    printf("Sending data to the server...\n");

    int j = 0;
    for (int i = 0; i < requests; i++) {
        gettimeofday(&timeVal, NULL);
        start[i] = (timeVal.tv_sec) + (timeVal.tv_usec) / 1000000;

        write(socketFD, &fileNumber, sizeof(fileNumber));
        write(socketFD, &matrixSize, sizeof(matrixSize));
        write(socketFD, key, sizeof(key));
        printf("Before condition\n");

        bzero(response, sizeof(response));
        poll(pollFD, 1, 0);
        if (pollFD->revents & POLLIN) {
            printf("Reading...\n");
            read(socketFD, response, sizeof(response));
            gettimeofday(&timeVal, NULL);
            end = (double)(timeVal.tv_sec) + ((double)(timeVal.tv_usec) / 1000000);
            cpuTime = end - start[j];

            printf("Program took %lf seconds\n", cpuTime);

            printf("Received following array from the server : [ ");

            for (int i = 0; i < 8 * 8; i ++) {
                printf("%d ", response[i]);
            }

            printf("]\n\n");

            j++;
        }
        printf("After condition\n");
        usleep(interRequest);
    }

    while (j < requests) {
        read(socketFD, response, sizeof(response));
        gettimeofday(&timeVal, NULL);
        end = (double)(timeVal.tv_sec) + ((double)(timeVal.tv_usec) / 1000000);
        cpuTime = end - start[j];

        printf("Program took %lf seconds\n", cpuTime);

        printf("Received following array from the server : [ ");

        for (int i = 0; i < 8*8; i ++) {
            printf("%d ", response[i]);
        }

        printf("]\n\n");

        j++;
    }
}

int main(int argc, char *argv[]) {
    int socketFD, connectionFD;
    struct sockaddr_in server = {0};
    int serverPort = 2241;

    printf("Creating client socket...\n");
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket successfully created.\n");
    }

    // initialize the server struct
    //! address and port below will change to a variable in the future
    server.sin_addr.s_addr = inet_addr("192.168.2.2");
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);

    printf("Trying to connect to the server...\n");
    int connectionResult = connect(socketFD, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    if (connectionResult < 0) {
        printf("Connection with server failed\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully connected to the server.\n");
    }

    //send messages here
    chat(socketFD, 1000);

    // close the socket
    close(socketFD);

    return 0;
}
