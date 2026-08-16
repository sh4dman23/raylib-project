#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

/* Globals and Constants */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GAME_WINDOW_TITLE "DXBall"

//* Game States
#define GS_MAIN_MENU 0   // 0 = main menu (unimplemented)
#define GS_MAIN_GAME 1   // 1 = main game
#define GS_GAME_END 2    // 2 = game end
#define GS_MAP_EDITOR 3  // 3 = map editor
#define GS_HIGH_SCORES 4 // 4 = high scores (unimplemented)

// Main menu
#define MAIN_MENU_LOGO_START 1
#define MAIN_MENU_LOGO_END 31
#define MAIN_MENU_BUTTON_WIDTH 200
#define MAIN_MENU_BUTTON_HEIGHT 40
#define MAIN_MENU_TEXTURES_PATH "./assets/main_menu/"

int gameState = GS_MAIN_MENU;
int gameStateDemo = GS_MAIN_MENU; // test

// Texture for main menu
Texture2D mainMenuLogo[MAIN_MENU_LOGO_END];

//* Core game statistics
int playerScore = 0;
double scoreMultiplier = 1.0;
int playtime = 0;
int lives = 0;
const int STARTING_LIVES = 3;

const int BASE_BRICK_HIT_SCORE = 50;

//* Core game UI
const double PADDING_ABOVE_UI = 20;
const double PADDING_SIDES_UI = 20;

bool debugView = false;

Texture2D lifeTexture;

//* Game End Screen
Texture2D victoryImage;
Texture2D defeatImage;

#define MAX_PLAYER_NAME_LENGTH 16
char nameInputStr[MAX_PLAYER_NAME_LENGTH + 1] = {'\0'};

//* Ball
typedef struct Ball
{
    Vector2 pos;
    Vector2 speed;
    double radius;
} Ball;

Ball ball;

Texture2D ballImage;
const Vector2 INITIAL_BALL_SPEED = (Vector2){300.0, -350};
// slow ball, fast ball speeds (to be implemented)

const double BASE_BALL_RADIUS = 5.0;

bool ballLockedToPaddle = true;           // makes ball stick to paddle, until player presses space
double lastBallLockTime = 0;              // in seconds
const double BALL_OSCILLATION_FREQ = 1.0; // oscillations per second

//* Paddle
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

//* Bricks
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
#define BRICK_TEXTURES_PATH "./assets/bricks"
Texture2D brickTextures[NUM_BRICK_TEXTURES + 1]; // stored as: 0 1 2 3 ... -3 -2 -1

//* Map
#define MAX_NUMBER_OF_MAPS 1000
#define MAP_FILES_PATH "./maps"

int currentMap = 0;
int numberOfMaps = 0;

//* Map Editor
#define NUM_MAP_EDITOR_BUTTONS 5
Rectangle mapEditorButtons[NUM_MAP_EDITOR_BUTTONS];
Texture2D mapEditorButtonTextures[NUM_MAP_EDITOR_BUTTONS];

/* Function Prototypes */
void initializeGame();

void manageDebugView();
void manageGameStateChanges();
void switchGameState(int state);

// Main Menu
void mainMenuScreen();
void createMainMenuButtons();
void checkMainMenuButtonClick(Vector2 mousePos);

// Updates
void updateLoop();

// Non-core game interactions
void manageGameEndUserInput();
void resetAllInput();

// Core Game Logic
void setNewGame();

void resetBall();
void resetPaddle();

void lockBall();
void updateBall();
void updatePaddle();

void checkAllCollisions();
bool checkBallNBrickCollisions(); //! to be improved

void bounceBallOnBoundaries();
bool bounceBallOnPaddle();

void updatePlayTime();

void resetStats();
void increaseScore(int change);

void checkGameEnd();

double distancePointLine(Vector2 point, Vector2 lPoint1, Vector2 lPoint2);

// Game Map
void setEmptyMap();
void initializeAllMaps();
void initializeCurrentMap();
void saveCurrentMap();
void switchToMap(int mapIndex);
void readMapFromFile(FILE *mapFile);
void writeMapToFile(FILE *outputFile);

// Map Editor
void addNewMap();
void deleteCurrentMap();
void setMapEditor();
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
void drawGameEnd();
void drawDebugView();

int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    // initialize game
    initializeGame();
    loadSprites();

    // make it so, esc key doesn't close the game
    SetExitKey(KEY_NULL);

    while (!WindowShouldClose())
    {
        manageDebugView();
        manageGameStateChanges();

        // updates
        updateLoop();

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

void mainMenuScreen()
{
    if (gameState != GS_MAIN_MENU)
        return;

    ClearBackground(BLACK);
    Vector2 mousePositionMainMenu = GetMousePosition();
    createMainMenuButtons();
    checkMainMenuButtonClick(mousePositionMainMenu);
}

// Manage all updates based on game state
void updateLoop()
{
    // core game
    if (gameState == GS_MAIN_GAME)
    {
        updatePlayTime();

        updatePaddle();
        updateBall();

        // collisions
        checkAllCollisions();

        // check whether lives end or breakable bricks are all broken
        checkGameEnd();
    }
    // end game screen
    else if (gameState == GS_GAME_END)
    {
        manageGameEndUserInput();
    }
    // map editor
    else if (gameState == GS_MAP_EDITOR)
    {
        checkMapEdit();
    }
}

// Manage game state changes due to ingame user interaction
void manageGameStateChanges()
{
    if (gameState == GS_MAIN_MENU)
    {
        mainMenuScreen();
        //? (tbd) code for main menu ui interactions that change gamestates go here

        //? currently, just default switch to main game if in main menu
        // switchGameState(GS_MAIN_GAME);
    }

    //! current working method to switch to map editor
    else if (gameState == GS_MAIN_GAME && IsKeyPressed(KEY_F2))
    {
        switchGameState(GS_MAP_EDITOR);
    }
    // switch from map editor back to main game
    else if (gameState == GS_MAP_EDITOR && IsKeyPressed(KEY_F2))
    {
        // open last saved map
        initializeCurrentMap();
        switchGameState(GS_MAIN_GAME);
    }
}

// Switches game states, and does necessary changes
void switchGameState(int state)
{
    if (gameState == state)
        return;

    // main game -> any other mode
    if (gameState == GS_MAIN_GAME)
    {
        debugView = false;
        lockBall();
    }

    // main game -> map editor
    if (gameState == GS_MAIN_GAME && state == GS_MAP_EDITOR)
    {
        setNewGame();
    }

    // end game -> any other mode (only main menu accessible)
    if (gameState == GS_GAME_END)
    {
        setNewGame();
    }

    // map editor -> any other mode
    if (gameState == GS_MAP_EDITOR)
    {
        // switch to first map
        switchToMap(0);
    }

    gameState = state;
}

// main menu buttons for changing states
void createMainMenuButtons()
{
    const int buttonTextFontSize = 22;
    // DrawText(TextFormat("State: %d", gameStateDemo), 10, 10, 100, RAYWHITE);
    Rectangle mainMenuButtonRect = {(WINDOW_WIDTH - MAIN_MENU_BUTTON_WIDTH) / 2, WINDOW_HEIGHT / 2 - MAIN_MENU_BUTTON_HEIGHT, MAIN_MENU_BUTTON_WIDTH, MAIN_MENU_BUTTON_HEIGHT};
    for (int i = 0; i < 6; i++)
    {
        char *menuButtonText;
        switch (i)
        {
        case 0:
            menuButtonText = "New Game";
            break;
        case 1:
            menuButtonText = "High Scores";
            break;
        case 2:
            menuButtonText = "Map Maker";
            break;
        case 3:
            menuButtonText = "Help";
            break;
        case 4:
            menuButtonText = "Exit";
            break;
        case 5:
            menuButtonText = "Ho Lee Shit";
            break;
        }
        int textWidth = MeasureText(menuButtonText, buttonTextFontSize);
        DrawRectangleRec(mainMenuButtonRect, RED);
        DrawText(menuButtonText, mainMenuButtonRect.x + (mainMenuButtonRect.width - textWidth) / 2, mainMenuButtonRect.y + (mainMenuButtonRect.height - buttonTextFontSize) / 2, buttonTextFontSize, WHITE);
        mainMenuButtonRect.y += (10 + mainMenuButtonRect.height);
    }
}

// checking which button was clicked
void checkMainMenuButtonClick(Vector2 mousePos)
{
    int x = (WINDOW_WIDTH - MAIN_MENU_BUTTON_WIDTH) / 2;
    int y = WINDOW_HEIGHT / 2 - MAIN_MENU_BUTTON_HEIGHT;
    bool insideRectX_Axis = mousePos.x >= x && mousePos.x <= (x + MAIN_MENU_BUTTON_WIDTH);
    if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT))
    {
        switchGameState(GS_MAIN_GAME);
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 1 + 10 * 1 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 2 + 10 * 1))
    {
        switchGameState(GS_MAIN_MENU); // for high scores //*unimplimented
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 2 + 10 * 2 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 3 + 10 * 2))
    {
        switchGameState(GS_MAP_EDITOR);
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 3 + 10 * 3 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 4 + 10 * 3))
    {
        gameStateDemo = 4;
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 4 + 10 * 4 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 5 + 10 * 4))
    {
        gameStateDemo = 5;
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 5 + 10 * 5 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 6 + 10 * 5))
    {
        gameStateDemo = 6;
    }
}

// Function called at start of program to initialize everything
void initializeGame()
{
    initializeAllMaps();
    setNewGame();
    setMapEditor();
}

// Reset everything in memory to prepare for new game
void setNewGame()
{
    numBricks = maxBrickCols * maxBrickRows;
    currentMap = 0;

    initializeCurrentMap();
    resetPaddle();
    resetBall();
    resetStats();
    resetAllInput();
}

// Update playtime variable every second
void updatePlayTime()
{
    static double time = 0;
    const double interval = 1.0;

    if (gameState != GS_MAIN_GAME || ballLockedToPaddle)
        return;

    time += GetFrameTime();
    if (time >= interval)
    {
        playtime++;
        time = 0;
    }
}

// Resets everything related to player input in memory
void resetAllInput()
{
    sprintf(nameInputStr, "");
}

void resetStats()
{
    playerScore = 0;
    lives = STARTING_LIVES;
    playtime = 0;
    scoreMultiplier = 1.0;
}

// Find number of maps and data related to that map
void initializeAllMaps()
{
    currentMap = 0;
    numberOfMaps = 0;

    while (true)
    {
        char mapFilePath[20] = {'\0'};
        sprintf(mapFilePath, "%s/%d.txt", MAP_FILES_PATH, currentMap);

        FILE *mapFile = fopen(mapFilePath, "r");
        if (mapFile == NULL)
        {
            break;
        }

        readMapFromFile(mapFile);
        fclose(mapFile);
        numberOfMaps++;
        currentMap++;
    }

    currentMap = 0;
}

// Initializes current map
void initializeCurrentMap()
{
    setEmptyMap();

    char mapFilePath[20];
    sprintf(mapFilePath, "%s/%d.txt", MAP_FILES_PATH, currentMap);

    FILE *mapInputFile = fopen(mapFilePath, "r");

    readMapFromFile(mapInputFile);
    fclose(mapInputFile);
}

// Switch to another map
void switchToMap(int mapIndex)
{
    if (mapIndex < 0)
        currentMap = numberOfMaps + mapIndex % numberOfMaps;
    else
        currentMap = mapIndex;

    currentMap %= numberOfMaps;

    initializeCurrentMap();
}

void saveCurrentMap()
{
    char mapFilePath[20];
    sprintf(mapFilePath, "%s/%d.txt", MAP_FILES_PATH, currentMap);

    FILE *mapFile = fopen(mapFilePath, "w");
    writeMapToFile(mapFile);
    fclose(mapFile);
}

// Add new map file
void addNewMap()
{
    if (numberOfMaps == MAX_NUMBER_OF_MAPS)
        return;
    numberOfMaps++;
    currentMap = numberOfMaps - 1;
    setEmptyMap();
    saveCurrentMap();
}

// Delete current map (if number of maps remaining > 1)
void deleteCurrentMap()
{
    if (numberOfMaps <= 1)
        return;

    int lastMap = currentMap;
    while (currentMap < numberOfMaps - 1)
    {
        // copy (i + 1)th map to the ith map
        switchToMap(currentMap + 1);
        currentMap--;
        saveCurrentMap();
        currentMap++;
    }
    numberOfMaps--;

    char mapFilePath[20];
    sprintf(mapFilePath, "%s/%d.txt", MAP_FILES_PATH, currentMap);

    // delete extra file
    FileRemove(mapFilePath);

    // switch to map originally succeeding the deleted map
    if (lastMap < numberOfMaps)
        currentMap = lastMap;
    else
        currentMap = numberOfMaps - 1;
    switchToMap(currentMap);
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
                0 // type 0 = empty brick
            };

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
    fscanf(mapFile, "%d %d ", &brickRows, &brickCols);

    int numBreakableBricks = 0;
    for (int i = 0; i < brickRows; i++)
    {
        for (int j = 0; j < brickCols; j++)
        {
            int brickType = 0;
            fscanf(mapFile, "%d ", &brickType);

            bricks[i * maxBrickCols + j].type = brickType;

            //* count number of bricks in level that can be broken
            if (brickType == 1 || brickType == 2 || brickType == 3)
                numBreakableBricks++;
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
    Vector2 mousePos = GetMousePosition();

    // save map
    if (IsKeyPressed(KEY_ENTER))
    {
        saveCurrentMap();
    }

    // reset all changes
    else if (IsKeyPressed(KEY_BACKSPACE))
    {
        initializeCurrentMap();
    }

    // cycle brick type right
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
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
        for (int i = 0; i < maxBrickCols * maxBrickRows; i++)
        {
            if (CheckCollisionPointRec(mousePos, bricks[i].rect))
            {
                bricks[i].type = 0;
            }
        }
    }

    // interaction with map editor buttons
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        // circle left
        if (CheckCollisionPointRec(mousePos, mapEditorButtons[1]))
        {
            switchToMap(currentMap - 1);
        }

        // circle right
        else if (CheckCollisionPointRec(mousePos, mapEditorButtons[2]))
        {
            switchToMap(currentMap + 1);
        }

        // add new map
        else if (CheckCollisionPointRec(mousePos, mapEditorButtons[3]))
        {
            addNewMap();
        }

        // delete current map
        else if (CheckCollisionPointRec(mousePos, mapEditorButtons[4]))
        {
            deleteCurrentMap();
        }
    }

    // arrow keys to change map
    if (IsKeyPressed(KEY_LEFT))
    {
        switchToMap(currentMap - 1);
    }
    else if (IsKeyPressed(KEY_RIGHT))
    {
        switchToMap(currentMap + 1);
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

        // store collision with brick
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
        if (bounceBallOnPaddle())
        {
            // signed distance between ball center and paddle center
            double delx = ball.pos.x - (paddle.rect.x + paddle.rect.width / 2);

            // add accelerated ball speed here
            ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * (fabs(delx) / (paddle.rect.width / 2)) * max(fabs(ball.speed.x), INITIAL_BALL_SPEED.x);

            // if dx and vx have opposite signs, flip vx
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
    else if (CheckCollisionCircleRec(ball.pos, ball.radius, (Rectangle){paddle.rect.x - 4, paddle.rect.y, paddle.rect.width + 8, paddle.rect.height}))
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

// Increase player score based on score multiplier
void increaseScore(int change)
{
    playerScore += change * scoreMultiplier;
}

// Check whether conditions for game end are met
void checkGameEnd()
{
    // death
    if (lives <= 0)
    {
        lockBall();
        switchGameState(GS_GAME_END);
    }
    //? implement change to next map or show victory screen
}

// Manage user name input by keyboard
void manageGameEndUserInput()
{
    char ch = GetCharPressed();

    // add character to name
    if (isalnum(ch) && strlen(nameInputStr) < MAX_PLAYER_NAME_LENGTH)
    {
        strncat(nameInputStr, &ch, 1);
    }

    // backspace
    else if (IsKeyPressed(KEY_BACKSPACE) && strlen(nameInputStr) > 0)
    {
        nameInputStr[strlen(nameInputStr) - 1] = '\0';
    }

    // save score
    else if (IsKeyPressed(KEY_ENTER))
    {
        //! (tbd) store high score
        setNewGame();
        switchGameState(GS_MAIN_MENU);
    }

    // delete score (no save)
    else if (IsKeyPressed(KEY_ESCAPE))
    {
        setNewGame();
        switchGameState(GS_MAIN_MENU);
    }
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
        dist = Vector2Distance(point, lPoint1);
    else
        dist = num / den;
    return dist;
}

// Load all textures
void loadSprites()
{
    // Loading Textures For Main Menu
    for (int i = MAIN_MENU_LOGO_START - 1; i < MAIN_MENU_LOGO_END; i++)
    {
        char mainMenuLogoTextureFilePath[50];
    }

    mapEditorButtonTextures[1] = LoadTexture("./assets/ui/larrow.png");
    mapEditorButtonTextures[2] = LoadTexture("./assets/ui/rarrow.png");
    mapEditorButtonTextures[3] = LoadTexture("./assets/ui/plus.png");
    mapEditorButtonTextures[4] = LoadTexture("./assets/ui/delete.png");

    victoryImage = LoadTexture("./assets/ui/victory.png");
    defeatImage = LoadTexture("./assets/ui/defeat.png");

    lifeTexture = LoadTexture("./assets/ui/life.png");
    ballImage = LoadTexture("./assets/ball.png");
    paddleImage = LoadTexture("./assets/paddles/0.png"); // base paddle

    // store brickTextures as: 0 1 2 3 ... -3 -2 -1
    for (int i = MIN_BRICK_TYPE; i <= MAX_BRICK_TYPE; i++)
    {
        char brickTextureFilePath[50];
        int brickTextureIndex = 0;

        if (i == 0)
            continue;
        else if (i > 0)
            brickTextureIndex = i;
        else if (i < 0)
            brickTextureIndex = NUM_BRICK_TEXTURES + i + 1;

        sprintf(brickTextureFilePath, "%s/%d.png", BRICK_TEXTURES_PATH, i);
        brickTextures[brickTextureIndex] = LoadTexture(brickTextureFilePath);
    }
}

// Inverse function to loadSprites(); unloads all sprites
void unloadSprites()
{
    for (int i = 0; i < NUM_MAP_EDITOR_BUTTONS; i++)
        UnloadTexture(mapEditorButtonTextures[i]);

    UnloadTexture(victoryImage);
    UnloadTexture(defeatImage);

    UnloadTexture(lifeTexture);
    UnloadTexture(ballImage);
    UnloadTexture(paddleImage);

    for (int i = 0; i <= NUM_BRICK_TEXTURES; i++)
        UnloadTexture(brickTextures[i]);
}

// Contains all draw calls; func called inside game loop
void drawLoop()
{
    // core game
    if (gameState == GS_MAIN_GAME)
    {
        drawPaddle();
        drawBall();
        drawBricks();
        if (debugView)
            drawDebugView();
        else
            drawMainGameUI();
    }
    // game end
    else if (gameState == GS_GAME_END)
    {
        drawGameEnd();
    }
    // map editor
    else if (gameState == GS_MAP_EDITOR)
    {
        drawMapEditor();
    }
}

void drawMainGameUI()
{
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
    for (int i = 0; i < lives; i++)
    {
        DrawTextureRec(lifeTexture,
                       (Rectangle){0, 0, lifeTexture.width, lifeTexture.height},
                       (Vector2){GetScreenWidth() - PADDING_SIDES_UI - (i + 1) * lifeTexture.width - i * 5,
                                 PADDING_ABOVE_UI + 20},
                       WHITE);
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
        // pick texture
        Texture2D brickImage;
        if (bricks[i].type == 0)
            continue;
        else if (bricks[i].type > 0)
            brickImage = brickTextures[bricks[i].type];
        else if (bricks[i].type < 0)
            brickImage = brickTextures[NUM_BRICK_TEXTURES + bricks[i].type + 1];

        // draw brick
        DrawTextureEx(brickImage, (Vector2){bricks[i].rect.x, bricks[i].rect.y}, 0.0f, 1.0f, WHITE);
    }
}

// Setup buttons in map editor
void setMapEditor()
{
    int px = 0, py = 20;

    const int fontSize = 16;
    const Vector2 mapNameBoxDimensions = {150, fontSize * 2};
    const Vector2 otherBoxDimensions = {fontSize * 2, fontSize * 2};
    const int spacing = 5;

    px = (GetScreenWidth() - mapNameBoxDimensions.x - otherBoxDimensions.x * 3 - spacing * 3) / 2;

    // map name box
    mapEditorButtons[0] = (Rectangle){px, py, mapNameBoxDimensions.x, mapNameBoxDimensions.y};
    px += mapNameBoxDimensions.x + spacing;

    // other 4
    for (int i = 0; i < 4; i++)
    {
        mapEditorButtons[i + 1] = (Rectangle){px + i * otherBoxDimensions.x + i * spacing, py, otherBoxDimensions.x, otherBoxDimensions.y};
    }
}

// Draw map editor on screen
void drawMapEditor()
{
    const int fontSize = 16;

    // box for showing # of current selected map
    DrawRectangleLinesEx(mapEditorButtons[0], 1, WHITE);

    char mapText[20];
    sprintf(mapText, "Map %d", currentMap + 1);
    DrawText(mapText, mapEditorButtons[0].x + 10, mapEditorButtons[0].y + (mapEditorButtons[0].height - fontSize) / 2, fontSize, WHITE);

    // buttons to change current selected map
    for (int i = 1; i <= 4; i++)
    {
        DrawRectangleLinesEx(mapEditorButtons[i], 1, WHITE);

        DrawTexturePro(
            mapEditorButtonTextures[i],
            (Rectangle){0, 0, mapEditorButtonTextures[i].width, mapEditorButtonTextures[i].height},
            (Rectangle){mapEditorButtons[i].x + (mapEditorButtons[i].width - fontSize) / 2, mapEditorButtons[i].y + fontSize / 2, fontSize, fontSize},
            (Vector2){0, 0},
            0.0f,
            ((i == 4 && numberOfMaps <= 1) || (i == 3 && numberOfMaps >= MAX_NUMBER_OF_MAPS)) ? GRAY : WHITE);
    }

    // color based on whether mouse points to corresponding brick
    Color notHighlighted = (Color){255, 255, 255, .35 * 255}, highlighted = RAYWHITE;
    Vector2 mousePos = GetMousePosition();

    // row and column numbers
    for (int i = 0; i < maxBrickCols || i < maxBrickRows; i++)
    {
        char numText[20];
        sprintf(numText, "%d", i + 1);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), numText, 15, 0);

        // check if mouse is on ANY brick at all
        bool mouseOnBricks = CheckCollisionPointRec(mousePos, (Rectangle){
                                                                  bricks[0].rect.x,
                                                                  bricks[0].rect.y,
                                                                  maxBrickCols * BRICK_WIDTH,
                                                                  maxBrickRows * BRICK_HEIGHT});

        Color numColor;
        if (i < maxBrickCols)
        {
            if (mousePos.x >= bricks[i].rect.x && mousePos.x <= bricks[i].rect.x + BRICK_WIDTH && mouseOnBricks)
                numColor = highlighted;
            else
                numColor = notHighlighted;

            DrawText(numText, bricks[i].rect.x + (BRICK_WIDTH - textSize.x) / 2, bricks[i].rect.y - textSize.y - 5, 15, numColor);
            DrawText(numText, bricks[i].rect.x + (BRICK_WIDTH - textSize.x) / 2, bricks[maxBrickRows * maxBrickCols - 1].rect.y + BRICK_HEIGHT + 5, 15, numColor);
        }

        if (i < maxBrickRows)
        {
            if (mousePos.y >= bricks[i * maxBrickCols].rect.y && mousePos.y <= bricks[i * maxBrickCols].rect.y + BRICK_HEIGHT && mouseOnBricks)
                numColor = highlighted;
            else
                numColor = notHighlighted;

            DrawText(numText, bricks[i * maxBrickCols].rect.x - textSize.x - 10, bricks[i * maxBrickCols].rect.y + (BRICK_HEIGHT - 15) / 2, 15, numColor);
            DrawText(numText, bricks[(i + 1) * maxBrickCols - 1].rect.x + BRICK_WIDTH + 10, bricks[(i + 1) * maxBrickCols - 1].rect.y + (BRICK_HEIGHT - 15) / 2, 15, numColor);
        }
    }

    // bricks
    drawBricks();

    // brick outlines
    for (int i = 0; i < numBricks; i++)
    {
        // check if mouse inside brick
        if (CheckCollisionPointRec(mousePos, bricks[i].rect))
            DrawRectangleLinesEx(bricks[i].rect, 2, highlighted);
        else
            DrawRectangleLinesEx(bricks[i].rect, 1, notHighlighted);
    }
}

void drawGameEnd()
{
    const int fontSize = 18;

    double px = 0, py = PADDING_ABOVE_MAP;
    Texture2D statusImage;

    // final map finished
    if (lives > 0)
        statusImage = victoryImage;
    // lives ran out
    else
        statusImage = defeatImage;

    DrawTexture(statusImage, (GetScreenWidth() - statusImage.width) / 2, py, WHITE);
    py += 25 + statusImage.height;

    // score
    char scoreText[50];
    sprintf(scoreText, "Score: %d", playerScore);
    DrawText(scoreText, (GetScreenWidth() - MeasureText(scoreText, fontSize)) / 2, py, fontSize, RAYWHITE);
    py += fontSize * 1.5;

    // time played
    char timeText[50];
    sprintf(timeText, "Time Played: ");
    if (playtime / 60 > 0)
    {
        char minText[20];
        sprintf(minText, "%d minutes, ", playtime / 60);
        strcat(timeText, minText);
    }
    char secText[20];
    sprintf(secText, "%d seconds", playtime % 60);
    strcat(timeText, secText);

    DrawText(timeText, (GetScreenWidth() - MeasureText(timeText, fontSize)) / 2, py, fontSize, WHITE);
    py += fontSize * 2;

    // show name prompt
    char *namePromptText = "Enter your name to save score: ";
    DrawText(namePromptText, (GetScreenWidth() - MeasureText(namePromptText, fontSize)) / 2, py, fontSize, WHITE);
    py += fontSize * 1.4;

    // show currently entered name
    char nameDisplayStr[MAX_PLAYER_NAME_LENGTH + 2];
    sprintf(nameDisplayStr, "%s%c", nameInputStr, time(NULL) % 2 ? '_' : ' ');
    DrawText(nameDisplayStr, (GetScreenWidth() - MeasureText(nameDisplayStr, fontSize)) / 2, py, fontSize, WHITE);
    py += fontSize * 1.4;
}

// Toggle debug view
void manageDebugView()
{
    if (gameState != GS_MAIN_GAME)
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
    DrawRectangleLinesEx((Rectangle){paddle.rect.x - 4, paddle.rect.y - 2, paddle.rect.width + 8, paddle.rect.height + 4}, 1, RED);

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
