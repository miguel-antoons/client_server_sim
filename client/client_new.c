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
    unsigned long long requestTime;
    unsigned long long requestStart;
} child_t;

typedef struct {
    size_t  keySize;
    int     rate;     
    int     time;
    char    *addr;
    int     port;
} arguments_t;

// get ranedom file nuimber and generate random key
int getRandom(unsigned char *key, int keySize) {
    // set a seed according to the time
    srand(time(NULL));
    // generate a random key
    for (int i = 0; i < keySize; i++) {
        key[i] = (unsigned char) rand();
    }

    // return a random number between 0 and 1000
    return (rand() % 1000);
}

// get current time in nano seconds
unsigned long long getCurrentTimeNano() {
    struct timespec timeVal;
    clock_gettime(CLOCK_MONOTONIC_RAW, &timeVal);
    return (unsigned long long) (timeVal.tv_sec) * 1000000000 + (timeVal.tv_nsec);
}

void generateCSV(child_t *childInfo, int nChilds) {
    FILE *ftp;
    ftp = fopen("stat.csv", "w");

    for (int i = 0; i < nChilds; i++) {
        fprintf(ftp, "%d, %llu, %llu\n", childInfo[i].childNumber, childInfo[i].requestStart, childInfo[i].requestTime);
    }
}

// send a request to the server
void sendRequest(int socketFD, child_t *childInfo) {
    unsigned char key[childInfo->keySize * childInfo->keySize]; // encryption key to send to the server
    int fileNumber = getRandom(key, childInfo->keySize);        // get a file number and generate a random key
    int errorCode, fileSize;                                    // error code and file size the client will receive from the server
    
    // get the time the client sends a request
    childInfo->requestStart = getCurrentTimeNano();

    // send requested file number, key size and key to the server
    write(socketFD, &fileNumber, sizeof(fileNumber));
    write(socketFD, &childInfo->keySize, sizeof(childInfo->keySize));
    write(socketFD, key, sizeof(key));

    // get the error code returned by the server, return -1 if there is an error
    read(socketFD, &errorCode, sizeof(errorCode));
    if (errorCode < 0) childInfo->requestTime = -1;

    // get the file szie from the server, return -1 if the fileSize is not valid
    read(socketFD, &fileSize, sizeof(fileSize));
    if (fileSize <= 0) childInfo->requestTime = -1;

    // get the file from the server
    int response[fileSize];
    read(socketFD, response, sizeof(response));

    // get the end time of the request
    childInfo->requestTime = getCurrentTimeNano() - childInfo->requestStart;
}

// child thread sends one requests to the server and waits for a response
void *childThread(void *childPointer) {
    child_t *childInfo = (child_t *) childPointer;
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
    sendRequest(socketFD, childInfo);

    // if the returned time is not valid, set the time and start values for this process to 0
    if (childInfo->requestTime < 0) {
        childInfo->requestStart = 0;
        childInfo->requestTime  = 0;
    }

    // close the socket and exit the cild thread
    close(socketFD);
    return NULL;
}

void parentThread(child_t *childInfo, pthread_t *threadIds, int nChilds) {
    // wait for all the child threads to finish
    for (int i = 0; i < nChilds; i++) pthread_join(threadIds[i], NULL);

    generateCSV(childInfo, nChilds);

    // print the time results to the screen
    for (int i = 0; i < nChilds; i++) {
        printf(
            "Child number %d sent a request at %llu and it took the server %llu nanoseconds to respond.\n",
            childInfo[i].childNumber,
            childInfo[i].requestStart,
            childInfo[i].requestTime
        );
    }
}

void getArguments(arguments_t *arguments,int argc, char *argv[]) {
    char *tmpAddr;
    const char separator[2] = ":";
    /* Arguments */
    if(argc != 8) exit(EXIT_FAILURE);
    for(int i=1; i < argc - 2; i++) {
        if(strcmp(argv[i], "-k") == 0) {
            arguments->keySize = atoi(argv[i + 1]);
        }
        
        if(strcmp(argv[i], "-r") == 0) {
            arguments->rate     = atoi(argv[i + 1]);
        } 

        if(strcmp(argv[i], "-t") == 0) {
            arguments->time     = atoi(argv[i + 1]);
            tmpAddr             = argv[i + 2];
            arguments->addr = strtok(tmpAddr, separator);
            arguments->port = atoi(strtok(NULL, separator));
        } 
    }
}

int main(int argc, char *argv[]) {
    arguments_t arguments;
    // get arguments 
    getArguments(&arguments, argc, argv);
    int socketFD;
    struct sockaddr_in server   = {0};
    double interRequestTime     = 1.0 / arguments.rate * 1000000.0;
    const int nRequests         = arguments.rate * arguments.time;
    pthread_t threadIds[nRequests];
    child_t childInfo[nRequests];

    
    // initialize the server struct
    //! address and port below will change to a variable in the future
    server.sin_addr.s_addr  = inet_addr(arguments.addr);
    server.sin_family       = AF_INET;
    server.sin_port         = htons(arguments.port);

    // create the different child processes
    int i;
    for (i = 0; i < nRequests; i++) {
        childInfo[i].serverInfo     = &server;
        childInfo[i].keySize        = arguments.keySize;
        childInfo[i].childNumber    = i;

        pthread_create(&threadIds[i], NULL, childThread, &childInfo[i]);
        usleep((int) interRequestTime);
    }

    parentThread(childInfo, threadIds, nRequests);

    return 0;
}
