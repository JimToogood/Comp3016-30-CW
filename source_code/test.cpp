#include <iostream>
#include <SDL.h>
using namespace std;

class Game {
    public:
        Game(): window(nullptr), renderer(nullptr), isRunning(false) {};

        void Initialise() {
            // Initialise SDL, output error if fails
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                cerr << "SDL could not initialise. Error: " << SDL_GetError() << endl;
                return;
            }

            // Initialise window, output error if fails
            window = SDL_CreateWindow(
                "Game Title Here",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WIDTH, HEIGHT,
                SDL_WINDOW_SHOWN
            );
            if (!window) {
                cerr << "Window could not initialise. Error: " << SDL_GetError() << endl;
                return;
            }

            // Initialise renderer, output error if fails
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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
                // Left or Right click pressed
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    cout << "Mouse button down." << endl;
                }
            }
        }

        //void Update() {}

        void Render() {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);   // white background (r, g, b, a)
            SDL_RenderClear(renderer);

            SDL_RenderPresent(renderer);
        }

        void Run() {
            while (isRunning) {
                HandleInput();
                //Update();
                Render();
            }
        }

        void CleanUp() {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }

    private:
        const int WIDTH = 800;
        const int HEIGHT = 600;

        SDL_Window* window;
        SDL_Renderer* renderer;

        bool isRunning;
};

int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
} 
