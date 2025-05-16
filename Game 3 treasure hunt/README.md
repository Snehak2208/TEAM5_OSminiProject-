# Treasure Hunt Game

A modular, text-based multiplayer treasure hunt game for two teams, featuring hidden treasures, visited markers, and flexible movement commands.


<img src="./Treasure Hunt.jpg" alt="Modular flow" >

---

## Features

- **2 Teams (A & B):** Each with 2 players, for a total of 4 players per game.
- **10x10 Grid:** Treasures are hidden until found. Players see only their positions, visited cells, and found treasures.
- **Flexible Movement:** Players enter moves as `direction steps` (e.g., `left 4`). If steps are omitted, defaults to 1.
- **Visited Markers:** Cells visited without finding a treasure are marked `V`.
- **Found Treasures:** Revealed and marked as `T` on the grid.
- **Bounded Movement:** If a player tries to move too far, they move as far as possible.
- **Invalid Input Handling:** Invalid commands (e.g., typos) display an error and skip the turn.
- **Live Score Updates:** Scores are updated and broadcast after every move.
- **Turn-based Play:** Only one player can move at a time.

---

## How to Play

1. **Start the Server:**  
   Run the server on your host machine.
   ```bash
   gcc treasure_hunt_server.c -o server -lpthread
   ./server
   ```
   > **Note:** Edit the server IP in the code (`servaddr.sin_addr.s_addr = inet_addr("YOUR_IP_HERE");`) to match your network setup.

2. **Start Clients (Players):**  
   Each player runs the client and connects to the server's IP.
   ```bash
   gcc treasure_hunt_client.c -o client -lpthread
   ./client <server-ip>
   ```

3. **Choose Team:**  
   When prompted, enter `A` or `B` to join a team.

4. **Gameplay:**  
   - On your turn, enter a move like `left 3`, `up`, or `right 2`.
   - If you enter an invalid direction or format, your turn is skipped.
   - The grid updates after every move, showing:
     - `A1`, `A2`, `B1`, `B2`: Player positions
     - `.`: Unvisited cell
     - `V`: Visited cell (no treasure found)
     - `T`: Found treasure

5. **Winning:**  
   The game ends when all treasures are found. The team with the most treasures wins!

---

## Example Output

```
 .  .  .  .  .  .  .  .  .  .     Scores: A=1, B=0
 .  .  .  .  .  .  .  .  .  .
 .  .  .  .  .  .  .  .  .  .
 .  .  .  .  .  .  .  .  .  .
 . B2  .  .  .  .  .  .  .  .
B1  .  .  .  .  .  T A1  .  .
 .  .  .  .  .  .  .  .  V  .
 .  .  .  .  .  .  .  .  .  .
 T  .  .  .  .  .  .  .  .  .
 .  .  . A2  .  .  .  .  .  .
Your turn: Player A1
Enter move (e.g., up, left 4, right 2, down):
```

---

## Controls

- **Move:**  
  `up [steps]`  
  `down [steps]`  
  `left [steps]`  
  `right [steps]`  
  (e.g., `left 3`, `up`)

- **Rules:**  
  - If steps are omitted, moves 1 step.
  - If steps exceed available space, moves as far as possible.
  - Invalid commands (e.g., `lef 2`, `jump`) skip your turn.

---

## Network Configuration

- **Server IP:**  
  Change the IP in `server.c` to the host machine's IP or use `INADDR_ANY` for local network play.
- **Port:**  
  Default is `8080`. Ensure this port is open on your firewall.

---

## Code Structure

- **treasure_hunt_server.c:**  
  Handles connections, team assignment, grid state, turn management, and broadcasting updates.

- **treasure_hunt_client.c:**  
  Handles user input, sends moves to server, and displays grid and messages.

---

## Customization

- **Grid Size & Treasures:**  
  Change `GRID_SIZE` and `MAX_TREASURES` in the code to adjust difficulty.

---
## Credits

Developed by Nirvan Jain.  
Inspired by classic grid-based multiplayer games.

---

**Enjoy the hunt!** 🏴‍☠️
