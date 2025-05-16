/*
filename server_ipaddress portno

argv[0] filename
argv[1] server_ipaddress
argv[2] portno

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void printBoard(int rows, int cols, int *board);
int valueIsInArray(int value, int *arr, int length);
int * calculateAllowedMovesPawn(int rows, int cols, int *board, int rowPosition, int columnPosition, int code);
int * calculateAllowedMovesKnight(int rows, int cols, int *board, int rowPosition, int columnPosition, int code);
int * calculateAllowedMovesTower(int rows, int cols, int *board, int rowPosition, int columnPosition, int code);
int * calculateMovesPiece(int rows, int cols, int *board, int rowPosition, int columnPosition, int code);
int checkIfMoveIsIn(int rowpos, int columnpos, int *moves, int movesLength);
void movePiece(int initRow, int initColumn, int endRow, int endColumn, int * board, int code, int * turn);
int lookForWhiteCheck(int rows, int cols, int *board);
int lookForBlackCheck(int rows, int cols, int *board);
void copyArray(int *arrayToCopy, int * copyingArray, int arrayToCopyLength);
void generateMenu();
void notationToIndices(const char *notation, int *row, int *col);
void printCaptured(const char *label, int *arr, int count);

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int charToInt(char value) {
    switch(value) {
        case '0':
            return 0;
            break;
        case '1':
            return 1;
            break;
        case '2':
            return 2;
            break;
        case '3':
            return 3;
            break;
        case '4':
            return 4;
            break;
        case '5':
            return 5;
            break;
        case '6':
            return 6;
            break;
        case '7':
            return 7;
            break;
        case '8':
            return 8;
            break;
        case '9':
            return 9;
            break;
	default:
	    return 10;
	    break;
    }
}

void fromBufferToBoard(char buffer[], int * board) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            switch(buffer[i*8 + j]) {
                case '0':
                    board[i*8+j] = 0;
                    break;
                case '1':
                    board[i*8+j] = 1;
                    break;
                case '2':
                    board[i*8+j] = 2;
                    break;
                case '3':
                    board[i*8+j] = 3;
                    break;
                case '4':
                    board[i*8+j] = 4;
                    break;
                case '5':
                    board[i*8+j] = 5;
                    break;
                case '6':
                    board[i*8+j] = 6;
                    break;
                case '7':
                    board[i*8+j] = 7;
                    break;
                case '8':
                    board[i*8+j] = 8;
                    break;
                case '9':
                    board[i*8+j] = 9;
                    break;
                case 'a':
                    board[i*8+j] = 10;
                    break;
                case 'b':
                    board[i*8+j] = 11;
                    break;
                case 'c':
                    board[i*8+j] = 12;
                    break;
            }
        }
    }
}

void printBuffer(char buffer[]) {
    for (int i = 0; i < 8; i++) {
        printf("\t \t");
        for (int j = 0; j < 8; j++) {
            switch(buffer[i*8 + j]) {
                case '0':
                    printf("|  |");
                    break;
                case '1':
                    printf("|♙ |");
                    break;
                case '2':
                    printf("|♖ |");
                    break;
                case '3':
                    printf("|♘ |");
                    break;
                case '4':
                    printf("|♗ |");
                    break;
                case '5':
                    printf("|♕ |");
                    break;
                case '6':
                    printf("|♔ |");
                    break;
                case '7':
                    printf("|♟ ︎|");
                    break;
                case '8':
                    printf("|♜ |");
                    break;
                case '9':
                    printf("|♞ |");
                    break;
                case 'a':
                    printf("|♝ |");
                    break;
                case 'b':
                    printf("|♛ |");
                    break;
                case 'c':
                    printf("|♚ |");
                    break;
            }
        }
        printf("\n");
    }
}

// Converts algebraic notation (e.g., "e2") to 0-based row and column indices
void notationToIndices(const char *notation, int *row, int *col) {
    if (notation == NULL || strlen(notation) < 2) {
        *row = *col = -1;
        return;
    }
    *col = notation[0] - 'a';
    *row = 8 - (notation[1] - '0');
}

void printCaptured(const char *label, int *arr, int count) {
    printf("%s: ", label);
    for (int i = 0; i < count; i++) {
        switch(arr[i]) {
            case 1: printf("♙ "); break;
            case 2: printf("♖ "); break;
            case 3: printf("♘ "); break;
            case 4: printf("♗ "); break;
            case 5: printf("♕ "); break;
            case 6: printf("♔ "); break;
            case 7: printf("♟ "); break;
            case 8: printf("♜ "); break;
            case 9: printf("♞ "); break;
            case 10: printf("♝ "); break;
            case 11: printf("♛ "); break;
            case 12: printf("♚ "); break;
            default: break;
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[255];
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname portn\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if (sockfd <0) {
        error("Error opening socket");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host.");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server ->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) & serv_addr, sizeof(serv_addr)) < 0) {
        error("Connection failed.");
    }

    int i1,i2,j1,j2;

 S:   
    bzero(buffer, 255);
    n = read(sockfd, buffer, 255);
    if (n <0) error("Error reading from socket");
    int board[8][8];
    int turn = charToInt(buffer[64]);
    fromBufferToBoard(buffer,board[0]);

    int capturedWhiteCount = (unsigned char)buffer[67];
    int capturedBlackCount = (unsigned char)buffer[68];
    int capturedByWhite[32], capturedByBlack[32];
    for (int i = 0; i < capturedWhiteCount; i++) {
        capturedByWhite[i] = (unsigned char)buffer[69 + i];
    }
    for (int i = 0; i < capturedBlackCount; i++) {
        capturedByBlack[i] = (unsigned char)buffer[69 + 32 + i];
    }

    printBuffer(buffer);
    printf("White Check: %d \n", buffer[65]);
	printf("Black Check: %d \n", buffer[66]);
	printf("Turn: %d \n", buffer[64]);

    printCaptured("White Pieces Captured", capturedByWhite, capturedWhiteCount);
    printCaptured("Black Pieces Captured", capturedByBlack, capturedBlackCount);

    char from[3], to[3];
    printf("Enter your move in chess notation : ");
    scanf("%2s %2s", from, to);

    // Optionally validate input here

    // Send the two notation strings to the server
    write(sockfd, from, 2);
    write(sockfd, to, 2);

    notationToIndices(from, &i1, &i2);
    notationToIndices(to, &j1, &j2);

    if (i1 > 7 || i2 > 7 || j1 > 7 || j2 > 7) {
			printf("Position values not valid: they lie outside the board \n");
            goto S;
		} else if (board[i1][i2] == 0) {
			printf("There is no piece in position [%d,%d] \n",i1,i2);
            goto S;
		} else {
			int pieceCode = board[i1][i2];
			if (turn == 0 && pieceCode >= 7) {
				printf("It's white's turn, can't move black pieces.\n");
                goto S;
			} else if (turn == 1 && pieceCode < 7) {
				printf("It's black's turn, can't move white pieces.\n");
                goto S;
			} else {
				int *possibleMoves = calculateMovesPiece(8, 8, board[0], i1, i2, pieceCode);
				int lengthMovesArr = possibleMoves[0] - 2;
				if (checkIfMoveIsIn(j1,j2,possibleMoves,lengthMovesArr)) {
					if (turn == 0 && lookForWhiteCheck(8,8,board[0])) {
						int temp[8][8];
						// implement copy function
						copyArray(board[0],temp[0],64);
						movePiece(i1,i2,j1,j2,temp[0],pieceCode, &turn);
						if (lookForWhiteCheck(8,8,temp[0])) {
							printf("You have to exit the check \n");
							turn--;
                            goto S;
						}
					} else if (turn == 1 && lookForBlackCheck(8,8,board[0])) {
						int temp[8][8];
						copyArray(board[0],temp[0],64);
						movePiece(i1,i2,j1,j2,temp[0],pieceCode, &turn);
						if (lookForBlackCheck(8,8,temp[0])) {
							printf("You have to exit the check \n");
							turn++;
                            goto S;
						} 
					}
				} else {
					printf("The inserted move is not allowed!\n");
                    goto S;
				}
				free(possibleMoves);
			}
		}

    bzero(buffer, 255);
    n = read(sockfd, buffer, 255);
    if (n < 0) error("Error");
    printBuffer(buffer);
    printf("White Check: %d \n", buffer[65]);
	printf("Black Check: %d \n", buffer[66]);
	printf("Turn: %d \n", buffer[64]);
    printCaptured("White Pieces Captured", capturedByWhite, capturedWhiteCount);
    printCaptured("Black Pieces Captured", capturedByBlack, capturedBlackCount);

    goto S;

    Q:
    printf("Exiting..");
    close(sockfd);
    return 0;
}
