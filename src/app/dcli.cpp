// dcli — headless trace harness for the Phase 0b reference slice.
//
//   dcli --script FILE [--jiffies N] [--second S] [--dump-maze FILE] [--trace FILE]
// Omitting --second is Original Mode: 377 build interrupts, SECOND = 6.
//   dcli --maze-hashes            (prints cleared-cell counts and RNG spin states)
//
// Emits a tab-separated trace: jiffy, clock counters, event kind, detail.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "daggorath/game.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/render_state.hpp"
#include "daggorath/snapshot.hpp"

namespace {

int usage() {
    std::cerr << "usage: dcli --script FILE [--jiffies N] [--second S]\n"
                 "            [--dump-maze FILE] [--trace FILE] [--present] [--bitmap FILE] [--level N]\n"
                 "       dcli --maze-summary\n"
                 "       --second sets a harness SECOND and skips the 377-interrupt\n"
                 "       Original Mode build clock.\n";
    return 2;
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
    int second = 0;
    int level = 0;

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
        else if (a == "--dump-maze") maze_out = next();
        else if (a == "--trace") trace_out = next();
        else if (a == "--present") present = true;
        else if (a == "--bitmap") bitmap_out = next();
        else return usage();
    }

    std::optional<dag::Game> held;
    if (have_second) held.emplace(static_cast<std::uint8_t>(second), level);
    else held.emplace();
    dag::Game& game = *held;

    if (!script_path.empty()) {
        std::ifstream in(script_path);
        if (!in) {
            std::cerr << "cannot open " << script_path << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        std::string error;
        auto keys = dag::parse_script(ss.str(), error);
        if (!error.empty()) {
            std::cerr << "script error: " << error << "\n";
            return 1;
        }
        game.load_script(std::move(keys));
    }

    game.advance_jiffies(jiffies);

    std::ostream* out = &std::cout;
    std::ofstream file;
    if (!trace_out.empty()) {
        file.open(trace_out);
        if (!file) { std::cerr << "cannot write " << trace_out << "\n"; return 1; }
        out = &file;
    }
    *out << "# jiffy\tclock\tevent\tdetail\n";
    for (const auto& e : game.trace()) *out << e.to_line() << "\n";
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
    return 0;
}
