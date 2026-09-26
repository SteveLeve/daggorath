// Conformance tests for the Phase 0b reference slice.
//
// Expectations come from the fixtures under docs/archaeology/phase-0b/fixtures,
// which were produced by an independent Python transliteration of the same
// assembly routines (tools/extract_fixtures.py). A test that agrees only
// because both sides share code would be worthless, so the maze test compares
// against the fixture's raw bytes on disk rather than recomputing them.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "daggorath/creature_move.hpp"
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
int live_count(const dag::Game& game);
int matrix_sum(const dag::Game& game);

bool seed_eq(const dag::Rng::Seed& s, std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    return s[0] == a && s[1] == b && s[2] == c;
}

void test_rom_level0_entry() {
    // Harness path stays at SECOND=1 so population-entry.txt remains the
    // source-derived comparison. Original Mode is the no-argument constructor.
    {
        dag::Game harness(1, 0);
        check(harness.counters().to_string() == "0:0:1.0.0",
              "harness Game(1) still starts at 0:0:1.0.0");
        check(harness.counters().total_jiffies == 0, "harness trace jiffy starts at 0");
    }

    dag::Scheduler clock;
    clock.advance_clock_counters(dag::kLevel0BuildInterrupts);
    check(clock.counters().to_string() == "0:0:6.2.5",
          "377 counter bumps land on 0:0:6.2.5",
          clock.counters().to_string());
    check(clock.counters().total_jiffies == 0,
          "build interrupts are not scheduler-entry jiffies");

    dag::Scheduler scanned;
    for (std::uint32_t i = 0; i < dag::kLevel0BuildInterrupts; ++i) scanned.interrupt({});
    check(scanned.counters().jiffy == clock.counters().jiffy &&
              scanned.counters().tenth == clock.counters().tenth &&
              scanned.counters().second == clock.counters().second &&
              scanned.counters().minute == clock.counters().minute &&
              scanned.counters().hour == clock.counters().hour,
          "empty-queue interrupts match the counter-only build step");

    dag::Game game;
    check(game.counters().to_string() == "0:0:6.2.5",
          "Original Mode scheduler entry is 0:0:6.2.5",
          game.counters().to_string());
    check(game.counters().total_jiffies == 0, "INIT stays at trace jiffy 0");
    check(game.counters().second == 6, "DGEN90 sees SECOND = 6");
    check(game.level().spin_count == 6, "level-0 DGEN90 draws 6 times");
    check(seed_eq(game.level().rng_before_spin, 0x3A, 0xCB, 0xDC),
          "DGEN90 entry seed is 3ACBDC");
    check(seed_eq(game.level().rng_after_spin, 0x8F, 0xC8, 0xAD),
          "DGEN90 exit seed is 8FC8AD");
    check(seed_eq(game.level().rng.seed(), 0x07, 0x66, 0xCB),
          "NEWLVL exit seed is 0766CB");
    check(live_count(game) == 24, "GAME50 has 24 live creatures");
    const dag::Ccb& first = game.creatures()[0];
    check(first.in_use && first.type == 3 && first.row == 28 && first.col == 5,
          "first live block is 0:3@28,5");
    const std::uint8_t row_at_entry[] = {9, 9, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0};
    bool row_ok = true;
    for (int i = 0; i < 12; ++i) {
        if (game.matrix_row()[static_cast<std::size_t>(i)] != row_at_entry[i]) row_ok = false;
    }
    check(row_ok, "build interrupts leave the CMTTAB row unchanged");

    game.advance_jiffies(1);
    check(live_count(game) == 24, "opening CREGEN does not birth");
    check(game.matrix_row()[5] == 1, "opening CREGEN increments type 5");
    check(matrix_sum(game) == 25, "opening CREGEN raises the matrix sum to 25");
    check(seed_eq(game.level().rng.seed(), 0xC3, 0x07, 0x66),
          "opening CREGEN leaves seed C30766");
    game.enter_level(0);
    check(live_count(game) == 25, "the next level-0 NEWLVL births 25");
}

void test_clock_rollovers() {
    // Harness clock, not the ROM build. Six jiffies from 0:0:1.0.0 roll a tenth.
    dag::Game game(1, 0);
    game.set_frozen(true);
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

dag::Ccb live_spider(int row, int col) {
    dag::Ccb c;
    c.in_use = 0xFF;
    c.type = 0;
    c.move_delay = 23;
    c.attack_delay = 11;
    c.row = static_cast<std::uint8_t>(row);
    c.col = static_cast<std::uint8_t>(col);
    c.dir = 0;
    return c;
}

void test_cmove_priorities() {
    dag::Rng rng({1, 2, 3});
    dag::Maze open;
    for (int r = 0; r < 32; ++r)
        for (int c = 0; c < 32; ++c) open.put(r, c, 0);
    std::array<dag::Ccb, dag::kCcbSlots> ccbs{};
    std::vector<dag::Ocb> objects;
    std::vector<std::string> events;
    dag::CmoveView view;

    ccbs[0] = live_spider(4, 4);
    view.frozen = true;
    view.player_row = 4;
    view.player_col = 5;
    dag::TaskResult r = dag::cmove(0, ccbs, objects, open, rng, view, events);
    check(r.queue == dag::Queue::Tenth && r.countdown == 23, "frozen creature requeues at the movement delay");
    check(ccbs[0].row == 4 && ccbs[0].col == 4, "frozen creature does not move");
    check(events.empty(), "frozen creature emits nothing");

    view.frozen = false;
    ccbs[0].in_use = 0;
    events.clear();
    r = dag::cmove(0, ccbs, objects, open, rng, view, events);
    check(r.queue == dag::Queue::Null, "dead creature leaves the scheduler queue");

    ccbs[0] = live_spider(4, 4);
    dag::Ocb torch;
    torch.level = 0;
    torch.row = 4;
    torch.col = 4;
    torch.owner = 0;
    torch.type = 15;
    objects.push_back(torch);
    events.clear();
    r = dag::cmove(0, ccbs, objects, open, rng, view, events);
    check(r.countdown == 23, "pickup requeues at the movement delay");
    check(objects[0].owner == 0xFF && ccbs[0].object_head == 0, "pickup sets creature ownership");
    check(ccbs[0].row == 4, "pickup does not also move");

    ccbs[1] = live_spider(6, 6);
    ccbs[1].type = 6;
    objects[0].owner = 0;
    objects[0].row = 6;
    objects[0].col = 6;
    view.player_row = 0;
    view.player_col = 0;
    events.clear();
    dag::Rng rng2({0x80, 0, 0});
    r = dag::cmove(1, ccbs, objects, open, rng2, view, events);
    check(objects[0].owner == 0, "scorpion does not pick up");
    check(r.queue == dag::Queue::Tenth, "scorpion still requeues");

    ccbs[2] = live_spider(8, 8);
    ccbs[2].power = 32;
    ccbs[2].physical_offense = 128;
    ccbs[2].physical_defense = 255;
    ccbs[2].magic_defense = 255;
    view.player_row = 8;
    view.player_col = 8;
    dag::Fighter defender;
    defender.power = 160;
    view.player = &defender;
    events.clear();
    r = dag::cmove(2, ccbs, objects, open, rng, view, events);
    check(r.countdown == 11, "same cell requeues at the attack delay");
    bool swung = false;
    for (const auto& e : events)
        if (e.find("MISS slot=2") != std::string::npos || e.find("HIT slot=2") != std::string::npos)
            swung = true;
    check(swung, "same cell resolves the attack");
    check(ccbs[2].row == 8 && ccbs[2].col == 8, "deferred attack does not move the creature");

    ccbs[3] = live_spider(2, 2);
    view.player_row = 2;
    view.player_col = 5;
    events.clear();
    r = dag::cmove(3, ccbs, objects, open, rng, view, events);
    check(ccbs[3].col == 3 && ccbs[3].row == 2, "aligned creature steps toward the player");
    check(ccbs[3].dir == static_cast<std::uint8_t>(dag::Dir::East), "aligned creature faces the player");
    check(r.countdown == 23, "aligned step uses the movement delay");

    ccbs[5] = live_spider(9, 9);
    view.player_row = 9;
    view.player_col = 10;
    events.clear();
    r = dag::cmove(5, ccbs, objects, open, rng, view, events);
    check(ccbs[5].col == 10 && r.countdown == 11, "stepping onto the player selects the attack delay");
    bool pupdat = false;
    for (const auto& e : events)
        if (e.find("PUPDAT slot=5") != std::string::npos) pupdat = true;
    check(pupdat, "landing on the player requests PUPDAT");

    ccbs[6] = live_spider(1, 1);
    ccbs[6].type = 11;
    objects[0].owner = 0;
    objects[0].row = 1;
    objects[0].col = 1;
    view.player_row = 0;
    view.player_col = 0;
    events.clear();
    r = dag::cmove(6, ccbs, objects, open, rng2, view, events);
    check(objects[0].owner == 0, "wizard does not pick up");

    dag::Maze box;
    box.put(10, 10, 0);
    ccbs[4] = live_spider(10, 10);
    view.player_row = 0;
    view.player_col = 0;
    events.clear();
    dag::Rng rng3({0xFF, 0xFF, 0xFF});
    r = dag::cmove(4, ccbs, objects, box, rng3, view, events);
    check(ccbs[4].row == 10 && ccbs[4].col == 10, "boxed creature stays put");
    check(r.countdown == 23, "boxed creature still spends the movement delay");

    int side_first = 0, right_bias = 0;
    for (int b = 0; b < 256; ++b) {
        const dag::Preference p = dag::movement_preference(static_cast<std::uint8_t>(b));
        if (p.side_first) ++side_first;
        if ((b & 0x80) == 0) {
            ++right_bias;
            check(p.relative[p.side_first ? 0 : 1] == 1, "bit7 clear prefers right before left");
        } else {
            check(p.relative[p.side_first ? 0 : 1] == 3, "bit7 set prefers left before right");
        }
    }
    check(side_first == 64, "side-first is 64 of 256 random bytes");
    check(right_bias == 128, "right-before-left is 128 of 256 random bytes");
}

void test_same_jiffy_creature_and_key() {
    dag::Game probe(1, 0);
    probe.advance_jiffies(400);
    std::uint64_t at = 0;
    bool saw = false;
    for (const auto& ev : probe.trace()) {
        if (ev.kind == "TASK" && ev.detail == "run CMOVE-6") {
            at = ev.jiffy;
            saw = true;
            break;
        }
    }
    check(saw, "a level-0 creature moves inside 400 jiffies");
    dag::Game game(1, 0);
    game.load_script({{at, static_cast<std::uint8_t>('M')}});
    game.advance_jiffies(at + 1);
    bool player = false, first = false, second = false, order = false;
    for (const auto& ev : game.trace()) {
        if (ev.jiffy != at) continue;
        if (ev.kind == "TASK" && ev.detail == "run PLAYER") player = true;
        if (ev.kind == "TASK" && ev.detail == "run CMOVE-6") {
            first = true;
            order = player && !second;
        }
        if (ev.kind == "TASK" && ev.detail == "run CMOVE-7") {
            second = true;
            order = order && first;
        }
    }
    // Jiffy queue is scanned before Q.TEN. Equal delays stay in CBIRTH order.
    check(player && first && second && order,
          "PLAYER runs before CMOVE-6, which runs before CMOVE-7");
}

void test_reentry_mid_move() {
    dag::Game game(1, 0);
    game.advance_jiffies(400);
    const auto before = game.creatures();
    game.enter_level(0);
    const auto after = game.creatures();
    int live = 0;
    bool changed = false;
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        if (after[static_cast<std::size_t>(i)].in_use) ++live;
        if (before[static_cast<std::size_t>(i)].row != after[static_cast<std::size_t>(i)].row ||
            before[static_cast<std::size_t>(i)].col != after[static_cast<std::size_t>(i)].col)
            changed = true;
    }
    check(live == 25, "re-entry births the regenerated level-0 row", std::to_string(live));
    check(changed, "re-entry replaces creature positions");
    bool dirs_cleared = true;
    for (const auto& c : after)
        if (c.in_use && c.dir != 0) dirs_cleared = false;
    check(dirs_cleared, "NEWLVL zeroes facing on the new control blocks");
    game.advance_jiffies(20);
}

void test_look() {
    dag::Game game(1, 0);
    game.load_script(type_at(2, "L"));
    game.advance_jiffies(40);
    check(game.display_mode() == dag::DisplayMode::Viewer, "LOOK selects the viewer");
}

void test_combat_fixtures_and_flow() {
    const std::string path =
        std::string(DAG_FIXTURE_DIR) + "/../../phase-3/fixtures/scal16-damage.json";
    std::ifstream in(path);
    check(in.good(), "phase 3 combat fixture is present");
    if (!in.good()) return;
    std::stringstream buf;
    buf << in.rdbuf();
    const std::string text = buf.str();
    // The fixture is a flat list of "value radix result" triples inside a JSON array
    // named scal16, encoded as decimal strings the extractor wrote one per line
    // between markers so this test does not need a JSON library.
    const auto marker = text.find("\"cases\"");
    check(marker != std::string::npos, "combat fixture has cases");
    int matched = 0;
    std::size_t pos = 0;
    while (true) {
        const auto line = text.find("\"S ", pos);
        if (line == std::string::npos) break;
        unsigned value = 0, radix = 0, result = 0;
        if (std::sscanf(text.c_str() + line, "\"S %u %u %u\"", &value, &radix, &result) == 3) {
            check(dag::scal16(static_cast<std::uint16_t>(value), static_cast<std::uint8_t>(radix)) ==
                      result,
                  "SCAL16 matches the Python fixture");
            ++matched;
        }
        pos = line + 3;
    }
    check(matched == 88, "SCAL16 fixture rows were checked", "rows=" + std::to_string(matched));

    // Index order matches tools/gen_combat_fixtures.py. The expected numbers
    // come from the fixture file, not from this table.
    struct Stat5 { int a, b, c, d, e; };
    const Stat5 creatures[] = {
        {32, 0, 255, 128, 255}, {56, 0, 255, 80, 128}, {200, 0, 255, 52, 192},
        {304, 0, 255, 96, 167}, {504, 0, 128, 96, 60}, {704, 0, 128, 128, 48},
        {400, 255, 128, 255, 128}, {800, 0, 64, 255, 8}, {800, 192, 16, 192, 8},
        {1000, 255, 5, 255, 3}, {1000, 255, 6, 255, 0}, {8000, 255, 6, 255, 0},
    };
    const int weapons[][2] = {{0, 5}, {0, 16}, {0, 40}, {64, 64}, {255, 255}};
    const int shields[][2] = {{0x80, 0x80}, {108, 128}, {96, 128}, {64, 64}};
    int damage_rows = 0;
    pos = 0;
    while (true) {
        const auto line = text.find("\"D ", pos);
        if (line == std::string::npos) break;
        unsigned ci = 0, wi = 0, si = 0, dealt = 0, taken = 0;
        if (std::sscanf(text.c_str() + line, "\"D %u %u %u %u %u\"", &ci, &wi, &si, &dealt,
                        &taken) == 5) {
            dag::Fighter weapon;
            weapon.power = 160;
            weapon.magic_offense = static_cast<std::uint8_t>(weapons[wi][0]);
            weapon.physical_offense = static_cast<std::uint8_t>(weapons[wi][1]);
            dag::Fighter creature;
            creature.power = static_cast<std::uint16_t>(creatures[ci].a);
            creature.magic_defense = static_cast<std::uint8_t>(creatures[ci].c);
            creature.physical_defense = static_cast<std::uint8_t>(creatures[ci].e);
            dag::apply_damage(weapon, creature);
            dag::Fighter player;
            player.magic_defense = static_cast<std::uint8_t>(shields[si][0]);
            player.physical_defense = static_cast<std::uint8_t>(shields[si][1]);
            dag::Fighter attacker;
            attacker.power = static_cast<std::uint16_t>(creatures[ci].a);
            attacker.magic_offense = static_cast<std::uint8_t>(creatures[ci].b);
            attacker.physical_offense = static_cast<std::uint8_t>(creatures[ci].d);
            dag::apply_damage(attacker, player);
            check(creature.damage == dealt && player.damage == taken,
                  "DAMAGE matches the Python fixture");
            ++damage_rows;
        }
        pos = line + 3;
    }
    check(damage_rows == 240, "DAMAGE fixture rows were checked",
          "rows=" + std::to_string(damage_rows));

    int attack_rows = 0;
    pos = 0;
    while (true) {
        const auto line = text.find("\"A ", pos);
        if (line == std::string::npos) break;
        unsigned power = 0, defender = 0, damage = 0, roll = 0, hit = 0;
        if (std::sscanf(text.c_str() + line, "\"A %u %u %u %u %u\"", &power, &defender, &damage,
                        &roll, &hit) == 5) {
            const bool connected = dag::attack_hits(static_cast<std::uint16_t>(power),
                                                     static_cast<std::uint16_t>(defender),
                                                     static_cast<std::uint16_t>(damage),
                                                     static_cast<std::uint8_t>(roll));
            check(connected == (hit == 1), "ATTACK matches the Python fixture");
            ++attack_rows;
        }
        pos = line + 3;
    }
    check(attack_rows == 256, "ATTACK fixture rows were checked",
          "rows=" + std::to_string(attack_rows));

    dag::Game game;
    const auto& objects = game.objects();
    int sword = -1;
    int torch = -1;
    for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
        if (objects[static_cast<std::size_t>(i)].owner == 1 &&
            objects[static_cast<std::size_t>(i)].type == 17)
            sword = i;
        if (objects[static_cast<std::size_t>(i)].owner == 1 &&
            objects[static_cast<std::size_t>(i)].type == 15)
            torch = i;
    }
    check(sword >= 0 && torch >= 0, "starting sword and torch exist");
    game.hold(false, sword);
    game.wield_torch(torch);

    int target = -1;
    int row = 0;
    int col = 0;
    int best_steps = 1000;
    const int start_r = game.player().row;
    const int start_c = game.player().col;
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        if (!game.creatures()[static_cast<std::size_t>(i)].in_use) continue;
        const int tr = game.creatures()[static_cast<std::size_t>(i)].row;
        const int tc = game.creatures()[static_cast<std::size_t>(i)].col;
        const int steps = std::abs(tr - start_r) + std::abs(tc - start_c);
        if (steps < best_steps) {
            best_steps = steps;
            target = i;
            row = tr;
            col = tc;
        }
    }
    check(target >= 0, "a creature exists to fight");
    // Walk with MOVE/TURN until the player shares the cell. The maze is open
    // under the Phase 0b movement rule, so Manhattan steps suffice.
    std::string script;
    std::uint64_t j = 1;
    auto add = [&](const std::string& keys) {
        for (char ch : keys) {
            const std::string key = ch == ' ' ? "SPACE" : std::string(1, ch);
            script += std::to_string(j) + " " + key + "\n";
            ++j;
        }
        script += std::to_string(j) + " CR\n";
        j += 30;
    };
    struct Node { int r, c, dir, parent; };
    std::vector<Node> nodes;
    std::vector<int> queue;
    std::vector<char> seen(32 * 32 * 4, 0);
    nodes.push_back({start_r, start_c, 0, -1});
    queue.push_back(0);
    seen[(start_r * 32 + start_c) * 4] = 1;
    int found = -1;
    for (std::size_t qi = 0; qi < queue.size() && found < 0; ++qi) {
        const Node here = nodes[static_cast<std::size_t>(queue[qi])];
        if (here.r == row && here.c == col) {
            found = queue[qi];
            break;
        }
        for (int turn = 0; turn < 3; ++turn) {
            const int ndir = (here.dir + (turn == 0 ? 0 : turn == 1 ? 1 : 3)) & 3;
            int nr = 0, nc = 0;
            if (!dag::step_ok(game.maze(), here.r, here.c, static_cast<dag::Dir>(ndir), nr, nc))
                continue;
            const int key = (nr * 32 + nc) * 4 + ndir;
            if (seen[static_cast<std::size_t>(key)]) continue;
            seen[static_cast<std::size_t>(key)] = 1;
            nodes.push_back({nr, nc, ndir, queue[qi]});
            queue.push_back(static_cast<int>(nodes.size()) - 1);
        }
    }
    check(found >= 0, "a walk reaches the creature");
    if (found < 0) return;
    std::vector<int> steps;
    for (int n = found; n >= 0; n = nodes[static_cast<std::size_t>(n)].parent) steps.push_back(n);
    std::reverse(steps.begin(), steps.end());
    int facing = 0;
    for (std::size_t pi = 1; pi < steps.size(); ++pi) {
        const int want = nodes[static_cast<std::size_t>(steps[pi])].dir;
        const int delta = (want - facing) & 3;
        if (delta == 1) add("TURN RIGHT");
        else if (delta == 3) add("TURN LEFT");
        else if (delta == 2) add("TURN AROUND");
        facing = want;
        add("MOVE");
    }
    const std::uint8_t before = game.matrix_row()[game.creatures()[static_cast<std::size_t>(target)].type];
    for (int n = 0; n < 40; ++n) add("ATTACK LEFT");
    game.set_frozen(true);
    std::string error;
    game.load_script(dag::parse_script(script, error));
    check(error.empty(), "fight script parses");
    game.advance_jiffies(j + 5);
    bool killed = false;
    for (const auto& e : game.trace())
        if (e.kind == "KILL") killed = true;
    int hits = 0, misses = 0, darks = 0;
    bool died = false;
    for (const auto& e : game.trace()) {
        if (e.kind == "HIT") ++hits;
        if (e.kind == "MISS") ++misses;
        if (e.kind == "DARK") ++darks;
        if (e.kind == "DEATH") died = true;
    }
    check(killed, "repeated ATTACK LEFT kills the creature",
          "player=" + std::to_string(game.player().row) + "," +
              std::to_string(game.player().col) + " target=" + std::to_string(row) + "," +
              std::to_string(col) + " hits=" + std::to_string(hits) +
              " misses=" + std::to_string(misses) + " dark=" + std::to_string(darks) +
              " dead=" + std::to_string(died) + " power=" + std::to_string(game.player().power) +
              " damage=" + std::to_string(game.player().damage));
    const std::uint8_t after = game.matrix_row()[game.creatures()[static_cast<std::size_t>(target)].type];
    check(after == static_cast<std::uint8_t>(before - 1), "kill decrements CMXLND");
}

}  // namespace

int main() {
    test_rng();
    test_level0_maze_against_fixture();
    test_all_levels_structure();
    test_entry_time_invariance();
    test_movement_rule();
    test_parser();
    test_rom_level0_entry();
    test_clock_rollovers();
    test_turn_and_move();
    test_keystroke_burst_in_one_jiffy();
    test_population_against_fixture();
    test_look();
    test_cmove_priorities();
    test_same_jiffy_creature_and_key();
    test_reentry_mid_move();
    test_combat_fixtures_and_flow();

    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks
              << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
