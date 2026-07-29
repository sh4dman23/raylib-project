#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>

/* Globals and Constants */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GAME_WINDOW_TITLE "DXBall"

bool debugView = false;

//? NOTE: all speeds are in units pixels per second
// Ball
typedef struct Ball {
    Vector2 pos;
    Vector2 speed;
    double radius;
} Ball;

Ball ball;
const Vector2 INITIAL_BALL_SPEED = (Vector2) { 300.0, -180.0 / 200 * 300 };
const double BASE_BALL_RADIUS = 7.0;

bool lockBallToPaddle = true;   // makes ball stick to paddle, until player presses space

// Paddle
typedef struct Paddle {
    Rectangle rect; // posx, posy, width, height
    Vector2 speed;
} Paddle;

Paddle paddle;
const int BASE_PADDLE_WIDTH = 74;
const int BASE_PADDLE_HEIGHT = 15;
const Vector2 PADDLE_SPEED = (Vector2) { 10, 0 };   //* unit: pixels per key input
const int SPACE_BELOW_PADDLE = 5;                   // pixels below paddle

// Bricks
#define MAX_NUMBER_OF_BRICKS 1000
const float BRICK_WIDTH = 60;
const float BRICK_HEIGHT = 20;

// values set based on map input
int numBricks = 5;
int brickRows = 1;
int brickCols = 5;

typedef struct Brick {
    Rectangle rect;
    int type;
} Brick;

Brick bricks[MAX_NUMBER_OF_BRICKS];

/* Function Prototypes */
void initializeGame();
void initializeMap();

// Main game logic
void manageDebugView();

void resetBall();
void resetPaddle();

void updateBall();
void updatePaddle();

void checkAllCollisions();
void checkBallNBrickCollisions();

void bounceBallOnWalls();
void bounceBallOnPaddle();

// Draws
void drawLoop();
void drawBall();
void drawPaddle();
void drawBricks();
void drawDebugView();

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    // initialize game
    initializeGame();

    while (!WindowShouldClose()) {
        manageDebugView();

        // updates
        updateBall();
        updatePaddle();

        // collisions
        checkAllCollisions();

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
    initializeMap();
    resetPaddle();
    resetBall();
}

void initializeMap() {
    //** test bricks go here
    bricks[0] = (Brick) {
        (Rectangle) {
            100, 100, BRICK_WIDTH, BRICK_HEIGHT
        },
        1
    };
    bricks[1] = (Brick) {
        (Rectangle) {
            300, 100, BRICK_WIDTH, BRICK_HEIGHT
        },
        2
    };
    bricks[2] = (Brick) {
        (Rectangle) {
            200, 100, BRICK_WIDTH, BRICK_HEIGHT
        },
        3
    };
    bricks[3] = (Brick) {
        (Rectangle) {
            400, 100, BRICK_WIDTH, BRICK_HEIGHT
        },
        0
    };
    bricks[4] = (Brick) {
        (Rectangle) {
            500, 100, BRICK_WIDTH, BRICK_HEIGHT
        },
        -1
    };
}

// Reset ball to starting position
void resetBall() {
    lockBallToPaddle = true;
    ball.pos = (Vector2) { paddle.rect.x + paddle.rect.width / 2, paddle.rect.y - ball.radius };
    ball.speed = INITIAL_BALL_SPEED;
    ball.radius = BASE_BALL_RADIUS;
}

// Reset paddle to starting position
void resetPaddle() {
    paddle.rect = (Rectangle) {
        (GetScreenWidth() - BASE_PADDLE_WIDTH) / 2,
        GetScreenHeight() - BASE_PADDLE_HEIGHT - SPACE_BELOW_PADDLE,
        BASE_PADDLE_WIDTH,
        BASE_PADDLE_HEIGHT
    };
    paddle.speed = PADDLE_SPEED;
}

// Update ball position based on collitions and stuff
void updateBall() {
    // ball locked to paddle position
    if (lockBallToPaddle) {
        ball.pos = (Vector2) { paddle.rect.x + paddle.rect.width / 2, paddle.rect.y - ball.radius };
    }
    else {
        const double dt = GetFrameTime();
        const Vector2 initialBallPos = ball.pos;
        Vector2 displacement = Vector2Scale(ball.speed, dt);

        // move ball in incremental amounts (minimum 1 times) and check for collisions (emulate spherecast)
        // this is required if the ball is moving too fast (like if displacement > brick height and similar)
        for (int i = 0; i < ceil(Vector2Length(displacement) / (2 * ball.radius)) || i < 1; i++) {
            ball.pos = Vector2Add(ball.pos, Vector2Scale(Vector2Normalize(displacement), 2 * ball.radius));
            checkBallNBrickCollisions();
            bounceBallOnPaddle();
        }
        // set to final position (in case of inaccuracies)
        ball.pos = Vector2Add(initialBallPos, displacement);

        bounceBallOnWalls();
        bounceBallOnPaddle();
    }
}

// Reflect ball off of the walls and ceiling, but cause death upon falling below
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
        if (ball.pos.y - ball.radius < 0) {
            ball.pos.y = ball.radius;
        }
        else {
            resetPaddle();
            resetBall();
            //? handle death logic here
        }
    }
}

// Reflect ball off of the paddle
void bounceBallOnPaddle() {
    if (ball.pos.x + ball.radius >= paddle.rect.x && ball.pos.x - ball.radius <= paddle.rect.x + paddle.rect.width &&
        ball.pos.y + ball.radius > paddle.rect.y) {
        ball.speed.y *= -1;
        ball.pos.y = paddle.rect.y - (ball.radius + 2);
    }
}

// Update paddle position based on player input
void updatePaddle() {
    // unlock ball from paddle
    if (IsKeyDown(KEY_SPACE))
        lockBallToPaddle = false;

    // move paddle sideways
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

void checkAllCollisions() {
    checkBallNBrickCollisions();
}

// Check collision between ball and brick
void checkBallNBrickCollisions() {
    for (int i = 0; i < numBricks; i++) {
        // no collisions for empty bricks
        if (bricks[i].type == 0)
            continue;

        if (!CheckCollisionCircleRec(ball.pos, ball.radius, bricks[i].rect))
            continue;

        // ball above brick
        if (ball.pos.y <= bricks[i].rect.y
            && ball.pos.x - ball.radius < bricks[i].rect.x + bricks[i].rect.width
            && ball.pos.x + ball.radius > bricks[i].rect.x
        ) {
            ball.speed.y *= -1;
            ball.pos.y = bricks[i].rect.y - ball.radius - 2;
        }
        // ball below brick
        else if (ball.pos.y >= bricks[i].rect.y + bricks[i].rect.height
            && ball.pos.x - ball.radius < bricks[i].rect.x + bricks[i].rect.width
            && ball.pos.x + ball.radius > bricks[i].rect.x
        ) {
            ball.speed.y *= -1;
            ball.pos.y = bricks[i].rect.y + bricks[i].rect.height + ball.radius + 2;
        }
        // ball to the left of brick
        else if (ball.pos.x <= bricks[i].rect.x
            && ball.pos.y - ball.radius < bricks[i].rect.y + bricks[i].rect.height
            && ball.pos.y + ball.radius > bricks[i].rect.y
        ) {
            ball.speed.x *= -1;
            ball.pos.x = bricks[i].rect.x - ball.radius - 2;
        }
        // ball to the right of brick
        else if (ball.pos.x >= bricks[i].rect.x + bricks[i].rect.width
            && ball.pos.y - ball.radius < bricks[i].rect.y + bricks[i].rect.height
            && ball.pos.y + ball.radius > bricks[i].rect.y
        ) {
            ball.speed.x *= -1;
            ball.pos.x = bricks[i].rect.x + bricks[i].rect.width + ball.radius + 2;
        }
    }
}

// Contains all draw calls; func called inside game loop
void drawLoop() {
    drawDebugView();
    drawPaddle();
    drawBricks();
    drawBall();
}

void drawBall() {
    DrawCircle(ball.pos.x, ball.pos.y, ball.radius, ORANGE);
}

void drawPaddle() {
    DrawRectangleRec(paddle.rect, RAYWHITE);
}

// Draw bricks at positions based on type
void drawBricks() {
    for (int i = 0; i < numBricks; i++) {
        // pick color
        Color brickColor = (Color) {0, 0, 0, 0};
        if (bricks[i].type == 0)
            continue;
        else if (bricks[i].type == 1)
            brickColor = BLUE;
        else if (bricks[i].type == 2)
            brickColor = YELLOW;
        else if (bricks[i].type == 3)
            brickColor = RED;
        else if (bricks[i].type == -1)
            brickColor = BROWN;

        // draw brick
        DrawRectangleRec(bricks[i].rect, brickColor);
    }
}

void manageDebugView() {
    if (IsKeyPressed(KEY_TAB)) {
        debugView = !debugView;
    }
}

void drawDebugView() {
    if (!debugView)
        return;
    DrawCircleLinesV(ball.pos, ball.radius + 5, RED);
    DrawRectangleLinesEx((Rectangle) {paddle.rect.x - 2, paddle.rect.y - 2, paddle.rect.width + 4, paddle.rect.height + 4}, 5, RED);

    char fpsText[50] = {'\0'};
    sprintf(fpsText, "FPS: %d", GetFPS());
    DrawText(fpsText, 10, 10, 15, RAYWHITE);
}
