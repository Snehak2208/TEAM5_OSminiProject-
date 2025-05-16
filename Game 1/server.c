/**
 * @file server.c
 * @brief Two-player number guessing game server implementation
 * 
 * This server implements a number guessing game where two players compete to guess
 * a number. Players take turns guessing, and scores are calculated based on how
 * close their guesses are to the target number.
 * 
 * @author Sneha Kumari
 * @date 2025-05-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

/** Port number for the server */
#define PORT 8080
/** Maximum number of turns per player */
#define MAX_TURNS 3

/** Array to store socket descriptors for both players */
int player_sockets[2];
/** Array to store scores for both players */
int player_scores[2] = {0, 0};
/** The number players need to guess */
int number_to_guess;
/** Winner of the game (-1 if no winner yet) */
int winner = -1;

/**
 * @brief Calculates the score based on how close the guess is to the actual number
 * 
 * @param guess The player's guessed number
 * @param actual The actual number to guess
 * @return int Score based on the difference (100 for exact match, scaled down for larger differences)
 */
int calculate_score(int guess, int actual) {
    int diff = abs(guess - actual);
    if (diff == 0) return 100;      // Perfect guess
    else if (diff <= 10) return 50;  // Very close
    else if (diff <= 20) return 45;
    else if (diff <= 30) return 40;
    else if (diff <= 40) return 35;
    else if (diff <= 50) return 30;
    else if (diff <= 60) return 25;
    else if (diff <= 70) return 20;
    else if (diff <= 80) return 15;
    else if (diff <= 90) return 10;
    else return 5;                   // Very far
}

/**
 * @brief Sends the current scorecard to both players
 * 
 * Formats and sends the current scores of both players in a visually appealing way
 */
void send_scorecard() {
    char scorecard[256];
    snprintf(scorecard, sizeof(scorecard),
        "\n========= SCORECARD =========\n"
        "Player 1: %d\n"
        "Player 2: %d\n"
        "=============================\n\n",
        player_scores[0], player_scores[1]);

    for (int i = 0; i < 2; i++) {
        send(player_sockets[i], scorecard, strlen(scorecard), 0);
    }
}

/**
 * @brief Main game logic handler running in a separate thread
 * 
 * Manages the game flow including:
 * - Player turns
 * - Score calculation
 * - Winner determination
 * - Game end conditions
 * 
 * @param arg Thread argument (unused)
 * @return void* NULL
 */
void* handle_game(void* arg) {
    for (int turn = 0; turn < MAX_TURNS && winner == -1; turn++) {
        for (int i = 0; i < 2 && winner == -1; i++) {
            char prompt[] = "Your turn to guess: ";
            send(player_sockets[i], prompt, strlen(prompt), 0);

            int guess;
            read(player_sockets[i], &guess, sizeof(guess));

            int score = calculate_score(guess, number_to_guess);
            player_scores[i] += score;

            if (score == 100) {
                winner = i;
            }

            char feedback[100];
            snprintf(feedback, sizeof(feedback),
                "You guessed %d. Score this turn: %d. Total: %d\n",
                guess, score, player_scores[i]);
            send(player_sockets[i], feedback, strlen(feedback), 0);

            send_scorecard();
        }
    }

    char end_msg[256];
    if (winner != -1) {
        snprintf(end_msg, sizeof(end_msg),
            "\nGame Over 🏁. Player %d guessed the correct number and wins with 100 points! 🏆\n",
            winner + 1);
    } else {
        if (player_scores[0] > player_scores[1]) {
            snprintf(end_msg, sizeof(end_msg),
                "\nGame Over 🏁. No one guessed the correct number 😿.\nBut Player 1 wins by score: %d 🏆\n",
                player_scores[0]);
        } else if (player_scores[1] > player_scores[0]) {
            snprintf(end_msg, sizeof(end_msg),
                "\nGame Over 🏁. No one guessed the correct number 😿.\nBut Player 2 wins by score: %d 🏆\n",
                player_scores[1]);
        } else {
            snprintf(end_msg, sizeof(end_msg),
                "\nGame Over 🏁. It's a draw! Both players scored %d 🤝\n",
                player_scores[0]);
        }
    }

    for (int i = 0; i < 2; i++) {
        send(player_sockets[i], end_msg, strlen(end_msg), 0);
        close(player_sockets[i]);
    }
    return NULL;
}

/**
 * @brief Main function - Server initialization and player connection handling
 * 
 * Sets up the server socket, waits for both players to connect, and starts the game.
 * The server:
 * 1. Creates a TCP socket
 * 2. Binds to the specified port
 * 3. Waits for 2 players to connect
 * 4. Starts the game thread
 * 
 * @return int 0 on successful execution, -1 on error
 */
int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    srand(time(0));
    printf("Enter number to guess ⌨️ : ");
    scanf("%d",&number_to_guess);
    printf("[Server] Number to guess is: %d 😸\n", number_to_guess);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 2);
    printf("[Server] Waiting for players to connect...⏳\n");

    for (int i = 0; i < 2; i++) {
        player_sockets[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        char msg[64];
        snprintf(msg, sizeof(msg), "Welcome Player %d 🥳!\n", i + 1);
        send(player_sockets[i], msg, strlen(msg), 0);
    }

    pthread_t game_thread;
    pthread_create(&game_thread, NULL, handle_game, NULL);
    pthread_join(game_thread, NULL);

    close(server_fd);
    return 0;
}

