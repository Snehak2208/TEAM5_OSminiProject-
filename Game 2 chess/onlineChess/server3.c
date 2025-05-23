#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
void printCaptured(const char *label, int *arr, int count);

char intToChar(int value) {
    switch(value) {
        case 0:
            return '0';
            break;
        case 1:
            return '1';
            break;
        case 2:
            return '2';
            break;
        case 3:
            return '3';
            break;
        case 4:
            return '4';
            break;
        case 5:
            return '5';
            break;
        case 6:
            return '6';
            break;
        case 7:
            return '7';
            break;
        case 8:
            return '8';
            break;
        case 9:
            return '9';
            break;
        case 10:
            return 'a';
            break;
        case 11:
            return 'b';
            break;
        case 12:
            return 'c';
            break;
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

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Port number not provided. Program terminated.\n");
        exit(1);
    }

    int sockfd , newsockfd , portno, n;
    char buffer[255];

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen; // 32 bit data type in socket.h

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket.");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) <0) {
        error("Binding failed");
    }



    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
        error("Error on Accept");
    }

    int board[8][8] = {
	{8,9,10,11,12,10,9,8},
	{7,7,7,7,7,7,7,7},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1},
	{2,3,4,5,6,4,3,2}
	};
	int i1,i2,j1,j2;
	int turn = 0;
    int capturedByWhite[32], capturedByBlack[32];
    int capturedWhiteCount = 0, capturedBlackCount = 0;
    char str[67];
  S:   
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            str[i * 8 + j] = intToChar(board[i][j]);
        }
    }
    str[64] = turn;
    str[65] = lookForWhiteCheck(8,8, board[0]);
    str[66] = lookForBlackCheck(8,8,board[0]);
    // Add captured counts
    str[67] = (char)capturedWhiteCount;
    str[68] = (char)capturedBlackCount;
    // Add captured pieces
    for (int i = 0; i < capturedWhiteCount; i++) {
        str[69 + i] = (char)capturedByWhite[i];
    }
    for (int i = 0; i < capturedBlackCount; i++) {
        str[69 + 32 + i] = (char)capturedByBlack[i];
    }
    int msglen = 69 + 32 + 32; // enough for all
    n = write(newsockfd, str, msglen);
    
    if (n< 0) error("Error writing to socket.");
    printBoard(8,8, board[0]);
    printf("White Check: %d \n", lookForWhiteCheck(8,8,board[0]));
    printf("Black Check: %d \n", lookForBlackCheck(8,8,board[0]));
    printf("Turn: %d \n", turn);
    printCaptured("White Pieces Captured", capturedByWhite, capturedWhiteCount);
    printCaptured("Black Pieces Captured", capturedByBlack, capturedBlackCount);

    char from[3], to[3];
    n = read(newsockfd, from, 2); from[2] = '\0';
    n = read(newsockfd, to, 2);   to[2] = '\0';
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
						} else {
							turn--;
                            // Before movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);
                            if (board[j1][j2] != 0) {
                                if (turn == 0) {
                                    capturedByWhite[capturedWhiteCount++] = board[j1][j2];
                                } else {
                                    capturedByBlack[capturedBlackCount++] = board[j1][j2];
                                }
                            }
							movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);
						}
					} else if (turn == 1 && lookForBlackCheck(8,8,board[0])) {
						int temp[8][8];
						copyArray(board[0],temp[0],64);
						movePiece(i1,i2,j1,j2,temp[0],pieceCode, &turn);
						if (lookForBlackCheck(8,8,temp[0])) {
							printf("You have to exit the check \n");
							turn++;
                            goto S;
						} else {
							turn++;
                            // Before movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);
                            if (board[j1][j2] != 0) {
                                if (turn == 0) {
                                    capturedByWhite[capturedWhiteCount++] = board[j1][j2];
                                } else {
                                    capturedByBlack[capturedBlackCount++] = board[j1][j2];
                                }
                            }
							movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);

						}

					} else {
                        // Before movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);
                        if (board[j1][j2] != 0) {
                            if (turn == 0) {
                                capturedByWhite[capturedWhiteCount++] = board[j1][j2];
                            } else {
                                capturedByBlack[capturedBlackCount++] = board[j1][j2];
                            }
                        }
						movePiece(i1,i2,j1,j2,board[0],pieceCode, &turn);
					}
				} else {
					printf("The inserted move is not allowed!\n");
                    goto S;
				}
				free(possibleMoves);
			}
		}
    P: 
    printBoard(8,8,board[0]);
    printf("White Check: %d \n", lookForWhiteCheck(8,8,board[0]));
    printf("Black Check: %d \n", lookForBlackCheck(8,8,board[0]));
    printf("Turn: %d \n", turn);

    printCaptured("White Pieces Captured", capturedByWhite, capturedWhiteCount);
    printCaptured("Black Pieces Captured", capturedByBlack, capturedBlackCount);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            str[i * 8 + j] = intToChar(board[i][j]);
        }
    }
    str[64] = turn;
    str[65] = lookForWhiteCheck(8,8, board[0]);
    str[66] = lookForBlackCheck(8,8,board[0]);
    // Add captured counts
    str[67] = (char)capturedWhiteCount;
    str[68] = (char)capturedBlackCount;
    // Add captured pieces
    for (int i = 0; i < capturedWhiteCount; i++) {
        str[69 + i] = (char)capturedByWhite[i];
    }
    for (int i = 0; i < capturedBlackCount; i++) {
        str[69 + 32 + i] = (char)capturedByBlack[i];
    }
    msglen = 69 + 32 + 32; // enough for all
    n = write(newsockfd, str, msglen);

    char from2[3], to2[3];
    printf("Enter move in chess notation: ");
    scanf("%2s %2s", from2, to2);
    int k1, k2, l1, l2;
    notationToIndices(from2, &k1, &k2);
    notationToIndices(to2, &l1, &l2);
    if (k1 < 0 || k1 > 7 || k2 < 0 || k2 > 7 || l1 < 0 || l1 > 7 || l2 < 0 || l2 > 7) {
        printf("Position values not valid: they lie outside the board \n");
        goto P;
    }

    if (k1 > 7 || k2 > 7 || l1 > 7 || l2 > 7) {
			printf("Position values not valid: they lie outside the board \n");
            goto P;
		} else if (board[k1][k2] == 0) {
			printf("There is no piece in position [%d,%d] \n",k1,k2);
            goto P;
		} else {
			int pieceCode = board[k1][k2];
			if (turn == 0 && pieceCode >= 7) {
				printf("It's white's turn, can't move black pieces.\n");
                goto P;
			} else if (turn == 1 && pieceCode < 7) {
				printf("It's black's turn, can't move white pieces.\n");
                goto P;
			} else {
				int *possibleMoves = calculateMovesPiece(8, 8, board[0], k1, k2, pieceCode);
				int lengthMovesArr = possibleMoves[0] - 2;
				if (checkIfMoveIsIn(l1,l2,possibleMoves,lengthMovesArr)) {
					if (turn == 0 && lookForWhiteCheck(8,8,board[0])) {
						int temp[8][8];
						// implement copy function
						copyArray(board[0],temp[0],64);
						movePiece(k1,k2,l1,l2,temp[0],pieceCode, &turn);
						if (lookForWhiteCheck(8,8,temp[0])) {
							printf("You have to exit the check \n");
                            goto P;
							turn--;
						} else {
							turn--;
                            // Before movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);
                            if (board[l1][l2] != 0) {
                                if (turn == 0) {
                                    capturedByWhite[capturedWhiteCount++] = board[l1][l2];
                                } else {
                                    capturedByBlack[capturedBlackCount++] = board[l1][l2];
                                }
                            }
							movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);

						}
					} else if (turn == 1 && lookForBlackCheck(8,8,board[0])) {
						int temp[8][8];
						copyArray(board[0],temp[0],64);
						movePiece(k1,k2,l1,l2,temp[0],pieceCode, &turn);
						if (lookForBlackCheck(8,8,temp[0])) {
							printf("You have to exit the check \n");
							turn++;
                            goto P;
						} else {
							turn++;
                            // Before movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);
                            if (board[l1][l2] != 0) {
                                if (turn == 0) {
                                    capturedByWhite[capturedWhiteCount++] = board[l1][l2];
                                } else {
                                    capturedByBlack[capturedBlackCount++] = board[l1][l2];
                                }
                            }
							movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);

						}

					} else {
                        // Before movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);
                        if (board[l1][l2] != 0) {
                            if (turn == 0) {
                                capturedByWhite[capturedWhiteCount++] = board[l1][l2];
                            } else {
                                capturedByBlack[capturedBlackCount++] = board[l1][l2];
                            }
                        }
						movePiece(k1,k2,l1,l2,board[0],pieceCode, &turn);

					}
				} else {
					printf("The inserted move is not allowed!\n");
                    goto P;
				}
				free(possibleMoves);
			}
		}

    goto S;

Q:  close(newsockfd);
    close(sockfd);
    return 0;
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