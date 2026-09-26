// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/snapshot.hpp"

#include <SDL3/SDL.h>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    SDL_Window* window = SDL_CreateWindow("Dungeons of Daggorath", 256 * 3, 192 * 3, 0);
    if (window == nullptr) return 1;
    dag::Game game;
    std::uint64_t owed = 0;
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                const SDL_Keycode key = event.key.key;
                if (key == SDLK_RETURN) game.press(0x0D);
                else if (key == SDLK_SPACE) game.press(0x20);
                else if (key >= SDLK_A && key <= SDLK_Z) game.press(static_cast<std::uint8_t>(key));
                else if (key >= 'a' && key <= 'z') game.press(static_cast<std::uint8_t>(key - 32));
            }
        }
        const int steps = dag::jiffies_due(16667, owed);
        if (steps > 0) game.advance_jiffies(static_cast<std::uint64_t>(steps));
        const auto frame = dag::rasterize(dag::snapshot_from(game));
        (void)frame;
        SDL_Delay(1);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
