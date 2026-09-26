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
#include "daggorath/examine.hpp"
#include "daggorath/mapper.hpp"
#include "daggorath/render_state.hpp"
#include "daggorath/text.hpp"

namespace {

int usage() {
    std::cerr << "usage: dcli --script FILE [--jiffies N] [--second S]\n"
                 "            [--dump-maze FILE] [--trace FILE] [--present]\n"
                 "            [--present-map] [--present-text]\n"
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
    std::string script_path, maze_out, trace_out;
    std::uint64_t jiffies = 600;
    bool have_second = false;
    bool present = false;
    bool present_map = false;
    bool present_text = false;
    int second = 0;

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
        else if (a == "--dump-maze") maze_out = next();
        else if (a == "--trace") trace_out = next();
        else if (a == "--present") present = true;
        else if (a == "--present-map") present_map = true;
        else if (a == "--present-text") present_text = true;
        else return usage();
    }

    std::optional<dag::Game> held;
    if (have_second) held.emplace(static_cast<std::uint8_t>(second), 0);
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
        dag::ViewSnapshot view;
        view.row = game.player().row;
        view.col = game.player().col;
        view.dir = static_cast<int>(game.player().dir);
        view.regular_light = game.player().regular_light;
        view.magic_light = game.player().magic_light;
        view.mode = static_cast<int>(game.display_mode());
        view.map_features = game.player().map_features;
        int r = view.row;
        int c = view.col;
        static constexpr int dr[4] = {-1, 0, 1, 0};
        static constexpr int dc[4] = {0, 1, 0, -1};
        for (int i = 0; i < 5; ++i) {
            if (r < 0 || c < 0 || r >= 32 || c >= 32) view.ahead[i] = 0xFF;
            else view.ahead[i] = game.maze().at(r, c);
            r += dr[view.dir & 3];
            c += dc[view.dir & 3];
        }
        std::cout << dag::project(view).to_text();
    }
    if (present_map) {
        dag::MapSnapshot snap;
        snap.cells = game.maze().bytes().data();
        snap.player_row = game.player().row;
        snap.player_col = game.player().col;
        snap.features = game.player().map_features;
        for (const auto& o : game.objects()) {
            if (o.owner == 0 && o.level == game.level_index())
                snap.objects.push_back({o.row, o.col});
        }
        for (const auto& c : game.creatures()) {
            if (c.in_use) snap.creatures.push_back({c.row, c.col});
        }
        for (int r = 0; r < 32; ++r) {
            for (int col = 0; col < 32; ++col) {
                if (dag::vfind(game.level_index(), r, col) >= 0)
                    snap.verticals.push_back({r, col});
            }
        }
        std::cout << dag::project_map(snap).text;
    }
    if (present_text) {
        dag::ExamineSnapshot exam;
        exam.creature = false;
        for (const auto& c : game.creatures()) {
            if (c.in_use && c.row == game.player().row && c.col == game.player().col)
                exam.creature = true;
        }
        for (const auto& o : game.objects()) {
            if (o.owner == 0 && o.level == game.level_index() &&
                o.row == game.player().row && o.col == game.player().col)
                exam.floor.push_back(dag::object_name(o));
        }
        int bag_i = 0;
        for (int i = game.player().bag_head; i >= 0;
             i = game.objects()[static_cast<std::size_t>(i)].next) {
            exam.bag.push_back(
                dag::object_name(game.objects()[static_cast<std::size_t>(i)]));
            if (i == game.player().torch) exam.torch_index = bag_i;
            ++bag_i;
        }
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
