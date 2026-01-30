# Socket-TicTacToe

CSE207 - Introduction to Networks final project

Game of TicTacToe through UDP sockets

## Presentation

This project is a simple implementation of the TicTacToe game using client-server architecture in line with my *Introduction to Networks* course. 
The game allows two players to compete against each other by making moves on a 3x3 grid until one player wins or the game ends in a draw.


## Prerequisites

Before running the TicTacToe game, make sure you have the following software installed on your system:

- GCC: The GNU Compiler Collection
- Make: A build automation tool


## Getting Started

To get started with the TicTacToe game, follow the steps below:

1. Clone or download the project files to your local machine.

2. Open a terminal and navigate to the project directory.

3. Compile the server and client programs using the provided Makefile. Run the following command: `make`

4. In a terminal window, start the server by running the following command: `./server <port>`. Replace `<port>` with the desired port number (e.g. 5000).

5. Open two new terminal windows and start the client programs by running the following command: `./client <server_address> <port>`. Replace `<server_address>` with the IP address or hostname where the server is running (127.0.0.1 for localhost), and `<port>` with the same port number used in the previous step. Do the same thing in both terminal windows.


## Gameplay

To start a game, a client has to send "hello" (or "Hello") to the server and once two clients are connected, the game will start.
The TicTacToe game follows the standard rules:

- Two players, represented by 'X' and 'O' symbols, take turns making moves on a 3x3 grid.
- The goal is to form a horizontal, vertical, or diagonal line of three of their symbols.
- Players make moves by entering the number corresponding to the desired position on the board.

The client 1 program will display the game board first and be prompted to make a move. To make a move, client 1 enters 'xy', referencing the column x and row y of the cell he is targeting. For example, for the top left cell, they enter `00`, for the top middle `10`, and so on.

After each move, the client program will display the updated game board. The program will prompt the current player for their move and provide instructions. Players take turns making moves by entering the number corresponding to the desired position on the board.

In case you make an incorrect move, the server will tell you your move was invalid and prompt you to make a new move.

The game continues until one player forms a horizontal, vertical, or diagonal line of three of their symbol ('X' or 'O'), indicating a win. If all positions on the grid are filled and no player has won, the game ends in a draw.

After the game ends, the program will terminate automatically. To exit the game before that, simply close the program or press Ctrl+C in the terminal.
