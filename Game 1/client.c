/**
 * @file client.c
 * @brief Client implementation for the two-player number guessing game
 * 
 * This client connects to the game server and allows a player to participate
 * in the number guessing game. It handles:
 * - Server connection
 * - User input for guesses
 * - Display of game progress and results
 * 
 * @author Sneha Kumari
 * @date 2025-05-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

/** Port number to connect to server */
#define PORT 8080
/** Server IP address - Change this to match your server's IP */
#define SERVER_IP "192.168.1.5"

/**
 * @brief Main function - Handles client connection and game interaction
 * 
 * The client:
 * 1. Creates a socket and connects to the server
 * 2. Enters a game loop where it:
 *    - Receives messages from server
 *    - Prompts for user input when it's their turn
 *    - Sends guesses to the server
 *    - Displays game progress and results
 * 
 * @return int 0 on successful execution, -1 on error
 */
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported ❌\n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed 🚫.\n");
        return -1;
    }

    // Main game loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        
        // Check for server disconnection
        if (valread <= 0) {
            break;
        }

        // Display server message
        printf("%s", buffer);

        // Check if it's our turn to guess
        if (strstr(buffer, "Your turn to guess")) {
            int guess;
            printf("Enter your guess 🫣: ");
            scanf("%d", &guess);
            send(sock, &guess, sizeof(guess), 0);
        }

        // Check for game end
        if (strstr(buffer, "Game Over")) {
            break;
        }
    }

    close(sock);
    return 0;
}

