// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/sound_mix.hpp"
#include "daggorath/text.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
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

bool tape_name_ok(const std::string& name) {
    if (name.empty() || name.size() > 8) return false;
    for (unsigned char c : name) {
        const bool letter = c >= 'A' && c <= 'Z';
        const bool digit = c >= '0' && c <= '9';
        if (!letter && !digit) return false;
    }
    return true;
}

std::filesystem::path save_file(const std::string& dir, const std::string& name) {
    return std::filesystem::path(dir) / (name + ".dagram");
}

// Copy each new ZSAVE off the in-memory cassette. The core still does not
// know about files; this is the platform envelope around DAGRAM 1.
void persist_saves(dag::Game& game, std::size_t& traced, const std::string& dir) {
    const auto& trace = game.trace();
    while (traced < trace.size()) {
        const dag::TraceEvent& ev = trace[traced++];
        if (ev.kind != "ZSAVE") continue;
        const auto sp = ev.detail.find(' ');
        const std::string name = ev.detail.substr(0, sp);
        if (!tape_name_ok(name)) continue;
        const std::string* image = game.cassette_image(name);
        if (image == nullptr) continue;
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        std::ofstream out(save_file(dir, name), std::ios::binary);
        if (!out) continue;
        out.write(image->data(), static_cast<std::streamsize>(image->size()));
    }
}

std::optional<std::string> read_save(const std::string& dir, const std::string& name) {
    if (!tape_name_ok(name)) return {};
    std::ifstream in(save_file(dir, name), std::ios::binary);
    if (!in) return {};
    std::ostringstream text;
    text << in.rdbuf();
    std::string image = text.str();
    if (image.rfind("DAGRAM 1", 0) != 0) return {};
    return image;
}

enum class DeathPrompt { Playing, Menu, LoadName };

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

    std::optional<dag::Game> held;
    held.emplace();
    dag::SoundMix mix;
    std::string save_dir = "saved";
    if (char* pref = SDL_GetPrefPath("daggorath", "dod")) {
        save_dir = pref;
        SDL_free(pref);
    }
    DeathPrompt prompt = DeathPrompt::Playing;
    std::string load_name;
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
    std::size_t traced = 0;
    bool audio_level = false;
    std::uint64_t audio_jiffy = 0;
    std::string message;
    bool running = true;
    auto reset_view = [&]() {
        heard = 0;
        traced = 0;
        audio_level = false;
        audio_jiffy = 0;
        heartbeat.clear();
        message.clear();
        mix = dag::SoundMix{};
        prompt = DeathPrompt::Playing;
        load_name.clear();
    };
    auto restart_game = [&]() {
        held.emplace();
        reset_view();
    };
    auto resume_view = [&](dag::Game& game) {
        heard = game.events().size();
        traced = game.trace().size();
        audio_level = game.heart().audio_level;
        audio_jiffy = game.counters().total_jiffies;
        heartbeat.clear();
        message.clear();
        prompt = DeathPrompt::Playing;
        load_name.clear();
    };
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) continue;
            const SDL_Keycode key = event.key.key;
            if (prompt == DeathPrompt::Menu) {
                if (key == SDLK_R) {
                    restart_game();
                    break;
                }
                else if (key == SDLK_L) {
                    prompt = DeathPrompt::LoadName;
                    load_name.clear();
                }
                continue;
            }
            if (prompt == DeathPrompt::LoadName) {
                if (key == SDLK_ESCAPE) {
                    prompt = DeathPrompt::Menu;
                    load_name.clear();
                } else if (key == SDLK_BACKSPACE) {
                    if (!load_name.empty()) load_name.pop_back();
                } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    std::optional<std::string> image = read_save(save_dir, load_name);
                    if (!image) {
                        if (const std::string* mem = held->cassette_image(load_name))
                            if (mem->rfind("DAGRAM 1", 0) == 0) image = *mem;
                    }
                    if (!image) {
                        message = "???";
                        prompt = DeathPrompt::Menu;
                        load_name.clear();
                    } else {
                        held->restore_ram_image(*image);
                        if (held->player().dead) prompt = DeathPrompt::Menu;
                        else resume_view(*held);
                    }
                } else if (key >= SDLK_A && key <= SDLK_Z && load_name.size() < 8) {
                    load_name.push_back(static_cast<char>('A' + (key - SDLK_A)));
                } else if (key >= SDLK_0 && key <= SDLK_9 && load_name.size() < 8) {
                    load_name.push_back(static_cast<char>('0' + (key - SDLK_0)));
                }
                continue;
            }
            // SDLK_A..SDLK_Z are 'a'..'z'. The line editor only accepts 'A'..'Z'.
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) held->press(0x0D);
            else if (key == SDLK_SPACE) held->press(0x20);
            else if (key == SDLK_BACKSPACE) held->press(0x08);
            else if (key >= SDLK_A && key <= SDLK_Z)
                held->press(static_cast<std::uint8_t>('A' + (key - SDLK_A)));
        }
        dag::Game& game = *held;
        const std::uint64_t now_ns = SDL_GetTicksNS();
        const std::uint64_t elapsed_us = now_ns > last_ns ? (now_ns - last_ns) / 1000 : 0;
        last_ns = now_ns;
        const int steps = dag::jiffies_due(elapsed_us, owed);
        if (steps > 0 && prompt == DeathPrompt::Playing)
            game.advance_jiffies(static_cast<std::uint64_t>(steps));
        persist_saves(game, traced, save_dir);
        if (prompt == DeathPrompt::Playing && game.player().dead) prompt = DeathPrompt::Menu;
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
        std::string command_override;
        if (prompt == DeathPrompt::Menu) command_override = "R RESTART OR L LOAD";
        else if (prompt == DeathPrompt::LoadName) {
            command_override = "LOAD " + load_name;
            if (command_override.size() < 32) command_override.push_back('_');
        }
        dag::paint_text_bands(frame.data(), dag::kScreenWidth, chrome, message, command_override);
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
