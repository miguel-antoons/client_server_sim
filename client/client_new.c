#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>

// struct with all the info the info to send and read a request
// that is : a struct with the server info, keySize, childNumber,
// requesTime field to write in and requestStart to write in.
typedef struct {
    struct sockaddr_in *serverInfo;
    int keySize;
    int childNumber;
    unsigned int requestTime;
    unsigned int requestStart;
} child_t;

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

// get current time in nano seconds
unsigned int getCurrentTimeNano() {
    struct timespec timeVal;
    clock_gettime(CLOCK_MONOTONIC_RAW, &timeVal);
    return (timeVal.tv_sec) * 1000000000 + (timeVal.tv_nsec);
}

// send a request to the server
long int sendRequest(int socketFD, struct child_t *childInfo) {
    int key[childInfo->keySize * childInfo->keySize];       // encryption key to send to the server
    int fileNumber = getRandom(key, childInfo->keySize);    // get a file number and generate a random key
    int errorCode, fileSize;                                // error code and file size the client will receive from the server
    
    // get the time the client sends a request
    childInfo->requestStart = getCurrentTimeNano;

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
    return childInfo->requestStart - getCurrentTimeNano();
}

// child thread sends one requests to the server and waits for a response
void childThread(struct child_t *childInfo) {
    int socketFD;   // file descriptor of the socket

    // create the socket and save its file descriptor
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // connect to the server with the socket
    int connectionResult = connect(socketFD, (struct sockaddr *) childInfo->serverInfo, sizeof(struct sockaddr_in));
    if (connectionResult < 0) {
        printf("Connection with server failed\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // send a request and get the request time
    childInfo->requestTime = sendRequest(socketFD, childInfo);

    // if the returned time is not valid, set the time and start values for this process to 0
    if (childInfo->requestTime < 0) {
        childInfo->requestStart = 0;
        childInfo->requestTime  = 0;
    }

    // close the socket and exit the cild thread
    close(socketFD);
    exit(0);
}

void parentThread(struct child_t *childInfo, pthread_t *threadIds, int nChilds) {
    // wait for all the child threads to finish
    for (int i = 0; i < nChilds; i++) pthread_join(threadIds[i], NULL);

    // print the time results to the screen
    for (int i = 0; i < nChilds; i++) {
        printf(
            "Child number %d sent a request at %u and it took the server %u seconds to respond.\n",
            childInfo[i].childNumber,
            childInfo[i].requestStart,
            childTime[i].requestTime
        );
    }
}

int main(int argc, char *argv[]) {
    int socketFDD;
    struct sockaddr_in server   = {0};
    int serverPort              = 2241;                                 // destination server port number
    int requestPerSecond        = 10;                                   // request per second the client must send
    int programDurationSec      = 3;                                    // length in seconds the client must send requests
    double interRequestTime     = 1.0 / requestPerSecond * 1000000.0;   // time between each request (in order to get 'requestPerSecond' requests per second)
    int keySize                 = 2;                                    // size of the key to send to the server (key matrix will be keySize * keySize)
    const int nRequests         = requestPerSecond * programDurationSec;
    pthread_t threadIds[nRequests];
    struct child_t childInfo[nRequests];

    // initialize the server struct
    //! address and port below will change to a variable in the future
    server.sin_addr.s_addr = inet_addr("192.168.2.2");
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);

    // create the different child processes
    int i;
    for (i = 0; i < nRequests; i++) {
        childInfo[i].serverInfo     = &server;
        childInfo[i].keySize        = keySize;
        childInfo[i].childNumber    = i;

        pthread_create(&threadIds[i], NULL, childThread, &childInfo[i]);
        usleep((int) interRequestTime);
    }

    parentThread(&childInfo, threadIds, nRequests);

    return 0;
}
