// Present when SDL3 is available. The core is advanced one jiffy at a time.
#include "daggorath/game.hpp"
#include "daggorath/crisp.hpp"
#include "daggorath/examine.hpp"
#include "daggorath/mapper.hpp"
#include "daggorath/overlay_bridge.hpp"
#include "daggorath/prefs.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/shell.hpp"
#include "daggorath/system_menu.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/snoise.hpp"
#include "daggorath/storage.hpp"
#include "daggorath/sound_mix.hpp"
#include "daggorath/text.hpp"
#include "daggorath/text_tables.hpp"
#include "daggorath/touch_overlay.hpp"

#include <SDL3/SDL.h>


#include <array>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <cmath>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
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

// Copy each new ZSAVE off the in-memory cassette into storage. The core
// still does not know about files; this is the platform envelope around
// DAGRAM 1. A save that could not be stored stays on the cassette for this
// session only, and the player is told so (the in-game ZSAVE itself succeeded);
// its name goes in `unstored` so a menu Restart can carry it across.
void persist_saves(dag::Game& game, std::size_t& traced, dag::platform::Storage& storage,
                   std::set<std::string>& unstored) {
    const auto& trace = game.trace();
    while (traced < trace.size()) {
        const dag::TraceEvent& ev = trace[traced++];
        if (ev.kind != "ZSAVE") continue;
        const auto sp = ev.detail.find(' ');
        const std::string name = ev.detail.substr(0, sp);
        if (!dag::platform::tape_name_ok(name)) continue;
        const std::string* image = game.cassette_image(name);
        if (image == nullptr) continue;
        if (storage.store_save(name, *image)) {
            unstored.erase(name);
        } else {
            unstored.insert(name);
            dag::platform::report_storage_problem(
                "ZSAVE " + name + " NOT STORED - storage full or blocked. "
                "It is kept for this session only.");
        }
    }
}

// Put every stored save on the core's cassette before play, like a tape that
// already holds earlier ZSAVEs. The player still types ZLOAD <name>.
// Returns how many the core accepted. Names in `skip` are left off: the
// session holds a newer, unstored image under that name.
std::size_t mount_saves(dag::Game& game, const dag::platform::Storage& storage,
                        const std::set<std::string>& skip = {}) {
    const auto loaded = storage.load_saves();
    std::size_t mounted = 0;
    for (const auto& save : loaded.saves)
        if (!skip.count(save.name) && game.insert_cassette_image(save.name, save.image)) ++mounted;
    if (!loaded.error.empty())
        dag::platform::report_storage_problem("SAVED GAMES UNAVAILABLE - " + loaded.error);
    return mounted;
}

std::array<std::uint8_t, dag::kScreenWidth * dag::kScreenHeight> turn_wipe(int bar_x) {
    // PTURN.ASM LRTURN/RLTURN: two horizontal lines and a vertical bar.
    // The sweep is eight positions inside one foreground burst. One bar is
    // what a single video frame can show.
    std::array<std::uint8_t, dag::kScreenWidth * dag::kScreenHeight> pixels{};
    auto plot = [&](int x, int y) {
        if (x < 0 || x >= dag::kScreenWidth || y < 0 || y >= dag::kViewportScanlineEnd) return;
        pixels[static_cast<std::size_t>(y * dag::kScreenWidth + x)] = 1;
    };
    for (int x = 0; x < dag::kScreenWidth; ++x) {
        plot(x, 16);
        plot(x, 136);
    }
    for (int y = 17; y <= 135; ++y) plot(bar_x, y);
    return pixels;
}

// --shots=<file>: a scripted session for looking at the window without a
// display (run with SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy). Each
// line is "<ms> key <SDL key name>", "<ms> tap <x> <y>", "<ms> shot <file.bmp>"
// or "<ms> quit", ms counted from the first frame. Keys and taps go through
// the same SDL event path as a real keyboard and mouse; a shot saves the
// next presented frame. Development tooling only.
struct ShotStep {
    std::uint64_t ms = 0;
    std::string verb;
    std::string arg;
    double x = 0, y = 0;
};

// 8.6.8: one row of the system menu (Top/ChooseSave/ChooseLoad/a
// confirmation), styled and hit-tested like the touch overlay's own pickers
// (input/touch_overlay.hpp's Choice) instead of plain unboxed text. `activate`
// is empty for a heading row (e.g. "SAVE SLOT 1-5"), which draws without a
// box and never hit-tests.
struct MenuRow {
    std::string label;
    dag::input::Rect rect;
    std::function<void()> activate;
};

std::vector<ShotStep> load_shots(const std::string& path) {
    std::vector<ShotStep> steps;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        ShotStep step;
        ls >> step.ms >> step.verb;
        if (step.verb == "tap") ls >> step.x >> step.y;
        else std::getline(ls >> std::ws, step.arg);
        steps.push_back(step);
    }
    return steps;
}

std::string g_shot_path;  // set: save the next presented frame here

void save_shot(SDL_Renderer* renderer) {
    if (g_shot_path.empty()) return;
    if (SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr)) {
        SDL_SaveBMP(surface, g_shot_path.c_str());
        SDL_DestroySurface(surface);
    }
    g_shot_path.clear();
}

// Screen polarity follows VDGINV; see dag::apply_vdginv (NEWLVL.ASM NLVL50).
// `game_x`/`game_w`/`game_h` place the fixed 4:3 game render within the
// window: equal to the whole window for Tablet4x3 (no letterboxing), offset
// and narrower than the window for PhoneLandscape (8.6.5), whose black side
// margins the touch overlay's corner/side buttons sit in (design doc,
// "Landscape, thumbs on the sides"). `overlay` draws on top of the blitted
// frame, before the flip -- the touch overlay's buttons/picker (8.6.1), the
// shell's system menu (8.6.2), and crisp's vector overdraw (8.6.3). Absent
// for the transitional animation frames (turn wipe, faint, wizard fade),
// which do not draw it.
void present_frame(SDL_Renderer* renderer, SDL_Texture* texture,
                   std::array<std::uint8_t, dag::kScreenWidth * dag::kScreenHeight> pixels,
                   int level, double game_x, double game_w, double game_h,
                   const std::function<void(SDL_Renderer*)>& overlay = nullptr) {
    dag::apply_vdginv(pixels, level);
    constexpr int kScale = 3;
    const auto scaled = dag::scale_frame(pixels, kScale);
    std::vector<std::uint8_t> rgb(scaled.size() * 3);
    for (std::size_t i = 0; i < scaled.size(); ++i) {
        const std::uint8_t value = scaled[i] ? 255 : 0;
        rgb[i * 3] = value;
        rgb[i * 3 + 1] = value;
        rgb[i * 3 + 2] = value;
    }
    const int pitch = dag::kScreenWidth * kScale * 3;
    SDL_UpdateTexture(texture, nullptr, rgb.data(), pitch);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);  // letterbox margins (PhoneLandscape) stay black every frame
    const SDL_FRect dst{static_cast<float>(game_x), 0, static_cast<float>(game_w),
                        static_cast<float>(game_h)};
    SDL_RenderTexture(renderer, texture, nullptr, &dst);
    if (overlay) overlay(renderer);
    save_shot(renderer);
    SDL_RenderPresent(renderer);
}

// Button labels: the boards' letters (A G P C E L) through the original char
// generator, and line-drawn icons for the ones the boards draw as symbols
// (⇤ ↑ ⇥ ↶ ↻ ↷ ↓ ≡), which that uppercase-only set cannot draw. 0 means
// "icon, no letter". The system menu, which no board draws, gets a pause
// sign.
char button_label(dag::input::ButtonId id) {
    using dag::input::ButtonId;
    switch (id) {
        case ButtonId::AttackLeft:
        case ButtonId::AttackRight:
            return 'A';
        case ButtonId::HandMenuLeft:
        case ButtonId::HandMenuRight:
            return 0;
        case ButtonId::MoveForward:
        case ButtonId::MoveBack:
        case ButtonId::MoveLeft:
        case ButtonId::MoveRight:
        case ButtonId::TurnLeft:
        case ButtonId::TurnRight:
        case ButtonId::TurnAround:
            return 0;
        case ButtonId::Climb:
            return 'C';
        case ButtonId::Examine:
            return 'E';
        case ButtonId::Look:
            return 'L';
        case ButtonId::SystemMenu:
            return 0;
    }
    return '?';
}

// Icon strokes in a unit box (-1..1, y down), scaled into the button.
using Stroke = std::vector<std::pair<float, float>>;

std::vector<Stroke> icon_strokes(dag::input::ButtonId id) {
    using dag::input::ButtonId;
    auto arc = [](float cx, float cy, float r, float from_deg, float to_deg) {
        Stroke out;
        constexpr int kSteps = 16;
        for (int i = 0; i <= kSteps; ++i) {
            const float a = (from_deg + (to_deg - from_deg) * i / kSteps) * 3.14159265f / 180.0f;
            out.push_back({cx + r * std::cos(a), cy + r * std::sin(a)});
        }
        return out;
    };
    switch (id) {
        case ButtonId::MoveForward:
            return {{{0, 0.8f}, {0, -0.8f}}, {{-0.5f, -0.3f}, {0, -0.8f}, {0.5f, -0.3f}}};
        case ButtonId::MoveBack:
            return {{{0, -0.8f}, {0, 0.8f}}, {{-0.5f, 0.3f}, {0, 0.8f}, {0.5f, 0.3f}}};
        case ButtonId::MoveLeft:
            return {{{-0.8f, -0.6f}, {-0.8f, 0.6f}}, {{0.8f, 0}, {-0.65f, 0}},
                    {{-0.15f, -0.5f}, {-0.65f, 0}, {-0.15f, 0.5f}}};
        case ButtonId::MoveRight:
            return {{{0.8f, -0.6f}, {0.8f, 0.6f}}, {{-0.8f, 0}, {0.65f, 0}},
                    {{0.15f, -0.5f}, {0.65f, 0}, {0.15f, 0.5f}}};
        case ButtonId::TurnLeft:  // ↶: over the top, ending pointing down at the left
            return {arc(0, 0.2f, 0.65f, 0, -180), {{-1.0f, -0.15f}, {-0.65f, 0.3f}, {-0.3f, -0.15f}}};
        case ButtonId::TurnRight:  // ↷
            return {arc(0, 0.2f, 0.65f, 180, 360), {{1.0f, -0.15f}, {0.65f, 0.3f}, {0.3f, -0.15f}}};
        case ButtonId::TurnAround:  // ↻: most of a circle, head at the top
            return {arc(0, 0, 0.7f, -60, 240), {{-0.05f, -0.95f}, {0.35f, -0.6f}, {-0.05f, -0.3f}}};
        case ButtonId::HandMenuLeft:
        case ButtonId::HandMenuRight:  // ≡
            return {{{-0.7f, -0.5f}, {0.7f, -0.5f}}, {{-0.7f, 0}, {0.7f, 0}}, {{-0.7f, 0.5f}, {0.7f, 0.5f}}};
        case ButtonId::SystemMenu:  // pause sign
            return {{{-0.3f, -0.6f}, {-0.3f, 0.6f}}, {{0.3f, -0.6f}, {0.3f, 0.6f}}};
        default:
            return {};
    }
}

void draw_icon(SDL_Renderer* renderer, const dag::input::Rect& rect, dag::input::ButtonId id) {
    const float cx = static_cast<float>(rect.x + rect.w / 2);
    const float cy = static_cast<float>(rect.y + rect.h / 2);
    const float half = static_cast<float>(rect.w * 0.32);
    for (const auto& stroke : icon_strokes(id)) {
        for (std::size_t i = 1; i < stroke.size(); ++i) {
            const float x0 = cx + stroke[i - 1].first * half, y0 = cy + stroke[i - 1].second * half;
            const float x1 = cx + stroke[i].first * half, y1 = cy + stroke[i].second * half;
            for (int dx = -1; dx <= 1; ++dx)  // three pixels wide, like the letters' dots
                for (int dy = -1; dy <= 1; ++dy)
                    SDL_RenderLine(renderer, x0 + dx, y0 + dy, x1 + dx, y1 + dy);
        }
    }
}

// glyph_rows takes the original char generator's own codes (text.cpp's
// code_for: 'A'-'Z' -> 1-26 via kSwcTab, not ASCII 0x41-0x5A); every button
// label here is a letter, so this is the only case that matters. Codes
// >= 0x20 index kSpcTab directly, but that table is only 28 bytes (4
// glyphs: the heart icon's two sizes, `paint_text_bands`' own use) -- not a
// general ASCII font, so digits/punctuation are NOT reachable this way
// (screenshot-verified blank when tried; see 8.6.6's menu text, which uses
// SDL_RenderDebugText instead for exactly this reason).
std::uint8_t glyph_code(char label) {
    if (label >= 'A' && label <= 'Z') return static_cast<std::uint8_t>(label - 'A' + 1);
    return 0x1D;  // code_for's '?': anything unmapped shows as a question mark
}

void draw_glyph(SDL_Renderer* renderer, double center_x, double center_y, char label) {
    std::uint8_t rows[7];
    dag::glyph_rows(glyph_code(label), rows);
    constexpr float kDot = 3.0f;
    for (int row = 0; row < 7; ++row) {
        for (int bit = 0; bit < 8; ++bit) {
            if ((rows[row] & (0x80 >> bit)) == 0) continue;
            const SDL_FRect px{static_cast<float>(center_x) - 4 * kDot + bit * kDot,
                              static_cast<float>(center_y) - 3.5f * kDot + row * kDot, kDot, kDot};
            SDL_RenderFillRect(renderer, &px);
        }
    }
}

void draw_button(SDL_Renderer* renderer, const dag::input::Rect& rect, char label,
                 bool pressed = false) {
    const SDL_FRect r{static_cast<float>(rect.x), static_cast<float>(rect.y),
                      static_cast<float>(rect.w), static_cast<float>(rect.h)};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if (pressed) SDL_RenderFillRect(renderer, &r);  // inverted, as the boards draw it
    else SDL_RenderRect(renderer, &r);
    if (pressed) SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    if (label != 0) draw_glyph(renderer, rect.x + rect.w / 2, rect.y + rect.h / 2, label);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

// 8.6.6: one line of left-aligned text for the system menu (Resume/Save/
// Load/Restart/Quit, ADR-0009 §6) -- this menu is this project's own new
// UI, not a projection of the original's display, so it uses SDL3's built-in
// debug font (SDL_RenderDebugText) rather than the original char generator
// (draw_glyph/glyph_rows, used for the touch overlay's single-letter button
// labels): kSpcTab -- the table glyph_rows indexes for any code >= 0x20 --
// is only 28 bytes (4 glyphs, the heart icon's sizes), not a general ASCII
// font, so it cannot draw slot names, digits, or punctuation (confirmed
// blank on screen when tried).
void draw_text_line(SDL_Renderer* renderer, double x, double y, const std::string& text,
                    float scale = 1.0f) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, static_cast<float>(x) / scale, static_cast<float>(y) / scale,
                        text.c_str());
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

// A label of 8x8 debug-font characters at `scale`, centred in `rect`.
void draw_text_centered(SDL_Renderer* renderer, const dag::input::Rect& rect,
                        const std::string& text, float scale) {
    const double w = 8.0 * scale * static_cast<double>(text.size());
    draw_text_line(renderer, rect.x + (rect.w - w) / 2, rect.y + (rect.h - 8.0 * scale) / 2, text,
                   scale);
}

// crisp render style (8.6.3, ADR-0010 §2): device-scaled line drawing
// for the same draw list `pixel` rasterises into a bitmap. Coordinates from
// dag::build_crisp_frame/build_crisp_map are source-256x192 space; this is
// the "platform maps them to device pixels" half ADR-0010 leaves to the
// caller. Text stays on the bitmap path (ADR-0010 §6, unchanged) -- these
// only draw over the viewport band (rows 0..kViewportScanlineEnd), never the
// status/command text bands below it.
void draw_crisp_view(SDL_Renderer* renderer, const dag::RenderState& state, int scale,
                     double x_offset, std::uint8_t ink, std::uint8_t paper) {
    const dag::CrispFrame crisp = dag::build_crisp_frame(state);
    for (const auto& line : crisp.lines) {
        const auto shade = dag::crisp_shade(line.fade, ink, paper);
        SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
        SDL_RenderLine(renderer, static_cast<float>(line.x0 * scale + x_offset),
                       static_cast<float>(line.y0 * scale), static_cast<float>(line.x1 * scale + x_offset),
                       static_cast<float>(line.y1 * scale));
    }
}

void draw_crisp_map(SDL_Renderer* renderer, const dag::MapSnapshot& snap, int scale,
                    double x_offset, std::uint8_t ink) {
    const dag::CrispMap crisp = dag::build_crisp_map(snap);
    SDL_SetRenderDrawColor(renderer, ink, ink, ink, 255);
    for (const auto& wall : crisp.walls) {
        const SDL_FRect r{static_cast<float>(wall.x * scale + x_offset),
                          static_cast<float>(wall.y * scale), static_cast<float>(wall.w * scale),
                          static_cast<float>(wall.h * scale)};
        SDL_RenderFillRect(renderer, &r);
    }
    for (const auto& mark : crisp.marks) {
        for (int row = 0; row < 6; ++row) {
            for (int bit = 0; bit < 8; ++bit) {
                if ((mark.rows[static_cast<std::size_t>(row)] & (0x80 >> bit)) == 0) continue;
                const SDL_FRect px{static_cast<float>((mark.x + bit) * scale + x_offset),
                                   static_cast<float>((mark.y + row) * scale),
                                   static_cast<float>(scale), static_cast<float>(scale)};
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}


// ADR-0011: in the browser the page's CSS sizes the canvas (a fixed-size
// window would be cropped or ignored), so the window is resizable there and
// SDL letterboxes the game's fixed window coordinates into it. Pointer events
// are mapped back to those coordinates as they are polled. The desktop keeps
// its fixed window, unchanged. No SDL_WINDOW_HIGH_PIXEL_DENSITY: with it,
// SDL 3.4 writes inline CSS sizes onto the canvas (and a 1x1 one when the
// WebGL renderer recreates the window), overriding the page's sizing.
#ifdef __EMSCRIPTEN__
constexpr SDL_WindowFlags kWindowFlags = SDL_WINDOW_RESIZABLE;
#else
constexpr SDL_WindowFlags kWindowFlags = 0;
#endif

void fit_to_window([[maybe_unused]] SDL_Renderer* renderer, [[maybe_unused]] double w,
                   [[maybe_unused]] double h) {
#ifdef __EMSCRIPTEN__
    SDL_SetRenderLogicalPresentation(renderer, static_cast<int>(w), static_cast<int>(h),
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--headless") return headless(argc, argv);
    // 8.6.5: --layout=phone simulates a 19.5:9 phone in landscape (design
    // doc: "the game renders full screen at 4:3, full height, centred...
    // controls sit in the black side margins"); default stays Tablet4x3,
    // which the fixed 4:3 window already matches without letterboxing.
    // Which layout starts: an explicit --layout flag, else the player's
    // remembered choice, else --default-layout (the web page passes one
    // from the screen's shape), else Tablet4x3.
    std::optional<dag::input::OverlayLayout> flag_layout;
    std::optional<dag::input::OverlayLayout> default_layout;
    std::string shots_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--layout=phone") flag_layout = dag::input::OverlayLayout::PhoneLandscape;
        else if (arg == "--layout=tablet") flag_layout = dag::input::OverlayLayout::Tablet4x3;
        else if (arg == "--default-layout=phone") default_layout = dag::input::OverlayLayout::PhoneLandscape;
        else if (arg == "--default-layout=tablet") default_layout = dag::input::OverlayLayout::Tablet4x3;
        else if (arg.rfind("--shots=", 0) == 0) shots_path = arg.substr(8);
    }
    std::vector<ShotStep> shots = shots_path.empty() ? std::vector<ShotStep>{} : load_shots(shots_path);
    std::size_t next_shot = 0;
    std::uint64_t shots_start_ms = 0;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    std::string save_dir = "saved";
    if (char* pref = SDL_GetPrefPath("daggorath", "dod")) {
        save_dir = pref;
        SDL_free(pref);
    }
    dag::platform::Storage storage(save_dir);
    // Scripted screenshot runs (--shots) neither read nor write preferences,
    // so they draw the same frames on every machine.
    const bool remember_prefs = shots_path.empty();
    dag::platform::Prefs prefs;
    if (remember_prefs)
        if (auto text = storage.load_prefs()) prefs = dag::platform::parse_prefs(*text);
    dag::input::OverlayLayout layout =
        flag_layout.value_or(prefs.layout.value_or(default_layout.value_or(dag::input::OverlayLayout::Tablet4x3)));
    constexpr int kScale = 3;
    constexpr double kGameW = dag::kScreenWidth * kScale;
    constexpr double kGameH = dag::kScreenHeight * kScale;
    double window_w = layout == dag::input::OverlayLayout::PhoneLandscape
                          ? kGameH * 19.5 / 9.0
                          : kGameW;
    const double window_h = kGameH;
    double game_x = (window_w - kGameW) / 2.0;
    SDL_Window* window = SDL_CreateWindow("Dungeons of Daggorath", static_cast<int>(window_w),
                                          static_cast<int>(window_h), kWindowFlags);
    if (window == nullptr) return 1;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             dag::kScreenWidth * kScale,
                                             dag::kScreenHeight * kScale);
    if (renderer == nullptr || texture == nullptr) return 1;
    fit_to_window(renderer, window_w, window_h);
    auto make_texture = [&]() {
        return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                 dag::kScreenWidth * kScale, dag::kScreenHeight * kScale);
    };

    dag::platform::OverlayBridge overlay(layout);
    // Buttons hit-test/render against the whole window (so PhoneLandscape's
    // corner buttons land in its side margins, per the design doc); pickers
    // and crisp stay bound to the game's own 768x576 render (game_x/kGameW/
    // kGameH below), never the margins.
    double viewport_w = window_w;
    const double viewport_h = window_h;
    // 8.6.7 (Controls menu entry): re-derives window_w/game_x/viewport_w for
    // the other layout and resizes the live window, so the Controls entry
    // acts at once instead of only on the next launch's --layout flag.
    auto set_layout = [&](dag::input::OverlayLayout next) {
        layout = next;
        window_w = layout == dag::input::OverlayLayout::PhoneLandscape ? kGameH * 19.5 / 9.0 : kGameW;
        game_x = (window_w - kGameW) / 2.0;
        viewport_w = window_w;
        overlay.set_layout(layout);
#ifdef __EMSCRIPTEN__
        // The page sizes the canvas; only the letterboxed logical size changes.
        fit_to_window(renderer, window_w, window_h);
        return;
#endif
        // SDL_SetWindowSize alone left a stale, uninitialized margin on the
        // offscreen test driver (confirmed here: SDL_GetRenderOutputSize
        // reported the new size, but SDL_RenderClear's black never reached
        // it, and recreating just the renderer against the same window
        // didn't help either) -- the window itself has to be rebuilt at the
        // new size for a reliably fresh backing surface across backends.
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        window = SDL_CreateWindow("Dungeons of Daggorath", static_cast<int>(window_w),
                                  static_cast<int>(window_h), kWindowFlags);
        renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
        texture = renderer ? make_texture() : nullptr;
        // Matches main()'s own start-up checks just above: a null here is
        // unrecoverable (nothing left to render into), not worth a fallback.
        if (window == nullptr || renderer == nullptr || texture == nullptr) {
            std::cerr << "Controls: window/renderer/texture recreation failed\n";
            std::exit(1);
        }
        fit_to_window(renderer, window_w, window_h);
    };

    std::optional<dag::Game> held;
    held.emplace();
    // ADR-0009 (8.6.2): re-emplaced alongside `held` in restart_game() below,
    // since Shell holds a Game& and must never outlive the Game it wraps.
    std::optional<dag::shell::Shell> shell;
    shell.emplace(*held);
    dag::SoundMix mix;
    // One line of start-up state, for the player's terminal and for
    // tools/web/storage-test.mjs, which reads it from the browser console.
    const std::size_t mounted = mount_saves(*held, storage);
    std::cout << "dod: layout=" << (layout == dag::input::OverlayLayout::PhoneLandscape ? "phone" : "tablet")
              << " video=" << (prefs.crisp ? "crisp" : "pixel") << " saves=" << mounted << std::endl;
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
    std::vector<std::uint8_t> effect_carry;
    std::size_t heard = 0;
    std::size_t traced = 0;
    bool audio_level = false;
    std::uint64_t audio_jiffy = 0;
    const std::string message;   // no platform message row; OUTSTI owns the text page
    bool running = true;
    bool crisp_style = prefs.crisp;  // 8.6.3: F1 toggles pixel (default) vs crisp (ADR-0010)
    dag::shell::MenuState menu;  // 8.6.6: system menu sub-screen (src/shell/system_menu.hpp)
    int shown_row = 0;
    int shown_col = 0;
    int shown_dir = 0;
    bool have_shown = false;
    bool was_fainted = false;
    int faint_light = 0;   // OLIGHT: RLIGHT saved when HUPD30 starts
    int faint_magic = 0;   // MLIGHT at the same moment
    bool was_dead = false;
    std::size_t seen_motion = 0;
    std::size_t seen_restart = 0;
    auto reset_view = [&]() {
        heard = 0;
        traced = 0;
        audio_level = false;
        audio_jiffy = 0;
        heartbeat.clear();
        effect_carry.clear();
        mix = dag::SoundMix{};
        have_shown = false;
        was_fainted = false;
        was_dead = false;
        seen_motion = 0;
        seen_restart = 0;
    };
    // ZSAVEs this session could not store (persist_saves). The core keeps its
    // cassette across a death restart (D-18); a menu Restart builds a new
    // Game, so these are carried across by hand to keep the "kept for this
    // session" promise.
    std::set<std::string> unstored;
    auto restart_game = [&]() {
        std::vector<std::pair<std::string, std::string>> carried;
        for (const auto& name : unstored)
            if (const std::string* image = held->cassette_image(name)) carried.emplace_back(name, *image);
        held.emplace();
        shell.emplace(*held);
        mount_saves(*held, storage, unstored);  // D-20: a fresh start sees the stored saves too
        for (const auto& [name, image] : carried) held->insert_cassette_image(name, image);
        reset_view();
    };
    // HUPDAT DEATH halts the foreground while CLOCK runs; any key restarts
    // GAME in the core (D-18), which keeps the cassette and the trace. Only
    // the view's own memory of the last frame starts over.
    auto restarted = [&](const dag::Game& game) {
        bool any = false;
        const auto& trace = game.trace();
        for (; seen_restart < trace.size(); ++seen_restart)
            any = any || trace[seen_restart].kind == "RESTART";
        return any;
    };
    // 8.6.6: SystemMenu's tap and Esc's key share MenuState::back_out, so
    // the two input paths can't drift apart.
    auto pause_or_back_out = [&]() { menu.back_out(*shell); };
    // Shared by the S/L/X/Q/1-5/Y/N key handling below and the menu row
    // taps (8.6.8): one place decides what a MenuState::press effect does
    // to the running app.
    auto apply_menu_key = [&](dag::shell::MenuKey key, std::size_t slot = 0) {
        const auto effect = menu.press(*shell, key, slot);
        if (effect == dag::shell::MenuEffect::Restart)
            restart_game();  // re-emplaces held and shell (fresh, unpaused)
        else if (effect == dag::shell::MenuEffect::Quit)
            running = false;
    };
    // 8.6.7's Video/Controls toggles, shared the same way: the F1 key, the
    // menu's V/C keys and their row taps all call these two.
    // Both are remembered for the next launch (dag::platform::Prefs).
    auto save_prefs = [&]() {
        if (!remember_prefs) return;
        const dag::platform::Prefs now{crisp_style, layout};
        if (!storage.store_prefs(dag::platform::serialize_prefs(now)))
            dag::platform::report_storage_problem("SETTINGS NOT SAVED - storage full or blocked.");
    };
    auto toggle_video = [&]() {
        crisp_style = !crisp_style;
        save_prefs();
    };
    auto toggle_controls = [&]() {
        set_layout(layout == dag::input::OverlayLayout::PhoneLandscape
                       ? dag::input::OverlayLayout::Tablet4x3
                       : dag::input::OverlayLayout::PhoneLandscape);
        save_prefs();
    };
    // 8.6.8: the system menu's current screen as clickable rows, styled
    // like the touch overlay's own pickers (200 wide, 44-tall boxes) instead
    // of plain unboxed text -- both the tap hit-test and the draw call use
    // this same list, one-frame-lag-consistent with current_buttons/
    // picker_rects above. A heading row (e.g. "SAVE SLOT 1-5") has no
    // `activate` and draws without a box.
    auto menu_rows = [&]() -> std::vector<MenuRow> {
        std::vector<MenuRow> rows;
        if (!shell->paused()) return rows;
        constexpr double kRowW = 200, kRowH = 44;
        const double x = game_x + kGameW / 2 - kRowW / 2;
        double y = 60;
        auto add = [&](std::string label, std::function<void()> activate) {
            rows.push_back(MenuRow{std::move(label), dag::input::Rect{x, y, kRowW, kRowH},
                                   std::move(activate)});
            y += kRowH;
        };
        const auto pending = shell->pending();
        if (pending != dag::shell::ConfirmKind::None) {
            const char* question = pending == dag::shell::ConfirmKind::Restart   ? "RESTART?"
                                   : pending == dag::shell::ConfirmKind::Quit    ? "QUIT?"
                                                                                 : "OVERWRITE?";
            add(question, nullptr);
            add("Y  YES", [&] { apply_menu_key(dag::shell::MenuKey::Yes); });
            add("N  NO", [&] { apply_menu_key(dag::shell::MenuKey::No); });
        } else if (menu.screen() == dag::shell::MenuScreen::Top) {
            add("S  SAVE", [&] { apply_menu_key(dag::shell::MenuKey::Save); });
            add("L  LOAD", [&] { apply_menu_key(dag::shell::MenuKey::Load); });
            add("X  RESTART", [&] { apply_menu_key(dag::shell::MenuKey::Restart); });
            add("Q  QUIT", [&] { apply_menu_key(dag::shell::MenuKey::Quit); });
            add(std::string("V  VIDEO: ") + (crisp_style ? "CRISP" : "PIXEL"), toggle_video);
            add(std::string("C  CONTROLS: ") +
                   (layout == dag::input::OverlayLayout::PhoneLandscape ? "PHONE" : "TABLET"),
               toggle_controls);
        } else {
            add(menu.screen() == dag::shell::MenuScreen::ChooseSave ? "SAVE SLOT 1-5" : "LOAD SLOT 1-5",
               nullptr);
            const auto& slots = shell->slots();
            for (std::size_t i = 0; i < slots.size(); ++i) {
                const std::string label =
                    std::to_string(i + 1) + "  " + (slots[i].occupied() ? slots[i].name : "EMPTY");
                add(label, [&, i] { apply_menu_key(dag::shell::MenuKey::Slot, i); });
            }
        }
        return rows;
    };
    while (running) {
        // Computed before polling so a tap this frame hit-tests the same
        // rects drawn last frame (one-frame lag on a hand-state change is
        // harmless: GET/DROP/STOW/etc. are core-UNIMPLEMENTED today anyway,
        // docs/architecture/touch-input.md §5).
        const dag::input::OverlayState overlay_state = dag::platform::overlay_state_from(*held);
        const auto& current_buttons = overlay.buttons(viewport_w, viewport_h, overlay_state);
        const auto keyboard = overlay.keyboard_open()
                                  ? dag::input::keyboard_layout(layout, viewport_w, viewport_h)
                                  : dag::input::KeyboardLayout{};
        // The open picker's choices, placed beside the button that opened it.
        std::optional<dag::input::Rect> picker_anchor_rect;
        const auto picker_anchor_id = dag::input::picker_anchor(overlay.pending(), overlay.pending_right_hand());
        std::vector<dag::input::Choice> picker_rects;
        if (picker_anchor_id) {
            for (const auto& b : current_buttons)
                if (b.id == *picker_anchor_id) picker_anchor_rect = b.rect;
        }
        if (picker_anchor_rect)
            picker_rects = dag::input::place_choices(
                overlay.pending(),
                dag::input::picker_choices(overlay.pending(), overlay.pending_right_hand(), overlay_state),
                *picker_anchor_rect, viewport_w, viewport_h);
        const auto menu_row_list = menu_rows();
        if (next_shot < shots.size()) {
            if (shots_start_ms == 0) shots_start_ms = SDL_GetTicks();
            const std::uint64_t now_ms = SDL_GetTicks() - shots_start_ms;
            while (next_shot < shots.size() && shots[next_shot].ms <= now_ms) {
                const ShotStep& step = shots[next_shot++];
                SDL_Event injected{};
                if (step.verb == "key") {
                    injected.type = SDL_EVENT_KEY_DOWN;
                    injected.key.key = SDL_GetKeyFromName(step.arg.c_str());
                    SDL_PushEvent(&injected);
                } else if (step.verb == "tap") {
                    injected.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    injected.button.button = SDL_BUTTON_LEFT;
                    injected.button.x = static_cast<float>(step.x);
                    injected.button.y = static_cast<float>(step.y);
                    SDL_PushEvent(&injected);
                } else if (step.verb == "shot") {
                    g_shot_path = step.arg;
                } else if (step.verb == "quit") {
                    running = false;
                }
            }
        }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
#ifdef __EMSCRIPTEN__
            SDL_ConvertEventToRenderCoordinates(renderer, &event);
#endif
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                const double mx = event.button.x;
                const double my = event.button.y;
                // ADR-0009 §6: SystemMenu is the one control that pauses,
                // checked ahead of the overlay bridge (which treats it as
                // not its job, 8.6.1) so it works whether or not a picker
                // is open.
                if (dag::input::hit_test(current_buttons, mx, my) ==
                    dag::input::ButtonId::SystemMenu) {
                    pause_or_back_out();
                } else if (shell->paused()) {
                    // 8.6.8: each row has its own hit rect (see menu_rows
                    // above); a tap off every row does nothing, same as a
                    // key this screen doesn't recognise.
                    for (const auto& row : menu_row_list) {
                        if (!row.activate || !row.rect.contains(mx, my)) continue;
                        row.activate();
                        break;
                    }
                } else if (overlay.keyboard_open()) {
                    // Taps off the keys do nothing, except the two A buttons the
                    // Incant board keeps up; ✕ closes.
                    bool on_key = false;
                    for (const auto& k : keyboard.keys)
                        if (k.rect.contains(mx, my)) {
                            overlay.press_key(k.label, *held);
                            on_key = true;
                            break;
                        }
                    if (!on_key) overlay.handle_attack_tap(mx, my, *held);
                } else if (overlay.picker_open()) {
                    bool chose = false;
                    for (const auto& [choice, rect] : picker_rects) {
                        if (!rect.contains(mx, my)) continue;
                        chose = true;
                        // G, P and I open the next picker or the keyboard;
                        // anything else finishes the line.
                        overlay.resolve_choice(choice, *held);
                        break;
                    }
                    // A tap outside every choice closes the picker unchanged,
                    // so an empty or unwanted picker is never a dead end.
                    if (!chose) overlay.cancel_picker();
                } else {
                    overlay.handle_tap(mx, my, *held);
                }
                continue;
            }
            if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) continue;
            const SDL_Keycode key = event.key.key;
            if (key == SDLK_ESCAPE) {
                pause_or_back_out();
                continue;
            }
            if (key == SDLK_F1) {  // 8.6.3: pixel/crisp render-style toggle, harmless while paused
                toggle_video();
                continue;
            }
            if (shell->paused()) {
                // 8.6.7/8.6.8: Video/Controls are presentation-only (render
                // style, window layout), not Shell/MenuState's concern, so
                // they're handled here directly rather than through
                // MenuState::press -- the same way F1's crisp toggle already
                // works outside the menu entirely. `toggle_video`/
                // `toggle_controls` are the same two functions a V/C row tap
                // calls (menu_rows above), so the key and the tap can't drift.
                if (menu.screen() == dag::shell::MenuScreen::Top &&
                    shell->pending() == dag::shell::ConfirmKind::None) {
                    if (key == SDLK_V) {
                        toggle_video();
                        continue;
                    }
                    if (key == SDLK_C) {
                        toggle_controls();
                        continue;
                    }
                }
                // 8.6.6: Resume/Save/Load/Restart/Quit (ADR-0009 §6).
                // `apply_menu_key` is the same function a Save/Load/.../slot
                // row tap calls (menu_rows above).
                std::optional<dag::shell::MenuKey> menu_key;
                std::size_t slot = 0;
                if (key == SDLK_S) menu_key = dag::shell::MenuKey::Save;
                else if (key == SDLK_L) menu_key = dag::shell::MenuKey::Load;
                else if (key == SDLK_X) menu_key = dag::shell::MenuKey::Restart;
                else if (key == SDLK_Q) menu_key = dag::shell::MenuKey::Quit;
                else if (key == SDLK_Y) menu_key = dag::shell::MenuKey::Yes;
                else if (key == SDLK_N) menu_key = dag::shell::MenuKey::No;
                else if (key >= SDLK_1 && key <= SDLK_5) {
                    menu_key = dag::shell::MenuKey::Slot;
                    slot = static_cast<std::size_t>(key - SDLK_1);
                }
                if (menu_key) apply_menu_key(*menu_key, slot);
                continue;  // the system menu owns input while open
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
        // shell->tick is a no-op while paused (D-16: no jiffy owed for
        // paused wall time), replacing the direct advance_jiffies call --
        // ADR-0009's pause-invariance test (tests/shell/shell_tests.cpp)
        // already proves this substitution changes nothing about the core
        // trace for an unpaused run.
        if (steps > 0) shell->tick(static_cast<std::uint64_t>(steps));
        persist_saves(game, traced, storage, unstored);
        if (restarted(game)) {
            have_shown = false;
            was_fainted = false;
            was_dead = false;
        }
        auto snap = dag::snapshot_from(game);
        bool stepped = false;
        bool turned = false;
        std::uint32_t turn_loops = 0;
        int step_relative = -1;
        const auto& motion = game.events();
        while (seen_motion < motion.size()) {
            const dag::CoreEvent& ev = motion[seen_motion++];
            if (ev.kind != dag::CoreEventKind::Block) continue;
            if (ev.block == dag::BlockKind::MoveAnimation) {
                stepped = true;
                step_relative = ev.step_relative;
            }
            if (ev.block == dag::BlockKind::TurnAnimation) {
                turned = true;
                turn_loops = ev.loop_count;
            }
        }
        int half_scale = 0;
        int sidestep_bar = -1;
        if (have_shown && stepped && snap.mode == 0) {
            const int dr = game.player().row - shown_row;
            const int dc = game.player().col - shown_col;
            static constexpr int forward_row[4] = {-1, 0, 1, 0};
            static constexpr int forward_col[4] = {0, 1, 0, -1};
            const int facing = shown_dir & 3;
            // PMOVE forward draws HLFSCL on the cell being left. Backward uses BAKSCL.
            if (dr == forward_row[facing] && dc == forward_col[facing]) half_scale = 1;
            else if (dr == -forward_row[facing] && dc == -forward_col[facing]) half_scale = 2;
            else if (dr == forward_row[(facing + 3) & 3] && dc == forward_col[(facing + 3) & 3])
                sidestep_bar = 8;    // MOVE LEFT then LRTURN
            else if (dr == forward_row[(facing + 1) & 3] && dc == forward_col[(facing + 1) & 3])
                sidestep_bar = 248;  // MOVE RIGHT then RLTURN
            else if (dr == 0 && dc == 0 && (step_relative == 0 || step_relative == 2))
                // PMOVE draws the HLFSTP/BAKSTP view before PSTEP, so a step
                // into a wall still shows the half-step, then THUD and the
                // standing view (PTURN.ASM:156-172, source-proven).
                half_scale = step_relative == 0 ? 1 : 2;
        }
        // MAPPER draws over the whole screen; HEARTF and the prompt are off in
        // map mode (PUSE.ASM USC210, HUMAN.ASM HMAN70). Hiding the text bands is
        // inferred (phase-7 reconciliation; capture C-20).
        const bool map_up = game.display_mode() == dag::DisplayMode::Mapper;
        auto frame = map_up ? dag::rasterize_map(dag::map_snapshot_from(game)) : dag::rasterize(snap);
        if (game.preparing()) {
            dag::paint_prepare(frame.data(), dag::kScreenWidth);   // MISC.ASM PREPAX
        } else if (game.display_mode() == dag::DisplayMode::Examine)   // PEXAM.ASM EXAMIN over the viewport
            dag::paint_examine(frame.data(), dag::kScreenWidth,
                               dag::project_examine(dag::examine_snapshot_from(game)));
        dag::TextSnapshot chrome;
        auto hand = [&](int index) -> std::optional<dag::Ocb> {
            if (index < 0 || static_cast<std::size_t>(index) >= game.objects().size()) return {};
            return game.objects()[static_cast<std::size_t>(index)];
        };
        chrome.left = hand(game.player().left_hand);
        chrome.right = hand(game.player().right_hand);
        chrome.line = game.line_buffer();
        chrome.has_page = true;
        chrome.page = game.primary_text();
        if (game.heart().heartf != 0) {
            chrome.heart = game.heart().hearts != 0 ? dag::HeartGlyph::Large
                                                    : dag::HeartGlyph::Small;
        }
        const auto& events = game.events();
        while (heard < events.size()) {
            const dag::CoreEvent& ev = events[heard++];
            // OUTSTI is already on the primary text page.
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
        const std::string command_override;
        if (!map_up)
            dag::paint_text_bands(frame.data(), dag::kScreenWidth, chrome, message, command_override);
        const bool faint_now = game.player().fainted || game.player().dead;
        if (faint_now && !was_fainted) {
            // HUPDAT HUPD30: each SYNC lowers MLIGHT and RLIGHT and redraws until
            // RLIGHT reaches -8, then ZFLOP blanks the screen. Presentation-only
            // pacing (D-14); the core spends no simulated time here. C-17 on the
            // ROM: 5 jiffies per step, and WIZIX starts 2 jiffies after the last step.
            auto dark = chrome;
            dark.has_page = true;
            dark.page = {};
            dark.line.clear();
            auto fading = snap;
            faint_light = snap.regular_light;
            faint_magic = snap.magic_light;
            // HUPD30 lowers MLIGHT, draws, then lowers RLIGHT and loops while
            // RLIGHT > -8.
            do {
                --fading.magic_light;
                auto step = dag::rasterize(fading);
                dag::paint_text_bands(step.data(), dag::kScreenWidth, dark, message, "");
                present_frame(renderer, texture, step, game.polarity_level(), game_x, kGameW, kGameH);
                SDL_Delay(83);
                --fading.regular_light;
            } while (fading.regular_light > -8);
            SDL_Delay(33);
        }
        if (game.player().dead && !was_dead) {
            auto dark = chrome;
            dark.has_page = true;
            dark.page = {};
            dark.line.clear();
            {
                // DEATH: WIZIX fades the wizard in, VCTFAD 32 down to 0 in steps of
                // two, then an explosion (MISC.ASM WIZI10, WIZI20). C-17 on the
                // ROM: 18 jiffies per step, about 4.8 s in all.
                for (int fade = 32; fade >= 0; fade -= 2) {
                    auto step = dag::rasterize_wizard(static_cast<std::uint8_t>(fade));
                    dag::paint_text_bands(step.data(), dag::kScreenWidth, dark, message, "");
                    present_frame(renderer, texture, step, game.polarity_level(), game_x, kGameW, kGameH);
                    SDL_Delay(300);
                }
                std::uint16_t noise = 1;
                dag::start_dac(effect_carry, dag::samples_for_cue(
                    static_cast<std::uint8_t>(dag::SoundCue::EXP1), 0xFF, noise));
            }
        }
        if (!faint_now && was_fainted) {
            // HUPD42: on waking, redraw and raise MLIGHT and RLIGHT one step per
            // pass from -8 back up to the level saved in OLIGHT. Presentation
            // only (D-14); 5 jiffies per step on the ROM (C-18).
            auto dark = chrome;
            dark.has_page = true;
            dark.page = {};
            dark.line.clear();
            auto rising = snap;
            // HUPD30 leaves RLIGHT at -8 (lower if it started there) and MLIGHT
            // down by the same count; HUPD42 draws, raises both, and loops while
            // RLIGHT <= OLIGHT, so it always draws at least one frame.
            const int down = faint_light > -8 ? faint_light + 8 : 1;
            rising.regular_light = faint_light - down;
            rising.magic_light = faint_magic - down;
            do {
                auto step = dag::rasterize(rising);
                dag::paint_text_bands(step.data(), dag::kScreenWidth, dark, message, "");
                present_frame(renderer, texture, step, game.polarity_level(), game_x, kGameW, kGameH);
                SDL_Delay(83);
                ++rising.magic_light;
                ++rising.regular_light;
            } while (rising.regular_light <= faint_light);
        }
        was_fainted = faint_now;
        was_dead = game.player().dead;
        if (game.player().dead) {
            frame = dag::rasterize_wizard(0);
            dag::paint_text_bands(frame.data(), dag::kScreenWidth, chrome, message, command_override);
        } else if (game.player().fainted) {
            frame.fill(0);
            dag::paint_text_bands(frame.data(), dag::kScreenWidth, chrome, message, command_override);
        }
        if (half_scale != 0) {
            // PMOVE draws HLFSCL or BAKSCL on the cell being left, then the
            // standing view of the cell entered.
            auto leaving = snap;
            leaving.row = shown_row;
            leaving.col = shown_col;
            leaving.dir = shown_dir;
            leaving.scale = half_scale;
            auto midway = dag::rasterize(leaving);
            dag::paint_text_bands(midway.data(), dag::kScreenWidth, chrome, message, command_override);
            present_frame(renderer, texture, midway, game.polarity_level(), game_x, kGameW, kGameH);
            SDL_Delay(12);
        } else if (sidestep_bar >= 0 ||
                   (turned && have_shown && snap.mode == 0 &&
                    game.player().row == shown_row && game.player().col == shown_col)) {
            int bar = sidestep_bar;
            if (bar < 0) {
                const int delta = (static_cast<int>(game.player().dir) - shown_dir) & 3;
                // Left sweeps in from x=8. Right and about-face start at x=248.
                bar = delta == 3 ? 8 : 248;
            }
            // TURN AROUND sweeps RLTURN twice (PTURN.ASM PTUR20 -> PTUR22).
            const int sweeps = sidestep_bar < 0 && turn_loops >= 8 ? static_cast<int>(turn_loops / 8) : 1;
            for (int sweep = 0; sweep < sweeps; ++sweep) {
                auto wipe = turn_wipe(bar);
                dag::paint_text_bands(wipe.data(), dag::kScreenWidth, chrome, message,
                                      command_override);
                present_frame(renderer, texture, wipe, game.polarity_level(), game_x, kGameW, kGameH);
                SDL_Delay(12);
            }
        }
        present_frame(renderer, texture, frame, game.polarity_level(), game_x, kGameW, kGameH,
                      [&](SDL_Renderer* r) {
            // 8.6.3: crisp overdraws the already-blitted pixel bitmap's
            // viewport band with device-scaled vector geometry (ADR-0010).
            // Skipped for death/faint/menu frames, which use their own
            // presentation-only fades (D-14) this pass does not reproduce
            // in crisp form, and for PREPARE! and EXAMINE, which replace
            // the viewport with text crisp has no vector form of.
            if (crisp_style && !game.player().dead &&
                !game.player().fainted && !game.preparing() &&
                game.display_mode() != dag::DisplayMode::Examine) {
                // NLVL50 polarity, as present_frame's apply_vdginv gives the pixel style
                // (source-proven for the view; the map screen is [INF], raster.hpp).
                const std::uint8_t ink = dag::vdginv(game.polarity_level()) ? 0 : 255;
                const std::uint8_t paper = static_cast<std::uint8_t>(255 - ink);
                SDL_SetRenderDrawColor(r, paper, paper, paper, 255);
                const SDL_FRect viewport_rect{static_cast<float>(game_x), 0,
                                              static_cast<float>(kGameW),
                                              static_cast<float>(dag::kViewportScanlineEnd * kScale)};
                SDL_RenderFillRect(r, &viewport_rect);
                if (map_up) draw_crisp_map(r, dag::map_snapshot_from(game), kScale, game_x, ink);
                else draw_crisp_view(r, dag::project(snap), kScale, game_x, ink, paper);
            }
            if (shell->paused()) {
                // 8.6.8: styled and hit-tested like the touch overlay's own
                // pickers (Picker/Popup boards) -- a black-filled,
                // white-bordered 200x44 box per row, instead of 8.6.6's
                // plain unboxed text. A heading row (no `activate`) draws
                // without a box, same width so it still lines up.
                for (const auto& row : menu_row_list) {
                    const SDL_FRect box{static_cast<float>(row.rect.x), static_cast<float>(row.rect.y),
                                        static_cast<float>(row.rect.w), static_cast<float>(row.rect.h)};
                    if (row.activate) {
                        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                        SDL_RenderFillRect(r, &box);
                        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                        SDL_RenderRect(r, &box);
                        draw_text_line(r, row.rect.x + 12, row.rect.y + row.rect.h / 2 - 6, row.label, 1.5f);
                    } else {
                        draw_text_line(r, row.rect.x + 12, row.rect.y + row.rect.h / 2 - 6, row.label, 1.5f);
                    }
                }
                for (const auto& button : current_buttons)
                    if (button.id == dag::input::ButtonId::SystemMenu)
                    {
                        draw_button(r, button.rect, 0);
                        draw_icon(r, button.rect, button.id);
                    }
                return;
            }
            if (overlay.keyboard_open()) {
                // Incant board: the text box echoes the line being typed.
                const auto& box = keyboard.text_box;
                const SDL_FRect bf{static_cast<float>(box.x), static_cast<float>(box.y),
                                   static_cast<float>(box.w), static_cast<float>(box.h)};
                SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                SDL_RenderFillRect(r, &bf);
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                SDL_RenderRect(r, &bf);
                const std::string shown = ".I " + overlay.typed() + "_";
                draw_text_line(r, box.x + 12, box.y + box.h / 2 - 8, shown, 2.0f);
                for (const auto& k : keyboard.keys) {
                    const SDL_FRect kf{static_cast<float>(k.rect.x), static_cast<float>(k.rect.y),
                                       static_cast<float>(k.rect.w), static_cast<float>(k.rect.h)};
                    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                    SDL_RenderFillRect(r, &kf);
                    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                    SDL_RenderRect(r, &kf);
                    if (k.label.size() == 1) {
                        draw_glyph(r, k.rect.x + k.rect.w / 2, k.rect.y + k.rect.h / 2, k.label[0]);
                    } else {
                        const std::string mark = k.label == "BACK" ? "<X" : k.label == "ENTER" ? "OK" : "X";
                        draw_text_centered(r, k.rect, mark, mark.size() > 2 ? 1.5f : 2.0f);
                    }
                }
                for (const auto& button : current_buttons)  // Incant board keeps A up
                    if (button.id == dag::input::ButtonId::AttackLeft ||
                        button.id == dag::input::ButtonId::AttackRight)
                        draw_button(r, button.rect, 'A');
                return;
            }
            // The buttons stay up under an open picker; the one that opened
            // it is drawn pressed (Picker, Popup boards).
            for (const auto& button : current_buttons) {
                const bool pressed = picker_anchor_id && button.id == *picker_anchor_id;
                draw_button(r, button.rect, button_label(button.id), pressed);
                if (button_label(button.id) == 0) {
                    const std::uint8_t ink = pressed ? 0 : 255;
                    SDL_SetRenderDrawColor(r, ink, ink, ink, 255);
                    draw_icon(r, button.rect, button.id);
                }
            }
            for (const auto& [choice, rect] : picker_rects) {
                const SDL_FRect panel{static_cast<float>(rect.x), static_cast<float>(rect.y),
                                      static_cast<float>(rect.w), static_cast<float>(rect.h)};
                SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                SDL_RenderFillRect(r, &panel);
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                SDL_RenderRect(r, &panel);
                const bool named = overlay.pending() == dag::input::PendingKind::FloorPicker ||
                                   overlay.pending() == dag::input::PendingKind::PackPicker;
                if (named) draw_text_line(r, rect.x + 12, rect.y + rect.h / 2 - 6, choice, 1.5f);
                else draw_glyph(r, rect.x + rect.w / 2, rect.y + rect.h / 2, choice[0]);
                if (overlay.pending() == dag::input::PendingKind::HandMenu) {
                    // Popup board: each letter captioned with its verb.
                    const std::string word = dag::input::hand_verb_caption(choice);
                    draw_text_line(r, rect.x + (rect.w - 8.0 * word.size()) / 2, rect.y + rect.h + 4, word);
                }
            }
        });
        shown_row = game.player().row;
        shown_col = game.player().col;
        shown_dir = static_cast<int>(game.player().dir);
        have_shown = true;
        mix.consume_events(game.events());
        dag::start_dac(effect_carry, mix.pending());
        mix.clear();
        dag::overlay_dac(heartbeat, effect_carry);
        if (audio != nullptr && !heartbeat.empty()) {
            SDL_PutAudioStreamData(audio, heartbeat.data(), static_cast<int>(heartbeat.size()));
            heartbeat.clear();
        }
        SDL_Delay(1);
    }
    if (audio != nullptr) SDL_DestroyAudioStream(audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
