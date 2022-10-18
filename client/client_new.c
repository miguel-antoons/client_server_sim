#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

// get ranedom file nuimber and generate random key
int getRandom(int *key, int keySize) {
    // set a seed according to the time
    srand(time(NULL));
    // generate a random key
    for (int i = 0; i < keySize; i++) {
        key[i] = (int) rand() * INT_MAX;
    }

    // return a random number between 0 and 1000
    return ((int) rand() * 1000);
}

// send a request to the server
double sendRequest(int socketFD, int keySize, double *start) {
    int key[keySize * keySize];                 // encryption key to send to the server
    int fileNumber = getRandom(key, keySize);   // get a file number and generate a random key
    struct timeval timeVal;                     // declare time struct to get start and end time
    int errorCode, fileSize;                    // error code and file size the client will receive from the server

    // get the time the client sends a request
    gettimeofday(&timeVal, NULL);
    *start = (timeVal.tv_sec) + (timeVal.tv_usec) / 1000000.0;

    // send requested file number, key size and key to the server
    write(socketFD, &fileNumber, sizeof(fileNumber));
    write(socketFD, &keySize, sizeof(keySize));
    write(socketFD, key, sizeof(key));

    // get the error code returned by the server, return -1 if there is an error
    read(socketFD, &errorCode, sizeof(response));
    if (errorCode < 0) return -1;

    // get the file szie from the server, return -1 if the fileSize is not valid
    read(socketFD, &fileSize, sizeof(fileSize));
    if (fileSize <= 0) return -1;

    // get the file from the server
    int response[fileSize];
    read(socketFD, response, sizeof(response));

    // get the end time of the request
    gettimeofday(&timeVal, NULL);
    end = (double)(timeVal.tv_sec) + (timeVal.tv_usec) / 1000000.0;

    // return the total request time
    return (end - *start);
}

// child thread sends one requests to the server and waits for a response
void childThread(sockaddr_in serverInfo, int keySize, int childNumber, double *times, double *start) {
    int socketFD;   // file descriptor of the socket

    // create the socket and save its file descriptor
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // connect to the server with the socket
    int connectionResult = connect(socketFD, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    if (connectionResult < 0) {
        printf("Connection with server failed\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // send a request and get the request time
    times[childNumber] = sendRequest(socketFD, keySize, &start[childNumber]);

    // if the returned time is not valid, set the time and start values for this process to 0
    if (times[childNumber] < 0) {
        times[childNumber] = 0;
        start[childNumber] = 0;
    }

    // close the socket and exit the cild thread
    close(socketFD);
    exit(0);
}

void parentThread(int *times, double *start, int nChilds) {
    // wait for all the child threads to finish
    for (int i = 0; i < nChilds; i++) wait(NULL);

    // print the time results to the screen
    for (int i = 0; i < nChilds; i++) {
        printf("Child number %d sent a request at %lf and it took the server %lf seconds to respond.\n", i, start[i], times[i]);
    }
}

int main(int argc, char *argv[]) {
    int socketFD, connectionFD;
    struct sockaddr_in server   = {0};
    int serverPort              = 2241;                                 // destination server port number
    int requestPerSecond        = 10;                                   // request per second the client must send
    int programDurationSec      = 3;                                    // length in seconds the client must send requests
    double interRequestTime     = 1.0 / requestPerSecond * 1000000.0;   // time between each request (in order to get 'requestPerSecond' requests per second)
    int keySize                 = 2                                     // size of the key to send to the server (key matrix will be keySize * keySize)
    double times[requestPerSecond * programDurationSec];
    double start[requestPerSecond * programDurationSec];

    // initialize the server struct
    //! address and port below will change to a variable in the future
    server.sin_addr.s_addr = inet_addr("192.168.2.2");
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);

    // create the different child processes
    int i;
    for (i = 0; i < requestPerSecond * programDurationSec; i++) {
        processId = fork();
        if (processId == 0) break;
        usleep((int) interRequestTime);
    }

    // if the processId is a negative number, print an error message
    if (processId < 0) {
        write(1, "Error while forking", sizeof("Error while forking"));
        exit(-1);
    } else if (processId == 0) {
        // if this is a child process, execute the child procedure
        childThread(server, keySize, i, times, start);
    } else {
        // if this is the parent process, execute the parent procedure
        parentThread(times, start, requestPerSecond * programDurationSec);
    }

    return 0;
}
