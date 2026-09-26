// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/sound_mix.hpp"
#include "daggorath/text.hpp"

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
    std::string samples_path;
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
        else if (arg == "--samples") samples_path = next();
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
    if (!samples_path.empty()) {
        dag::SoundMix mix;
        mix.consume(game.trace());
        std::ofstream out(samples_path, std::ios::binary);
        if (!out) return 1;
        out.write(reinterpret_cast<const char*>(mix.pending().data()),
                  static_cast<std::streamsize>(mix.pending().size()));
    }
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
    else std::cerr << "audio device unavailable\n";
    std::uint64_t owed = 0;
    std::uint64_t last_ns = SDL_GetTicksNS();
    std::vector<std::uint8_t> heartbeat;
    std::size_t heard = 0;
    bool audio_level = false;
    std::uint64_t audio_jiffy = 0;
    std::string message;
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                const SDL_Keycode key = event.key.key;
                // SDLK_A..SDLK_Z are 'a'..'z'. The line editor only accepts 'A'..'Z'.
                if (key == SDLK_RETURN) game.press(0x0D);
                else if (key == SDLK_SPACE) game.press(0x20);
                else if (key == SDLK_BACKSPACE) game.press(0x08);
                else if (key >= SDLK_A && key <= SDLK_Z)
                    game.press(static_cast<std::uint8_t>('A' + (key - SDLK_A)));
            }
        }
        const std::uint64_t now_ns = SDL_GetTicksNS();
        const std::uint64_t elapsed_us = now_ns > last_ns ? (now_ns - last_ns) / 1000 : 0;
        last_ns = now_ns;
        const int steps = dag::jiffies_due(elapsed_us, owed);
        if (steps > 0) game.advance_jiffies(static_cast<std::uint64_t>(steps));
        auto frame = dag::rasterize(dag::snapshot_from(game));
        dag::TextSnapshot chrome;
        auto hand = [&](int index) -> std::optional<dag::Ocb> {
            if (index < 0 || static_cast<std::size_t>(index) >= game.objects().size()) return {};
            return game.objects()[static_cast<std::size_t>(index)];
        };
        chrome.left = hand(game.player().left_hand);
        chrome.right = hand(game.player().right_hand);
        chrome.line = game.line_buffer();
        if (game.heart().heartf != 0) {
            chrome.heart = game.heart().hearts != 0 ? dag::HeartGlyph::Large
                                                    : dag::HeartGlyph::Small;
        }
        const auto& events = game.events();
        while (heard < events.size()) {
            const dag::CoreEvent& ev = events[heard++];
            if (ev.kind == dag::CoreEventKind::Text) message = ev.text;
            if (ev.kind != dag::CoreEventKind::Heartbeat) continue;
            const std::uint64_t span = ev.jiffy > audio_jiffy ? ev.jiffy - audio_jiffy : 0;
            const std::uint8_t sample = audio_level ? 0xFF : 0x00;
            for (std::uint64_t n = 0; n < span * 100; ++n) heartbeat.push_back(sample);
            audio_level = ev.audio_level;
            audio_jiffy = ev.jiffy;
        }
        const std::uint64_t now_jiffy = game.counters().total_jiffies;
        if (now_jiffy > audio_jiffy) {
            const std::uint8_t sample = audio_level ? 0xFF : 0x00;
            for (std::uint64_t n = 0; n < (now_jiffy - audio_jiffy) * 100; ++n)
                heartbeat.push_back(sample);
            audio_jiffy = now_jiffy;
        }
        dag::paint_text_bands(frame.data(), dag::kScreenWidth, chrome, message);
        const auto scaled = dag::scale_frame(frame, kScale);
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
        if (audio != nullptr) {
            if (!heartbeat.empty()) {
                SDL_PutAudioStreamData(audio, heartbeat.data(),
                                       static_cast<int>(heartbeat.size()));
                heartbeat.clear();
            }
            if (!mix.pending().empty()) {
                SDL_PutAudioStreamData(audio, mix.pending().data(),
                                       static_cast<int>(mix.pending().size()));
                mix.clear();
            }
        }
        SDL_Delay(1);
    }
    if (audio != nullptr) SDL_DestroyAudioStream(audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
