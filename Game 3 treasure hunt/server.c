// treasure_hunt_server.c
//
// Multiplayer Treasure Hunt Game - Server
// Features:
// - Multi-step moves (e.g., "left 4")
// - Hidden treasures, revealed only when found
// - Visited markers for explored cells
// - Robust input parsing and turn management
//
// Author: [Your Name]
// Date: [Current Date]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#define PORT 8080
#define GRID_SIZE 10
#define MAX_TREASURES 10
#define MAX_PLAYERS 4

/**
 * Represents a single cell on the grid.
 * - treasure: 1 if treasure is hidden here, 0 otherwise
 * - treasure_found: 1 if treasure at this cell has been found
 * - player_id: Player index if occupied, -1 if empty
 * - visited: 1 if visited by any player and no treasure found
 */
typedef struct {
    int treasure;
    int treasure_found;
    int player_id;
    int visited;
} Cell;

/**
 * Represents a player in the game.
 * - id: Player index (0-3)
 * - x, y: Current grid coordinates
 * - team: 0 for Team A, 1 for Team B
 * - sockfd: Player's socket file descriptor
 * - turn_index: Player's turn order (0-3)
 * - team_number: 1 or 2 (A1/A2 or B1/B2)
 * - thread: Thread handling this player
 */
typedef struct {
    int id, x, y, team, sockfd, turn_index, team_number;
    pthread_t thread;
} Player;

// Global game state
Cell grid[GRID_SIZE][GRID_SIZE];
Player players[MAX_PLAYERS];
int player_count = 0;
int team_counts[2] = {0, 0}; // Number of players per team
int score[2] = {0};          // Team scores: [A, B]
int treasures_left = MAX_TREASURES;
int current_turn = 0;
pthread_mutex_t lock;        // Mutex for synchronizing game state

/**
 * Trims leading and trailing whitespace from a string (in-place).
 */
void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace(str[len-1])) str[--len] = 0;
    int i = 0;
    while (isspace(str[i])) i++;
    if (i > 0) memmove(str, str+i, len-i+1);
}

/**
 * Randomly places MAX_TREASURES treasures on the grid.
 * Ensures each treasure is placed in a unique cell.
 */
void place_treasures() {
    srand(time(NULL));
    for (int i = 0; i < MAX_TREASURES;) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;
        if (!grid[x][y].treasure) {
            grid[x][y].treasure = 1;
            grid[x][y].treasure_found = 0;
            i++;
        }
    }
}

/**
 * Places a player at a random empty cell (no player, no treasure).
 * Updates the player's coordinates and the grid.
 */
void place_player(Player *p) {
    while (1) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;
        if (grid[x][y].player_id == -1 && grid[x][y].treasure == 0) {
            p->x = x;
            p->y = y;
            grid[x][y].player_id = p->id;
            break;
        }
    }
}

/**
 * Sends a message to a specific player via their socket.
 */
void send_to_player(Player *p, const char *msg) {
    send(p->sockfd, msg, strlen(msg), 0);
}

/**
 * Broadcasts a message to all connected players.
 */
void broadcast(const char *msg) {
    for (int i = 0; i < player_count; i++) {
        send_to_player(&players[i], msg);
    }
}

/**
 * Sends the current state of the grid to all players.
 * - Player positions: A1, A2, B1, B2
 * - Visited cells: V
 * - Found treasures: T
 * - Unvisited cells: .
 */
void send_grid_to_all() {
    char buffer[4096] = "\n";
    for (int i = 0; i < GRID_SIZE; i++) {
        char row_buffer[400] = "";
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j].player_id != -1) {
                Player *p = &players[grid[i][j].player_id];
                char label[4];
                sprintf(label, "%c%d", p->team == 0 ? 'A' : 'B', p->team_number);
                strcat(row_buffer, label);
            } else if (grid[i][j].treasure_found) {
                strcat(row_buffer, " T");
            } else if (grid[i][j].visited) {
                strcat(row_buffer, " V");
            } else {
                strcat(row_buffer, " .");
            }
            strcat(row_buffer, " ");
        }
        if (i == 0) {
            char score_part[100];
            sprintf(score_part, "    Scores: A=%d, B=%d", score[0], score[1]);
            strcat(row_buffer, score_part);
        }
        strcat(buffer, row_buffer);
        strcat(buffer, "\n");
    }
    broadcast(buffer);
}

/**
 * Notifies all players whose turn it is.
 * The current player is prompted for a move; others are told to wait.
 */
void notify_turn() {
    for (int i = 0; i < player_count; i++) {
        if (i == current_turn) {
            char msg[50];
            sprintf(msg, "Your turn: Player %c%d\n",
                    players[i].team == 0 ? 'A' : 'B', players[i].team_number);
            send_to_player(&players[i], msg);
        } else {
            send_to_player(&players[i], "Waiting for your turn...\n");
        }
    }
}

/**
 * Parses a move command from the client.
 * Accepts: direction [steps]
 * Returns 1 if valid, 0 if invalid. Fills direction and steps.
 */
int parse_move(const char *input, char *direction, int *steps) {
    char buf[100];
    strncpy(buf, input, 99);
    buf[99] = 0;
    trim(buf);

    char *tok = strtok(buf, " ");
    if (!tok) return 0;
    strcpy(direction, tok);

    tok = strtok(NULL, " ");
    if (tok) {
        // Check if tok is a number
        for (int i = 0; tok[i]; i++)
            if (!isdigit(tok[i])) return 0;
        *steps = atoi(tok);
        if (*steps <= 0) *steps = 1;
    } else {
        *steps = 1;
    }
    return 1;
}

/**
 * Thread function for handling a single player's gameplay.
 * Listens for move commands, updates the grid, manages scoring and turn order.
 */
void* player_handler(void* arg) {
    Player *p = (Player*) arg;
    char buf[100];
    while (1) {
        int r = recv(p->sockfd, buf, sizeof(buf)-1, 0);
        if (r <= 0) break;
        buf[r] = 0;
        trim(buf);

        pthread_mutex_lock(&lock);
        if (p->turn_index != current_turn) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        char direction[16];
        int steps;
        int valid = parse_move(buf, direction, &steps);

        int nx = p->x, ny = p->y;
        int moved = 0, found_treasure = 0;

        if (valid) {
            // Convert direction to lower case
            for (int i = 0; direction[i]; i++) direction[i] = tolower(direction[i]);
            if (strcmp(direction, "up") == 0) {
                int move = steps > nx ? nx : steps;
                for (int i = 1; i <= move; i++) {
                    if (grid[nx-i][ny].player_id == -1) moved++;
                    else break;
                }
                nx -= moved;
            } else if (strcmp(direction, "down") == 0) {
                int move = steps > (GRID_SIZE-1 - nx) ? (GRID_SIZE-1 - nx) : steps;
                for (int i = 1; i <= move; i++) {
                    if (grid[nx+i][ny].player_id == -1) moved++;
                    else break;
                }
                nx += moved;
            } else if (strcmp(direction, "left") == 0) {
                int move = steps > ny ? ny : steps;
                for (int i = 1; i <= move; i++) {
                    if (grid[nx][ny-i].player_id == -1) moved++;
                    else break;
                }
                ny -= moved;
            } else if (strcmp(direction, "right") == 0) {
                int move = steps > (GRID_SIZE-1 - ny) ? (GRID_SIZE-1 - ny) : steps;
                for (int i = 1; i <= move; i++) {
                    if (grid[nx][ny+i].player_id == -1) moved++;
                    else break;
                }
                ny += moved;
            } else {
                valid = 0;
            }
        }

        if (!valid) {
            send_to_player(p, "Invalid move! Turn skipped.\n");
        } else if (moved == 0) {
            send_to_player(p, "Move blocked! Turn skipped.\n");
        } else {
            // Vacate old cell
            grid[p->x][p->y].player_id = -1;

            // If no treasure found at old cell, mark as visited
            if (!grid[p->x][p->y].treasure_found && !grid[p->x][p->y].treasure)
                grid[p->x][p->y].visited = 1;

            // Move player
            p->x = nx; p->y = ny;

            // Check for treasure
            if (grid[nx][ny].treasure && !grid[nx][ny].treasure_found) {
                grid[nx][ny].treasure_found = 1;
                grid[nx][ny].treasure = 0;
                score[p->team]++;
                treasures_left--;
                found_treasure = 1;
                char msg[100];
                sprintf(msg, "Team %c found a treasure!\n", p->team == 0 ? 'A' : 'B');
                broadcast(msg);
            } else {
                grid[nx][ny].visited = 1;
            }
            grid[nx][ny].player_id = p->id;
        }

        send_grid_to_all();
        current_turn = (current_turn + 1) % player_count;
        notify_turn();

        // End game if all treasures found
        if (treasures_left == 0) {
            char result[100];
            sprintf(result, "Game Over! Final Score: A=%d B=%d\n", score[0], score[1]);
            broadcast(result);
            exit(0);
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

/**
 * Thread function to handle a new client connection.
 * Assigns team, places player, and starts their game thread.
 */
void* setup_handler(void *arg) {
    int sockfd = *(int*)arg;
    free(arg);

    pthread_mutex_lock(&lock);
    if (player_count >= MAX_PLAYERS) {
        const char *msg = "Server is full. Disconnecting.\n";
        send(sockfd, msg, strlen(msg), 0);
        close(sockfd);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    pthread_mutex_unlock(&lock);

    int a_remaining = 2 - team_counts[0];
    int b_remaining = 2 - team_counts[1];
    char msg[100];
    sprintf(msg, "Choose team (A/B). A has %d spots left, B has %d spots left: ", a_remaining, b_remaining);
    send(sockfd, msg, strlen(msg), 0);

    char choice[2];
    int r = recv(sockfd, choice, sizeof(choice), 0);
    if (r <= 0) {
        close(sockfd);
        return NULL;
    }
    choice[r] = '\0';

    int chosen_team = -1;
    if (choice[0] == 'A' || choice[0] == 'a') {
        chosen_team = 0;
    } else if (choice[0] == 'B' || choice[0] == 'b') {
        chosen_team = 1;
    } else {
        if (team_counts[0] < 2) chosen_team = 0;
        else if (team_counts[1] < 2) chosen_team = 1;
        else {
            const char *error_msg = "No teams available. Disconnecting.\n";
            send(sockfd, error_msg, strlen(error_msg), 0);
            close(sockfd);
            return NULL;
        }
    }

    pthread_mutex_lock(&lock);
    if (team_counts[chosen_team] >= 2) {
        int other_team = 1 - chosen_team;
        if (team_counts[other_team] < 2) chosen_team = other_team;
        else {
            const char *error_msg = "No teams available. Disconnecting.\n";
            send(sockfd, error_msg, strlen(error_msg), 0);
            close(sockfd);
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }

    team_counts[chosen_team]++;
    Player *p = &players[player_count];
    p->id = player_count;
    p->sockfd = sockfd;
    p->team = chosen_team;
    p->team_number = team_counts[chosen_team];
    p->turn_index = player_count;

    place_player(p);
    pthread_create(&p->thread, NULL, player_handler, p);

    player_count++;

    if (player_count == MAX_PLAYERS) {
        send_grid_to_all();
        notify_turn();
    }

    pthread_mutex_unlock(&lock);
    return NULL;
}

/**
 * Main function.
 * Sets up the server socket, initializes the grid, places treasures,
 * and listens for player connections.
 */
int main() {
    int sockfd, newsock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    // Set this to your server's IP or use INADDR_ANY for local network
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_addr.s_addr = inet_addr("172.22.148.76");
    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(sockfd, MAX_PLAYERS);

    pthread_mutex_init(&lock, NULL);
    // Initialize grid cells
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid[i][j].player_id = -1, grid[i][j].visited = 0, grid[i][j].treasure_found = 0, grid[i][j].treasure = 0;

    place_treasures();

    printf("Server started. Waiting for players...\n");
    fflush(stdout);

    // Accept client connections and spawn setup handler threads
    while (1) {
        newsock = accept(sockfd, (struct sockaddr*)&cliaddr, &len);
        if (newsock < 0) {
            perror("Accept failed");
            continue;
        }
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = newsock;
        pthread_t thread;
        pthread_create(&thread, NULL, setup_handler, sock_ptr);
    }

    // Join all player threads before exit (not reached in normal operation)
    for (int i = 0; i < player_count; i++)
        pthread_join(players[i].thread, NULL);
    close(sockfd);
    return 0;
}
