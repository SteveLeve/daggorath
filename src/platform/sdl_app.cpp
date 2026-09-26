// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/snapshot.hpp"

#include <SDL3/SDL.h>

#include <vector>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    constexpr int kScale = 3;
    SDL_Window* window = SDL_CreateWindow("Dungeons of Daggorath",
                                          dag::kScreenWidth * kScale,
                                          dag::kScreenHeight * kScale, 0);
    if (window == nullptr) return 1;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             dag::kScreenWidth * kScale,
                                             dag::kScreenHeight * kScale);
    if (renderer == nullptr || texture == nullptr) return 1;

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
        const auto scaled = dag::scale_frame(dag::rasterize(dag::snapshot_from(game)), kScale);
        std::vector<std::uint8_t> rgb(scaled.size() * 3);
        for (std::size_t i = 0; i < scaled.size(); ++i) {
            const std::uint8_t value = scaled[i] ? 255 : 0;
            rgb[i * 3] = value;
            rgb[i * 3 + 1] = value;
            rgb[i * 3 + 2] = value;
        }
        const int pitch = dag::kScreenWidth * kScale * 3;
        SDL_UpdateTexture(texture, nullptr, rgb.data(), pitch);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
