// Conformance tests for the Phase 0b reference slice.
//
// Expectations come from the fixtures under docs/archaeology/phase-0b/fixtures,
// which were produced by an independent Python transliteration of the same
// assembly routines (tools/extract_fixtures.py). A test that agrees only
// because both sides share code would be worthless, so the maze test compares
// against the fixture's raw bytes on disk rather than recomputing them.
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "daggorath/game.hpp"
#include "daggorath/maze.hpp"
#include "daggorath/parser.hpp"
#include "daggorath/rng.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

std::string fixture_dir() {
#ifdef DAG_FIXTURE_DIR
    return DAG_FIXTURE_DIR;
#else
    return "fixtures";
#endif
}

// ---------------------------------------------------------------- RNG
void test_rng() {
    // Cross-check every vector in fixtures/rng-vectors.txt, which the Python
    // transliteration produced from RANDOM.ASM.
    std::ifstream in(fixture_dir() + "/rng-vectors.txt");
    check(static_cast<bool>(in), "fixtures/rng-vectors.txt is readable");
    std::string line;
    int vectors = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> fields;
        std::string cur;
        for (const char ch : line) {
            if (ch == ' ') {
                if (!cur.empty()) { fields.push_back(cur); cur.clear(); }
            } else if (ch != ':') {
                cur.push_back(ch);
            } else {
                if (!cur.empty()) { fields.push_back(cur); cur.clear(); }
                fields.push_back(":");
            }
        }
        if (!cur.empty()) fields.push_back(cur);

        auto hex = [](const std::string& s) {
            return static_cast<std::uint8_t>(std::strtoul(s.c_str(), nullptr, 16));
        };
        // seed(3) : outputs(16) : final seed(3), with two ":" markers
        if (fields.size() != 3 + 1 + 16 + 1 + 3) continue;
        dag::Rng rng({hex(fields[0]), hex(fields[1]), hex(fields[2])});
        bool ok = true;
        std::string detail;
        for (int i = 0; i < 16; ++i) {
            const std::uint8_t want = hex(fields[static_cast<std::size_t>(4 + i)]);
            const std::uint8_t got = rng.next();
            if (got != want) {
                ok = false;
                char buf[96];
                std::snprintf(buf, sizeof buf,
                              "draw %d: got 0x%02X, fixture 0x%02X", i, got, want);
                detail = buf;
                break;
            }
        }
        check(ok, "RNG stream matches fixture vector " + std::to_string(vectors), detail);
        if (ok) {
            const auto& s = rng.seed();
            check(s[0] == hex(fields[21]) && s[1] == hex(fields[22]) &&
                      s[2] == hex(fields[23]),
                  "RNG seed after 16 calls matches fixture vector " +
                      std::to_string(vectors));
        }
        ++vectors;
    }
    check(vectors == 3, "all three RNG fixture vectors were exercised",
          "vectors=" + std::to_string(vectors));
}

// --------------------------------------------------------------- mazes
std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>());
}

void test_level0_maze_against_fixture() {
    const std::string path = fixture_dir() + "/maze-level-0.bin";
    const std::vector<std::uint8_t> expected = read_file(path);
    check(expected.size() == 1024, "fixture maze-level-0.bin is 1024 bytes",
          "size=" + std::to_string(expected.size()) + " path=" + path);
    if (expected.size() != 1024) return;

    const dag::GeneratedLevel g = dag::generate_level(0, 1);
    const auto& got = g.maze.bytes();
    std::size_t first_div = 1025;
    for (std::size_t i = 0; i < 1024; ++i) {
        if (got[i] != expected[i]) { first_div = i; break; }
    }
    char buf[160];
    if (first_div < 1024) {
        std::snprintf(buf, sizeof buf,
                      "first divergence at byte %zu (row %zu col %zu): got 0x%02X, "
                      "fixture 0x%02X", first_div, first_div / 32, first_div % 32,
                      got[first_div], expected[first_div]);
    } else {
        buf[0] = '\0';
    }
    check(first_div == 1025, "level-0 maze matches the independently derived fixture",
          buf);
}

void test_all_levels_structure() {
    for (int level = 0; level < 5; ++level) {
        const dag::GeneratedLevel g = dag::generate_level(level, 1);
        int cleared = 0;
        for (const std::uint8_t b : g.maze.bytes()) {
            if (b != 0xFF) ++cleared;
        }
        check(cleared == 500, "level " + std::to_string(level) + " carves 500 cells",
              "cleared=" + std::to_string(cleared));

        // Reciprocal edges: a door or passage on one side must appear on the other.
        bool reciprocal = true;
        for (int r = 0; r < 32 && reciprocal; ++r) {
            for (int c = 0; c < 32 && reciprocal; ++c) {
                if (g.maze.at(r, c) == 0xFF) continue;
                for (int d = 0; d < 4; ++d) {
                    int nr = r, nc = c;
                    dag::step(nr, nc, static_cast<dag::Dir>(d));
                    if (!dag::Maze::in_bounds(nr, nc)) continue;
                    if (g.maze.at(nr, nc) == 0xFF) continue;
                    const auto a = g.maze.edge(r, c, static_cast<dag::Dir>(d));
                    const auto b = g.maze.edge(nr, nc, static_cast<dag::Dir>((d + 2) & 3));
                    if (a != b) { reciprocal = false; break; }
                }
            }
        }
        check(reciprocal, "level " + std::to_string(level) + " has reciprocal edges");
    }
}

void test_entry_time_invariance() {
    const dag::GeneratedLevel base = dag::generate_level(0, 1);
    for (const std::uint8_t second : {std::uint8_t{0}, std::uint8_t{7},
                                      std::uint8_t{30}, std::uint8_t{59}}) {
        const dag::GeneratedLevel g = dag::generate_level(0, second);
        check(g.maze.bytes() == base.maze.bytes(),
              "level-0 maze is identical at SECOND=" + std::to_string(second));
        check(g.rng_before_spin == base.rng_before_spin,
              "pre-spin RNG state is identical at SECOND=" + std::to_string(second));
        const bool differs = (g.rng_after_spin != base.rng_after_spin);
        check(differs || second == 1,
              "post-spin RNG state differs at SECOND=" + std::to_string(second));
    }
    // SECOND == 0 must spin 256 times, not zero (DEC B / BNE).
    check(dag::generate_level(0, 0).spin_count == 256,
          "SECOND=0 spins the RNG 256 times");
}

void test_movement_rule() {
    const dag::GeneratedLevel g = dag::generate_level(0, 1);
    // The starting cell is carved.
    check(g.maze.at(0x10, 0x0B) != 0xFF, "starting cell (16,11) is carved");
    // Every carved cell is reachable from the start: the generator carves one
    // connected walk, and STEPOK only rejects never-carved cells.
    std::vector<std::uint8_t> seen(1024, 0);
    std::vector<std::pair<int, int>> stack{{0x10, 0x0B}};
    seen[static_cast<std::size_t>(dag::Maze::index(0x10, 0x0B))] = 1;
    int reached = 1;
    while (!stack.empty()) {
        const auto [r, c] = stack.back();
        stack.pop_back();
        for (int d = 0; d < 4; ++d) {
            int nr = 0, nc = 0;
            if (!dag::step_ok(g.maze, r, c, static_cast<dag::Dir>(d), nr, nc)) continue;
            auto& s = seen[static_cast<std::size_t>(dag::Maze::index(nr, nc))];
            if (s) continue;
            s = 1;
            ++reached;
            stack.push_back({nr, nc});
        }
    }
    check(reached == 500, "all 500 carved cells are reachable under STEPOK",
          "reached=" + std::to_string(reached));
}

// -------------------------------------------------------------- parser
void test_parser() {
    struct Case {
        const char* line;
        dag::ParseStatus status;
        int type;
    };
    const Case cases[] = {
        {"MOVE", dag::ParseStatus::Matched, 7},
        {"M", dag::ParseStatus::Matched, 7},
        {"T", dag::ParseStatus::Matched, 11},
        {"LOOK", dag::ParseStatus::Matched, 6},
        {"Z", dag::ParseStatus::NoMatch, 0},      // ZLOAD and ZSAVE both match
        {"ZL", dag::ParseStatus::Matched, 13},
        {"ZS", dag::ParseStatus::Matched, 14},
        {"", dag::ParseStatus::NoToken, 0},
        {"   ", dag::ParseStatus::NoToken, 0},
        {"XYZZY", dag::ParseStatus::NoMatch, 0},
        {"MOVEX", dag::ParseStatus::NoMatch, 0},  // longer than the entry
    };
    for (const Case& c : cases) {
        std::size_t pos = 0;
        const dag::ParseResult r = dag::parse(dag::kCmdTab, c.line, pos);
        check(r.status == c.status, std::string("parse \"") + c.line + "\" status");
        if (c.status == dag::ParseStatus::Matched) {
            check(r.type == c.type, std::string("parse \"") + c.line + "\" type",
                  "got " + std::to_string(r.type));
        }
    }
    // Every command has a unique single or two-letter abbreviation, and the
    // ambiguous ones must be rejected rather than silently resolved.
    std::size_t pos = 0;
    check(dag::parse(dag::kDirTab, "B", pos).status == dag::ParseStatus::Matched,
          "DIRTAB 'B' resolves to BACK");
    pos = 0;
    check(dag::parse(dag::kDirTab, "BACKWARD", pos).status == dag::ParseStatus::NoMatch,
          "DIRTAB rejects BACKWARD: the table entry is BACK");
}

// ------------------------------------------------------- clock / tasks
void test_clock_rollovers() {
    dag::Game game(1, 0);
    game.advance_jiffies(6);
    check(game.counters().tenth == 1, "6 jiffies make one tenth",
          "tenth=" + std::to_string(game.counters().tenth));
    game.advance_jiffies(54);
    check(game.counters().second == 2, "60 jiffies make one second",
          "second=" + std::to_string(game.counters().second));
    game.advance_jiffies(59 * 60);
    check(game.counters().minute == 1, "3600 jiffies make one minute",
          "minute=" + std::to_string(game.counters().minute));
    check(game.counters().total_jiffies == 3600, "total jiffy count is exact");
}

std::vector<dag::KeyEvent> type_at(std::uint64_t start, const std::string& text) {
    std::vector<dag::KeyEvent> out;
    std::uint64_t j = start;
    for (const char ch : text) {
        out.push_back({j, static_cast<std::uint8_t>(ch == ' ' ? 0x20 : ch)});
        ++j;
    }
    out.push_back({j, 0x0D});
    return out;
}

void test_turn_and_move() {
    {
        dag::Game game(1, 0);
        game.load_script(type_at(2, "TURN RIGHT"));
        game.advance_jiffies(40);
        check(game.player().dir == dag::Dir::East, "TURN RIGHT from North faces East",
              "dir=" + std::to_string(static_cast<int>(game.player().dir)));
    }
    {
        dag::Game game(1, 0);
        game.load_script(type_at(2, "TURN LEFT"));
        game.advance_jiffies(40);
        check(game.player().dir == dag::Dir::West, "TURN LEFT from North faces West");
    }
    {
        dag::Game game(1, 0);
        game.load_script(type_at(2, "TURN AROUND"));
        game.advance_jiffies(40);
        check(game.player().dir == dag::Dir::South, "TURN AROUND from North faces South");
    }
    {
        dag::Game game(1, 0);
        game.load_script(type_at(2, "T U"));   // TURN UP is not a legal argument
        game.advance_jiffies(40);
        check(game.player().dir == dag::Dir::North, "TURN UP leaves the facing alone");
        bool saw_err = false;
        for (const auto& e : game.trace()) {
            if (e.kind == "OUTPUT" && e.detail == "???") saw_err = true;
        }
        check(saw_err, "TURN UP prints ???");
    }
    {
        // A blocked MOVE still pays the exertion cost (PMOV90 is on every path).
        dag::Game game(1, 0);
        game.load_script(type_at(2, "M"));
        game.advance_jiffies(40);
        bool exerted = false;
        bool moved_flag_present = false;
        for (const auto& e : game.trace()) {
            if (e.kind == "EXERT") exerted = true;
            if (e.kind == "MOVE") moved_flag_present = true;
        }
        check(moved_flag_present, "MOVE emits a MOVE event");
        check(exerted, "MOVE always pays exertion, blocked or not");
        check(game.player().damage >= 3, "exertion adds at least 3 damage",
              "damage=" + std::to_string(game.player().damage));
    }
}

void test_keystroke_burst_in_one_jiffy() {
    // PLAYER drains the whole keyboard buffer on its turn, so a burst delivered
    // inside one jiffy is consumed together and the embedded CR dispatches.
    dag::Game game(1, 0);
    std::vector<dag::KeyEvent> keys;
    for (const char ch : std::string("TURN RIGHT")) {
        keys.push_back({3, static_cast<std::uint8_t>(ch == ' ' ? 0x20 : ch)});
    }
    keys.push_back({3, 0x0D});
    game.load_script(std::move(keys));
    game.advance_jiffies(20);
    check(game.player().dir == dag::Dir::East,
          "a whole command typed inside one jiffy is dispatched in one PLAYER turn");
}

int live_count(const dag::Game& game) {
    int n = 0;
    for (const dag::Ccb& c : game.creatures()) {
        if (c.in_use) ++n;
    }
    return n;
}

int matrix_sum(const dag::Game& game) {
    int n = 0;
    for (const std::uint8_t v : game.matrix_row()) n += v;
    return n;
}

void test_population_against_fixture() {
    std::ifstream in(fixture_dir() + "/population-entry.txt");
    check(static_cast<bool>(in), "fixtures/population-entry.txt is readable");
    std::string line;
    int creatures_checked = 0;
    int objects_checked = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string kind;
        ls >> kind;
        if (kind == "creature") {
            int level, second, slot, type, row, col, power, mgo, mgd, pho, phd, mv, at, use;
            ls >> level >> second >> slot >> type >> row >> col >> power >> mgo >> mgd >>
                pho >> phd >> mv >> at >> use;
            dag::Game game(static_cast<std::uint8_t>(second), level);
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
            const bool ok = c.in_use == use && c.type == type && c.row == row && c.col == col &&
                            c.power == power && c.magic_offense == mgo &&
                            c.magic_defense == mgd && c.physical_offense == pho &&
                            c.physical_defense == phd && c.move_delay == mv &&
                            c.attack_delay == at;
            char buf[160];
            std::snprintf(buf, sizeof buf,
                          "L%d S%d slot %d got type %u at %u,%u power %u", level, second, slot,
                          c.type, c.row, c.col, c.power);
            check(ok, "creature record matches fixture", buf);
            ++creatures_checked;
        } else if (kind == "object") {
            int level, second, index, type, olevel, owner, cls, reveal, mgo, pho, s0, s1, s2,
                carrier;
            ls >> level >> second >> index >> type >> olevel >> owner >> cls >> reveal >> mgo >>
                pho >> s0 >> s1 >> s2 >> carrier;
            dag::Game game(static_cast<std::uint8_t>(second), level);
            const dag::Ocb& o = game.objects()[static_cast<std::size_t>(index)];
            const bool ok = o.type == type && o.level == olevel && o.owner == owner &&
                            o.cls == cls && o.reveal == reveal && o.magic_offense == mgo &&
                            o.physical_offense == pho && o.spec[0] == s0 && o.spec[1] == s1 &&
                            o.spec[2] == s2 && o.carrier == carrier;
            char buf[160];
            std::snprintf(buf, sizeof buf, "L%d object %d got type %u carrier %d owner %u",
                          level, index, o.type, o.carrier, o.owner);
            check(ok, "object record matches fixture", buf);
            ++objects_checked;
        } else if (kind == "cregen") {
            int before, mid, sum_after, inc, after;
            ls >> before >> mid >> sum_after >> inc >> after;
            dag::Game game(1, 0);
            check(live_count(game) == before, "level 0 births the CMTTAB count");
            check(matrix_sum(game) == before, "matrix sum matches the birth count");
            game.advance_jiffies(1);
            check(live_count(game) == mid, "opening CREGEN does not birth a creature");
            check(matrix_sum(game) == sum_after, "opening CREGEN increments the matrix");
            check(game.matrix_row()[static_cast<std::size_t>(inc)] == 1,
                  "opening CREGEN increments the fixture's type");
            game.enter_level(0);
            check(live_count(game) == after, "re-entry births the incremented matrix");
            check(matrix_sum(game) == after, "re-entry leaves the matrix unchanged");
        } else if (kind == "vft") {
            int expect[5];
            for (int i = 0; i < 5; ++i) ls >> expect[i];
            bool ok = true;
            for (int i = 0; i < 5; ++i) {
                if (dag::vft_pointer(i) != expect[i]) ok = false;
            }
            check(ok, "NEWLVL vertical-feature pointer matches the fixture");
        }
    }
    check(creatures_checked > 100, "creature fixture rows were checked",
          "rows=" + std::to_string(creatures_checked));
    check(objects_checked > 20, "object fixture rows were checked",
          "rows=" + std::to_string(objects_checked));
}

void test_look() {
    dag::Game game(1, 0);
    game.load_script(type_at(2, "L"));
    game.advance_jiffies(40);
    check(game.display_mode() == dag::DisplayMode::Viewer, "LOOK selects the viewer");
}

}  // namespace

int main() {
    test_rng();
    test_level0_maze_against_fixture();
    test_all_levels_structure();
    test_entry_time_invariance();
    test_movement_rule();
    test_parser();
    test_clock_rollovers();
    test_turn_and_move();
    test_keystroke_burst_in_one_jiffy();
    test_population_against_fixture();
    test_look();

    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks
              << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
