#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


void chat(int socketFD) {
    char messageBuffer[80];
    int n;

    while (1) {
        bzero(messageBuffer, 80);
        printf("Enter a new message : ");
        n = 0;
        
        // let the user enter a message and wait until he hits enter
        while((messageBuffer[n++] = getchar()) != '\n');

        // write the message to the server
        write(socketFD, messageBuffer, sizeof(messageBuffer));
        bzero(messageBuffer, sizeof(messageBuffer));

        // read response from the server
        read(socketFD, messageBuffer, sizeof(messageBuffer));

        printf("From server : %s", messageBuffer);

        if ((strncmp(messageBuffer, "exit", 4)) == 0) {
            printf("Exiting message loop...\n");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int socketFD, connectionFD;
    struct sockaddr_in server, client;

    printf("Creating client socket...\n");
    if (socketFD = socket(AF_INET, SOCK_STREAM, 0) < 0) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket successfully created.\n");
    }

    // set the server struct to 0
    bzero(&server, sizeof(server));

    // initialize the server struct
    server.sin_family = AF_INET;
    //! address and port below will change to a variable int the future
    server.sin_addr.s_addr = inet_addr("192.168.2.2");
    server.sin_port = htons("2241");

    printf("Trying to connect to the server...\n");
    if (connect(socketFD, (struct sockaddr *) &server, sizeof(server)) != 0) {
        printf("Connection with server failed\nExiting...\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully connected to the server.\n");
    }

    //send messages here
    chat(socketFD);

    // close the socket
    close(socketFD);

    return 0;
}
