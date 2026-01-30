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
int nbr_clients = 0; // Number of clients connected
struct sockaddr_in list_sock_players[2]; // List of the sock address of the two players
int game_array[9] = {0,0,0,0,0,0,0,0,0}; // Game array
int empty = 9; // Number of empty places
int player_turn = 0; // Number of the player whose turn it is

// Functions
int check_move(int row, int col);
int check_terminated_game();
int send_FYI(struct sockaddr_in dest);
int send_MYM(struct sockaddr_in dest);
int send_TXT(char* text, struct sockaddr_in dest);
int turn();
void end_game(int res);
int process_answer(unsigned char* buf, struct sockaddr_in from);

int main(int argc, char *argv[]) {
    if (argc != 2){
        fprintf(stderr, "You have %d arguments instead of 3\n", argc);
        return -1;
    }
    char* p = argv[1];
    int port = atoi(p);

    struct sockaddr_in serv_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("Socket created.\n");
    
    // Bind address of the server
    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) { 
		fprintf(stderr, "Could not bind socket: %s\n", strerror(errno));
		return -1;
    }
    printf("Bind to port %d.\n", port);

    // Waiting for two clients
    printf("Waiting for connections...\n");
    while (nbr_clients < 2){
        char *buf = (char *) malloc (MAX_SIZE);
        struct sockaddr_in from;

        int res = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *) &from, (socklen_t *) &addr_size);
        if (res < 0){
            fprintf(stderr, "Error in recvfrom: %s\n", strerror(errno));
            exit(1);
        }

        res = process_answer((unsigned char*) buf, from);
        if (res < 0){
            fprintf(stderr, "Error in answer processing\n");
            return -1;
        }
        free(buf);
    }

    // Start of the game
    player_turn = 1;
    while(1){
        printf("\nPlayer %d turn.\n", player_turn);
        int res = turn();
        if (res == -1){
            printf("Error during the player's turn\n");
            return -1;
        } else if (res != 3){
            // End of the game, send end message
            end_game(res);
            break;
        }
        // Change player
        player_turn = 3 - player_turn;
    }

    close(sockfd);
    return 0;
}



// CHECK_MOVE: Checks if a move is valid by verifying: 1) if the chosen position is within the board boundaries
//                                                     2) if it is not already occupied

int check_move(int row, int col){ 
    if ((row <0) || (row > 2)){ // Check row number
        printf("Problem with row\n");
        return -1;
    } else if ((col <0) || (col > 2)){ // Check column number
        printf("Problem with column\n");
        return -1;
    } else if (game_array[3*row + col] != 0){ // Check if the position is taken
        printf("Place already taken\n");
        return -1;
    }
    return 0;
}



// CHECK_TERMINATED_GAME: Checks if the game has ended by examining the current game board. It checks for three in 
//                        a row (horizontally, vertically, and diagonally), or a full board with no winner.

int check_terminated_game(){ 
    int row, col;
    //Check rows
    for (row=0; row < 3; row++){
        if ((game_array[3*row] == game_array[3*row+1]) && (game_array[3*row] == game_array[3*row+2]) && (game_array[3*row] != 0)){
            return game_array[3*row];
        }
    }
    //Check columns
    for (col=0; col < 3; col++){
        if ((game_array[col] == game_array[col+3]) && (game_array[col] == game_array[col+6]) && (game_array[col] != 0)){
            return game_array[col];
        }
    }
    //Check diagonals
    if ((game_array[0] == game_array[4]) && (game_array[0] == game_array[8]) && (game_array[0] != 0)){
        return game_array[0];
    }
    if ((game_array[2] == game_array[4]) && (game_array[2] == game_array[6]) && (game_array[2] != 0)){
        return game_array[2];
    }
    // Check if it's a draw (if the game is full)
    if (empty == 0){
        return 0;
    }

    return 3;
}



// SEND_FYI: Sends the FYI (For Your Information) message to a client which contains the current game state.

int send_FYI(struct sockaddr_in dest){ 
    printf(" [FYI] \n");

    // Create buffer
    unsigned char* buf = (unsigned char *) malloc (MAX_SIZE);
    buf[0] = 0x01;
    int buf_size = 2, i;
    buf[1] = 9 - empty;
    for (i=0; i<9; i++){
        if (game_array[i] != 0){
            buf[buf_size] = game_array[i];
            buf[buf_size+1] = i%3;
            buf[buf_size+2] = i/3;
            buf_size += 3;
        }
    }

    //Send buffer to the client
    int s = sendto(sockfd, buf, buf_size, 0, (struct sockaddr *) &dest, sizeof(dest));
    if (s < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        return -1;
    }

    free(buf);

    return 0;
}



// SEND_MYM: Sends the MYM (Make Your Move) message to a client when it's their turn to make a move.

int send_MYM(struct sockaddr_in dest){
    printf(" [MYM] \n");

    //Create buffer
    unsigned char* buf = (unsigned char*) malloc (1);
    buf[0] = 0x02;

    // Send buffer to the client
    int s = sendto(sockfd, buf, 1, 0, (struct sockaddr *) &dest, sizeof(dest));
    if (s < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        return -1;
    }

    free(buf);

    return 0;
}

// SEND_TXT: Sends a TXT message to a client

int send_TXT(char* text, struct sockaddr_in dest){ 
    printf(" [TXT] \n");

    //Create buffer
    char* buf = (char*) malloc (MAX_SIZE);
    buf[0] = 0x04;
    strncpy(buf+1, text, strlen(text));
    buf[strlen(buf)] = 0;

    // Send buffer to the client
    printf("Sending message: %s\n", buf);
    int s = sendto(sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *) &dest, sizeof(dest));
    if (s < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        return -1;
    }

    bzero(buf, MAX_SIZE);
    free(buf);

    return 0;
}



// END_GAME: Sends a message to both clients indicating the game has ended and gives the result (win or draw)

void end_game(int res){ 
    unsigned char *bufend = (unsigned char *) malloc (2);

    // Create buffer
    bufend[0] = 0x03;
    bufend[1] = res;

    // Sends buffer to the first player
    printf(" [END] [1]\n");
    int result = sendto(sockfd, bufend, 2, 0, (struct sockaddr *) &list_sock_players[0], sizeof(list_sock_players[0]));
    if (result < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        exit(0);
    }
    // Sends buffer to the second player
    printf(" [END] [2]\n");
    result = sendto(sockfd, bufend, 2, 0, (struct sockaddr *) &list_sock_players[1], sizeof(list_sock_players[1]));
    if (result < 0){
        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
        exit(0);
    }

    printf("Game has ended.\n\n");

    free(bufend);
}



//  TURN: Executes a player's turn, sending the FYI and MYM messages to the player, processing their move, and 
//        checking if the game has ended or if we keep playing.

int turn(){
    // Get the socket address of the player playing
    struct sockaddr_in dest = list_sock_players[player_turn -1];

    //Send FYI
    int res = send_FYI(dest);
    if (res < 0){
        printf("Error in sending FYI\n");
        return -1;
    }

    //Send MYM
    res = send_MYM(dest);
    if (res < 0){
        printf("Error in sending MYM\n");
        return -1;
    }

    unsigned char *buf = (unsigned char *) malloc (MAX_SIZE);
    struct sockaddr_in from;

    // Receive MOV message from client
    res = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *) &from, (socklen_t *) &addr_size);
    if (res < 0){
        fprintf(stderr, "Error in recvfrom: %s\n", strerror(errno));
        return -1;
    }

    // Process the answer
    res = process_answer(buf, from);
    free(buf);
    while (res == 4){ // If unvalid or if a third client tried to connect, receive again
        unsigned char *buf = (unsigned char *) malloc (MAX_SIZE);
        struct sockaddr_in from;

        res = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *) &from, (socklen_t *) &addr_size);
        if (res < 0){
            fprintf(stderr, "Error in recvfrom: %s\n", strerror(errno));
            return -1;
        }

        res = process_answer(buf, from);
        free(buf);
    }
    if (res < 0){
        fprintf(stderr, "Error in answer processing\n");
        return -1;
    } 
    return res;
}



// PROCESS_ANSWER: Handles cloents joining the server, processes their messages and performs the appropriate actions 
///                based on the message.

int process_answer(unsigned char* buf, struct sockaddr_in from){
    int s;
    switch (buf[0]){
        case 0x04: // TXT
            printf(" [r] [TXT] \n");
            printf(" Received message: %s\n", buf);
            if (nbr_clients < 2){ // Look for new connections
                char Hello[5] = "Hello";
                char hello[5] = "hello";
                if ((strncmp((char *)(buf+1), Hello, 5) == 0) || (strncmp((char *)(buf+1), hello, 5) == 0)){ // Check if hello is received
                    // If it is the case, accept the new client
                    nbr_clients++;
                    list_sock_players[nbr_clients-1] = from; // Adds the new player in the list

                    // Send a welcome message
                    if (nbr_clients == 1){
                        s = send_TXT("Welcome! You are player 1 in game, you play with X", from);
                        if (s < 0){
                            printf("Problem in sending TXT\n");
                            return -1;
                        }
                    } else {
                        s = send_TXT("Welcome! You are player 2 in game, you play with O", from);
                        if (s < 0){
                            printf("Problem in sending TXT\n");
                            return -1;
                        }
                    }
                    printf("Player %d assigned.\n\n", nbr_clients);
                } else { // Another word then hello is received
                    s = send_TXT("Wrong message, you are supposed to say 'hello'.", from);
                    if (s < 0){
                        printf("Problem in sending TXT\n");
                        return -1;
                    }

                    // Send an end message 
                    unsigned char *bufend = (unsigned char *) malloc (2);
                    bufend[0] = 0x03;
                    bufend[1] = 0xEE;

                    printf(" [END] wrong word \n\n");
                    s = sendto(sockfd, bufend, 2, 0, (struct sockaddr *) &from, sizeof(from));
                    if (s < 0){
                        fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
                        return -1;
                    }
                    free(bufend);
                }
            } else { // A third player is trying to connect to the game
                unsigned char *bufend = (unsigned char *) malloc (2);

                bufend[0] = 0x03;
                bufend[1] = 0xFF;

                // Send an end message
                printf(" [END] to third client \n");
                s = sendto(sockfd, bufend, 2, 0, (struct sockaddr *) &from, sizeof(from));
                if (s < 0){
                    fprintf(stderr, "Error in sendto: %s\n", strerror(errno));
                    return -1;
                }
                free(bufend);
                return 4;
            }
            break;
        case 0x05: // MOV
            printf(" [r] [MOV] \n");
            printf("Received movement was col=%d, row=%d\n",buf[1],buf[2]);
            s = check_move(buf[2], buf[1]); // Check the validity of the MOV
            if (s == -1){ // Unvalid position
                s = send_TXT("Unvalid position. Please try again.", from);
                if (s < 0){
                    printf("Problem in sending TXT\n");
                    return -1;
                }

                s = send_MYM(from);
                if (s < 0){
                    printf("Problem in sending MYM\n");
                    return -1;
                }
                return 4;
            }

            //Update board if valid position
            game_array[3*buf[2] + buf[1]] = player_turn;
            empty -= 1;
            printf("Position inserted.\n");

            //Check if the game is terminated
            int res = check_terminated_game();
            return res;
        default: // Non-defined instruction
            fprintf(stderr, "Non-defined buffer type: buf[0] = %c\n", buf[0]);
            return -1;
    }
    return 0;
}