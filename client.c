#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_SIZE 2048

// Global variables
int sockfd;
int addr_size = sizeof(struct sockaddr_in);
struct sockaddr_in dest; // Sock address of the server
int game_array[9] = {0,0,0,0,0,0,0,0,0}; // Game array

// Functions
int init();
void update_array(unsigned char* buf, int n);
void print_array();
void FYI(unsigned char* buf);
int MYM();
int process_answer(unsigned char* buf);



// MAIN: It initializes the socket, establishes a connection with the server, and enters a loop to receive and process server responses.

int main(int argc, char *argv[]) {
    if (argc != 3){
        fprintf(stderr, "You have %d arguments instead of 3\n", argc);
        return -1;
    }
    char *addr = argv[1];
    char* p = argv[2];
    int port = atoi(p);

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // dest is the server socket address
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, addr, &(dest.sin_addr.s_addr));
    dest.sin_port = htons(port);

    // initialize the connection (send hello)
    int s = init();
    if (s == -1){
        fprintf(stderr, "Error in init\n");
        return -1;
    }

    // Game (wait for server's instructions and process them)
    while (1){
        unsigned char *buf_receive = (unsigned char *) malloc (MAX_SIZE);
        struct sockaddr_in from;

        s = recvfrom(sockfd, buf_receive, MAX_SIZE, 0, (struct sockaddr *) &from, (socklen_t *) &addr_size);
        if (s < 0){
            fprintf(stderr, "Error in recvfrom: %s\n", strerror(errno));
            return -1;
        }
        s = process_answer(buf_receive);
        if (s == -1){
            fprintf(stderr, "Error in process_answer\n");
            return -1;
        }

        free(buf_receive);
    }

    return 0;
}



// INIT: Handles the initial setup of the game and returns the status of the initialization process.

int init(){
    char *buf = (char *) malloc (MAX_SIZE);
    unsigned char *buf_receive = (unsigned char *) malloc (MAX_SIZE);
    struct sockaddr_in from;
    buf[0] = 0x04;

    // Client sends hello
    printf("Welcome in Tic-Tac-Toe ! \n\n");
    printf("Enter 'hello' to start playing: \n");
    fgets(buf+1, MAX_SIZE, stdin);
    buf[strlen(buf)-1] = 0;

    printf("\nConnecting ...\n");

    int s = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &dest, sizeof(dest));
    if (s < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        return -1;
    }

    // Wait for answer from the server
    s = recvfrom(sockfd, buf_receive, MAX_SIZE, 0, (struct sockaddr *) &from, (socklen_t *) &addr_size);
    if (s < 0){
        fprintf(stderr, "Error in recvfrom: %s\n", strerror(errno));
        return -1;
    }

    s = process_answer(buf_receive);
    if (s == -1){
        fprintf(stderr, "Error in process_answer\n");
        return -1;
    }

    free(buf);
    free(buf_receive);

    return 0;
}



// UPDATE_ARRAY: Updates the game array based on the information from the server

void update_array(unsigned char* buf, int n){
    int i;

    for (i=2; i< 3*n+2; i+= 3){
        game_array[buf[i+2]*3 + buf[i+1]] = buf[i];
    }
}



// PRINT_ARRAY: Prints current gameboard based on the game array

void print_array(){
    int i;
    for (i=0; i<9; i++){
        if (i%3 == 0){
            printf("\n");
            printf("   -----------\n");
            printf("  |");
        } 
        if (game_array[i] == 0){
            printf("   |");
        } else if (game_array[i] == 1){
            printf(" X |");
        } else if (game_array[i] == 2){
            printf(" O |");
        } 
    }
    if (i%3 == 0){
        printf("\n");
        printf("   -----------\n");
    } 
}



// FYI: Handles the FYI message received from the server. It displays the number of filled positions on the board, updates the game array and displays it.

void FYI(unsigned char* buf){
    printf("  [FYI] -------------- \n");
    printf(" %d filled positions. \n", buf[1]);

    update_array(buf, buf[1]);
    print_array();

    printf(" \n--------------------- \n");
}



// MYM: Prompts the user to enter their next move; reads the input, converts it to the required format, and sends it to the server. 

int MYM(){
    void *buf = (void *) malloc (MAX_SIZE);
    unsigned char *newbuf = (unsigned char *) malloc (3);
    int res;

    printf(" [MYM] Where do you want to play next ?\n");

    // Asks the user for its next move
    fgets(buf, MAX_SIZE, stdin);
    res = atoi(buf);
    newbuf[0] = 0x05;
    newbuf[1] = res/10;
    newbuf[2] = res%10;

    // Send the move to the server
    int s = sendto(sockfd, newbuf, 3, 0, (struct sockaddr *) &dest, sizeof(dest));
    if (s < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        return -1;
    }
    printf("You just played in column %d, row %d \n\n", newbuf[1], newbuf[2]);

    free(buf);
    free(newbuf);

    return 0;
}



// PROCESS_ANSWER: Processes the server's response and reacts based on the message type received, calling other functions (FYI and MYM) to handle specific messages

int process_answer(unsigned char* buf){
    int s;
    switch (buf[0]){
        case 0x01: // FYI
            FYI(buf);
            break;
        case 0x02: // MYM
            s = MYM();
            if (s == -1){
                printf("Error in function MYM\n");
                return -1;
            }
            break;
        case 0x03: // END 
            if (buf[1] == 0){
                printf(" [END] It's a draw !\n");
            } else if(buf[1] == 0xFF){
                printf(" [END] Game already started, please try again later\n");
            } else if (buf[1] == 0xEE){
                printf(" [END] Wrong word\n");
            } else { 
                printf(" [END] The game has ended, player %d won!\n", buf[1]);
            }
            close(sockfd);
            exit(0);
        case 0x04: // TXT
            printf(" [TXT] %s \n\n", buf);
            break;
        default: // Non-defined instruction
            fprintf(stderr, "Non-defined buffer type: buf[0] = %c\n", buf[0]);
            return -1;
    }
    return 0;
}