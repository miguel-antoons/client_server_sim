#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int     nThreads;
    int     port;
} argument_t;

// linked list node
typedef struct node_t {
    struct node_t*  previous;
    int             fileDescriptor;
} node_t;

char    *files[1000];
node_t  *tail = NULL;
node_t  *head = NULL;
size_t  fileSize    = 0;
size_t  size        = 0;
bool    sig_caught  = false;

pthread_cond_t  threadCond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;

void signal_handler(int sig) { if (sig == SIGINT) sig_caught = true; }

// get the head of the FIFO queue and delete the head (kinda like pop)
int pop() {
    if(size <= 0) return 0;

    int result = head->fileDescriptor;
    node_t* temp = head->previous;
    free(head);
    head = temp;

    size--;
    return result; 
}

// add new element to the FIFO queue (push operation)
void add(int fileDescriptor){
    node_t *newNode = malloc(sizeof(node_t));
    if(newNode == NULL) return;

    newNode->fileDescriptor = fileDescriptor;

    if(size == 0){
        tail = newNode;
        head = newNode;
    } else {
        tail->previous = newNode; 
        tail = newNode;
    }
    size++; 
}

void matrixToVector(size_t matrixSize, unsigned char C[][matrixSize], unsigned char *vector) {
     /* Matrix to Vector */
    int count = 0;

    for(int i=0; i < matrixSize; i++) {
        for(int j=0; j < matrixSize; j++) {
            vector[count] = C[i][j];
            count++;
        }
    }
}

void vectorToMatrix(size_t matrixSize, unsigned char* vector, unsigned char matrix[][matrixSize]) {
     /* Matrix to Vector */
    for(int i=0; i < matrixSize; i++) {
        for(int j=0; j < matrixSize; j++) {
            matrix[i][j] = *vector;
            vector++;
        }
    }
}

void encryption(size_t fileSize, size_t keySize, unsigned char key[][keySize], unsigned char file[][fileSize], unsigned char result[][fileSize]) {
    /* Matrices */
    int temp[keySize][keySize];                  
    int i,j,k,line,col; 

    for(line = 0; line < fileSize; line += keySize) {  
        for(col = 0; col < fileSize; col += keySize) {
        
            /* Mettre le block que l'on veut multiplier danns une matrice temporaire */ 
            for (i = 0; i < keySize; i++) {
                for (j = 0; j < keySize; j++) {
                    temp[i][j] = file[line + i][col + j];
                }
            }
        
            /*
            * multiplier la clé avec la matrice temporaire, 
            * puis ajouter le resultat en block dans la matrice final
            */
            for (i = 0; i < keySize; i++) {
                for (j = 0; j < keySize; j++) {
                    for (k = 0; k < keySize; k++) {
                        result[line + i][col + j] += key[i][k] * temp[k][j];
                    }
                }
            }
        }
    }
}

void processData(int connectionFD, unsigned int keySize, unsigned int fileNumber, unsigned char *key) {
    unsigned int errorCode  = 0;

    /* transform key to matrix  */
    unsigned char keyMatrix [keySize][keySize]; 
    vectorToMatrix((size_t)keySize, key, keyMatrix);

    /* transform file to matrix */
    unsigned char file [fileSize][fileSize]; 
    vectorToMatrix(fileSize, files[fileNumber], file);

    /* encryption */
    unsigned char result [fileSize][fileSize]; 
    bzero(result, sizeof(result));
    encryption(fileSize, (size_t)keySize, keyMatrix, file, result);

    /* encryption to vector */
    unsigned char resultVector[fileSize * fileSize]; 
    bzero(resultVector, sizeof(resultVector));
    matrixToVector((size_t) fileSize, result, resultVector);

    write(connectionFD, &errorCode, sizeof(errorCode));
    write(connectionFD, &fileSize, sizeof(fileSize));
    write(connectionFD, file, sizeof(file));
}

void *childThread() {
    int connectionFD;
    unsigned int fileNumber = 0;
    unsigned int keySize    = 0;
    
    while (1) {
        pthread_mutex_lock(&threadMutex);
        while (size == 0) pthread_cond_wait(&threadCond, &threadMutex);

        connectionFD = pop();
        pthread_mutex_unlock(&threadMutex);

        read(connectionFD, &fileNumber, sizeof(fileNumber));
        read(connectionFD, &keySize, sizeof(keySize));

        unsigned char key[keySize];;
        read(connectionFD, key, sizeof(key));

        processData(connectionFD, keySize, fileNumber, key);        
    }
    
    return NULL;
}

// main thread procedure
void parentThread(int nThreads, pthread_t *threadIds, int socketFD) {
    struct sockaddr client;
    int             connectionFD;
    int             len = sizeof(client);

    while (1) {
        if ((connectionFD = accept(socketFD, (struct sockaddr *)&client, &len)) < 0) {
            printf("Unable to accept client connection.\nExiting...\n");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&threadMutex);
        add(connectionFD);
        pthread_cond_signal(&threadCond);
        pthread_mutex_unlock(&threadMutex);

        if (sig_caught) {
            printf("Cauxght exit signal.\nExiting...\n");
            close(connectionFD);
            break;
        }
    }

    for (int i = 0; i < nThreads; i++) pthread_join(threadIds[i], NULL);
}

// free RAM space allocated with malloc syscall
void freeFiles(int nFiles) {
    // free all memory and set memory pointers to NULL
    for (int i = 0; i < nFiles; i++) {
        free(files[i]);
        files[i] = NULL;
    }
}

// create a certain amount of files
void createFiles(int nFiles) {
    int matrixSize = fileSize * fileSize;
    // generate 'nfiles' malloc to create files in RAM
    for (int i = 0; i < nFiles; i++) {
        files[i] = (char *) malloc(matrixSize * sizeof(char));
        // set value of assigned memory to 1
        memset(files[i], 1, matrixSize * sizeof(char));
    }
}

// get the arguments from the user
void getArguments(argument_t *arguments, int argc, char *argv[]) {
    if(argc != 7) exit(EXIT_FAILURE);

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-j") == 0) arguments->nThreads = atoi(argv[i + 1]);
        if(strcmp(argv[i], "-s") == 0) fileSize            = atoi(argv[i + 1]);
        if(strcmp(argv[i], "-p") == 0) arguments->port     = atoi(argv[i + 1]);
    }
}

int main(int argc, char *argv[]) {
    int         socketFD;
    argument_t  arguments;
    struct      sockaddr client;
    struct      sockaddr_in server  = {0};
    int         len                 = sizeof(client);
    const int   nFiles              = 1000;

    getArguments(&arguments, argc, argv);
    createFiles(nFiles);
    pthread_t threadIds[arguments.nThreads];

    // configure server network parameters
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(arguments.port);
    
    // create the socket and check if everything went well
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Socket creation failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // bind the socket to the correct addresses and ports of the server
    if ((bind(socketFD, (struct sockaddr*)&server, sizeof(server))) != 0) {
        printf("Socket bind failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    // passively listen to incoming connections
    if ((listen(socketFD, 1000000)) != 0) {
        printf("Socket listen call has failed.\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < arguments.nThreads; i++) {
        pthread_create(&threadIds[i], NULL, childThread, NULL);
    }

    parentThread(arguments.nThreads, threadIds, socketFD);
    freeFiles(nFiles);

    return 0;
}
