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

void* createFiles(size_t vectorSize){
    char *str;
    str = (char *) malloc(vectorSize * sizeof(char));
    //bzero(str, vectorSize * sizeof(char));
    memset(str, 1, vectorSize * sizeof(char));
    
    return str;
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

// Function designed for chat between client and server.
void chat(int connfd, size_t fileSize, char *files[1000]) {
    char fileNumber = 0;
    char keySize = 0;
    char key[4] = {0};
    // char response[fileSize] = {0};

    while (1) {
        read(connfd, &fileNumber, sizeof(fileNumber));
        read(connfd, &keySize, sizeof(keySize));
        read(connfd, key, sizeof(key));
        printf("\nkey size : %d\n", keySize);
        printf("filenumber : %d\n", fileNumber);

        for (int i = 0; i < keySize * keySize; i++) {
            printf("%d ", key[i]);
        }
        printf("\n");

        /* transform key to matrix  */
        unsigned char keyMatrix [keySize][keySize]; 
        vectorToMatrix((size_t)keySize, key, keyMatrix);

        printf("\nPARAM ADDR : %p\n", files);

        /* transform file to matrix */
        unsigned char file [fileSize][fileSize]; 
        vectorToMatrix((size_t)fileSize, files[fileNumber], file);

        /* encryption */
        unsigned char result [fileSize][fileSize]; 
        bzero(result, sizeof(result));
        encryption(fileSize, (size_t)keySize, keyMatrix, file, result);

        /* encryption to vector */
        unsigned char resultVector[fileSize * fileSize]; 
        bzero(resultVector, sizeof(resultVector));
        matrixToVector((size_t) fileSize, result, resultVector);

        //printf("Received following key from client : [ ");

        // for (int i = 0; i < 4; i++) {
        //     printf("%d ", key[i]);
        //     key[i] *= (char) 2;
        // }

        //printf("]\n\n");

        printf("\n");
        // sleep(1);

        write(connfd, resultVector, sizeof(resultVector));
    }

    printf("\nClosed connection.\n");
}

int main(int argc, char *argv[]) {
    int     socketFD, connectionFD, len;
    struct  sockaddr_in server  = {0};
    struct  sockaddr client;
    char    *files[1000];
    int     threadNumber        = 0;
    size_t  fileSize            = 0;
    int     port                = 0;

    /* Arguments */
    if(argc != 7) return -1;
    for(int i=1; i < argc - 2; i++) {
        if(strcmp(argv[i], "-j") == 0) {
            threadNumber = atoi(argv[i + 1]);
        }
        
        if(strcmp(argv[i], "-s") == 0) {
            fileSize     = atoi(argv[i + 1]);
        } 

        if(strcmp(argv[i], "-p") == 0) {
            port         = atoi(argv[i + 1]);
        } 
    }

    /* create files */
    for(int i=0; i < 1000; i++) {
        files[i] = createFiles(fileSize * fileSize);
        //printf("%p\n", files[i]);
    }

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

        chat(connectionFD, fileSize, files);

        if (sig_caught) {
            printf("Cauxght exit signal.\nExiting...\n");
            close(connectionFD);
            return 0;
        }
    }
}
