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
const Vector2 INITIAL_BALL_SPEED = (Vector2) { 200, -180 };
const double BASE_BALL_RADIUS = 7;

// Paddle
typedef struct Paddle
{
    Rectangle rect; // posx, posy, width, height
    Vector2 speed;
} Paddle;

Paddle paddle;
const int BASE_PADDLE_WIDTH = 75;
const int BASE_PADDLE_HEIGHT = 12;
const Vector2 PADDLE_SPEED = (Vector2) { 10, 0 };   //* unit: pixels per key input
const int SPACE_BELOW_PADDLE = 5;                   // pixels below paddle


/* Function Prototypes */
void initializeGame();

// Main game logic
void resetBall();
void resetPaddle();

void updateBall();
void updatePaddle();

void bounceBallOnWalls();
void bounceBallOnPaddle();

// Draws
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
        updatePaddle();

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
    resetBall();
    resetPaddle();
}

void resetBall() {
    ball.pos = (Vector2) { GetScreenWidth() / 2, GetScreenHeight() / 2 };
    ball.speed = INITIAL_BALL_SPEED;
    ball.radius = BASE_BALL_RADIUS;
}

void resetPaddle() {
    paddle.rect = (Rectangle) {
        (GetScreenWidth() - BASE_PADDLE_WIDTH) / 2,
        GetScreenHeight() - BASE_PADDLE_HEIGHT - SPACE_BELOW_PADDLE,
        BASE_PADDLE_WIDTH,
        BASE_PADDLE_HEIGHT
    };
    paddle.speed = PADDLE_SPEED;
}

void updateBall() {
    const double dt = GetFrameTime();
    ball.pos = Vector2Add(ball.pos, Vector2Scale(ball.speed, dt));

    bounceBallOnWalls();
    bounceBallOnPaddle();
}

void bounceBallOnWalls() {
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

void bounceBallOnPaddle() {
    if (ball.pos.x + ball.radius >= paddle.rect.x && ball.pos.x - ball.radius <= paddle.rect.x + paddle.rect.width &&
        ball.pos.y + ball.radius > paddle.rect.y) {
        ball.speed.y *= -1;
        ball.pos.y = paddle.rect.y - (ball.radius + 2);
    }
}

void updatePaddle() {
    // handle inputs
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        paddle.rect.x -= paddle.speed.x;
    else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        paddle.rect.x += paddle.speed.x;

    // bound within the walls
    if (paddle.rect.x < 0) {
        paddle.rect.x = 0;
    }
    else if (paddle.rect.x + paddle.rect.width > GetScreenWidth()) {
        paddle.rect.x = GetScreenWidth() - paddle.rect.width;
    }
}

// Contains all draw calls; func called inside game loop
void drawLoop() {
    drawBall();
    drawPaddle();
}

void drawBall() {
    DrawCircle(ball.pos.x, ball.pos.y, ball.radius, ORANGE);
}

void drawPaddle() {
    DrawRectangleRec(paddle.rect, RAYWHITE);
}
