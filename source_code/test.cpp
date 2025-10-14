#include <iostream>
#include <SDL.h>
using namespace std;

struct Constants {
    static constexpr int WIN_WIDTH = 800;
    static constexpr int WIN_HEIGHT = 600;
    static constexpr float GRAVITY = 1800.0f;
    static constexpr float TERMINAL_VELOCITY = 1080.0f;
};

struct Vector2 {
    float x, y;
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
            isJumping(false),
            isGrounded(false),
            speed(300.0f),
            jumpVelocity(-660.0f)
        {}

        void HandleInput(const Uint8* keystate) {
            // Reset horizontal velocity
            vel.x = 0;

            // Move Left
            if (keystate[SDL_SCANCODE_A]) {
                vel.x = -speed;
            }
            // Move Right
            if (keystate[SDL_SCANCODE_D]) {
                vel.x = speed;
            }
            // Jump
            if (keystate[SDL_SCANCODE_SPACE]) {
                if (isGrounded && !isJumping) {
                    vel.y = jumpVelocity;
                    isJumping = true;
                }
            } else {
                isJumping = false;
            }
        }

        void Update(vector<SDL_Rect> platforms, float deltaTime) {
            // Apply gravity
            vel.y += Constants::GRAVITY * deltaTime;

            if (vel.y > Constants::TERMINAL_VELOCITY) {
                vel.y = Constants::TERMINAL_VELOCITY;
            }
            
            // Apply horizontal velocity
            pos.x += vel.x * deltaTime;
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
        }

        void Render(SDL_Renderer* renderer) {
            SDL_SetRenderDrawColor(renderer, 62, 146, 204, 255);
            SDL_RenderFillRect(renderer, &body);
        }
    
    private:
        Vector2 pos;
        Vector2 vel;
        SDL_Rect body;
        bool isJumping;
        bool isGrounded;
        float speed;
        float jumpVelocity;
};

class Game {
    public:
        Game():
            window(nullptr),
            renderer(nullptr),
            previousTick(0),
            isRunning(false),
            deltaTime(0),
            player(385, 275, 30, 45),
            platforms{
                {0, 550, 800, 50},
                {150, 450, 100, 20},
                {350, 350, 100, 20},
                {550, 250, 100, 20},
            }
        {}

        void Initialise() {
            // Initialise SDL, output error if fails
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

            const Uint8* keystate = SDL_GetKeyboardState(nullptr);
            player.HandleInput(keystate);
        }

        void Update() {
            // Calculate deltaTime to normalise movement
            Uint32 currentTick = SDL_GetTicks();
            deltaTime = (currentTick - previousTick) / 1000.0f;
            previousTick = currentTick;

            player.Update(platforms, deltaTime);
        }

        void Render() {
            SDL_SetRenderDrawColor(renderer, 29, 62, 94, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 42, 98, 143, 255);
            for (auto& platform : platforms) {
                SDL_RenderFillRect(renderer, &platform);
            }

            player.Render(renderer);

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
        
        Uint32 previousTick;
        bool isRunning;
        float deltaTime;
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
