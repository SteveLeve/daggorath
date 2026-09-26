// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/sound_mix.hpp"

#include <SDL3/SDL.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

int headless(int argc, char** argv) {
    std::string script_path;
    std::uint64_t jiffies = 80;
    bool have_second = false;
    int second = 0;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) return {};
            return argv[++i];
        };
        if (arg == "--script") script_path = next();
        else if (arg == "--jiffies") jiffies = std::strtoull(next().c_str(), nullptr, 10);
        else if (arg == "--second") {
            second = std::atoi(next().c_str());
            have_second = true;
        }
    }
    std::optional<dag::Game> held;
    if (have_second) held.emplace(static_cast<std::uint8_t>(second), 0);
    else held.emplace();
    dag::Game& game = *held;
    if (!script_path.empty()) {
        std::ifstream in(script_path);
        if (!in) return 1;
        std::ostringstream text;
        text << in.rdbuf();
        std::string error;
        auto keys = dag::parse_script(text.str(), error);
        if (!error.empty()) return 1;
        game.load_script(std::move(keys));
    }
    game.advance_jiffies(jiffies);
    std::cout << "# jiffy\tclock\tevent\tdetail\n";
    for (const auto& event : game.trace()) std::cout << event.to_line() << "\n";
    std::cout << "# final\trow=" << game.player().row << "\tcol=" << game.player().col
              << "\tdir=" << static_cast<int>(game.player().dir)
              << "\tdamage=" << game.player().damage << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--headless") return headless(argc, argv);
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
    dag::SoundMix mix;
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_U8;
    spec.channels = 1;
    spec.freq = 6000;
    SDL_AudioStream* audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                                       nullptr, nullptr);
    if (audio != nullptr) SDL_ResumeAudioStreamDevice(audio);
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
        mix.consume(game.trace());
        if (audio != nullptr && !mix.pending().empty()) {
            SDL_PutAudioStreamData(audio, mix.pending().data(),
                                   static_cast<int>(mix.pending().size()));
            mix.clear();
        }
        SDL_Delay(1);
    }
    if (audio != nullptr) SDL_DestroyAudioStream(audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
