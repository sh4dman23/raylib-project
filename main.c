#include "raylib.h"
#include "raymath.h"

/* Globals and Constants */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GAME_WINDOW_TITLE "DXBall"

//? all speeds are in units pixels per second

// Ball
typedef struct Ball {
    Vector2 pos;
    Vector2 speed;
    double radius;
} Ball;

Ball ball;
const Vector2 INITIAL_BALL_SPEED = (Vector2) {200, -180};
const double BASE_BALL_RADIUS = 7;

/* Function Prototypes */
void initializeGame();

void updateBall();

void drawLoop();
void drawBall();
void drawPaddle();

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    // initialize game
    initializeGame();

    while (!WindowShouldClose()) {
        // updates
        updateBall();

        // draws
        BeginDrawing();
        ClearBackground(BLACK);
        drawLoop();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void initializeGame() {
    ball.pos = (Vector2) { GetScreenWidth() / 2, GetScreenHeight() / 2 };
    ball.speed = INITIAL_BALL_SPEED;
    ball.radius = BASE_BALL_RADIUS;
}

void updateBall() {
    const double dt = GetFrameTime();
    ball.pos = Vector2Add(ball.pos, Vector2Scale(ball.speed, dt));

    // ball bounces off walls
    if (ball.pos.x - ball.radius < 0 || ball.pos.x + ball.radius > GetScreenWidth()) {
        ball.speed.x *= -1;
        if (ball.pos.x - ball.radius < 0)
            ball.pos.x = ball.radius;
        else
            ball.pos.x = GetScreenWidth() - ball.radius;
    }

    // ball bounces off ceiling and floor (for now)
    if (ball.pos.y - ball.radius < 0 || ball.pos.y + ball.radius > GetScreenHeight()) {
        ball.speed.y *= -1;
        if (ball.pos.y - ball.radius < 0)
            ball.pos.y = ball.radius;
        else
            ball.pos.y = GetScreenHeight() - ball.radius;
    }
}

// Contains all draw calls; func called inside game loop
void drawLoop() {
    drawBall();
}

void drawBall() {
    DrawCircle(ball.pos.x, ball.pos.y, ball.radius, WHITE);
}
