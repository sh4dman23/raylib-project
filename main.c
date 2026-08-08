#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Globals and Constants */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GAME_WINDOW_TITLE "DXBall"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

//* Game States
// 0 = main menu (unimplemented)
// 1 = main game
// 2 = game end (unimplemented)
// 3 = map editor
int gameState = 1;

// Core game statistics
int playerScore = 0;
double scoreMultiplier = 1.0;
int playtime = 0;
int lives = 0;
const int STARTING_LIVES = 3;

const int BASE_BRICK_HIT_SCORE = 50;

// Core game UI
const double PADDING_ABOVE_UI = 20;
const double PADDING_SIDES_UI = 20;

bool debugView = false;

Texture2D lifeTexture;

// Ball
typedef struct Ball
{
    Vector2 pos;
    Vector2 speed;
    double radius;
} Ball;

Ball ball;

Texture2D ballImage;
const Vector2 INITIAL_BALL_SPEED = (Vector2){300.0, -350};
const double BASE_BALL_RADIUS = 5.0;

bool ballLockedToPaddle = true;       // makes ball stick to paddle, until player presses space
double lastBallLockTime = 0;        // in seconds
const double BALL_OSCILLATION_FREQ = 1.0;   // oscillations per second

// Paddle
typedef struct Paddle
{
    Rectangle rect; // posx, posy, width, height
    Vector2 speed;
} Paddle;

Paddle paddle;

const int BASE_PADDLE_WIDTH = 74;
const int BASE_PADDLE_HEIGHT = 15;
const Vector2 PADDLE_SPEED = (Vector2){10, 0}; //* unit: pixels per key input
const int SPACE_BELOW_PADDLE = 5;              // pixels below paddle

// paddle texture
Texture2D paddleImage;

// Bricks
#define MAX_NUMBER_OF_BRICKS 1000
const float BRICK_WIDTH = 60;
const float BRICK_HEIGHT = 20;

// paddings for map
const double PADDING_ABOVE_MAP = 100;
const double PADDING_BELOW_MAP = 50;
const double PADDING_ON_MAP_SIDES = ((WINDOW_WIDTH) % (int)BRICK_WIDTH + (int)BRICK_WIDTH) / 2;

// values based on dimensions
int maxBrickRows = (WINDOW_HEIGHT - PADDING_ABOVE_MAP - PADDING_BELOW_MAP) / BRICK_HEIGHT;
int maxBrickCols = (WINDOW_WIDTH - PADDING_ON_MAP_SIDES * 2) / BRICK_WIDTH;
int numBricks;

// brick types = 0 (empty), 1, 2, 3 (standard), -1 (unbreakable)
typedef struct Brick
{
    Rectangle rect;
    int type;
} Brick;

// required for map editor brick changes
const int MAX_BRICK_TYPE = 3;
const int MIN_BRICK_TYPE = -1;

Brick bricks[MAX_NUMBER_OF_BRICKS];

// brick textures
#define NUM_BRICK_TEXTURES 4
Texture2D brickTextures[NUM_BRICK_TEXTURES];

// Map Related
#define MAP_FILE_PATH "./map.txt"

/* Function Prototypes */
void initializeGame();
void initializeMap();

void manageDebugView();
void manageGameStates();
void switchGameState(int state);

// Core Game Logic
void setNewGame();

void resetBall();
void resetPaddle();

void lockBall();
void updateBall();
void updatePaddle();

void checkAllCollisions();
bool checkBallNBrickCollisions();

void bounceBallOnBoundaries();
bool bounceBallOnPaddle();

void updatePlayTime();

void resetStats();
void increaseScore(int change);

void checkGameEnd();

double distancePointLine(Vector2 point, Vector2 lPoint1, Vector2 lPoint2);

// Game Map
void setEmptyMap();
void readMapFromFile(FILE *mapFile);
void writeMapToFile(FILE *outputFile);

// Map Editor
void checkMapEdit();

// Sprites
void loadSprites();
void unloadSprites();

// Draw Functions
void drawLoop();
void drawMainGameUI();
void drawBall();
void drawPaddle();
void drawBricks();
void drawMapEditor();
void drawDebugView();

int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    // initialize game
    initializeGame();
    loadSprites();

    // make it so, esc key doesn't close the game
    SetExitKey(0);

    while (!WindowShouldClose())
    {
        manageDebugView();
        manageGameStates();

        //* updates

        // core game
        if (gameState == 1)
        {
            updatePlayTime();

            updatePaddle();
            updateBall();

            // collisions
            checkAllCollisions();

            // check death
            checkGameEnd();
        }
        // map editor
        else if (gameState == 3)
        {
            checkMapEdit();
        }

        // draw
        BeginDrawing();
        ClearBackground(BLACK);
        drawLoop();
        EndDrawing();
    }

    unloadSprites();
    CloseWindow();
    return 0;
}

// Manage game state changes due to ingame user interaction
void manageGameStates()
{
    if (gameState == 0)
    {
        // code for main menu ui interactions that change gamestates go here
    }
    //! current working method to switch to map editor
    else if (gameState == 1 && IsKeyPressed(KEY_F2))
    {
        switchGameState(3);
    }
    // switch from map editor back to main game
    else if (gameState == 3 && IsKeyPressed(KEY_F2))
    {
        // open last saved map
        FILE *mapFile = fopen(MAP_FILE_PATH, "r");
        readMapFromFile(mapFile);
        fclose(mapFile);
        switchGameState(1);
    }
}

// Switches game states, and does necessary changes
void switchGameState(int state)
{
    if (gameState == 1 & state != 1)
    {
        debugView = false;
        lockBall();
    }

    if (gameState == 1 && state == 3)
    {
        setNewGame();
    }

    gameState = state;
}

void initializeGame()
{
    setNewGame();
}

void setNewGame()
{
    numBricks = maxBrickCols * maxBrickRows;

    initializeMap();
    resetPaddle();
    resetBall();
    resetStats();
}

void updatePlayTime() {
    static double time = 0;
    const double interval = 1.0;

    if (gameState != 1 || ballLockedToPaddle)
        return;

    time += GetFrameTime();
    if (time >= interval) {
        playtime++;
        time = 0;
    }
}

void resetStats()
{
    playerScore = 0;
    lives = STARTING_LIVES;
    playtime = 0;
    scoreMultiplier = 1.0;
}

void initializeMap()
{
    setEmptyMap();

    FILE *mapInputFile = fopen(MAP_FILE_PATH, "r");
    readMapFromFile(mapInputFile);
    fclose(mapInputFile);
}

// Set all bricks in map to empty bricks (type 0)
void setEmptyMap()
{
    double px = PADDING_ON_MAP_SIDES;
    double py = PADDING_ABOVE_MAP;

    for (int i = 0; i < maxBrickRows; i++)
    {
        for (int j = 0; j < maxBrickCols; j++)
        {
            bricks[i * maxBrickCols + j] = (Brick){
                (Rectangle){
                    px,
                    py,
                    BRICK_WIDTH,
                    BRICK_HEIGHT},
                0};

            px += BRICK_WIDTH;
        }

        px = PADDING_ON_MAP_SIDES;
        py += BRICK_HEIGHT;
    }
}

// Read map from file to memory
void readMapFromFile(FILE *mapFile)
{
    setEmptyMap();

    int brickRows = 0, brickCols = 0;
    fscanf(mapFile, "%d %d", &brickRows, &brickCols);

    for (int i = 0; i < brickRows; i++)
    {
        for (int j = 0; j < brickCols; j++)
        {
            int brickType = 0;
            fscanf(mapFile, "%d", &brickType);

            bricks[i * maxBrickCols + j].type = brickType;
        }
    }
}

// Save map from memory to file
void writeMapToFile(FILE *outputFile)
{
    fprintf(outputFile, "%d %d\n", maxBrickRows, maxBrickCols);
    for (int i = 0; i < maxBrickRows; i++)
    {
        for (int j = 0; j < maxBrickCols; j++)
        {
            fprintf(outputFile, "%d ", bricks[i * maxBrickCols + j].type);
        }
        fprintf(outputFile, "\n");
    }
}

// Checks changes to map in map editor
void checkMapEdit()
{
    // save map
    if (IsKeyPressed(KEY_ENTER))
    {
        FILE *mapFile = fopen(MAP_FILE_PATH, "w");
        writeMapToFile(mapFile);
        fclose(mapFile);
    }
    // reset all changes
    else if (IsKeyPressed(KEY_BACKSPACE))
    {
        FILE *mapFile = fopen(MAP_FILE_PATH, "r");
        readMapFromFile(mapFile);
        fclose(mapFile);
    }
    // cycle brick type right
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < maxBrickCols * maxBrickRows; i++)
        {
            if (CheckCollisionPointRec(mousePos, bricks[i].rect))
            {
                bricks[i].type = bricks[i].type + 1;
                if (bricks[i].type > MAX_BRICK_TYPE)
                    bricks[i].type = MIN_BRICK_TYPE;
            }
        }
    }
    // cycle brick type left
    else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < maxBrickCols * maxBrickRows; i++)
        {
            if (CheckCollisionPointRec(mousePos, bricks[i].rect))
            {
                bricks[i].type = bricks[i].type - 1;
                if (bricks[i].type < MIN_BRICK_TYPE)
                    bricks[i].type = MAX_BRICK_TYPE;
            }
        }
    }
    // set empty brick
    else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < maxBrickCols * maxBrickRows; i++)
        {
            if (CheckCollisionPointRec(mousePos, bricks[i].rect))
            {
                bricks[i].type = 0;
            }
        }
    }
}

// Reset ball to starting position
void resetBall()
{
    lockBall();
    ball.pos = (Vector2){paddle.rect.x + paddle.rect.width / 2, paddle.rect.y - ball.radius};
    ball.speed = INITIAL_BALL_SPEED;
    ball.radius = BASE_BALL_RADIUS;
}

// Reset paddle to starting position
void resetPaddle()
{
    paddle.rect = (Rectangle){
        (GetScreenWidth() - BASE_PADDLE_WIDTH) / 2,
        GetScreenHeight() - BASE_PADDLE_HEIGHT - SPACE_BELOW_PADDLE,
        BASE_PADDLE_WIDTH,
        BASE_PADDLE_HEIGHT};
    paddle.speed = PADDLE_SPEED;
}

// Lock ball to paddle
void lockBall()
{
    ballLockedToPaddle = true;
    lastBallLockTime = GetTime();
}

// Update ball position based on collitions and stuff
void updateBall()
{
    // ball locked to paddle position
    if (ballLockedToPaddle)
    {
        // oscillate ball
        double amp = paddle.rect.width / 2.0 - ball.radius;
        double timeSinceBallLock = GetTime() - lastBallLockTime;
        double delx = amp * sin(2 * 3.14159 * BALL_OSCILLATION_FREQ * timeSinceBallLock);

        ball.pos = (Vector2){paddle.rect.x + paddle.rect.width / 2 + delx, paddle.rect.y - ball.radius};

        // ball speed depends on displacement from center at the time
        ball.speed.x = (delx / amp) * INITIAL_BALL_SPEED.x;
    }
    // ball free to move across the map
    else
    {
        const double dt = GetFrameTime();
        const Vector2 initialBallPos = ball.pos;
        Vector2 displacement = Vector2Scale(ball.speed, dt);

        bool collision = false;

        // move ball in incremental amounts (minimum 1 times) and check for collisions (emulate spherecast)
        // this is required if the ball is moving too fast (like if displacement > brick height and similar)
        for (int i = 0, divs = 10; i < divs; i++)
        {
            ball.pos = Vector2Add(ball.pos, Vector2Scale(displacement, 1.0 / divs));
            if (checkBallNBrickCollisions())
            {
                collision = true;
                break;
            }
        }

        // set to final position (in case of inaccuracies)
        if (!collision)
            ball.pos = Vector2Add(initialBallPos, displacement);

        bounceBallOnBoundaries();

        // bounce ball on paddle and angle ball using that
        if (bounceBallOnPaddle()) {
            double delx = ball.pos.x - (paddle.rect.x + paddle.rect.width / 2);

            // add accelerated ball speed here
            ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * (fabs(delx) / (paddle.rect.width / 2)) * max(fabs(ball.speed.x), INITIAL_BALL_SPEED.x);

            if (delx > 0 != ball.speed.x > 0)
                ball.speed.x *= -1;
        }
    }
}

// Reflect ball off of the walls and ceiling, but cause death upon falling below
void bounceBallOnBoundaries()
{
    // ball bounces off walls
    if (ball.pos.x - ball.radius < 0 || ball.pos.x + ball.radius > GetScreenWidth())
    {
        ball.speed.x *= -1;
        if (ball.pos.x - ball.radius < 0)
            ball.pos.x = ball.radius;
        else
            ball.pos.x = GetScreenWidth() - ball.radius;
    }

    // ball bounces off ceiling
    if (ball.pos.y - ball.radius < 0)
    {
        ball.speed.y *= -1;
        ball.pos.y = ball.radius;

    }
    else if (ball.pos.y + ball.radius > GetScreenHeight())
    {
        resetPaddle();
        resetBall();
        if (lives > 0)
            lives--;
        checkGameEnd();
    }
}

// Reflect ball off of the paddle
bool bounceBallOnPaddle()
{
    bool collision = false;

    // 4 pixels of extra length on both sides to allow for fairer jump
    if (ball.pos.x >= paddle.rect.x - 4 && ball.pos.x <= paddle.rect.x + paddle.rect.width + 4 &&
        ball.pos.y + ball.radius >= paddle.rect.y)
    {
        ball.speed.y *= -1;
        ball.pos.y = paddle.rect.y - (ball.radius + 1);
        collision = true;
    }
    else if (CheckCollisionCircleRec(ball.pos, ball.radius, (Rectangle) {paddle.rect.x - 4, paddle.rect.y, paddle.rect.width + 8, paddle.rect.height}))
    {
        ball.speed.y *= -1;
        collision = true;
    }

    return collision;
}

// Update paddle position based on player input
void updatePaddle()
{
    // unlock ball from paddle
    if (IsKeyDown(KEY_SPACE))
        ballLockedToPaddle = false;

    // move paddle sideways
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        paddle.rect.x -= paddle.speed.x;
    else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        paddle.rect.x += paddle.speed.x;

    // check collisions with ball since its fast moving
    bounceBallOnPaddle();

    // bound within the walls
    if (paddle.rect.x < 0)
    {
        paddle.rect.x = 0;
    }
    else if (paddle.rect.x + paddle.rect.width > GetScreenWidth())
    {
        paddle.rect.x = GetScreenWidth() - paddle.rect.width;
    }
}

void checkAllCollisions()
{
    bounceBallOnPaddle();
    bounceBallOnBoundaries();
    checkBallNBrickCollisions();
}

// Check collision between ball and brick and return brick index
bool checkBallNBrickCollisions()
{
    bool collision = false;
    for (int i = 0; i < numBricks; i++)
    {
        bool collisionNow = false;
        // no collisions for empty bricks
        if (bricks[i].type == 0 || !CheckCollisionCircleRec(ball.pos, ball.radius, bricks[i].rect))
            continue;

        double bX = bricks[i].rect.x, bY = bricks[i].rect.y;

        Vector2 leftTop = {bX, bY};
        Vector2 leftBottom = {bX, bY + BRICK_HEIGHT};
        Vector2 rightTop = {bX + BRICK_WIDTH, bY};
        Vector2 rightBottom = {bX + BRICK_WIDTH, bY + BRICK_HEIGHT};

        // ball to the left of brick
        if (ball.pos.y >= bY && ball.pos.y <= bY + BRICK_HEIGHT && distancePointLine(ball.pos, leftTop, leftBottom) <= ball.radius + 0.5)
        {
            collisionNow = true;
            ball.speed.x *= -1;
            ball.pos.x = bX - ball.radius - 1;

        }
        // ball to the right of brick
        else if (ball.pos.y >= bY && ball.pos.y <= bY + BRICK_HEIGHT && distancePointLine(ball.pos, rightTop, rightBottom) <= ball.radius + 0.5)
        {
            collisionNow = true;
            ball.speed.x *= -1;
            ball.pos.x = bX + BRICK_WIDTH + ball.radius + 1;
        }

        // ball above brick
        if (ball.pos.x >= bX && ball.pos.x <= bX + BRICK_WIDTH && distancePointLine(ball.pos, leftTop, rightTop) <= ball.radius + 0.5)
        {
            collisionNow = true;
            ball.speed.y *= -1;
            ball.pos.y = bY - ball.radius - 1;
        }
        // ball below brick
        else if (ball.pos.x >= bX && ball.pos.x <= bX + BRICK_WIDTH && distancePointLine(ball.pos, leftBottom, rightBottom) <= ball.radius + 0.5)
        {
            collisionNow = true;
            ball.speed.y *= -1;
            ball.pos.y = bY + BRICK_HEIGHT + ball.radius + 1;
        }

        //* fallback detection (in case ball gets completely inside the brick)
        if (!collisionNow)
        {
            // ball to the left of brick
            if (ball.pos.x <= bX && ball.pos.y - ball.radius < bY + BRICK_HEIGHT && ball.pos.y + ball.radius > bY)
            {
                ball.speed.x *= -1;
                ball.pos.x = bX - ball.radius - 1;
            }
            // ball to the right of brick
            else if (ball.pos.x >= bX + BRICK_WIDTH && ball.pos.y - ball.radius < bY + BRICK_HEIGHT && ball.pos.y + ball.radius > bY)
            {
                ball.speed.x *= -1;
                ball.pos.x = bX + BRICK_WIDTH + ball.radius + 1;
            }
            // ball above brick
            else if (ball.pos.y <= bY && ball.pos.x - ball.radius < bX + BRICK_WIDTH && ball.pos.x + ball.radius > bX)
            {
                ball.speed.y *= -1;
                ball.pos.y = bY - ball.radius - 1;
            }
            // ball below brick
            else if (ball.pos.y >= bY + BRICK_HEIGHT && ball.pos.x - ball.radius < bX + BRICK_WIDTH && ball.pos.x + ball.radius > bX)
            {
                ball.speed.y *= -1;
                ball.pos.y = bY + BRICK_HEIGHT + ball.radius + 1;
            }

            collisionNow = true;
        }

        //? NOTE: change this to account for ball velocity later
        if (bricks[i].type > 0)
        {
            increaseScore(BASE_BRICK_HIT_SCORE);

            // degrade brick
            bricks[i].type--;
        }

        collision = true;
    }

    return collision;
}

void increaseScore(int change)
{
    playerScore += change * scoreMultiplier;
}

// Check whether conditions for game end are met
// and transition to end game state (gameState = 2)
void checkGameEnd() {
    //? to be implemented
}

// Calculate perpendicular distance between point and a straight line
// (NOT line segment)
double distancePointLine(Vector2 point, Vector2 lPoint1, Vector2 lPoint2)
{
    // slope
    // double num = fabs((lPoint2.x - lPoint1.x) * (lPoint1.y - point.y) - (lPoint1.x - point.x) * (lPoint2.y - lPoint1.y));
    double num = fabs((lPoint2.y - lPoint1.y) * point.x - (lPoint2.x - lPoint1.x) * point.y + lPoint2.x * lPoint1.y - lPoint2.y * lPoint1.x);
    double den = sqrt(pow(lPoint2.x - lPoint1.x, 2) + pow(lPoint2.y - lPoint1.y, 2));
    double dist = 0;

    // if lPoint1 and lPoint2 are same
    if (den == 0)
        dist = sqrt(pow(lPoint2.x - point.x, 2) + pow(lPoint2.y - point.y, 2));
    else
        dist = num / den;
    return dist;
}

// Load all textures
void loadSprites()
{
    lifeTexture = LoadTexture("./assets/life.png");
    ballImage = LoadTexture("./assets/ball.png");
    paddleImage = LoadTexture("./assets/paddles/0.png");        // base paddle
    brickTextures[0] = LoadTexture("./assets/bricks/1.png");    // Type 1
    brickTextures[1] = LoadTexture("./assets/bricks/2.png");    // Type 2
    brickTextures[2] = LoadTexture("./assets/bricks/3.png");    // Type 3
    brickTextures[3] = LoadTexture("./assets/bricks/-1.png");   // Type -1
}

// Inverse function to loadSprites(); unloads all sprites
void unloadSprites()
{
    UnloadTexture(lifeTexture);
    UnloadTexture(ballImage);
    UnloadTexture(paddleImage);
    for (int i = 0; i < NUM_BRICK_TEXTURES; i++)
        UnloadTexture(brickTextures[i]);
}

// Contains all draw calls; func called inside game loop
void drawLoop() {
    // core game
    if (gameState == 1) {
        drawPaddle();
        drawBall();
        drawBricks();
        if (debugView)
            drawDebugView();
        else
            drawMainGameUI();
    }
    // map editor
    else if (gameState == 3)
    {
        drawMapEditor();
    }
}

void drawMainGameUI() {
    // time (left)
    DrawText("Time", PADDING_SIDES_UI, PADDING_ABOVE_UI, 16, RAYWHITE);
    char timeText[20];
    sprintf(timeText, "%02d : %02d", playtime / 60, playtime % 60);
    DrawText(timeText, PADDING_SIDES_UI, PADDING_ABOVE_UI + 16 * 1.4, 16, RAYWHITE);

    // score (middle)
    DrawText("Score", (GetScreenWidth() - MeasureText("Score", 18)) / 2, PADDING_ABOVE_UI, 18, RAYWHITE);
    char scoreText[20];
    sprintf(scoreText, "%d", playerScore);
    DrawText(scoreText, (GetScreenWidth() - MeasureText(scoreText, 18)) / 2, PADDING_ABOVE_UI + 18 * 1.4, 18, RAYWHITE);

    // lives (right)
    for (int i = 0; i < lives; i++) {
        DrawTextureRec(lifeTexture,
            (Rectangle) { 0, 0, lifeTexture.width, lifeTexture.height },
            (Vector2) { GetScreenWidth() - PADDING_SIDES_UI - (i + 1) * lifeTexture.width - i * 5,
                        PADDING_ABOVE_UI + 20 },
            WHITE
        );
    }
}

void drawBall()
{
    DrawTextureEx(ballImage, (Vector2){ball.pos.x - ball.radius, ball.pos.y - ball.radius}, 0.0f, 1.0f, WHITE);
}

void drawPaddle()
{
    DrawTextureEx(paddleImage, (Vector2){paddle.rect.x, paddle.rect.y}, 0.0f, 1.0f, WHITE);
}

// Draw bricks at positions based on type
void drawBricks()
{
    for (int i = 0; i < numBricks; i++)
    {
        // pick color
        Color brickColor = (Color){0, 0, 0, 0};
        Texture2D brickImage;
        if (bricks[i].type == 0)
            continue;
        else if (bricks[i].type == 1)
            brickImage = brickTextures[0];
        else if (bricks[i].type == 2)
            brickImage = brickTextures[1];
        else if (bricks[i].type == 3)
            brickImage = brickTextures[2];
        else if (bricks[i].type == -1)
            brickImage = brickTextures[3];

        // draw brick
        // DrawRectangleRec(bricks[i].rect, brickColor);
        DrawTextureEx(brickImage, (Vector2){bricks[i].rect.x, bricks[i].rect.y}, 0.0f, 1.0f, WHITE);
    }
}

// Draw map editor on screen
void drawMapEditor() {
    // text
    for (int i = 0; i < maxBrickCols || i < maxBrickRows; i++) {
        char numText[20];
        sprintf(numText, "%d", i + 1);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), numText, 15, 0);
        if (i < maxBrickCols){
            DrawText(numText, bricks[i].rect.x + (BRICK_WIDTH - textSize.x) / 2, bricks[i].rect.y - textSize.y - 5, 15, RAYWHITE);
            DrawText(numText, bricks[i].rect.x + (BRICK_WIDTH - textSize.x) / 2, bricks[maxBrickRows * maxBrickCols - 1].rect.y + BRICK_HEIGHT + 5, 15, RAYWHITE);
        }
        if (i < maxBrickRows) {
            DrawText(numText, bricks[i * maxBrickCols].rect.x - textSize.x - 10, bricks[i * maxBrickCols].rect.y + (BRICK_HEIGHT - 15) / 2, 15, RAYWHITE);
            DrawText(numText, bricks[(i + 1) * maxBrickCols - 1].rect.x + BRICK_WIDTH + 10, bricks[(i + 1) * maxBrickCols - 1].rect.y + (BRICK_HEIGHT - 15) / 2, 15, RAYWHITE);
        }
    }

    // bricks & outlines
    drawBricks();
    for (int i = 0; i < numBricks; i++)
        DrawRectangleLinesEx(bricks[i].rect, 1, RAYWHITE);
}

// Toggle debug view
void manageDebugView()
{
    if (gameState != 1)
        return;
    if (IsKeyPressed(KEY_TAB))
    {
        debugView = !debugView;
    }

    if (debugView)
    {
        if (IsKeyDown(KEY_EQUAL))
        {
            ball.speed.x = ball.speed.x + 10 * (ball.speed.x > 0 ? 1 : -1);
            ball.speed.y = ball.speed.y + 10 * (ball.speed.y > 0 ? 1 : -1);
        }
        else if (IsKeyDown(KEY_MINUS))
        {
            ball.speed.x = ball.speed.x - 10 * (ball.speed.x > 0 ? 1 : -1);
            ball.speed.y = ball.speed.y - 10 * (ball.speed.y > 0 ? 1 : -1);
        }
    }
}

// Draw debug view
void drawDebugView()
{
    if (!debugView)
        return;

    // outlines
    DrawCircleLinesV(ball.pos, ball.radius, RED);
    DrawRectangleLinesEx((Rectangle){paddle.rect.x - 4, paddle.rect.y - 2, paddle.rect.width + 8    , paddle.rect.height + 4}, 1, RED);

    // stats
    char fpsText[20] = {'\0'};
    sprintf(fpsText, "FPS: %d", GetFPS());
    DrawText(fpsText, 10, 10, 15, RAYWHITE);

    char scoreText[20] = {'\0'};
    sprintf(scoreText, "Score: %d", playerScore);
    DrawText(scoreText, 10, 30, 15, RAYWHITE);

    char playTimeText[20] = {'\0'};
    sprintf(playTimeText, "PlayTime: %d", playtime);
    DrawText(playTimeText, 10, 50, 15, RAYWHITE);

    char livesText[20] = {'\0'};
    sprintf(livesText, "Lives: %d", lives);
    DrawText(livesText, 10, 70, 15, RAYWHITE);

    char ballSpeedText[50] = {'\0'};
    sprintf(ballSpeedText, "SpeedX: %.2f; SpeedY: %.2f", ball.speed.x, ball.speed.y);
    DrawText(ballSpeedText, GetScreenWidth() - 250, 10, 15, RAYWHITE);

    // map area
    DrawRectangle(PADDING_ON_MAP_SIDES, PADDING_ABOVE_MAP, GetScreenWidth() - PADDING_ON_MAP_SIDES * 2, GetScreenHeight() - PADDING_ABOVE_MAP - PADDING_BELOW_MAP, (Color){255, 0, 0, 50});

    // brick outlines
    for (int i = 0; i < numBricks; i++)
        DrawRectangleLinesEx(bricks[i].rect, 1, RAYWHITE);
}
