#include "raylib.h"

/* Globals and Constants */
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GAME_WINDOW_TITLE "ligma"

/* Function Prototypes */
void drawLoop();

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        drawLoop();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// Contains all draw calls; func called inside game loop
void drawLoop() {
    DrawCircle(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 10.0, WHITE);
}
