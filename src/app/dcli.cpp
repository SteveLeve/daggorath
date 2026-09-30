// dcli — headless trace harness for the Phase 0b reference slice.
//
//   dcli --script FILE [--jiffies N] [--second S] [--dump-maze FILE] [--trace FILE]
// Omitting --second is Original Mode: 377 build interrupts, SECOND = 6.
//   dcli --maze-hashes            (prints cleared-cell counts and RNG spin states)
//
// Emits a tab-separated trace: jiffy, clock counters, event kind, detail.
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "daggorath/examine.hpp"
#include "daggorath/game.hpp"
#include "daggorath/mapper.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/render_state.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/text.hpp"

namespace {

int usage() {
    std::cerr << "usage: dcli --script FILE [--jiffies N] [--second S]\n"
                 "            [--dump-maze FILE] [--trace FILE] [--present] [--bitmap FILE]\n"
                 "            [--events] [--present-map] [--present-text] [--level N]\n"
                 "            [--poke JIFFY:FIELD:VALUE ...]\n"
                 "       dcli --maze-summary\n"
                 "       --second sets a harness SECOND and skips the 377-interrupt\n"
                 "       Original Mode build clock.\n"
                 "       --poke sets player damage, power, position, the worn\n"
                 "       torch's timer, or creature slot 6's position at a jiffy\n"
                 "       boundary (FIELD is damage, power, position, torch, or\n"
                 "       creature6; position/creature6's VALUE is row*256+col),\n"
                 "       mirroring tools/rom/capture.lua's DOD_POKE for a\n"
                 "       harness-modified capture. Repeatable.\n";
    return 2;
}

enum class PokeField { Damage, Power, Position, Torch, Creature6 };

struct Poke {
    std::uint64_t jiffy;
    PokeField field;
    std::uint16_t value;
};

std::vector<Poke> parse_pokes(const std::vector<std::string>& specs, std::string& error) {
    std::vector<Poke> pokes;
    for (const std::string& spec : specs) {
        const std::size_t first = spec.find(':');
        const std::size_t second = spec.find(':', first == std::string::npos ? first : first + 1);
        if (first == std::string::npos || second == std::string::npos) {
            error = "bad --poke '" + spec + "', want JIFFY:FIELD:VALUE";
            return {};
        }
        const std::string jiffy_str = spec.substr(0, first);
        const std::string field = spec.substr(first + 1, second - first - 1);
        const std::string value_str = spec.substr(second + 1);
        PokeField pf;
        if (field == "damage") pf = PokeField::Damage;
        else if (field == "power") pf = PokeField::Power;
        else if (field == "position") pf = PokeField::Position;
        else if (field == "torch") pf = PokeField::Torch;
        else if (field == "creature6") pf = PokeField::Creature6;
        else {
            error = "bad --poke field '" + field +
                    "', want damage, power, position, torch, or creature6";
            return {};
        }
        Poke p;
        p.jiffy = std::strtoull(jiffy_str.c_str(), nullptr, 10);
        p.field = pf;
        p.value = static_cast<std::uint16_t>(std::strtoul(value_str.c_str(), nullptr, 10));
        pokes.push_back(p);
    }
    std::sort(pokes.begin(), pokes.end(),
              [](const Poke& a, const Poke& b) { return a.jiffy < b.jiffy; });
    return pokes;
}

int maze_summary() {
    for (int level = 0; level < 5; ++level) {
        const dag::GeneratedLevel g = dag::generate_level(level, 1);
        int cleared = 0;
        for (const std::uint8_t b : g.maze.bytes()) {
            if (b != 0xFF) ++cleared;
        }
        std::cout << "level " << level << "  cleared=" << cleared
                  << "  rng_before_spin=";
        for (const std::uint8_t b : g.rng_before_spin) {
            std::cout << std::hex << (b < 16 ? "0" : "") << static_cast<int>(b);
        }
        std::cout << "  rng_after_spin=";
        for (const std::uint8_t b : g.rng_after_spin) {
            std::cout << (b < 16 ? "0" : "") << static_cast<int>(b);
        }
        std::cout << std::dec << "  spins=" << g.spin_count << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    std::string script_path, maze_out, trace_out, bitmap_out;
    std::uint64_t jiffies = 600;
    bool have_second = false;
    bool present = false;
    bool events = false;
    bool present_map = false;
    bool present_text = false;
    int second = 0;
    int level = 0;
    bool frozen = false;
    std::vector<std::string> poke_specs;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::exit(usage()); }
            return argv[++i];
        };
        if (a == "--maze-summary") return maze_summary();
        else if (a == "--script") script_path = next();
        else if (a == "--jiffies") jiffies = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--second") {
            second = std::atoi(next().c_str());
            have_second = true;
        }
        else if (a == "--level") level = std::atoi(next().c_str());
        else if (a == "--frozen") frozen = true;
        else if (a == "--dump-maze") maze_out = next();
        else if (a == "--trace") trace_out = next();
        else if (a == "--present") present = true;
        else if (a == "--bitmap") bitmap_out = next();
        else if (a == "--events") events = true;
        else if (a == "--present-map") present_map = true;
        else if (a == "--present-text") present_text = true;
        else if (a == "--poke") poke_specs.push_back(next());
        else return usage();
    }

    std::string poke_error;
    const std::vector<Poke> pokes = parse_pokes(poke_specs, poke_error);
    if (!poke_error.empty()) {
        std::cerr << poke_error << "\n";
        return 1;
    }

    std::optional<dag::Game> held;
    if (have_second) held.emplace(static_cast<std::uint8_t>(second), level);
    else held.emplace();
    dag::Game& game = *held;
    if (frozen) game.set_frozen(true);

    if (!script_path.empty()) {
        std::ifstream in(script_path);
        if (!in) {
            std::cerr << "cannot open " << script_path << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        std::string error;
        const std::string body = ss.str();
        auto keys = dag::parse_script(body, error);
        if (!error.empty()) {
            std::cerr << "script error: " << error << "\n";
            return 1;
        }
        game.load_script(std::move(keys));
    }

    std::uint64_t done = 0;
    for (const Poke& p : pokes) {
        if (p.jiffy > jiffies) continue;
        if (p.jiffy > done) {
            game.advance_jiffies(p.jiffy - done);
            done = p.jiffy;
        }
        switch (p.field) {
            case PokeField::Power: game.set_player_power(p.value); break;
            case PokeField::Damage: game.set_player_damage(p.value); break;
            case PokeField::Position:
                game.place_player(p.value >> 8, p.value & 0xFF);
                break;
            case PokeField::Torch:
                game.set_torch_timer(static_cast<std::uint8_t>(p.value));
                break;
            case PokeField::Creature6:
                game.place_creature(6, p.value >> 8, p.value & 0xFF);
                break;
        }
    }
    if (jiffies > done) game.advance_jiffies(jiffies - done);

    std::ostream* out = &std::cout;
    std::ofstream file;
    if (!trace_out.empty()) {
        file.open(trace_out);
        if (!file) { std::cerr << "cannot write " << trace_out << "\n"; return 1; }
        out = &file;
    }
    *out << "# jiffy\tclock\tevent\tdetail\n";
    if (events) {
        *out << "# CoreEvent stream (ADR-0004)\n";
        for (const auto& e : game.events()) *out << e.to_line() << "\n";
    } else {
        for (const auto& e : game.trace()) *out << e.to_line() << "\n";
    }
    *out << "# final\trow=" << game.player().row << "\tcol=" << game.player().col
         << "\tdir=" << static_cast<int>(game.player().dir)
         << "\tdamage=" << game.player().damage << "\n";

    if (!maze_out.empty()) {
        std::ofstream mf(maze_out, std::ios::binary);
        const auto& b = game.maze().bytes();
        mf.write(reinterpret_cast<const char*>(b.data()),
                 static_cast<std::streamsize>(b.size()));
    }
    if (present) {
        std::cout << dag::project(dag::snapshot_from(game)).to_text();
    }
    if (!bitmap_out.empty()) {
        std::ofstream image(bitmap_out);
        if (!image) {
            std::cerr << "cannot write " << bitmap_out << "\n";
            return 1;
        }
        image << dag::bitmap_pbm(dag::rasterize(dag::snapshot_from(game)));
    }
    if (present_map) {
        const dag::MapSnapshot snap = dag::map_snapshot_from(game);
        std::cout << dag::project_map(snap).text;
    }
    if (present_text) {
        const dag::ExamineSnapshot exam = dag::examine_snapshot_from(game);
        dag::TextSnapshot text;
        const auto& p = game.player();
        if (p.left_hand >= 0)
            text.left = game.objects()[static_cast<std::size_t>(p.left_hand)];
        if (p.right_hand >= 0)
            text.right = game.objects()[static_cast<std::size_t>(p.right_hand)];
        text.heart = game.display_mode() == dag::DisplayMode::Mapper
                         ? dag::HeartGlyph::Off
                         : dag::HeartGlyph::Small;
        text.line = game.line_buffer();
        std::cout << dag::project_examine(exam).text;
        std::cout << dag::project_text(text).text;
    }
    return 0;
}
