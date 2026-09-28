// Phase 8.5 replay-equivalence test (ADR-0009 §4 / D-17).
//
// A touch gesture's whole-line burst (dag::input::GestureLine, 8.1) must
// replay through the core exactly as the same command, hand-authored as a
// scripted burst on the same jiffy, already does. `TURN RIGHT` typed as one
// jiffy-5 burst is the committed `t3-burst-one-jiffy` fixture
// (docs/archaeology/phase-0b/traces/); the touch control that types the
// same command is TurnRight ("T R", docs/design/touch-controls/README.md).
// This test drives a fresh Game from the gesture adapter's own KeyEvents and
// requires the resulting trace to match that committed fixture byte for
// byte -- the same command, replayed from a different origin, is the same
// trace.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "daggorath/game.hpp"
#include "daggorath/gesture.hpp"

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

std::string slurp(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

#ifndef DAG_PHASE0B_TRACE_DIR
#define DAG_PHASE0B_TRACE_DIR "docs/archaeology/phase-0b/traces"
#endif

// Matches dcli's own trace + "# final" footer (src/app/dcli.cpp), so this
// test's output is directly comparable to a fixture dcli produced.
std::string render_trace(const dag::Game& game) {
    std::ostringstream os;
    os << "# jiffy\tclock\tevent\tdetail\n";
    for (const auto& e : game.trace()) os << e.to_line() << "\n";
    os << "# final\trow=" << game.player().row << "\tcol=" << game.player().col
       << "\tdir=" << static_cast<int>(game.player().dir)
       << "\tdamage=" << game.player().damage << "\n";
    return os.str();
}

void test_touch_turn_right_matches_scripted_burst() {
    // gesture_turn_right() -> "T R"; GestureLine appends CR: the exact four
    // bytes t3-burst-one-jiffy.script types at jiffy 5 (T, SPACE, R, CR).
    const dag::input::GestureLine gl(dag::input::gesture_turn_right(), 5);
    check(gl.ok(), "TurnRight composes within the adapter's length limit");
    check(gl.keys().size() == 4, "TurnRight is a 4-byte burst",
          std::to_string(gl.keys().size()));

    dag::Game touch_game;
    touch_game.load_script(gl.keys());
    touch_game.advance_jiffies(200);

    const std::string got = render_trace(touch_game);
    const std::string expected =
        slurp(std::string(DAG_PHASE0B_TRACE_DIR) + "/t3-burst-one-jiffy.trace");
    check(!expected.empty(), "the committed scripted-burst trace fixture is present");
    check(got == expected,
          "a touch session's trace matches the scripted keystroke transcript byte for "
          "byte");
}

void test_touch_move_matches_scripted_forward_step() {
    // gesture_move_forward() -> "M"; the shortest possible command line,
    // exercising the one-character-plus-CR case the coverage table (§1)
    // relies on for MOVE.
    const dag::input::GestureLine gl(dag::input::gesture_move_forward(), 5);
    check(gl.ok(), "MoveForward composes within the adapter's length limit");
    check(gl.keys().size() == 2, "MoveForward is a 2-byte burst (M, CR)");

    dag::Game touch_game;
    touch_game.load_script(gl.keys());
    touch_game.advance_jiffies(200);

    dag::Game typed_game;
    // The same command, typed one character per jiffy, finishing (CR) on
    // the same jiffy the touch burst uses -- both must dispatch MOVE
    // identically, since PLAYER drains whatever the buffer holds when it
    // next polls, not when each byte individually arrived.
    typed_game.load_script({{3, 'M'}, {5, 0x0D}});
    typed_game.advance_jiffies(200);

    check(render_trace(touch_game) == render_trace(typed_game),
          "a one-key touch burst matches the same command typed across jiffies, "
          "as long as both finish on the same jiffy");
}

}  // namespace

int main() {
    test_touch_turn_right_matches_scripted_burst();
    test_touch_move_matches_scripted_forward_step();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
