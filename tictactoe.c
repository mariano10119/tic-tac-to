// tictactoe.c
//
// A two-player Tic-Tac-Toe game for the terminal.
//  - Player X and Player O take turns on a 3x3 board.
//  - A scoreboard tracks X wins, O wins, and draws.
//  - The scoreboard is saved to a file (scores.txt) so it persists
//    after the program is closed.
//
// Compile:  gcc -Wall -o tictactoe tictactoe.c
// Run:      ./tictactoe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------- Constants -----------------------------

#define BoardSize   3
#define ScoreFile   "scores.txt"
#define MaxUserIn   64

// ------------------------------ Types --------------------------------

// Scoreboard
// Holds the running totals across every game ever played.
typedef struct {
    int xWins;
    int oWins;
    int draws;
} Scoreboard;

// --------------------------------------------------------------------
// readLine
//
// Purpose:
//   Safely reads one line of text from the keyboard into a buffer.
//   Using fgets (instead of scanf) prevents bad input such as letters
//   from getting stuck in the input stream and causing infinite loops.
//
// Parameters:
//   buffer - the character array to store the line in.
//   size   - the size of that array.
//
// Returns:
//   1 if a line was read successfully,
//   0 if input ended (EOF, e.g. Ctrl+D), so callers can exit cleanly.
int readLine(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }
    // Remove the trailing newline if one was read.
    buffer[strcspn(buffer, "\n")] = '\0';
    return 1;
}

// --------------------------------------------------------------------
// loadScores
//
// Purpose:
//   Reads the saved scoreboard from ScoreFile so scores carry over
//   between runs of the program. If the file does not exist yet (first
//   time playing) or is damaged, the scoreboard starts at all zeros.
//
// Parameters:
//   sb - pointer to the Scoreboard that will be filled in.
//
// Returns:
//   Nothing. The Scoreboard is updated through the pointer.
void loadScores(Scoreboard *sb) {
    FILE *fp;

    sb->xWins = 0;
    sb->oWins = 0;
    sb->draws = 0;

    fp = fopen(ScoreFile, "r");
    if (fp == NULL) {
        return; // No saved file yet; keep the zeros.
    }

    if (fscanf(fp, "%d %d %d", &sb->xWins, &sb->oWins, &sb->draws) != 3) {
        // File was unreadable or corrupt; reset to zeros.
        sb->xWins = 0;
        sb->oWins = 0;
        sb->draws = 0;
    }

    fclose(fp);
}

// --------------------------------------------------------------------
// saveScores
//
// Purpose:
//   Writes the current scoreboard to ScoreFile, overwriting the old
//   contents. Called after every finished game so the scores are never
//   lost, even if the program is closed unexpectedly.
//
// Parameters:
//   sb - pointer to the Scoreboard to save.
//
// Returns:
//   Nothing. Prints a warning if the file could not be written.
void saveScores(const Scoreboard *sb) {
    FILE *fp = fopen(ScoreFile, "w");

    if (fp == NULL) {
        printf("Warning: could not save scores to %s.\n", ScoreFile);
        return;
    }

    fprintf(fp, "%d %d %d\n", sb->xWins, sb->oWins, sb->draws);
    fclose(fp);
}

// --------------------------------------------------------------------
// showScoreboard
//
// Purpose:
//   Displays the current scoreboard (X wins, O wins, draws, and total
//   games played) in a neat box.
//
// Parameters:
//   sb - pointer to the Scoreboard to display.
//
// Returns:
//   Nothing.
void showScoreboard(const Scoreboard *sb) {
    int total = sb->xWins + sb->oWins + sb->draws;

    printf("\n==============================\n");
    printf("          SCOREBOARD          \n");
    printf("==============================\n");
    printf("  Player X wins : %d\n", sb->xWins);
    printf("  Player O wins : %d\n", sb->oWins);
    printf("  Draws         : %d\n", sb->draws);
    printf("------------------------------\n");
    printf("  Games played  : %d\n", total);
    printf("==============================\n\n");
}

// --------------------------------------------------------------------
// initBoard
//
// Purpose:
//   Resets the board to an empty state at the start of a new game.
//   Every cell is filled with a space character ' ' meaning "empty".
//
// Parameters:
//   board - the 3x3 character array to clear.
//
// Returns:
//   Nothing.
void initBoard(char board[BoardSize][BoardSize]) {
    int r, c;

    for (r = 0; r < BoardSize; r++) {
        for (c = 0; c < BoardSize; c++) {
            board[r][c] = ' ';
        }
    }
}

// --------------------------------------------------------------------
// printBoard
//
// Purpose:
//   Draws the board in the terminal. Empty cells show their position
//   number (1-9) so the player knows which number to type to claim
//   that square. Filled cells show X or O.
//
// Parameters:
//   board - the 3x3 board to draw.
//
// Returns:
//   Nothing.
void printBoard(char board[BoardSize][BoardSize]) {
    int r, c;

    printf("\n");
    for (r = 0; r < BoardSize; r++) {
        printf(" ");
        for (c = 0; c < BoardSize; c++) {
            if (board[r][c] == ' ') {
                printf("%d", r * BoardSize + c + 1);
            } else {
                printf("%c", board[r][c]);
            }
            if (c < BoardSize - 1) {
                printf(" | ");
            }
        }
        printf("\n");
        if (r < BoardSize - 1) {
            printf("---+---+---\n");
        }
    }
    printf("\n");
}

// --------------------------------------------------------------------
// PlayerMove
//
// Purpose:
//   Tries to place a player's mark on the board at a position 1-9
//   (1 = top-left, 9 = bottom-right, counting left to right, top to
//   bottom). The move is only made if the position is in range and
//   the square is not already taken.
//
// Parameters:
//   board    - the 3x3 board to modify.
//   position - the square number the player chose (1-9).
//   player   - the mark to place, 'X' or 'O'.
//
// Returns:
//   1 if the move was placed successfully,
//   0 if the move was invalid (out of range or square already used).
int PlayerMove(char board[BoardSize][BoardSize], int position, char player) {
    int row, col;

    if (position < 1 || position > BoardSize * BoardSize) {
        return 0;
    }

    row = (position - 1) / BoardSize;
    col = (position - 1) % BoardSize;

    if (board[row][col] != ' ') {
        return 0;
    }

    board[row][col] = player;
    return 1;
}

// --------------------------------------------------------------------
// checkWinner
//
// Purpose:
//   Checks all 8 possible winning lines (3 rows, 3 columns, and
//   2 diagonals) to see if either player has three in a row.
//
// Parameters:
//   board - the 3x3 board to examine.
//
// Returns:
//   'X' if X has won,
//   'O' if O has won,
//   ' ' (space) if nobody has won yet.
char checkWinner(char board[BoardSize][BoardSize]) {
    int i;

    // Check each row and each column.
    for (i = 0; i < BoardSize; i++) {
        if (board[i][0] != ' ' &&
            board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
        if (board[0][i] != ' ' &&
            board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
            return board[0][i];
        }
    }

    // Check the two diagonals.
    if (board[1][1] != ' ') {
        if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
            return board[1][1];
        }
        if (board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
            return board[1][1];
        }
    }

    return ' ';
}

// --------------------------------------------------------------------
// isBoardFull
//
// Purpose:
//   Determines whether every square has been filled. Combined with
//   checkWinner, a full board with no winner means the game is a draw.
//
// Parameters:
//   board - the 3x3 board to examine.
//
// Returns:
//   1 if no empty squares remain,
//   0 if at least one empty square remains.
int isBoardFull(char board[BoardSize][BoardSize]) {
    int r, c;

    for (r = 0; r < BoardSize; r++) {
        for (c = 0; c < BoardSize; c++) {
            if (board[r][c] == ' ') {
                return 0;
            }
        }
    }
    return 1;
}

// --------------------------------------------------------------------
// getPlayerMove
//
// Purpose:
//   Asks the current player which square they want, and keeps asking
//   until they enter a valid, unused square number (1-9). Handles
//   non-numeric input without crashing.
//
// Parameters:
//   board  - the board (modified when a valid move is placed).
//   player - the player whose turn it is, 'X' or 'O'.
//
// Returns:
//   1 if a move was successfully made,
//   0 if input ended (EOF) and the game should be abandoned.
int getPlayerMove(char board[BoardSize][BoardSize], char player) {
    char input[MaxUserIn];
    char *end;
    long position;

    while (1) {
        printf("Player %c, choose a square (1-9): ", player);

        if (!readLine(input, MaxUserIn)) {
            return 0;
        }

        position = strtol(input, &end, 10);

        // Reject empty input or anything with leftover non-digits.
        if (end == input || *end != '\0') {
            printf("That's not a number. Try again.\n");
            continue;
        }

        if (PlayerMove(board, (int)position, player)) {
            return 1;
        }

        printf("Invalid move: pick an empty square from 1 to 9.\n");
    }
}

// --------------------------------------------------------------------
// playGame
//
// Purpose:
//   Runs one complete game of Tic-Tac-Toe. X always goes first and
//   players alternate turns. After each move the game checks for a
//   winner or a draw. When the game ends, the result is announced,
//   the scoreboard is updated, and the new scores are saved to disk.
//
// Parameters:
//   sb - pointer to the Scoreboard to update with the result.
//
// Returns:
//   Nothing. If input ends mid-game (EOF), the game is abandoned and
//   no score is recorded.
void playGame(Scoreboard *sb) {
    char board[BoardSize][BoardSize];
    char current = 'X';
    char winner = ' ';

    initBoard(board);
    printf("\n--- New Game: X goes first ---\n");

    while (1) {
        printBoard(board);

        if (!getPlayerMove(board, current)) {
            printf("\nInput closed. Game abandoned.\n");
            return;
        }

        winner = checkWinner(board);
        if (winner != ' ') {
            break;
        }
        if (isBoardFull(board)) {
            break;
        }

        // Switch to the other player.
        current = (current == 'X') ? 'O' : 'X';
    }

    printBoard(board);

    if (winner == 'X') {
        printf("*** Player X wins! ***\n");
        sb->xWins++;
    } else if (winner == 'O') {
        printf("*** Player O wins! ***\n");
        sb->oWins++;
    } else {
        printf("*** It's a draw! ***\n");
        sb->draws++;
    }

    saveScores(sb);
}

// --------------------------------------------------------------------
// showMenu
//
// Purpose:
//   Prints the main menu options to the screen.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
void showMenu(void) {
    printf("=========== TIC-TAC-TOE ===========\n");
    printf("  1) Play game\n");
    printf("  2) Check scoreboard\n");
    printf("  3) Close game\n");
    printf("===================================\n");
    printf("Choose an option: ");
}

// --------------------------------------------------------------------
// main
//
// Purpose:
//   Entry point. Loads the saved scoreboard, then repeatedly shows
//   the menu and handles the user's choice until they close the game:
//     case 1 - play a game of Tic-Tac-Toe
//     case 2 - display the scoreboard
//     case 3 - exit the program
//   Any other input shows an error and redisplays the menu.
//
// Returns:
//   0 on normal exit.
int main(void) {
    Scoreboard sb;
    char input[MaxUserIn];
    int choice;
    int running = 1;

    loadScores(&sb);

    while (running) {
        showMenu();

        if (!readLine(input, MaxUserIn)) {
            printf("\n");
            break; // Input closed; exit cleanly.
        }

        choice = atoi(input);

        switch (choice) {
            case 1:
                playGame(&sb);
                break;

            case 2:
                showScoreboard(&sb);
                break;

            case 3:
                saveScores(&sb);
                printf("Thanks for playing. Goodbye!\n");
                running = 0;
                break;

            default:
                printf("\nInvalid choice. Please enter 1, 2, or 3.\n\n");
                break;
        }
    }

    return 0;
}
