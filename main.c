#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#define MAX_SIZE 6
#define TILE_SIZE 70
#define GAP 5
#define WINDOW_W 620    // Window width
#define WINDOW_H 670    // window height

int SIZE = 4; //  (default) board size
int board[MAX_SIZE][MAX_SIZE], undoBoard[MAX_SIZE][MAX_SIZE];
int score = 0, prevScore = 0, highScore = 0;
int theme = 0; // 0=light, 1=dark

// Theme palettes and UI colors 
Color tileLight[] = {
    {205,193,180,255},{238,228,218,255},{237,224,200,255},
    {242,177,121,255},{245,149,99,255},{246,124,95,255},{246,94,59,255}
};
Color tileDark[]  = {
    {60,60,80,255},{110,100,130,255},{130,115,100,255},
    {175,130,45,255},{130,100,40,255},{160,75,30,255},{155,45,45,255}
};
Color textCol[2]   = {BLACK, RAYWHITE};
Color borderCol[2] = {GRAY, WHITE};
Color bgCol[2]     = {RAYWHITE, (Color){35,36,44,255}};// custom color for dark theme (dark bluish)
Color btnCol[2]    = {LIGHTGRAY, (Color){65,65,80,255}};//slightly lighter gray-blue color.

//  Get tile color based on value and theme 
Color getTileColor(int value) {
    int index = 0;
    while (value > 2 && index < 6) {
        value /= 2;
        index++;
    }
    return theme ? tileDark[index] : tileLight[index];
}

// Save board for Undo 
void saveUndo() {
    for (int i=0; i<SIZE; i++)
        for (int j=0; j<SIZE; j++)
            undoBoard[i][j] = board[i][j];
    prevScore = score;
}

// Undo last move
void undoMove() {
    for (int i=0; i<SIZE; i++)
        for (int j=0; j<SIZE; j++)
            board[i][j] = undoBoard[i][j];
    score = prevScore;
}

//  Add a random tile (2 or 4) 
void addRandomTile() {
    int emptyCells[MAX_SIZE * MAX_SIZE][2];
    int emptyCount = 0;

    // Collect all empty cell positions
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            if (board[row][col] == 0) {
                emptyCells[emptyCount][0] = row;
                emptyCells[emptyCount][1] = col;
                emptyCount++;
            }
        }
    }
    // If empty cells exist, place a new tile
    if (emptyCount > 0) {
        int randomIndex = rand() % emptyCount;
        int r = emptyCells[randomIndex][0];
        int c = emptyCells[randomIndex][1];
        board[r][c] = (rand() % 10 == 0) ? 4 : 2;
    }
}

//  Rotate board for move directions
void rotateBoard() {
    // Transpose
    for (int i = 0; i < SIZE; i++) {
        for (int j = i + 1; j < SIZE; j++) {
            int temp = board[i][j];
            board[i][j] = board[j][i];
            board[j][i] = temp;
        }
    }
    // Reverse each row
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE / 2; j++) {
            int temp = board[i][j];
            board[i][j] = board[i][SIZE - j - 1];
            board[i][SIZE - j - 1] = temp;
        }
    }
}
//  Slide left (core merging logic) 
int slideLeft() {
    int moved = 0;
    for (int i = 0; i < SIZE; i++) {
        int nonZero[MAX_SIZE], nz = 0; // nz=non zero
        // Collect non-zero values
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] != 0)
                nonZero[nz++] = board[i][j];
        // Merge adjacent equal tiles
        int temp[MAX_SIZE] = {0}, pos = 0;
        for (int j = 0; j < nz; j++) {
            if (j < nz - 1 && nonZero[j] == nonZero[j + 1]) {
                temp[pos++] = nonZero[j] * 2;
                score += temp[pos-1];
                j++; // skip next because merged
            } else {
                temp[pos++] = nonZero[j];
            }
        }
        // 3. Check movement and update board
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] != temp[j]) 
            moved = 1;
            board[i][j] = temp[j];
        }
    }
    return moved;
}

//  (0:left, 1:up, 2:right, 3:down) 
int moveBoard(int dir) {
    saveUndo();
    for (int i=0; i<dir; i++) 
        rotateBoard();
    int moved = slideLeft();
    for (int i=0; i<(4-dir)%4; i++) 
        rotateBoard();
    return moved;
}

// any empty spaces left
int canMove() {
    for (int i=0; i<SIZE; i++)
        for (int j=0; j<SIZE; j++)
            if (board[i][j] == 0) 
                 return 1;
    for (int i=0; i<SIZE-1; i++)
        for (int j=0; j<SIZE-1; j++)
            if (board[i][j] == board[i][j+1] || board[i][j] == board[i+1][j]) 
                return 1;
    return 0;
}

// Load/save high-score from file
void loadHighScore() {
    FILE *f = fopen("highscore.txt", "r");
    highScore = 0; 
    if (f) { 
        fscanf(f,"%d", &highScore); 
        fclose(f); 
    }
}
void saveHighScore() {
    FILE *f = fopen("highscore.txt", "w");
    if (f) { 
        fprintf(f,"%d", highScore); 
        fclose(f); 
    }
}

// Draw a single tile and number 
void DrawTile(int x, int y, int value) {
    Rectangle tile = {x, y, TILE_SIZE, TILE_SIZE};
    DrawRectangleRec(tile, getTileColor(value));
    DrawRectangleLinesEx(tile, 3, borderCol[theme]);
    if (value) {
        char text[10];
        sprintf(text, "%d", value);
        DrawText(text, x+TILE_SIZE/2-MeasureText(text, 30)/2, y+TILE_SIZE/2-15, 30, textCol[theme]);
    }
}

// Draw the full board bx=board x and by=board y
void DrawBoard(int bx, int by) {
    for (int i=0; i<SIZE; i++)
        for (int j=0; j<SIZE; j++)
            DrawTile(bx + j*(TILE_SIZE+GAP), by + i*(TILE_SIZE+GAP), board[i][j]);
}

// Draw sun/moon icon for theme 
void DrawThemeIcon(int x, int y) {
    if (theme == 0) {   
        // Light theme
        DrawCircle(x, y, 11, YELLOW);
    } 
    else {             
        // Dark theme
        DrawCircle(x, y, 11, WHITE);     
    }
}

int main() {
    srand(time(NULL));
    loadHighScore();

    InitWindow(WINDOW_W, WINDOW_H, "2048 Game");
    SetTargetFPS(60);

    // GRID SIZE SELECTION MENU (THEME MATCHED) 
    int selecting = 1;
    Rectangle btn4 = {WINDOW_W/2 - 140, 250, 280, 65};
    Rectangle btn5 = {WINDOW_W/2 - 140, 350, 280, 65};
    Rectangle btn6 = {WINDOW_W/2 - 140, 450, 280, 65};

    while (selecting && !WindowShouldClose()) {

        Vector2 mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, btn4)) {
                 SIZE = 4; selecting = 0; }
            if (CheckCollisionPointRec(mouse, btn5)) { 
                SIZE = 5; selecting = 0; }
            if (CheckCollisionPointRec(mouse, btn6)) { 
                SIZE = 6; selecting = 0; }
        }

        BeginDrawing();
        ClearBackground(bgCol[theme]);

        // Title
        DrawText("Select Grid Size",  WINDOW_W/2 - MeasureText("Select Grid Size", 42)/2,
            140, 42, textCol[theme]);
        // 4x4 button 
        DrawRectangleRounded(btn4, 0.75, 10, btnCol[theme]);
        DrawText("4 x 4", btn4.x + 95, btn4.y + 15, 32, textCol[theme]);
        // 5x5 button
        DrawRectangleRounded(btn5, 0.75, 10, btnCol[theme]);
        DrawText("5 x 5", btn5.x + 95, btn5.y + 15, 32, textCol[theme]);
        // 6x6 button
        DrawRectangleRounded(btn6, 0.75, 10, btnCol[theme]);
        DrawText("6 x 6", btn6.x + 95, btn6.y + 15, 32, textCol[theme]);
        // Footer
        DrawText("Developed by Insiya & Musfira",
            WINDOW_W/2 - MeasureText("Developed by Insiya & Musfira", 23)/2,
            WINDOW_H - 60, 23, textCol[theme]);
        EndDrawing();
    }

    // Initialize board after selection
    score = 0;
    // zero all MAX_SIZE to be safe
    for (int i=0; i<MAX_SIZE; i++)
        for (int j=0; j<MAX_SIZE; j++)
            board[i][j] = 0;
    addRandomTile();
    addRandomTile();

    while (!WindowShouldClose()) {
        Rectangle undoBtn = {32, 20, 106, 38};
        Rectangle themeBtn = {WINDOW_W - 86, 20, 46, 38};
        Vector2 mouse = GetMousePosition();

        int moved = 0;

        if (!canMove()) {
            if (score > highScore) { 
                highScore = score;
                 saveHighScore(); }
            BeginDrawing();
            ClearBackground(bgCol[theme]);
            // Draw Board
            DrawText("GAME OVER!", WINDOW_W/2-130, 60, 48, RED);
            DrawBoard((WINDOW_W-(SIZE*TILE_SIZE+(SIZE-1)*GAP))/2, 110);

            DrawText(TextFormat("Score: %d", score), 40, WINDOW_H-110, 30, textCol[theme]);
            int hiW = MeasureText(TextFormat("High Score: %d", highScore), 30);
            DrawText(TextFormat("High Score: %d", highScore), WINDOW_W-hiW-40, WINDOW_H-110, 30, textCol[theme]);
            DrawText("Developed by Insiya & Musfira", WINDOW_W/2 - MeasureText("Developed by Insiya & Musfira", 23)/2, WINDOW_H-52, 23, textCol[theme]);
            EndDrawing();
            WaitTime(5.0);
            break;
        }

        // Input handling
        if (IsKeyPressed(KEY_LEFT)) 
          moved = moveBoard(0);
        if (IsKeyPressed(KEY_UP))    
          moved = moveBoard(1);
        if (IsKeyPressed(KEY_RIGHT))
          moved = moveBoard(2);
        if (IsKeyPressed(KEY_DOWN)) 
          moved = moveBoard(3);

        if (CheckCollisionPointRec(mouse, undoBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
          undoMove();
        if (CheckCollisionPointRec(mouse, themeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
         theme = 1-theme;
        if (moved) 
        addRandomTile();

        BeginDrawing();
        ClearBackground(bgCol[theme]);
        
        DrawText("2048 GAME", WINDOW_W/2-112, 24, 38, textCol[theme]);
        DrawBoard((WINDOW_W-(SIZE*TILE_SIZE+(SIZE-1)*GAP))/2, 110);
        DrawText(TextFormat("Score: %d", score), 40, WINDOW_H-110, 30, textCol[theme]);
        int hiW = MeasureText(TextFormat("High Score: %d", highScore), 30);
        DrawText(TextFormat("High Score: %d", highScore), WINDOW_W-hiW-40, WINDOW_H-110, 30, textCol[theme]);
        DrawRectangleRounded(undoBtn,0.22,8,btnCol[theme]);
        DrawText("Undo", undoBtn.x+18, undoBtn.y+6, 24, textCol[theme]);
        DrawRectangleRounded(themeBtn,0.22,8,btnCol[theme]);
        DrawThemeIcon(themeBtn.x+23, themeBtn.y+19);
        DrawText("Developed by Insiya & Musfira", WINDOW_W/2 - MeasureText("Developed by Insiya & Musfira", 23)/2, WINDOW_H-52, 23, textCol[theme]);
        EndDrawing();
    }

    saveHighScore();
    CloseWindow();
    return 0;
}