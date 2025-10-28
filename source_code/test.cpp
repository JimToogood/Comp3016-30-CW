#include <iostream>
#include <SDL.h>
using namespace std;

// Forward declarations
class Player;
class Enemy;
class Game;

// Define constants needed throughout code
struct Constants {
    static constexpr int WIN_WIDTH = 1400;
    static constexpr int WIN_HEIGHT = 800;
    static constexpr int LEVEL_WIDTH = 2250;
    static constexpr int FLOOR_LEVEL = 700;
    static constexpr float GRAVITY = 1800.0f;
    static constexpr float TERMINAL_VELOCITY = 1200.0f;
    static constexpr float CAMERA_DELAY = 15.0f;
    static constexpr float HORIZONTAL_KNOCKBACK = 600.0f;
    static constexpr float VERTICAL_KNOCKBACK = -200.0f;
};

struct Vector2 {
    float x, y;
};

struct Colour {
    Uint8 r, g, b, a;
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

void calcKnockback(Vector2 pos, Vector2& vel, Vector2 damageLocation) {
    Vector2 direction = {pos.x - damageLocation.x, pos.y - damageLocation.y};
    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);

    if (length > 0.0f) {
        direction.x /= length;
        direction.y /= length;
        vel.x = direction.x * Constants::HORIZONTAL_KNOCKBACK;
        vel.y = direction.y * Constants::HORIZONTAL_KNOCKBACK;
        if (fabs(vel.y) < 10) {
            vel.y = Constants::VERTICAL_KNOCKBACK;
        }
    }
}

// Use enum class to store attack direction, as it is more efficient than a string
enum class AttackDirection{UP, DOWN, LEFT, RIGHT};


// Player class
class Player {
    public:
        Player(int x, int y, int width, int height):
            pos{(float)x, (float)y},
            vel{0, 0},
            body{x, y, width, height},
            attackHitbox{x, y, width, width},
            canDash{true},
            isDashing{false},
            dashPressedLastFrame{false},
            dashTimer{0.0f},
            dashCooldown{0.0f},
            isAttacking{false},
            attackPressedLastFrame{false},
            attackDirection{AttackDirection::RIGHT},
            attackTimer{0.0f},
            attackCooldown{0.0f},
            isGrounded{false},
            coyoteTimer{0.0f},
            damageCooldown{0.0f},
            knockbackTimer{0.0f},
            isJumping(false),
            facingLeft(false),
            speed(300.0f),
            jumpVelocity(-960.0f)
        {};

        void HandleInput(const Uint8* keystate, SDL_GameController* controller) {
            bool dashPressed = keystate[SDL_SCANCODE_LSHIFT];
            bool attackPressed = keystate[SDL_SCANCODE_E];

            float leftStickXAxis = 0.0f;
            float leftStickYAxis = 0.0f;
            if (controller) {
                leftStickXAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
                leftStickYAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;

                // Leave slight deadzone on stick input
                if (fabs(leftStickXAxis) < 0.2f) {
                    leftStickXAxis = 0.0f;
                }
                if (fabs(leftStickYAxis) < 0.5f) {
                    leftStickYAxis = 0.0f;
                }

                if (!dashPressed) {
                    dashPressed = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
                }
                if (!attackPressed) {
                    attackPressed = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
                }
            }

            // Dash
            if (dashPressed && !dashPressedLastFrame && canDash && dashCooldown <= 0.0f) {
                isDashing = true;
                canDash = false;
                dashTimer = 0.3f;
                dashCooldown = 0.75f;
            }
            
            // Stop player from jumping or changing direction whilst dashing
            if (!isDashing && knockbackTimer <= 0.0f) {
                // Reset horizontal velocity
                vel.x = 0;

                // Move Left
                if (keystate[SDL_SCANCODE_A] || leftStickXAxis < 0.0f) {
                    facingLeft = true;
                    vel.x = -speed;
                }
                // Move Right
                if (keystate[SDL_SCANCODE_D] || leftStickXAxis > 0.0f) {
                    facingLeft = false;
                    vel.x = speed;
                }

                // Jump
                if (keystate[SDL_SCANCODE_SPACE] || SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) {
                    if (isGrounded && !isJumping) {
                        vel.y = jumpVelocity;
                        isJumping = true;
                    }
                } else {
                    isJumping = false;
                }
            }

            // Attack
            if (attackPressed && !attackPressedLastFrame && !isAttacking && attackCooldown <= 0.0f) {
                isAttacking = true;
                attackTimer = 0.5f;
                attackCooldown = 0.75f;

                if (keystate[SDL_SCANCODE_W] || leftStickYAxis < 0.0f) {
                    attackDirection = AttackDirection::UP;
                } else if ((keystate[SDL_SCANCODE_S] || leftStickYAxis > 0.0f) && !isGrounded) {
                    attackDirection = AttackDirection::DOWN;
                } else if (facingLeft) {
                    attackDirection = AttackDirection::LEFT;
                } else {
                    attackDirection = AttackDirection::RIGHT;
                }
            }

            // Check if button pressed last frame so player cant hold down button button to keep doing action
            dashPressedLastFrame = dashPressed;
            attackPressedLastFrame = attackPressed;
        }

        void Update(vector<SDL_Rect> platforms, Camera& camera, float deltaTime) {
            if (knockbackTimer <= 0.0f) {
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

                if (isAttacking) {
                    attackTimer -= deltaTime;
                    if (attackTimer <= 0.0f) {
                        isAttacking = false;
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
            } else {
                knockbackTimer -= deltaTime;
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
            bool groundedThisFrame = false;
            
            for (auto& platform : platforms) {
                // If player is colliding with platform
                if (AABB(body, platform)) {
                    // If moving down, allign players bottom edge with platforms top edge
                    if (vel.y > 0) {
                        body.y = platform.y - body.h;
                        groundedThisFrame = true;
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

            // Allow player to still jump for a few frames after leaving the ground (makes movement feel smoother)
            if (groundedThisFrame) {
                isGrounded = true;
                coyoteTimer = 0.05f; 
            } else {
                if (coyoteTimer > 0.0f) {
                    coyoteTimer -= deltaTime;
                    isGrounded = true;
                } else {
                    isGrounded = false;
                }
            }

            // Make camera follow player
            camera.targetX = pos.x + body.w / 2 - camera.w / 2;
            camera.targetY = pos.y + body.h / 2 - camera.h / 1.8;

            // Make attack hitbox follow player
            if (isAttacking) {
                switch (attackDirection) {
                    case AttackDirection::UP:
                        attackHitbox.x = pos.x;
                        attackHitbox.y = pos.y - attackHitbox.h;
                        break;
                    case AttackDirection::DOWN:
                        attackHitbox.x = pos.x;
                        attackHitbox.y = pos.y + body.h;
                        break;
                    case AttackDirection::LEFT:
                        attackHitbox.x = pos.x - attackHitbox.w;
                        attackHitbox.y = pos.y + attackHitbox.h/2;
                        break;
                    case AttackDirection::RIGHT:
                        attackHitbox.x = pos.x + attackHitbox.w;
                        attackHitbox.y = pos.y + attackHitbox.h/2;
                        break;
                } 
            }

            // Apply cooldowns
            if (dashCooldown > 0.0f) { dashCooldown -= deltaTime; }
            if (attackCooldown > 0.0f) { attackCooldown -= deltaTime; }
            if (damageCooldown > 0.0f) { damageCooldown -= deltaTime; }

            // If player is grounded, allow them to dash again (can only dash once middair)
            if (isGrounded && !canDash) {
                canDash = true;
            }
        }

        void Render(SDL_Renderer* renderer, Camera camera) {
            if (isAttacking) {
                SDL_SetRenderDrawColor(renderer, 204, 62, 146, 255);
                // Draw attack relative to camera position
                SDL_Rect drawAttack = {(int)roundf(attackHitbox.x - camera.x), (int)(attackHitbox.y - camera.y), attackHitbox.w, attackHitbox.h};
                SDL_RenderFillRect(renderer, &drawAttack);
            }

            SDL_SetRenderDrawColor(renderer, 62, 146, 204, 255);
            // Draw player relative to camera position
            SDL_Rect drawPlayer = {(int)roundf(body.x - camera.x), (int)(body.y - camera.y), body.w, body.h};
            SDL_RenderFillRect(renderer, &drawPlayer);
        }

        void DealDamage(vector<Enemy>& enemies);

        void TakeDamage(int damage, Vector2 damageLocation) {
            if (damageCooldown <= 0.0f) {
                knockbackTimer = 0.1f;
                damageCooldown = 0.75f;
                
                // Apply knockback
                calcKnockback(pos, vel, damageLocation);
                
                // TODO: Take damage and die
            }
        }

        // Getters
        Vector2 getPos() { return pos; }
        SDL_Rect getBody() { return body; }

    private:
        Vector2 pos;
        Vector2 vel;
        SDL_Rect body;
        SDL_Rect attackHitbox;

        bool canDash;
        bool isDashing;
        bool dashPressedLastFrame;
        float dashTimer;
        float dashCooldown;

        bool isAttacking;
        bool attackPressedLastFrame;
        AttackDirection attackDirection;
        float attackTimer;
        float attackCooldown;

        bool isGrounded;
        float coyoteTimer;

        float damageCooldown;
        float knockbackTimer;

        bool isJumping;
        bool facingLeft;
        float speed;
        float jumpVelocity;
};


// Enemy class
class Enemy {
    public:
        Enemy(int x, int y, int width, int height, int health, Uint8 r, Uint8 g, Uint8 b):
            pos{(float)x, (float)y},
            vel{0, 0},
            body{x, y, width, height},
            damageCooldown(0.0f),
            knockbackTimer(0.0f),
            colour{r, g, b, 255},
            health(health),
            onScreen(false)
        {};

        void Update(vector<SDL_Rect> platforms, float deltaTime, Vector2 playerPos) {
            if (onScreen) {
                // If not currently taking knockback
                if (knockbackTimer <= 0.0f) {
                    if (playerPos.x < pos.x - body.w + 1) {
                        vel.x = -150.0f;
                    } else if (playerPos.x > pos.x + body.w - 1) {
                        vel.x = 150.0f;
                    } else {
                        vel.x = 0;
                    }

                    // Apply gravity
                    vel.y += Constants::GRAVITY * deltaTime;

                    if (vel.y > Constants::TERMINAL_VELOCITY) {
                        vel.y = Constants::TERMINAL_VELOCITY;
                    }
                } else {
                    knockbackTimer -= deltaTime;
                }
                
                // Apply horizontal velocity
                pos.x += vel.x * deltaTime;
                body.x = (int)pos.x;
                
                for (auto& platform : platforms) {
                    // If enemy is colliding with platform
                    if (AABB(body, platform)) {
                        // If moving left, allign enemys right edge with platforms left edge
                        if (vel.x > 0) {
                            body.x = platform.x - body.w;
                        }
                        // If moving right, allign enemys left edge with platforms right edge
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
                
                for (auto& platform : platforms) {
                    // If enemy is colliding with platform
                    if (AABB(body, platform)) {
                        // If moving down, allign enemys bottom edge with platforms top edge
                        if (vel.y > 0) {
                            body.y = platform.y - body.h;
                        }
                        // If moving up, allign enemys top edge with platforms bottom edge
                        else if (vel.y < 0) {
                            body.y = platform.y + platform.h;
                        }

                        // Sync pos with body after collision
                        pos.y = body.y;
                        
                        // Reset vertical velocity
                        vel.y = 0;
                    }
                }
            }

            // Apply cooldowns
            if (damageCooldown > 0.0f) { damageCooldown -= deltaTime; }
        }

        void Render(SDL_Renderer* renderer, Camera camera) {
            if (onScreen) {
                SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
                // Draw enemy relative to camera position
                SDL_Rect drawEnemy = {(int)roundf(body.x - camera.x), (int)(body.y - camera.y), body.w, body.h};
                SDL_RenderFillRect(renderer, &drawEnemy);
            }
        }

        void DealDamage(Player& player);

        bool TakeDamage(int damage, Vector2 damageLocation) {
            if (damageCooldown <= 0.0f) {
                knockbackTimer = 0.1f;
                damageCooldown = 0.75f;

                // Apply knockback
                calcKnockback(pos, vel, damageLocation);

                // TODO: Take damage and die

                return true;
            } else {
                return false;
            }
        }

        void CheckOnScreen(SDL_Rect cameraRect) {
            if (AABB(body, cameraRect)) {
                onScreen = true;
            } else {
                onScreen = false;
            }
        }

        // Getters
        bool getOnScreen() { return onScreen; }
        SDL_Rect getBody() { return body; }

    private:
        Vector2 pos;
        Vector2 vel;
        SDL_Rect body;

        float damageCooldown;
        float knockbackTimer;

        Colour colour;
        int health;
        bool onScreen;
};


// Class iteration logic (has to be defined after to avoid forward-declare errors)
void Player::DealDamage(vector<Enemy>& enemies) {
    // Only check if player is attacking
    if (!isAttacking) { return; }

    for (auto& enemy : enemies) {
        // Only check enemies that are visible
        if (!enemy.getOnScreen()) { return; }

        if (AABB(enemy.getBody(), attackHitbox)) {
            bool tookDamage = enemy.TakeDamage(0, pos);
            if (tookDamage && !isJumping) {
                switch (attackDirection) {
                    case AttackDirection::DOWN:
                        // Player bounce on enemy when hit
                        vel.y = jumpVelocity * 1.5;
                        attackCooldown = 0.0f;
                    default:
                        break;
                }
            }
        }
    }
}

void Enemy::DealDamage(Player& player) {
    if (onScreen) {
        if (AABB(player.getBody(), body)) {
            player.TakeDamage(0, pos);
        }
    }
}


// Main game logic class
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
            cameraRect({0, 0, Constants::WIN_WIDTH, Constants::WIN_HEIGHT}),
            player(100, 250, 55, 100),
            enemies{
                {1150, 125, 55, 100, 50, 205, 50, 50}
            },
            platforms{
                {0, Constants::FLOOR_LEVEL, Constants::LEVEL_WIDTH, 130},   // Floor
                {0, Constants::FLOOR_LEVEL - 150, 300, 150},
                {550, Constants::FLOOR_LEVEL - 350, 125, 50},
                {1050, Constants::FLOOR_LEVEL - 275, 300, 275},
                {1350, Constants::FLOOR_LEVEL - 140, 150, 140},
                {1625, Constants::FLOOR_LEVEL - 475, 125, 50},
                {1225, Constants::FLOOR_LEVEL - 625, 125, 50},
                {750, Constants::FLOOR_LEVEL - 775, 125, 50},
                {0, Constants::FLOOR_LEVEL - 1005, 700, 50},
                {925, Constants::FLOOR_LEVEL - 1005, Constants::LEVEL_WIDTH - 925, 50}
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
            
            for (auto& enemy : enemies) {
                enemy.CheckOnScreen(cameraRect);
                enemy.Update(platforms, deltaTime, player.getPos());
                enemy.DealDamage(player);
            }

            player.Update(platforms, camera, deltaTime);
            player.DealDamage(enemies);

            cameraRect.x = (int)(camera.x);
            cameraRect.y = (int)(camera.y);

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
            
            for (auto& enemy : enemies) {
                enemy.Render(renderer, camera);
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
        SDL_Rect cameraRect;
        Player player;

        vector<Enemy> enemies;
        vector<SDL_Rect> platforms;
};


// Run game
int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
} 
