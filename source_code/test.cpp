#include <iostream>
#include <SDL.h>
using namespace std;

// Define constants needed throughout code
struct Constants {
    static constexpr int WIN_WIDTH = 1400;
    static constexpr int WIN_HEIGHT = 800;
    static constexpr int LEVEL_WIDTH = 2250;
    static constexpr int FLOOR_LEVEL = 700;
    static constexpr float GRAVITY = 1800.0f;
    static constexpr float TERMINAL_VELOCITY = 1200.0f;
    static constexpr float CAMERA_DELAY = 15.0f;
};

struct Vector2 {
    float x, y;
};

struct Camera {
    float targetX, targetY;
    float x, y;
    int w, h;
};

// Axis-aligned bounding box collision
bool AABB(const SDL_Rect& a, const SDL_Rect& b) {
    return (
        a.x < b.x + b.w &&      // IF left edge of 'A' is further left than right edge of 'B'
        a.x + a.w > b.x &&      // AND right edge of 'A' is further right than left edge of 'B'
        a.y < b.y + b.h &&      // AND top edge of 'A' is above bottom edge of 'B'
        a.y + a.h > b.y         // AND bottom edge of 'A' is below top edge of 'B'
    );                          // THEN 'A' and 'B' are colliding (return true)
}


class Player {
    public:
        Player(int x, int y, int width, int height):
            pos{(float)x, (float)y},
            vel{0, 0},
            body{x, y, width, height},
            canDash{true},
            isDashing{false},
            dashPressedLastFrame{false},
            dashTimer{0.0f},
            dashCooldown{0.0f},
            isJumping(false),
            isGrounded(false),
            facingLeft(true),
            speed(300.0f),
            jumpVelocity(-960.0f)
        {};

        void HandleInput(const Uint8* keystate, SDL_GameController* controller) {
            bool jumpPressed = keystate[SDL_SCANCODE_SPACE];
            bool dashPressed = keystate[SDL_SCANCODE_LSHIFT];

            float leftStickAxis = 0.0f;
            if (controller) {
                leftStickAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
                // Leave 20% deadzone on stick input
                if (fabs(leftStickAxis) < 0.2f) {
                    leftStickAxis = 0.0f;
                }
                if (!jumpPressed) {
                    jumpPressed = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
                }
                if (!dashPressed) {
                    dashPressed = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
                }
            }

            // Dash
            if (dashPressed && !dashPressedLastFrame && canDash && dashCooldown <= 0.0f) {
                isDashing = true;
                canDash = false;
                dashTimer = 0.3f;
                dashCooldown = 0.75f;
            } else {
                // Reset horizontal velocity
                vel.x = 0;

                // Move Left
                if (keystate[SDL_SCANCODE_A] || leftStickAxis < 0.0f) {
                    facingLeft = true;
                    vel.x = -speed;
                }
                // Move Right
                if (keystate[SDL_SCANCODE_D] || leftStickAxis > 0.0f) {
                    facingLeft = false;
                    vel.x = speed;
                }
            }
            // Jump
            if (jumpPressed) {
                if (isGrounded && !isJumping) {
                    vel.y = jumpVelocity;
                    isJumping = true;
                }
            } else {
                isJumping = false;
            }

            // Check if dash pressed last frame so player cant hold down dash button to keep dashing
            dashPressedLastFrame = dashPressed;
        }

        void Update(vector<SDL_Rect> platforms, Camera& camera, float deltaTime) {
            if (isDashing) {
                // Apply dash velocity (multiply by dashTimer so dash starts fast then slows down)
                if (facingLeft) {
                    vel.x = -speed * 15 * dashTimer;
                } else {
                    vel.x = speed * 15 * dashTimer;
                }
                
                dashTimer -= deltaTime;
                if (dashTimer <= 0.0f) {
                    isDashing = false;
                }
            }

            // Apply gravity
            vel.y += Constants::GRAVITY * deltaTime;

            // Shorten jump if player stopped pressed jump early by increasing gravity
            if (!isJumping && vel.y < 0) {
                vel.y += Constants::GRAVITY * deltaTime * 3.0f;
            }

            if (vel.y > Constants::TERMINAL_VELOCITY) {
                vel.y = Constants::TERMINAL_VELOCITY;
            }
            
            // Apply horizontal velocity
            pos.x += vel.x * deltaTime;

            // Stop playing from going off screen
            if (pos.x < 0) {
                pos.x = 0;
                vel.x = 0;
            } else if (pos.x + body.w > Constants::LEVEL_WIDTH) {
                pos.x = Constants::LEVEL_WIDTH - body.w;
                vel.x = 0;
            }

            body.x = (int)pos.x;
            
            for (auto& platform : platforms) {
                // If player is colliding with platform
                if (AABB(body, platform)) {
                    // If moving left, allign players right edge with platforms left edge
                    if (vel.x > 0) {
                        body.x = platform.x - body.w;
                    }
                    // If moving right, allign players left edge with platforms right edge
                    else if (vel.x < 0) {
                        body.x = platform.x + platform.w;
                    }

                    // Sync pos with body after collision
                    pos.x = body.x;
                    
                    // Reset horizontal velocity
                    vel.x = 0;
                }
            }

            // Apply vertical velocity
            pos.y += vel.y * deltaTime;
            body.y = (int)pos.y;
            isGrounded = false;
            
            for (auto& platform : platforms) {
                // If player is colliding with platform
                if (AABB(body, platform)) {
                    // If moving down, allign players bottom edge with platforms top edge
                    if (vel.y > 0) {
                        body.y = platform.y - body.h;
                        isGrounded = true;
                    }
                    // If moving up, allign players top edge with platforms bottom edge
                    else if (vel.y < 0) {
                        body.y = platform.y + platform.h;
                    }

                    // Sync pos with body after collision
                    pos.y = body.y;
                    
                    // Reset vertical velocity
                    vel.y = 0;
                }
            }

            // Make camera follow player
            camera.targetX = pos.x + body.w / 2 - camera.w / 2;
            camera.targetY = pos.y + body.h / 2 - camera.h / 1.8;

            // Apply dash cooldown
            if (dashCooldown > 0.0f) {
                dashCooldown -= deltaTime;
            }

            // If player is grounded, allow them to dash again (can only dash once middair)
            if (isGrounded && !canDash) {
                canDash = true;
            }
        }

        void Render(SDL_Renderer* renderer, Camera camera) {
            SDL_SetRenderDrawColor(renderer, 62, 146, 204, 255);

            // Draw player relative to camera position
            SDL_Rect drawPlayer = {(int)roundf(body.x - camera.x), (int)(body.y - camera.y), body.w, body.h};
            SDL_RenderFillRect(renderer, &drawPlayer);
        }
    
    private:
        Vector2 pos;
        Vector2 vel;
        SDL_Rect body;

        bool canDash;
        bool isDashing;
        bool dashPressedLastFrame;
        float dashTimer;
        float dashCooldown;

        bool isJumping;
        bool isGrounded;
        bool facingLeft;
        float speed;
        float jumpVelocity;
};

class Game {
    public:
        Game():
            window(nullptr),
            renderer(nullptr),
            controller(nullptr),
            previousTick(0),
            isRunning(false),
            deltaTime(0),
            camera({0.0f, 0.0f, 0.0f, 0.0f, Constants::WIN_WIDTH, Constants::WIN_HEIGHT}),
            player(100, 250, 55, 100),
            platforms{
                {0, Constants::FLOOR_LEVEL, Constants::LEVEL_WIDTH, 130},
                {0, Constants::FLOOR_LEVEL - 150, 300, 150},
                {550, Constants::FLOOR_LEVEL - 350, 125, 50},
                {1050, Constants::FLOOR_LEVEL - 275, 300, 275},
                {1350, Constants::FLOOR_LEVEL - 140, 150, 140},
                {1625, Constants::FLOOR_LEVEL - 475, 125, 50},
                {1225, Constants::FLOOR_LEVEL - 625, 125, 50},
                {750, Constants::FLOOR_LEVEL - 775, 125, 50},
            }
        {};

        void Initialise() {
            // Initialise SDL, output error if fails
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
                cerr << "SDL could not initialise. Error: " << SDL_GetError() << endl;
                return;
            }

            // Initialise window, output error if fails
            window = SDL_CreateWindow(
                "Game",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                Constants::WIN_WIDTH, Constants::WIN_HEIGHT,
                SDL_WINDOW_SHOWN
            );
            if (!window) {
                cerr << "Window could not initialise. Error: " << SDL_GetError() << endl;
                return;
            }

            // Initialise renderer, output error if fails
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!renderer) {
                cerr << "Renderer could not initialise. Error: " << SDL_GetError() << endl;
                return;
            }

            // Find controller
            for (int i = 0; i < SDL_NumJoysticks(); i++) {
                if (SDL_IsGameController(i)) {
                    controller = SDL_GameControllerOpen(i);
                    cout << "Controller found: " << SDL_GameControllerName(controller) << endl;
                    break;
                }
            }

            // If everything has been initialised without error, run game 
            isRunning = true;
        }

        void HandleInput() {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    isRunning = false;
                }
            }

            // Get keyboard inputs
            const Uint8* keystate = SDL_GetKeyboardState(nullptr);
            player.HandleInput(keystate, controller);
        }

        void Update() {
            // Calculate deltaTime to normalise movement
            Uint32 currentTick = SDL_GetTicks();
            deltaTime = (currentTick - previousTick) / 1000.0f;
            previousTick = currentTick;

            // Clamp deltaTime to avoid clipping at low FPS
            if (deltaTime > 0.05f) {
                deltaTime = 0.05f;
            }

            player.Update(platforms, camera, deltaTime);

            // Clamp camera to avoid out of bounds
            if (camera.targetX < 0) {
                camera.targetX = 0;
            } else if (camera.targetX > Constants::LEVEL_WIDTH - camera.w) {
                camera.targetX = Constants::LEVEL_WIDTH - camera.w;
            }

            if (camera.targetY > 0.0f) {
                camera.targetY = 0.0f;
            }

            // Make camera move smoothly to avoid stuttering
            camera.y += (camera.targetY - camera.y) * Constants::CAMERA_DELAY * deltaTime;
            camera.x += (camera.targetX - camera.x) * Constants::CAMERA_DELAY * deltaTime;
        }

        void Render() {
            // Draw background
            SDL_SetRenderDrawColor(renderer, 29, 62, 94, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 42, 98, 143, 255);
            for (auto& platform : platforms) {
                // Draw platforms relative to camera position
                SDL_Rect drawPlatform = {(int)(platform.x - camera.x), (int)(platform.y - camera.y), platform.w, platform.h};
                SDL_RenderFillRect(renderer, &drawPlatform);
            }

            player.Render(renderer, camera);

            SDL_RenderPresent(renderer);
        }

        void Run() {
            while (isRunning) {
                HandleInput();
                Update();
                Render();
            }
        }

        void CleanUp() {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }

    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_GameController* controller;
        
        Uint32 previousTick;
        bool isRunning;
        float deltaTime;
        Camera camera;
        Player player;
        vector<SDL_Rect> platforms;
};

int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
} 
