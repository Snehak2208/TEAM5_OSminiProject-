// treasure_hunt_client.c
//
// Multiplayer Treasure Hunt Game - Client
// Features:
// - Supports direction + steps (e.g., "left 4")
// - Robust input and real-time game state display
//
// Author: [Your Name]
// Date: [Current Date]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

// Global variables for socket and thread management
int sockfd;
pthread_t recv_thread;
volatile int can_play = 0;          // 1 if it's this client's turn to play
volatile int can_choose_team = 0;   // 1 if client can choose team

/**
 * Thread function to receive and display messages from the server.
 * Sets flags for team selection and turn prompts.
 */
void* receive_handler(void* arg) {
    char buffer[2048];
    while (1) {
        int r = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (r <= 0) {
            printf("Disconnected from server.\n");
            exit(0);
        }
        buffer[r] = 0;
        printf("%s", buffer);

        // Update state flags based on server prompts
        if (strstr(buffer, "Choose team")) {
            can_choose_team = 1;
        } else if (strstr(buffer, "Your turn")) {
            can_play = 1;
        } else if (strstr(buffer, "Game Over") || strstr(buffer, "Time's up")) {
            exit(0);
        }
    }
    return NULL;
}

/**
 * Main function.
 * Connects to the server, handles user input for team selection and moves,
 * and manages the receive thread for server communication.
 */
int main(int argc, char* argv[]) {
    struct sockaddr_in servaddr;
    char buffer[100];
    enum {TEAM_SELECTION, GAME} state = TEAM_SELECTION;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    // Convert and set server IP address from command line argument
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
      perror("Invalid IP address");
      exit(1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to the server.\n");

    // Start thread to handle incoming server messages
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    // Main loop: handle user input for team selection and moves
    while (1) {
        if (state == TEAM_SELECTION && can_choose_team) {
            printf("Enter team choice (A/B): ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(sockfd, buffer, strlen(buffer), 0);
            state = GAME;
            can_choose_team = 0;
        } else if (state == GAME && can_play) {
            printf("Enter move (e.g., up, left 4, right 2, down): ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(sockfd, buffer, strlen(buffer), 0);
            can_play = 0;
        }
        usleep(100000); // Sleep briefly to reduce CPU usage
    }

    // Wait for receive thread to finish (should not reach here)
    pthread_join(recv_thread, NULL);
    close(sockfd); // Close socket on exit
    return 0;
}
