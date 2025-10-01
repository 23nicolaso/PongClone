/**
* Author: Nicolas Ollivier
* Assignment: Pong Clone
* Date due: 2025-10-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/cs3113.h"

// ---------- Enums ----------
enum Team {BLUETEAM, REDTEAM};
enum GameStatus {RUNNING, ENDSCREEN, TERMINATED};
enum Mode {MULTIPLAYER, SINGLEPLAYER};
enum Direction {UP = 1, DOWN = - 1, NEUTRAL = 0};

// --------- Global Constants --------- 
constexpr int SCREEN_WIDTH  = 1600 / 2,
              SCREEN_HEIGHT = 900 / 2,
              FPS           = 60,
              SIZE          = 200 / 2,
              SPEED         = 250,
              VERTICAL_MULT = 5,
              ALLOWED_DIST  = 10;

constexpr float PADDING          = 50.0f;
          float BALL_X_VELOCITY  = 600.0f;

constexpr char    BG_COLOUR[]     = "#F8F1C8";
constexpr Vector2   ORIGIN          = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
                    BASE_SIZE       = { (float) SIZE, (float) SIZE  },
                    BG_SCALE        = { SCREEN_WIDTH, SCREEN_HEIGHT },
                    LEFT_INIT_POS   = { ORIGIN.x - 300.0f, ORIGIN.y },
                    RIGHT_INIT_POS  = { ORIGIN.x + 300.0f, ORIGIN.y },
                    BALL_SCALE      = { 40.0f, 40.0f },
                    PLAYER_HITBOX   = { 1, (float) SIZE };

// Textures
constexpr char LEFTPLAYER[]  = "assets/leftplayer.png";
constexpr char RIGHTPLAYER[] = "assets/rightplayer.png";
constexpr char BALL[] = "assets/ball.png";
constexpr char BG[] = "assets/MAP.png";
constexpr char BLUEWIN[] = "assets/bluewinscreen.png";
constexpr char REDWIN[] = "assets/redwinscreen.png";

// ------ Global Variables ------
float     gAngle         = 0.0f,
          gPreviousTicks = 0.0f,
          gNewLeftY      = 0.0f,
          gNewRightY     = 0.0f,
          gNewBallY      = 0.0f;

Team gWinner = BLUETEAM;
GameStatus gGameStatus = RUNNING;
Mode gMode = MULTIPLAYER;
Direction gBotDirection = UP;

// Balls struct to reduce redundancy
struct Ball
{
    Vector2 initialPosition;
    Vector2 initialVelocity;
    Vector2 position;
    Vector2 velocity;

    Ball(Vector2 pos, Vector2 vel)
        : initialPosition(pos),
          initialVelocity(vel),
          position(pos),
          velocity(vel) {}
    
    void Reset(){
        position = initialPosition;
        velocity = initialVelocity;
    }
};

Ball gBall  = {{ORIGIN.x, ORIGIN.y}, {200.0f, 0.0f}},
     gBall2 = {{ORIGIN.x, ORIGIN.y - 100.0f}, {-100.0f, 0.0f}},
     gBall3 = {{ORIGIN.x, ORIGIN.y + 100.0f}, {50.0f, 0.0f}}; 

Vector2 gLeftPlayerPos       = LEFT_INIT_POS,
        gRightPlayerPos      = RIGHT_INIT_POS,
        gLeftPlayerMovement  = { 0.0f, 0.0f },
        gRightPlayerMovement = { 0.0f, 0.0f },
        gRacketScale         = BASE_SIZE,
        gMousePosition = GetMousePosition();

Texture2D gLeftPlayerTexture,
          gRightPlayerTexture,
          gBallTexture,
          gBGTexture,
          gBlueWinScreenTexture,
          gRedWinScreenTexture;

unsigned int startTime;
int gBallCount = 1;

// ---- Function Declarations ----
void initialise();
void processInput();
void update();
void updateBall(Ball * ball, float deltaTime);
void render();
void shutdown();
bool isColliding(const Vector2 *positionA, const Vector2 *scaleA, const Vector2 *positionB, const Vector2 *scaleB);

// --------------- Function Definitions --------------- 
/**
 * @brief Checks for a square collision between 2 Rectangle objects.
 * 
 * @see 
 * 
 * @param positionA The position of the first object
 * @param scaleA The scale of the first object
 * @param positionB The position of the second object
 * @param scaleB The scale of the second object
 * @return true if a collision is detected,
 * @return false if a collision is not detected
 */
bool isColliding(const Vector2 *positionA,  const Vector2 *scaleA, 
                 const Vector2 *positionB, const Vector2 *scaleB)
{
    float xDistance = fabs(positionA->x - positionB->x) - ((scaleA->x + scaleB->x) / 2.0f);
    float yDistance = fabs(positionA->y - positionB->y) - ((scaleA->y + scaleB->y) / 2.0f);

    if (xDistance < 0.0f && yDistance < 0.0f) return true;

    return false;
}

void renderObject(const Texture2D *texture, const Vector2 *position, 
                  const Vector2 *scale)
{
    // Whole texture (UV coordinates)
    Rectangle textureArea = {
        // top-left corner
        0.0f, 0.0f,

        // bottom-right corner (of texture)
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };

    // Destination rectangle – centred on gPosition
    Rectangle destinationArea = {
        position->x,
        position->y,
        static_cast<float>(scale->x),
        static_cast<float>(scale->y)
    };

    // Origin inside the source texture (centre of the texture)
    Vector2 originOffset = {
        static_cast<float>(scale->x) / 2.0f,
        static_cast<float>(scale->y) / 2.0f
    };

    // Render the texture on screen
    DrawTexturePro(
        *texture, 
        textureArea, destinationArea, originOffset,
        gAngle, WHITE
    );
}

void restart(){
    gLeftPlayerPos = LEFT_INIT_POS;
    gRightPlayerPos = RIGHT_INIT_POS;
    gBall.Reset();
    gBall2.Reset();
    gBall3.Reset();
    gGameStatus = RUNNING;
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "User Input / Collision Detection");

    startTime = time(NULL);

    gLeftPlayerTexture    = LoadTexture(LEFTPLAYER);
    gRightPlayerTexture   = LoadTexture(RIGHTPLAYER);
    gBallTexture          = LoadTexture(BALL);
    gBGTexture            = LoadTexture(BG);
    gBlueWinScreenTexture = LoadTexture(BLUEWIN);
    gRedWinScreenTexture  = LoadTexture(REDWIN);

    SetTargetFPS(FPS);
}

void processInput() 
{
    // Reset movement without input
    gLeftPlayerMovement = { 0.0f, 0.0f };
    gRightPlayerMovement = { 0.0f, 0.0f };

    // Movement controls
    if      (IsKeyDown(KEY_W)) gLeftPlayerMovement.y = -1;
    else if (IsKeyDown(KEY_S)) gLeftPlayerMovement.y =  1;
    if      (IsKeyDown(KEY_UP)) gRightPlayerMovement.y = -1;
    else if (IsKeyDown(KEY_DOWN)) gRightPlayerMovement.y = 1;

    // Settings
    if      (IsKeyPressed(KEY_ONE)) gBallCount = 1;
    else if (IsKeyPressed(KEY_TWO)) gBallCount = 2; 
    else if (IsKeyPressed(KEY_THREE)) gBallCount = 3; 
    if      (IsKeyPressed(KEY_T)) gMode = gMode == MULTIPLAYER ? SINGLEPLAYER : MULTIPLAYER;
    if      (IsKeyPressed(KEY_Q) || WindowShouldClose()) gGameStatus = TERMINATED;
    if      (IsKeyPressed(KEY_R)) restart();
}

void updateBall(Ball * ball, float deltaTime){
    // Update ball position based on velocity, but only if allowed
    gNewBallY = (ball -> position.y) + (ball -> velocity.y) * deltaTime;
    if (gNewBallY > SCREEN_HEIGHT - PADDING || gNewBallY < PADDING){
        (ball -> velocity.y) *= -1;
        (ball -> position) = {
            (ball -> position.x) + (ball -> velocity.x) * deltaTime,
            (ball -> position.y)
        };
    }
    else{
        (ball -> position) = {
            (ball -> position.x) += (ball -> velocity.x) * deltaTime,
            gNewBallY
        };
    }

    // Check for ball / player collisions 
    if (isColliding(&ball->position, &BALL_SCALE, &gLeftPlayerPos, &BASE_SIZE)) 
    {
        (ball -> velocity.x) = BALL_X_VELOCITY;
        (ball -> velocity.y) = ((ball -> position.y) - gLeftPlayerPos.y)*VERTICAL_MULT;
    }

    if (isColliding(&ball->position, &BALL_SCALE, &gRightPlayerPos, &BASE_SIZE)) 
    {
        (ball -> velocity.x) = -BALL_X_VELOCITY;
        (ball -> velocity.y) = ((ball -> position.y) - gRightPlayerPos.y)*VERTICAL_MULT;
    }

    // Ball off screen, change to end screen!
    if ((ball -> position.x) > SCREEN_WIDTH){
        gWinner = REDTEAM;
        gGameStatus = ENDSCREEN;
    }
    else if ((ball -> position.x) < 0){
        gWinner = BLUETEAM;
        gGameStatus = ENDSCREEN;
    }
}

void update() 
{
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Update left player
    gNewLeftY = gLeftPlayerPos.y + SPEED * gLeftPlayerMovement.y * deltaTime;
    if (gNewLeftY < SCREEN_HEIGHT - PADDING && gNewLeftY > PADDING){
        gLeftPlayerPos = {
            gLeftPlayerPos.x,
            gNewLeftY
        };
    }

    // Update right player
    if ( gMode == MULTIPLAYER ){
        gNewRightY = gRightPlayerPos.y + SPEED * gRightPlayerMovement.y * deltaTime;
        if (gNewRightY < SCREEN_HEIGHT - PADDING && gNewRightY > PADDING){
            gRightPlayerPos = {
                gRightPlayerPos.x,
                gRightPlayerPos.y + SPEED * gRightPlayerMovement.y * deltaTime
            };
        }
    }
    else {
        // Bot finds closest ball (X axis), and then moves towards its Y.
        // FIND CLOSEST BALL
        Ball * ball = &gBall;
        if (gBallCount >= 2){
            if (gBall.position.x < gBall2.position.x){
                ball = &gBall2;
            }
        }
        if (gBallCount >= 3){
            if (ball->position.x < gBall.position.x){
                ball = &gBall3;
            }
        }

        if (ball -> position.y > gRightPlayerPos.y + ALLOWED_DIST) gBotDirection = UP;
        else if (ball -> position.y < gRightPlayerPos.y - ALLOWED_DIST) gBotDirection = DOWN;
        else gBotDirection = NEUTRAL;
        gNewRightY = gRightPlayerPos.y + SPEED * gBotDirection * deltaTime;
        gRightPlayerPos = {
            gRightPlayerPos.x,
            gNewRightY
        };
    }

    // Update balls 
    updateBall(&gBall, deltaTime);
    if (gBallCount >= 2)    updateBall(&gBall2, deltaTime);
    if (gBallCount >= 3)    updateBall(&gBall3, deltaTime);
}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    if (gGameStatus == RUNNING){
        renderObject(&gBGTexture, &ORIGIN, &BG_SCALE);
        renderObject(&gLeftPlayerTexture, &gLeftPlayerPos, &gRacketScale);
        renderObject(&gRightPlayerTexture, &gRightPlayerPos, &gRacketScale);
        renderObject(&gBallTexture, &gBall.position, &BALL_SCALE);

        if (gBallCount >= 2) renderObject(&gBallTexture, &gBall2.position, &BALL_SCALE);
        if (gBallCount >= 3) renderObject(&gBallTexture, &gBall3.position, &BALL_SCALE);
    }
    else if (gGameStatus == ENDSCREEN){
        if (gWinner == REDTEAM) renderObject(&gRedWinScreenTexture, &ORIGIN, &BG_SCALE);
        else                    renderObject(&gBlueWinScreenTexture, &ORIGIN, &BG_SCALE);
    }

    EndDrawing();

}

void shutdown() { CloseWindow(); }

int main(void)
{
    initialise();

    while (gGameStatus == RUNNING || gGameStatus == ENDSCREEN)
    {
        if (gGameStatus == RUNNING){
            processInput();
            update();
            render();
        }
        else {
            processInput();
            render();
        }
    }

    shutdown();

    return 0;
}