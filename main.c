#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

/* Globals and Constants */
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GAME_WINDOW_TITLE "DXBall"

//* Game States
#define GS_MAIN_MENU 0   // 0 = main menu
#define GS_MAIN_GAME 1   // 1 = main game
#define GS_GAME_END 2    // 2 = game end
#define GS_MAP_EDITOR 3  // 3 = map editor
#define GS_HIGH_SCORES 4 // 4 = high scores

//* Main menu
#define MAIN_MENU_LOGO_START 1
#define MAIN_MENU_LOGO_END 31
#define MAIN_MENU_BALL_START 1
#define MAIN_MENU_BALL_END 14
#define MAIN_MENU_BUTTON_WIDTH 200
#define MAIN_MENU_BUTTON_HEIGHT 40
#define MAIN_MENU_TEXTURES_PATH "./assets/main_menu/"

int gameState = GS_MAIN_MENU;
bool exitGame = false;

int mainMenuLogoCurrentFrame = 0;
const float mainMenuLogoFrameTime = 0.065f;
float mainMenuLogoTimer = 0.0f;
int mainMenuBallCurrentFrame = 0;
const float mainMenuBallFrameTime = (mainMenuLogoFrameTime / MAIN_MENU_LOGO_END) * MAIN_MENU_BALL_END / 1.8;
float mainMenuBallTimer = 0.0f;

// Texture for main menu
Texture2D mainMenuLogo[MAIN_MENU_LOGO_END];
Texture2D mainMenuBall[MAIN_MENU_BALL_END];

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

//* High Scores
#define MAX_HIGH_SCORES 10
#define HIGH_SCORES_FILE_PATH "./data/highscores.txt"

typedef struct HSEntry
{
    int score;
    int playtime;
    char name[MAX_PLAYER_NAME_LENGTH + 1];
} HSEntry;

HSEntry highScores[MAX_HIGH_SCORES];
int numHighScores = 0; // store how many high scores are present in memory

Texture2D highScoresTitleImage; // image for high scores title

//* Ball
typedef struct Ball
{
    Vector2 pos;
    Vector2 speed;
    double radius;
} Ball;

Ball ball;

Texture2D ballImage;
const Vector2 INITIAL_BALL_SPEED = (Vector2) {300, -350};       // base starting speed
const Vector2 ACCELERATED_BALL_SPEED = (Vector2) {350, -400};   // speed after which the ball will start decelerating
const Vector2 BALL_ACCELERATION = (Vector2) { 15, 15 };        // deceleration rate for ball
// slow ball, fast ball speeds (to be implemented)

const double BASE_BALL_RADIUS = 5.0;

bool ballLockedToPaddle = true;           // makes ball stick to paddle, until player presses space
double lastBallLockTime = 0;              // in seconds
const double BALL_OSCILLATION_FREQ = 1.0; // oscillations per second

//* Paddle
typedef struct Paddle
{
    Rectangle rect;         // posx, posy, width, height
    Vector2 speed;          // paddle speed
    Texture2D image;        // paddle texture
} Paddle;

#define NUMBER_OF_PADDLES 3
#define BASE_PADDLE 0
#define EXPANDED_PADDLE 1
#define SHRUNK_PADDLE 2

Paddle paddles[3];

int currentPaddle = BASE_PADDLE;

const int BASE_PADDLE_WIDTH = 74;
const int BASE_PADDLE_HEIGHT = 15;
const Vector2 PADDLE_SPEED = (Vector2){10, 0}; //* unit: pixels per key input
const int SPACE_BELOW_PADDLE = 5;              // pixels below paddle


//* Perks
#define PERKS_IMG_PATH "./assets/perks/"
const Vector2 PERK_IMG_SIZE = {32, 30};

const Vector2 PERK_SPEED = {0, 200};
const double DELAY_AFTER_PERK_SPAWN = 2; // seconds
const double TIMED_PERK_DURATION = 10;
const int BASE_PERK_ACTIVATE_SCORE = 50;

bool canSpawnPerk = true;

typedef struct Perk
{
    char filename[30];          // without .png
    double spawnChance;         // in %
    bool timed;
    Vector2 pos;
    double duration;            // remaining duration
    Texture2D img;
} Perk;

#define NUMBER_OF_PERKS 7
Perk perks[NUMBER_OF_PERKS] = {
    // Kill Paddle
    (Perk){
        "killpaddle",
        10,
        false,
    },

    // Extra Life
    (Perk){
        "extralife",
        0.5,
        false,
    },

    // Double Points
    (Perk){
        "doublepoints",
        4,
        true,
    },

    // Expand Paddle
    (Perk){
        "expandpaddle",
        4,
        false,
    },

    // Shrink Paddle
    (Perk){
        "shrinkpaddle",
        4,
        false,
    },

    // Slow Ball
    (Perk) {
        "slowball",
        4,
        false,
    },

    // Fast Ball
    (Perk) {
        "fastball",
        8,
        false,
    },

};

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

int breakableBricksLeft = 0; // number of bricks remaining until level ends

// brick types = 0 (empty), 1, 2, 3 (standard), -1 (unbreakable)
typedef struct Brick
{
    Rectangle rect;
    int type;
} Brick;

// required for map editor brick changes
const int MAX_BRICK_TYPE = 3;
const int MIN_BRICK_TYPE = -1;
bool justMouseClicked = false;

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

//* Audio
#define MUSIC_FILES_PATH "./audio/music"

const int NUMBER_OF_MUSIC_FILES = 5;
int currMusicIndex = 0;
Music currMusic;

// check whether music has just been stopped, flag reset after changing music
bool musicStopped = false;

/* Function Prototypes */
void initializeGame();

void manageDebugView();
void manageGameStateChanges();
void switchGameState(int state);

//* Main Menu
void manageMainMenuScreen();
void mainMenuLogoAnimations();
void createMainMenuButtons();
void checkMainMenuButtonClick(Vector2 mousePos);

//* General updates
void updateLoop();

//* Non-core game interactions
void manageGameEndUserInput();
void resetAllInput();

//* Core Game Logic
void setNewGame();
void setNewLevel();

void checkLevelEnd();
void checkGameEnd();

// Ball and Paddle
void resetBall();
void resetPaddle();

void lockBall();
void updateBall();
void manageBallAcceleration();

void switchPaddle(int type);
void updatePaddle();

void killPaddle();
void increaseLives();

// Collisions
void checkAllCollisions();
bool checkBallNBrickCollisions();
void degradeBrick(int brickIndex);

void bounceBallOnBoundaries();
bool bounceBallOnPaddle();

// Stats
void updatePlayTime();
void resetStats();
void increaseScore(int change);

// Perks
void resetPerks();
void spawnPerk(int brickIndex);
void updatePerks();
void delayPerkSpawn();
void activatePerk(int perkIndex);
void deactivatePerk(int perkIndex);
void checkPerkAndBrickCollision(int perkIndex);

// Math
double distancePointLine(Vector2 point, Vector2 lPoint1, Vector2 lPoint2);

//* Game Map
void setEmptyMap();
void initializeAllMaps();
void initializeCurrentMap();
void saveCurrentMap();
void switchToMap(int mapIndex);
void readMapFromFile(FILE *mapFile);
void writeMapToFile(FILE *outputFile);

//* Map Editor
void addNewMap();
void deleteCurrentMap();
void setMapEditor();
void checkMapEdit();

//* High Scores
void readHighScores();
void sortHighScores();
void saveNewScore();
void writeHighScores();

//* Sprites
void loadSprites();
void unloadSprites();

//* Draw Functions
void drawLoop();
void drawMainGame();
void drawDebugView();
void drawMapEditor();
void drawHighScoresScreen();
void drawGameEnd();

void drawMainGameUI();
void drawBall();
void drawPaddle();
void drawBricks();
void drawPerks();

//* Music
void updateAudio();
void rotateMusic();
void switchMusic(int musicIndex);
void unloadAudio();
void checkMusicChange();

int main(void)
{
    SetTraceLogLevel(LOG_ALL);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    InitAudioDevice();

    SetTargetFPS(60);

    loadSprites();

    // initialize game
    initializeGame();

    // make it so, esc key doesn't close the game
    SetExitKey(KEY_NULL);

    while (!WindowShouldClose() && !exitGame)
    {
        manageDebugView();
        manageGameStateChanges();

        // updates
        updateLoop();
        updateAudio();

        // draw
        BeginDrawing();
        ClearBackground(BLACK);
        drawLoop();
        EndDrawing();
    }

    unloadSprites();
    unloadAudio();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void manageMainMenuScreen()
{
    if (gameState != GS_MAIN_MENU)
        return;

    ClearBackground(BLACK);
    mainMenuLogoAnimations();
    createMainMenuButtons();
}

// Manage all updates based on game state
void updateLoop()
{
    // main menu
    if (gameState == GS_MAIN_MENU)
    {
        manageMainMenuScreen();
    }

    // core game
    if (gameState == GS_MAIN_GAME)
    {
        updatePlayTime();

        updatePerks();
        updatePaddle();
        updateBall();

        // collisions
        checkAllCollisions();

        // check if lives are over
        checkGameEnd();

        // check whether level is beaten
        checkLevelEnd();
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
        checkMainMenuButtonClick(GetMousePosition());
    }

    // escape key pressed from any game state except main menu
    if (gameState != GS_MAIN_MENU && IsKeyPressed(KEY_ESCAPE))
    {
        switchGameState(GS_MAIN_MENU);
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

    // main game -> main menu
    if (gameState == GS_MAIN_GAME && state == GS_MAIN_MENU)
    {
        // erase progress
        setNewGame();
    }

    // main menu / main game -> map editor
    if ((gameState == GS_MAIN_MENU || gameState == GS_MAIN_GAME) && state == GS_MAP_EDITOR)
    {
        setNewGame();
    }

    // main menu -> main game
    if (gameState == GS_MAIN_MENU && state == GS_MAIN_GAME) {
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

// main menu animations
void mainMenuLogoAnimations()
{
    float frameTime = GetFrameTime();
    mainMenuLogoTimer += frameTime;
    mainMenuBallTimer += frameTime;
    if (mainMenuLogoTimer >= mainMenuLogoFrameTime)
    {
        mainMenuLogoTimer = 0.0f;
        mainMenuLogoCurrentFrame = (mainMenuLogoCurrentFrame + 1) % MAIN_MENU_LOGO_END;
    }
    if (mainMenuBallTimer >= mainMenuBallFrameTime)
    {
        mainMenuBallTimer = 0.0f;
        mainMenuBallCurrentFrame = (mainMenuBallCurrentFrame + 1) % MAIN_MENU_BALL_END;
    }

    for (int i = MAIN_MENU_LOGO_START - 1; i < MAIN_MENU_LOGO_END; i++)
    {
        DrawTexture(mainMenuLogo[mainMenuLogoCurrentFrame], (WINDOW_WIDTH - 600) / 2, WINDOW_HEIGHT / 2 - 230, WHITE);
    }
    for (int i = MAIN_MENU_BALL_START - 1; i < MAIN_MENU_BALL_END; i++)
    {
        DrawTexture(mainMenuBall[mainMenuBallCurrentFrame], WINDOW_WIDTH - 180, WINDOW_HEIGHT - 180, WHITE);
    }
}

// main menu buttons for changing states
void createMainMenuButtons()
{
    const int buttonTextFontSize = 22;
    Rectangle mainMenuButtonRect = {(WINDOW_WIDTH - MAIN_MENU_BUTTON_WIDTH) / 2, WINDOW_HEIGHT / 2 - MAIN_MENU_BUTTON_HEIGHT, MAIN_MENU_BUTTON_WIDTH, MAIN_MENU_BUTTON_HEIGHT};
    for (int i = 0; i < 4; i++)
    {
        char *menuButtonText;
        switch (i)
        {
        case 0:
            menuButtonText = "New Game";
            break;
        case 1:
            menuButtonText = "Map Maker";
            break;
        case 2:
            menuButtonText = "High Scores";
            break;
        case 3:
            menuButtonText = "Exit";
            break;
        default:
            break;
        }
        int textWidth = MeasureText(menuButtonText, buttonTextFontSize);
        DrawText(menuButtonText, mainMenuButtonRect.x + (mainMenuButtonRect.width - textWidth) / 2, mainMenuButtonRect.y + (mainMenuButtonRect.height - buttonTextFontSize) / 2, buttonTextFontSize, WHITE);
        mainMenuButtonRect.y += (10 + mainMenuButtonRect.height);
    }
}

// Check button clicks in main menu
void checkMainMenuButtonClick(Vector2 mousePos)
{
    int x = (WINDOW_WIDTH - MAIN_MENU_BUTTON_WIDTH) / 2;
    int y = WINDOW_HEIGHT / 2 - MAIN_MENU_BUTTON_HEIGHT;
    bool insideRectX_Axis = (mousePos.x >= x && mousePos.x <= (x + MAIN_MENU_BUTTON_WIDTH));

    if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT))
    {
        switchGameState(GS_MAIN_GAME);
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 1 + 10 * 1 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 2 + 10 * 1))
    {
        justMouseClicked = true;
        switchGameState(GS_MAP_EDITOR);
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 2 + 10 * 2 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 3 + 10 * 2))
    {
        switchGameState(GS_HIGH_SCORES);
    }
    else if (insideRectX_Axis && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (mousePos.y >= y + MAIN_MENU_BUTTON_HEIGHT * 3 + 10 * 3 && mousePos.y <= y + MAIN_MENU_BUTTON_HEIGHT * 4 + 10 * 3))
    {
        exitGame = true;
    }
}

// Function called at start of program to initialize everything
void initializeGame()
{
    srand(time(NULL));
    SetRandomSeed(time(NULL));

    initializeAllMaps();
    setNewGame();
    setMapEditor();

    readHighScores();

    switchMusic(currMusicIndex);
}

// Reset everything in memory to prepare for new game
void setNewGame()
{
    numBricks = maxBrickCols * maxBrickRows;
    currentMap = 0;

    initializeCurrentMap();

    resetBall();
    resetPaddle();
    resetPerks();

    resetStats();
    resetAllInput();
}

// Transition to new map
void setNewLevel()
{
    resetPaddle();
    resetBall();
    resetAllInput();
    switchToMap(currentMap);
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

// Switch to another map and initializes it
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

    breakableBricksLeft = numBreakableBricks;
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
    //* So that the map doesn't update automatically
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT))
        justMouseClicked = false;
    if (justMouseClicked)
        return;

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
    ball.pos = (Vector2){paddles[currentPaddle].rect.x + paddles[currentPaddle].rect.width / 2, paddles[currentPaddle].rect.y - ball.radius - 1};
    ball.speed = (Vector2) {fabs(INITIAL_BALL_SPEED.x), -fabs(INITIAL_BALL_SPEED.y)};
    ball.radius = BASE_BALL_RADIUS;
}

// Switch paddle at current position
void switchPaddle(int type)
{
    if (type < 0 || type > NUMBER_OF_PADDLES)
        return;

    Rectangle oldRec = paddles[currentPaddle].rect;
    currentPaddle = type;

    paddles[type].rect = (Rectangle) {
        oldRec.x + (oldRec.width - paddles[type].image.width) / 2,
        oldRec.y + (oldRec.height - paddles[type].image.height) / 2,
        paddles[type].image.width,
        paddles[type].image.height
    };
    paddles[type].speed = PADDLE_SPEED;
}

// Reset paddle to starting position and base type
void resetPaddle()
{
    switchPaddle(BASE_PADDLE);

    paddles[currentPaddle].rect = (Rectangle) {
        (GetScreenWidth() - paddles[currentPaddle].image.width) / 2,
        GetScreenHeight() - paddles[currentPaddle].image.height - SPACE_BELOW_PADDLE,
        paddles[currentPaddle].image.width,
        paddles[currentPaddle].image.height
    };

    paddles[currentPaddle].speed = PADDLE_SPEED;
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
        double amp = paddles[currentPaddle].rect.width / 2.0 - ball.radius;
        double timeSinceBallLock = GetTime() - lastBallLockTime;
        double delx = amp * sin(2 * 3.14159 * BALL_OSCILLATION_FREQ * timeSinceBallLock);

        ball.pos = (Vector2){paddles[currentPaddle].rect.x + paddles[currentPaddle].rect.width / 2 + delx, paddles[currentPaddle].rect.y - ball.radius - 1};

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
            double delx = ball.pos.x - (paddles[currentPaddle].rect.x + paddles[currentPaddle].rect.width / 2);

            // accelerated speed based on distance from center
            ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * (fabs(delx) / (paddles[currentPaddle].rect.width / 2)) * max(fabs(ball.speed.x), INITIAL_BALL_SPEED.x) * 2;

            // if dx and vx have opposite signs, flip vx
            if (delx > 0 != ball.speed.x > 0)
                ball.speed.x *= -1;

            // max speed from bouncing is accelerated speed
            if (fabs(ball.speed.x) > fabs(ACCELERATED_BALL_SPEED.x))
                ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * fabs(ACCELERATED_BALL_SPEED.x);
        }

        manageBallAcceleration();
    }
}

void manageBallAcceleration() {
    const Vector2 initialSpeed = ball.speed;
    const double dt = GetFrameTime();

    if (ballLockedToPaddle)
        return;

    Vector2 oldSpeed = ball.speed;

    // acceleration on x
    if (fabs(ball.speed.x) < fabs(INITIAL_BALL_SPEED.x)) {
        ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * (fabs(ball.speed.x) + fabs(BALL_ACCELERATION.x) * dt);
        ball.speed.y = ball.speed.x * initialSpeed.y / initialSpeed.x;
        if (fabs(ball.speed.y) > fabs(ACCELERATED_BALL_SPEED.y))
            ball.speed = oldSpeed;
    }

    // acceleration on y
    if (fabs(ball.speed.y) < fabs(INITIAL_BALL_SPEED.y)) {
        ball.speed.y = (ball.speed.y > 0 ? 1 : -1) * (fabs(ball.speed.y) + fabs(BALL_ACCELERATION.y) * dt);
        ball.speed.x = ball.speed.y * initialSpeed.x / initialSpeed.y;
        if (fabs(ball.speed.x) > fabs(ACCELERATED_BALL_SPEED.x))
            ball.speed = oldSpeed;
    }

    // deceleration on x
    if (fabs(ball.speed.x) > fabs(ACCELERATED_BALL_SPEED.x)) {
        ball.speed.x = (ball.speed.x > 0 ? 1 : -1) * (fabs(ball.speed.x) - fabs(BALL_ACCELERATION.x) * dt);
        ball.speed.y = ball.speed.x * initialSpeed.y / initialSpeed.x;
    }

    // deceleration on y
    if (fabs(ball.speed.y) > fabs(ACCELERATED_BALL_SPEED.y)) {
        ball.speed.y = (ball.speed.y > 0 ? 1 : -1) * (fabs(ball.speed.y) - fabs(BALL_ACCELERATION.y) * dt);
        ball.speed.x = ball.speed.y * initialSpeed.x / initialSpeed.y;
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
        killPaddle();
    }
}

// Ball falls below paddle
void killPaddle()
{
    if (lives > 0)
        lives--;
    scoreMultiplier = 1.0;

    resetPaddle();
    resetBall();

    resetPerks();
    checkGameEnd();
}

void increaseLives()
{
    if (lives >= STARTING_LIVES * 2)
        return;

    lives++;
    //? play +life sound
}

// Reflect ball off of the paddle
bool bounceBallOnPaddle()
{
    bool collision = false;

    // 4 pixels of extra length on both sides to allow for fairer jump
    if (ball.pos.x >= paddles[currentPaddle].rect.x - 4 && ball.pos.x <= paddles[currentPaddle].rect.x + paddles[currentPaddle].rect.width + 4 &&
        ball.pos.y + ball.radius > paddles[currentPaddle].rect.y)
    {
        ball.speed.y *= -1;
        ball.pos.y = paddles[currentPaddle].rect.y - ball.radius - 1;
        collision = true;
    }
    else if (CheckCollisionCircleRec(ball.pos, ball.radius, (Rectangle){paddles[currentPaddle].rect.x - 4, paddles[currentPaddle].rect.y, paddles[currentPaddle].rect.width + 8, paddles[currentPaddle].rect.height}))
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
    if (ballLockedToPaddle && IsKeyPressed(KEY_SPACE))
        ballLockedToPaddle = false;

    // check collisions with ball since its fast moving
    bounceBallOnPaddle();

    // move paddle sideways
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        paddles[currentPaddle].rect.x -= paddles[currentPaddle].speed.x;
    else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        paddles[currentPaddle].rect.x += paddles[currentPaddle].speed.x;

    // bound within the walls
    if (paddles[currentPaddle].rect.x < 0)
    {
        paddles[currentPaddle].rect.x = 0;
    }
    else if (paddles[currentPaddle].rect.x + paddles[currentPaddle].rect.width > GetScreenWidth())
    {
        paddles[currentPaddle].rect.x = GetScreenWidth() - paddles[currentPaddle].rect.width;
    }
}

void checkAllCollisions()
{
    bounceBallOnPaddle();
    bounceBallOnBoundaries();
    checkBallNBrickCollisions();
}

// Check collision between ball and bricks
bool checkBallNBrickCollisions()
{
    bool collision = false;

    for (int i = 0; i < numBricks; i++)
    {

        // no collisions for empty bricks
        if (bricks[i].type == 0 || !CheckCollisionCircleRec(ball.pos, ball.radius, bricks[i].rect))
            continue;

        collision = true;

        // distance between centers of rectangle and ball
        double dX = ball.pos.x - (bricks[i].rect.x + BRICK_WIDTH / 2);
        double dY = ball.pos.y - (bricks[i].rect.y + BRICK_HEIGHT / 2);

        // overlaps between ball and rectangle
        double overlapX = (BRICK_WIDTH / 2 + ball.radius) - fabs(dX);
        double overlapY = (BRICK_HEIGHT / 2 + ball.radius) - fabs(dY);

        if (overlapX <= overlapY)
        {
            if (dX > 0)
            {
                ball.speed.x *= -1;
                ball.pos.x = bricks[i].rect.x + BRICK_WIDTH + ball.radius + 1;
            }
            else
            {
                ball.speed.x *= -1;
                ball.pos.x = bricks[i].rect.x - ball.radius - 1;
            }
        }

        if (overlapY <= overlapX)
        {
            if (dY > 0)
            {
                ball.speed.y *= -1;
                ball.pos.y = bricks[i].rect.y + BRICK_HEIGHT + ball.radius + 1;
            }
            else
            {
                ball.speed.y *= -1;
                ball.pos.y = bricks[i].rect.y - ball.radius - 1;
            }
        }

        degradeBrick(i);
    }

    return collision;
}

// Degrade brick, based on its type
void degradeBrick(int brickIndex)
{
    // empty bricks and unbreakable bricks
    if (bricks[brickIndex].type == 0 || bricks[brickIndex].type == -1)
        return;

    // standard bricks
    if (bricks[brickIndex].type > 0)
    {
        increaseScore(
            BASE_BRICK_HIT_SCORE * (1 + (fabs(ball.speed.x) / fabs(ACCELERATED_BALL_SPEED.x) / 2)));

        // degrade brick
        bricks[brickIndex].type--;

        // brick destroyed completely
        if (bricks[brickIndex].type == 0)
            breakableBricksLeft--;
    }

    spawnPerk(brickIndex);
}

// [Deprecated - DO NOT USE] Check collision between ball and bricks
bool checkBallNBrickCollisions2()
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

// Manage perk positions and durations
void updatePerks()
{
    const double dt = GetFrameTime();
    delayPerkSpawn();

    // durations
    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        if (perks[i].duration == 0)
            continue;

        perks[i].duration -= dt;
        if (perks[i].duration <= 0) {
            perks[i].duration = 0;
            deactivatePerk(i);
        }
    }

    // positions
    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        // despawn perk
        if (perks[i].pos.x <= 0 || perks[i].pos.x + PERK_IMG_SIZE.x >= GetScreenWidth() ||
            perks[i].pos.y <= 0 || perks[i].pos.y + PERK_IMG_SIZE.y >= GetScreenHeight())
        {
            perks[i].pos = (Vector2){-1, -1};
        }
        else
        {
            perks[i].pos = Vector2Add(perks[i].pos, Vector2Scale(PERK_SPEED, dt));
        }

        checkPerkAndBrickCollision(i);
    }
}

// Check collisions between perk and brick
void checkPerkAndBrickCollision(int perkIndex)
{
    bool collision = CheckCollisionRecs(
        (Rectangle) {perks[perkIndex].pos.x, perks[perkIndex].pos.y, PERK_IMG_SIZE.x, PERK_IMG_SIZE.y},
        paddles[currentPaddle].rect
    );

    if (collision)
    {
        // despawn and activate
        perks[perkIndex].pos = (Vector2){-1, -1};
        activatePerk(perkIndex);
    }
}

// Activate effects of perk
void activatePerk(int perkIndex)
{
    canSpawnPerk = false;

    // score
    increaseScore(BASE_PERK_ACTIVATE_SCORE);
    //? play sound

    char *name = perks[perkIndex].filename;
    if (strcmp(name, "killpaddle") == 0) {
        killPaddle();
    }
    else if (strcmp(name, "extralife") == 0) {
        increaseLives();
    }
    else if (strcmp(name, "doublepoints") == 0) {
        if (perks[perkIndex].duration <= 0)
            scoreMultiplier *= 2;
    }
    else if (strcmp(name, "expandpaddle") == 0) {
        switchPaddle(currentPaddle == SHRUNK_PADDLE ? BASE_PADDLE : EXPANDED_PADDLE);
    }
    else if (strcmp(name, "shrinkpaddle") == 0) {
        switchPaddle(currentPaddle == EXPANDED_PADDLE ? BASE_PADDLE : SHRUNK_PADDLE);
    }
    else if (strcmp(name, "slowball") == 0) {
        double slowSpeed = min(Vector2Length(INITIAL_BALL_SPEED) / 2, Vector2Length(ball.speed));
        ball.speed = Vector2Scale(Vector2Normalize(ball.speed), slowSpeed);
    }
    else if (strcmp(name, "fastball") == 0) {
        double fastSpeed = max(Vector2Length(ACCELERATED_BALL_SPEED) * 1.2, Vector2Length(ball.speed));
        ball.speed = Vector2Scale(Vector2Normalize(ball.speed), fastSpeed);

        // limitation (based on high y velocity)
        if (fabs(ball.speed.y) > fabs(ACCELERATED_BALL_SPEED.y)) {
            ball.speed.y = (ball.speed.y > 0 ? 1 : -1) * fabs(ACCELERATED_BALL_SPEED.y);
            ball.speed.x = ball.speed.x / fabs(ball.speed.y) * fabs(ACCELERATED_BALL_SPEED.y);
        }
    }

    // set duration to max initial duration
    if (perks[perkIndex].timed)
        perks[perkIndex].duration = TIMED_PERK_DURATION;
}

// Remove effects of perk
void deactivatePerk(int perkIndex)
{
    perks[perkIndex].duration = 0;

    char *name = perks[perkIndex].filename;
    if (strcmp(name, "doublepoints") == 0) {
        scoreMultiplier /= 2;
    }
    else if (strcmp(name, "expandpaddle") == 0 || strcmp(name, "shrinkpaddle") == 0) {
        switchPaddle(BASE_PADDLE);
    }
}

// Manage delay for perk spawn
void delayPerkSpawn()
{
    static double time = 0;
    if (canSpawnPerk)
        return;

    time += GetFrameTime();
    if (time > DELAY_AFTER_PERK_SPAWN)
    {
        time = 0;
        canSpawnPerk = true;
    }
}

// Spawn perk after collision with brick
void spawnPerk(int brickIndex)
{
    if (!canSpawnPerk)
        return;

    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        // perk already on screen => no spawn
        if (!Vector2Equals(perks[i].pos, (Vector2){-1, -1}))
            continue;

        char *name = perks[i].filename;

        // limits on specific perks to spawn
        if (strcmp(name, "killpaddle") == 0 && lives >= STARTING_LIVES * 2) {
            continue;
        }
        else if (strcmp(name, "expandpaddle") == 0 && currentPaddle == EXPANDED_PADDLE)
            continue;
        else if (strcmp(name, "shrinkpaddle") == 0 && currentPaddle == SHRUNK_PADDLE)
            continue;

        // roll for rng
        double roll = ((double) rand() / RAND_MAX) * 100;
        if (roll != 0 && roll <= perks[i].spawnChance) {
            perks[i].pos = (Vector2) {
                bricks[brickIndex].rect.x + (BRICK_WIDTH - PERK_IMG_SIZE.x) / 2,
                bricks[brickIndex].rect.y};

            return;
        }
    }
}

void resetPerks()
{
    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        perks[i].duration = 0;
        perks[i].pos = (Vector2){-1, -1};
    }
}

// Increase player score
void increaseScore(int change)
{
    // score based on speed
    // 50% more score for smaller paddle
    int increase = change + 0.5 * change * (currentPaddle == SHRUNK_PADDLE);
    increase *= scoreMultiplier;

    playerScore += increase;
}

// Check whether all breakable bricks in current map are destroyed
void checkLevelEnd()
{
    if (breakableBricksLeft > 0 || lives <= 0)
        return;

    // more levels left
    if (currentMap + 1 < numberOfMaps)
    {
        currentMap++;
        setNewLevel();
    }

    // all levels finished
    else
    {
        lockBall();
        switchGameState(GS_GAME_END);
    }
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
        saveNewScore();

        // switch to main menu
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

// Read high scores from file
void readHighScores()
{
    FILE *hsFile = fopen(HIGH_SCORES_FILE_PATH, "r");
    if (hsFile == NULL)
    {
        TraceLog(LOG_ERROR, "Leaderboards data has been lost.\n");

        // open empty file
        hsFile = fopen(HIGH_SCORES_FILE_PATH, "w");
        if (hsFile != NULL)
            fclose(hsFile);
        return;
    }

    // read entries line by line
    for (int i = 0; i < MAX_HIGH_SCORES; i++)
    {
        if (fscanf(hsFile, "%d %d", &highScores[i].score, &highScores[i].playtime) != 2)
            break;

        char c = '\0';
        fscanf(hsFile, "%c", &c); // read extra space
        for (int j = 0; j < MAX_PLAYER_NAME_LENGTH && fscanf(hsFile, "%c", &c) == 1 && c != '\n'; j++)
        {
            highScores[i].name[j] = c;
            highScores[i].name[j + 1] = '\0';
        }

        // exhaust current line
        while (c != '\n' && fscanf(hsFile, "%c", &c) == 1);
        numHighScores++;
    }

    // sort high scores
    sortHighScores();

    fclose(hsFile);
}

// Sort high score entries based on score and playtime
void sortHighScores()
{
    for (int i = 0; i < numHighScores; i++)
    {
        for (int j = i + 1; j < numHighScores; j++)
        {
            if (highScores[j].score > highScores[i].score ||
                highScores[j].score == highScores[i].score && highScores[j].playtime < highScores[i].playtime)
            {
                HSEntry tmp = highScores[i];
                highScores[i] = highScores[j];
                highScores[j] = tmp;
            }
        }
    }
}

// Save score (called after entering name in end game screen)
void saveNewScore()
{
    // no need to save score with empty name
    if (strlen(nameInputStr) == 0)
        return;

    // find index to store
    int i = 0;
    while (i < numHighScores && (highScores[i].score > playerScore ||
                                 (highScores[i].score == playerScore && highScores[i].playtime <= playtime)))
    {
        i++;
    }

    // cannot be placed
    if (i >= MAX_HIGH_SCORES)
        return;

    // shift and replace
    for (int j = MAX_HIGH_SCORES - 1; j > i; j--)
    {
        highScores[j] = highScores[j - 1];
    }
    highScores[i].score = playerScore;
    highScores[i].playtime = playtime;
    strcpy(highScores[i].name, nameInputStr);

    // increase count
    if (numHighScores + 1 < MAX_HIGH_SCORES)
        numHighScores++;

    // save to file
    writeHighScores();
}

// Write high scores list from memory to file
void writeHighScores()
{
    FILE *hsFile = fopen(HIGH_SCORES_FILE_PATH, "w");
    if (hsFile == NULL)
    {
        TraceLog(LOG_ERROR, "High scores file could not be opened.");
    }

    // Format: {score} {playtime} {name}
    for (int i = 0; i < numHighScores; i++)
        fprintf(hsFile, "%d %d %s\n", highScores[i].score, highScores[i].playtime, highScores[i].name);

    fclose(hsFile);
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
    for (int i = MAIN_MENU_LOGO_START - 1; i < MAIN_MENU_LOGO_END; i++)
    {
        char mainMenuLogoTextureFilePath[50];
        sprintf(mainMenuLogoTextureFilePath, "%slogo/%d.png", MAIN_MENU_TEXTURES_PATH, i + 1);
        mainMenuLogo[i] = LoadTexture(mainMenuLogoTextureFilePath);
    }

    for (int i = MAIN_MENU_BALL_START - 1; i < MAIN_MENU_BALL_END; i++)
    {
        char mainMenuBallTextureFilePath[50];
        sprintf(mainMenuBallTextureFilePath, "%sball/%d.png", MAIN_MENU_TEXTURES_PATH, i + 1);
        mainMenuBall[i] = LoadTexture(mainMenuBallTextureFilePath);
    }

    // map editor buttons
    mapEditorButtonTextures[1] = LoadTexture("./assets/ui/larrow.png");
    mapEditorButtonTextures[2] = LoadTexture("./assets/ui/rarrow.png");
    mapEditorButtonTextures[3] = LoadTexture("./assets/ui/plus.png");
    mapEditorButtonTextures[4] = LoadTexture("./assets/ui/delete.png");

    // victory or defeat title
    victoryImage = LoadTexture("./assets/ui/victory.png");
    defeatImage = LoadTexture("./assets/ui/defeat.png");

    // main game ui
    lifeTexture = LoadTexture("./assets/ui/life.png");
    ballImage = LoadTexture("./assets/ball.png");

    // paddles
    for (int i = 0; i < NUMBER_OF_PADDLES; i++) {
        char filename[50];
        sprintf(filename, "./assets/paddles/%d.png", i);
        paddles[i].image = LoadTexture(filename);
    }

    // high score title
    highScoresTitleImage = LoadTexture("./assets/ui/highScores.png");

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

    // perks
    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        char filename[50];
        sprintf(filename, "%s%s.png", PERKS_IMG_PATH, perks[i].filename);
        perks[i].img = LoadTexture(filename);
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

    for (int i = 0; i < NUMBER_OF_PADDLES; i++)
        UnloadTexture(paddles[i].image);

    UnloadTexture(highScoresTitleImage);

    for (int i = 0; i <= NUM_BRICK_TEXTURES; i++)
        UnloadTexture(brickTextures[i]);

    for (int i = 0; i < NUMBER_OF_PERKS; i++)
        UnloadTexture(perks[i].img);

    for (int i = MAIN_MENU_LOGO_START - 1; i < MAIN_MENU_LOGO_END; i++)
    {
        UnloadTexture(mainMenuLogo[i]);
    }
    for (int i = MAIN_MENU_BALL_START - 1; i < MAIN_MENU_BALL_END; i++)
    {
        UnloadTexture(mainMenuBall[i]);
    }
}

// Contains all draw calls; func called inside game loop
void drawLoop()
{
    // core game
    if (gameState == GS_MAIN_GAME)
    {
        drawMainGame();
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
    // high scores
    else if (gameState == GS_HIGH_SCORES)
    {
        drawHighScoresScreen();
    }
}

// Draw main game
void drawMainGame()
{
    drawPaddle();
    drawBall();
    drawBricks();
    drawPerks();
    if (debugView)
        drawDebugView();
    else
        drawMainGameUI();
}

// Draw ui for main game
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
    DrawTextureEx(paddles[currentPaddle].image, (Vector2){paddles[currentPaddle].rect.x, paddles[currentPaddle].rect.y}, 0.0f, 1.0f, WHITE);
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

void drawPerks()
{
    for (int i = 0; i < NUMBER_OF_PERKS; i++)
    {
        if (perks[i].pos.x != -1 && perks[i].pos.y != -1)
        {
            // DrawRectangle(perks[i].pos.x, perks[i].pos.y, 32, 30, WHITE);
            DrawTextureEx(perks[i].img, perks[i].pos, 0, 1, WHITE);
        }
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
        Color buttonColor = GRAY;

        // grey out map addition and deletion buttons
        if ((i == 4 && numberOfMaps <= 1) || (i == 3 && numberOfMaps >= MAX_NUMBER_OF_MAPS))
            buttonColor = GRAY;

        // highlight buttons on hover
        else if (CheckCollisionPointRec(GetMousePosition(), mapEditorButtons[i]))
        {
            buttonColor = WHITE;
        }

        DrawRectangleLinesEx(mapEditorButtons[i], 1, WHITE);

        DrawTexturePro(
            mapEditorButtonTextures[i],
            (Rectangle){0, 0, mapEditorButtonTextures[i].width, mapEditorButtonTextures[i].height},
            (Rectangle){mapEditorButtons[i].x + (mapEditorButtons[i].width - fontSize) / 2, mapEditorButtons[i].y + fontSize / 2, fontSize, fontSize},
            (Vector2){0, 0},
            0.0f,
            buttonColor);
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
        bool mouseOnBricks = CheckCollisionPointRec(
            mousePos,
            (Rectangle){
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

// Draw leaderboards
void drawHighScoresScreen()
{
    // size and spacings for table
    const int fontSize = 16;
    const double rowHeight = 18 * 2;

    const int NUM_HEADERS = 4;
    const char *colHeaders[4] = {
        "#",
        "Name",
        "Score",
        "Time"};

    // 60% of screen width
    const double tableWidth = 0.65 * GetScreenWidth();
    const double colWidth[4] = {
        0.10 * tableWidth,
        0.45 * tableWidth,
        0.25 * tableWidth,
        0.20 * tableWidth};

    // variables to keep track of position to draw stuff
    double px = (GetScreenWidth() - highScoresTitleImage.width) / 2;
    double py = PADDING_ABOVE_MAP - 20;

    // draw high scores title
    DrawTexture(highScoresTitleImage, px, py, WHITE);
    py += highScoresTitleImage.height + 20;

    // draw table headers
    px = (GetScreenWidth() - tableWidth) / 2;
    for (int i = 0; i < NUM_HEADERS; i++)
    {
        // cell outline
        DrawRectangleLines(px, py, colWidth[i], rowHeight, RAYWHITE);

        DrawText(colHeaders[i], px + (colWidth[i] - MeasureText(colHeaders[i], fontSize + 2)) / 2, py + (rowHeight - fontSize - 2) / 2, fontSize + 2, RAYWHITE);

        px += colWidth[i];
    }

    px = (GetScreenWidth() - tableWidth) / 2;
    py += rowHeight;

    // draw cells and values inside them
    for (int i = 0; i < MAX_HIGH_SCORES; i++)
    {
        for (int j = 0; j < NUM_HEADERS; j++)
        {
            // cell outline
            DrawRectangleLines(px, py, colWidth[j], rowHeight, RAYWHITE);

            char cellText[50] = {'\0'};

            if (j == 0)
            { // #
                sprintf(cellText, "%d", i + 1);
            }
            else if (j == 1)
            { // name
                sprintf(cellText, "%s", highScores[i].name);
            }
            else if (j == 2)
            { // score
                sprintf(cellText, "%d", highScores[i].score);
            }
            else if (j == 3)
            { // playtime
                sprintf(cellText, "%02d : %02d", highScores[i].playtime / 60, highScores[i].playtime % 60);
            }

            // empty cell
            if (i >= numHighScores && j != 0)
            {
                sprintf(cellText, "-");
            }

            DrawText(
                cellText,
                px + (colWidth[j] - MeasureText(cellText, fontSize + 2)) / 2,
                py + (rowHeight - fontSize - 2) / 2,
                fontSize + 2,
                i == 0 ? (Color){211, 175, 55, 255} : i == 1 ? (Color){187, 194, 204, 255}
                                                  : i == 2   ? (Color){228, 149, 60, 255}
                                                             : RAYWHITE);

            px += colWidth[j];
        }

        px = (GetScreenWidth() - tableWidth) / 2;
        py += rowHeight;
    }
}

// Draw victory/defeat screen
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
    DrawRectangleLinesEx((Rectangle){paddles[currentPaddle].rect.x - 4, paddles[currentPaddle].rect.y, paddles[currentPaddle].rect.width + 8, paddles[currentPaddle].rect.height}, 1, RED);

    // stats
    char textStr[50] = {'\0'};
    sprintf(textStr, "FPS: %d", GetFPS());
    DrawText(textStr, 10, 10, 15, RAYWHITE);

    sprintf(textStr, "Score: %d", playerScore);
    DrawText(textStr, 10, 30, 15, RAYWHITE);

    sprintf(textStr, "PlayTime: %d", playtime);
    DrawText(textStr, 10, 50, 15, RAYWHITE);

    sprintf(textStr, "Lives: %d", lives);
    DrawText(textStr, 10, 70, 15, RAYWHITE);

    sprintf(textStr, "SpeedX: %.2f; SpeedY: %.2f", ball.speed.x, ball.speed.y);
    DrawText(textStr, GetScreenWidth() - 250, 10, 15, RAYWHITE);

    // map area
    DrawRectangle(PADDING_ON_MAP_SIDES, PADDING_ABOVE_MAP, GetScreenWidth() - PADDING_ON_MAP_SIDES * 2, GetScreenHeight() - PADDING_ABOVE_MAP - PADDING_BELOW_MAP, (Color){255, 0, 0, 50});

    // brick outlines
    for (int i = 0; i < numBricks; i++)
        DrawRectangleLinesEx(bricks[i].rect, 1, RAYWHITE);
}

void updateAudio()
{
    //? updates for all sfx go here
    // UpdateAudioStream()

    // update current music stream
    UpdateMusicStream(currMusic);

    // music rotation
    rotateMusic();

    // check user input
    checkMusicChange();
}

// Switch to new music
void switchMusic(int musicIndex)
{
    if (IsMusicValid(currMusic))
    {
        StopMusicStream(currMusic);
        UnloadMusicStream(currMusic);
    }

    char musicFile[strlen(MUSIC_FILES_PATH) + 10];

    // check whether selected music index is valid
    if (musicIndex >= NUMBER_OF_MUSIC_FILES)
        currMusicIndex = 0;
    else if (musicIndex < 0)
        currMusicIndex = NUMBER_OF_MUSIC_FILES - 1;
    else
        currMusicIndex = musicIndex;

    // load music
    sprintf(musicFile, "%s/%d.mp3", MUSIC_FILES_PATH, currMusicIndex);
    currMusic = LoadMusicStream(musicFile);
    currMusic.looping = false;

    // start playing
    PlayMusicStream(currMusic);

    //! unimplemented: game audio
    SetMusicVolume(currMusic, 1.0);

    // reset music stopped flag
    musicStopped = false;
}

// Rotate music, basically switching to next track
void rotateMusic()
{
    // Check whether music not playing and music not stopped earlier
    if (!IsMusicStreamPlaying(currMusic) && !musicStopped)
    {
        // setting flag makes sure we dont switch multiple times before next music loads
        musicStopped = true;
        switchMusic(currMusicIndex + 1);
    }
}

// Unload all sfx and music
void unloadAudio()
{
    if (IsMusicValid(currMusic))
        UnloadMusicStream(currMusic);
}

// Check user input for changing music
void checkMusicChange()
{
    if (IsKeyDown(KEY_LEFT_SHIFT))
    {
        if (IsKeyPressed(KEY_APOSTROPHE))
            switchMusic(currMusicIndex + 1);
        else if (IsKeyPressed(KEY_SEMICOLON))
            switchMusic(currMusicIndex - 1);
    }
}
