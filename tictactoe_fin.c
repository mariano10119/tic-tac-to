// tictactoe.c
//
// A two-player Tic-Tac-Toe game for the terminal.
//  - Each game, the two players enter their names. X goes first.
//  - A scoreboard tracks every named player's wins as X and wins as O,
//    plus the total number of draws.
//  - The scoreboard is saved to a file (scores.txt) so it persists
//    after the program is closed.
//
// Compile:  gcc -Wall -o tictactoe tictactoe.c
// Run:      ./tictactoe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ----------------------------- Constants -----------------------------

#define BoardSize   3
#define ScoreFile   "scores.txt"
#define MaxUserIn   64
#define MaxName     32
#define MaxPlayers  100

// ------------------------------ Types --------------------------------

// Player
// One person's record: their name and how many games they have won
// while playing as X and while playing as O.
typedef struct {
    char name[MaxName];
    int winsAsX;
    int winsAsO;
} Player;

// Scoreboard
// Holds every known player, how many players there are, and the total
// number of drawn games across all players.
typedef struct {
    Player players[MaxPlayers];
    int count;
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
// trimSpaces
//
// Purpose:
//   Removes spaces and tabs from the start and end of a string, in
//   place. This keeps "  Bob " and "Bob" from becoming two different
//   players.
//
// Parameters:
//   text - the string to trim (modified directly).
//
// Returns:
//   Nothing.
void trimSpaces(char *text) {
    int start = 0;
    int end = (int)strlen(text);

    // Find the first character that is not whitespace.
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }
    // Walk the end backwards past trailing whitespace.
    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }

    // Shift the trimmed text to the front and terminate it.
    memmove(text, text + start, end - start);
    text[end - start] = '\0';
}

// --------------------------------------------------------------------
// namesMatch
//
// Purpose:
//   Compares two names ignoring upper/lower case, so "bob" and "Bob"
//   count as the same player.
//
// Parameters:
//   a - the first name.
//   b - the second name.
//
// Returns:
//   1 if the names are the same ignoring case,
//   0 if they are different.
int namesMatch(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b; // Both must have ended at the same time.
}

// --------------------------------------------------------------------
// loadScores
//
// Purpose:
//   Reads the saved scoreboard from ScoreFile so scores carry over
//   between runs of the program. The file layout is:
//     line 1:      Draws: <number>
//     other lines: <winsAsX> <winsAsO> <player name>
//   If the file does not exist yet (first time playing) or the first
//   line is not in this format, the scoreboard starts empty. Bad
//   player lines are skipped.
//
// Parameters:
//   sb - pointer to the Scoreboard that will be filled in.
//
// Returns:
//   Nothing. The Scoreboard is updated through the pointer.
void loadScores(Scoreboard *sb) {
    FILE *fp;
    char line[MaxUserIn + MaxName];
    int winsX, winsO, offset;

    sb->count = 0;
    sb->draws = 0;

    fp = fopen(ScoreFile, "r");
    if (fp == NULL) {
        return; // No saved file yet; start empty.
    }

    // First line holds the draw total.
    if (fgets(line, sizeof(line), fp) == NULL ||
        sscanf(line, "Draws: %d", &sb->draws) != 1) {
        sb->draws = 0;
        fclose(fp);
        return; // Unknown or old file format; start empty.
    }

    // Remaining lines each hold one player.
    while (sb->count < MaxPlayers && fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (sscanf(line, "%d %d %n", &winsX, &winsO, &offset) != 2) {
            continue; // Skip damaged lines.
        }
        if (line[offset] == '\0' || strlen(line + offset) >= MaxName) {
            continue; // Missing or too-long name.
        }

        strcpy(sb->players[sb->count].name, line + offset);
        sb->players[sb->count].winsAsX = winsX;
        sb->players[sb->count].winsAsO = winsO;
        sb->count++;
    }

    fclose(fp);
}

// --------------------------------------------------------------------
// saveScores
//
// Purpose:
//   Writes the current scoreboard to ScoreFile, overwriting the old
//   contents, using the layout described in loadScores. Called after
//   every finished game so the scores are never lost, even if the
//   program is closed unexpectedly.
//
// Parameters:
//   sb - pointer to the Scoreboard to save.
//
// Returns:
//   Nothing. Prints a warning if the file could not be written.
void saveScores(const Scoreboard *sb) {
    FILE *fp = fopen(ScoreFile, "w");
    int i;

    if (fp == NULL) {
        printf("Warning: could not save scores to %s.\n", ScoreFile);
        return;
    }

    fprintf(fp, "Draws: %d\n", sb->draws);
    for (i = 0; i < sb->count; i++) {
        fprintf(fp, "%d %d %s\n", sb->players[i].winsAsX,
                sb->players[i].winsAsO, sb->players[i].name);
    }

    fclose(fp);
}

// --------------------------------------------------------------------
// findPlayer
//
// Purpose:
//   Looks through the scoreboard for a player with the given name
//   (ignoring upper/lower case).
//
// Parameters:
//   sb   - pointer to the Scoreboard to search.
//   name - the name to look for.
//
// Returns:
//   The index of the player in sb->players if found,
//   -1 if there is no player with that name.
int findPlayer(const Scoreboard *sb, const char *name) {
    int i;

    for (i = 0; i < sb->count; i++) {
        if (namesMatch(sb->players[i].name, name)) {
            return i;
        }
    }
    return -1;
}

// --------------------------------------------------------------------
// findOrAddPlayer
//
// Purpose:
//   Returns the scoreboard index for a name. If the player is new,
//   they are added with 0 wins as X and 0 wins as O.
//
// Parameters:
//   sb   - pointer to the Scoreboard to search and possibly extend.
//   name - the player's name.
//
// Returns:
//   The index of the player in sb->players,
//   -1 if the player is new but the scoreboard is already full.
int findOrAddPlayer(Scoreboard *sb, const char *name) {
    int index = findPlayer(sb, name);

    if (index != -1) {
        return index;
    }
    if (sb->count >= MaxPlayers) {
        return -1;
    }

    index = sb->count;
    strcpy(sb->players[index].name, name);
    sb->players[index].winsAsX = 0;
    sb->players[index].winsAsO = 0;
    sb->count++;
    return index;
}

// --------------------------------------------------------------------
// showScoreboard
//
// Purpose:
//   Displays the scoreboard as a table with one row per player:
//     [name]  [wins playing X]  [wins playing O]
//   followed by the total number of draws. If nobody has played yet,
//   a short message is shown instead.
//
// Parameters:
//   sb - pointer to the Scoreboard to display.
//
// Returns:
//   Nothing.
void showScoreboard(const Scoreboard *sb) {
    int i;

    printf("\n=====================================================\n");
    printf("                      SCOREBOARD                     \n");
    printf("=====================================================\n");

    if (sb->count == 0) {
        printf("  No games played yet.\n");
    } else {
        printf("  %-*s  %10s  %10s\n", MaxName - 1, "Name", "Wins as X", "Wins as O");
        printf("  %-*s  %10s  %10s\n", MaxName - 1, "----", "---------", "---------");
        for (i = 0; i < sb->count; i++) {
            printf("  %-*s  %10d  %10d\n", MaxName - 1, sb->players[i].name,
                   sb->players[i].winsAsX, sb->players[i].winsAsO);
        }
    }

    printf("-----------------------------------------------------\n");
    printf("  Draws: %d\n", sb->draws);
    printf("=====================================================\n\n");
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
// getPlayerName
//
// Purpose:
//   Asks for a player's name and keeps asking until the name is
//   usable: not empty (after trimming spaces) and short enough to fit
//   in a Player's name field.
//
// Parameters:
//   prompt - the question to display (e.g. "Player X name: ").
//   name   - a character array of at least MaxName characters where
//            the cleaned-up name is stored.
//
// Returns:
//   1 if a valid name was entered,
//   0 if input ended (EOF) and the game should be abandoned.
int getPlayerName(const char *prompt, char *name) {
    char input[MaxUserIn];

    while (1) {
        printf("%s", prompt);

        if (!readLine(input, MaxUserIn)) {
            return 0;
        }

        trimSpaces(input);

        if (input[0] == '\0') {
            printf("Name can't be empty. Try again.\n");
            continue;
        }
        if (strlen(input) >= MaxName) {
            printf("Name is too long (max %d characters). Try again.\n", MaxName - 1);
            continue;
        }

        strcpy(name, input);
        return 1;
    }
}

// --------------------------------------------------------------------
// choosePlayers
//
// Purpose:
//   Asks who is playing X and who is playing O, makes sure they are
//   two different people, and finds (or creates) each person's entry
//   on the scoreboard.
//
// Parameters:
//   sb     - pointer to the Scoreboard holding all players.
//   xIndex - where to store the scoreboard index of the X player.
//   oIndex - where to store the scoreboard index of the O player.
//
// Returns:
//   1 if both players were chosen,
//   0 if the game should be abandoned (input ended, or the scoreboard
//   has no room for a new player).
int choosePlayers(Scoreboard *sb, int *xIndex, int *oIndex) {
    char xName[MaxName];
    char oName[MaxName];

    while (1) {
        if (!getPlayerName("Player X name: ", xName)) {
            return 0;
        }
        if (!getPlayerName("Player O name: ", oName)) {
            return 0;
        }

        if (namesMatch(xName, oName)) {
            printf("The two players need different names. Try again.\n");
            continue;
        }
        break;
    }

    *xIndex = findOrAddPlayer(sb, xName);
    *oIndex = findOrAddPlayer(sb, oName);

    if (*xIndex == -1 || *oIndex == -1) {
        printf("The scoreboard is full (%d players). Can't add a new player.\n", MaxPlayers);
        return 0;
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
//   player - the mark of the player whose turn it is, 'X' or 'O'.
//   name   - that player's name, shown in the prompt.
//
// Returns:
//   1 if a move was successfully made,
//   0 if input ended (EOF) and the game should be abandoned.
int getPlayerMove(char board[BoardSize][BoardSize], char player, const char *name) {
    char input[MaxUserIn];
    char *end;
    long position;

    while (1) {
        printf("%s (%c), choose a square (1-9): ", name, player);

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
//   Runs one complete game of Tic-Tac-Toe. First the two players
//   enter their names. X always goes first and players alternate
//   turns. After each move the game checks for a winner or a draw.
//   When the game ends, the result is announced, the winner's win
//   count for the mark they played (X or O) goes up (or the draw
//   total goes up), and the scoreboard is saved to disk.
//
// Parameters:
//   sb - pointer to the Scoreboard to update with the result.
//
// Returns:
//   Nothing. If input ends before the game finishes (EOF), the game
//   is abandoned and no score is recorded.
void playGame(Scoreboard *sb) {
    char board[BoardSize][BoardSize];
    char current = 'X';
    char winner = ' ';
    int xIndex, oIndex;
    int currentIndex;

    printf("\n--- New Game ---\n");
    if (!choosePlayers(sb, &xIndex, &oIndex)) {
        printf("\nGame cancelled.\n");
        return;
    }

    initBoard(board);
    printf("\n%s is X and goes first. %s is O.\n",
           sb->players[xIndex].name, sb->players[oIndex].name);

    while (1) {
        printBoard(board);

        currentIndex = (current == 'X') ? xIndex : oIndex;
        if (!getPlayerMove(board, current, sb->players[currentIndex].name)) {
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
        printf("*** %s wins as X! ***\n", sb->players[xIndex].name);
        sb->players[xIndex].winsAsX++;
    } else if (winner == 'O') {
        printf("*** %s wins as O! ***\n", sb->players[oIndex].name);
        sb->players[oIndex].winsAsO++;
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
    static Scoreboard sb; // static: the player table is fairly large.
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
