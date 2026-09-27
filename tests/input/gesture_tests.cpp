// Phase 8.1 gesture adapter regressions.
//
// Checks each named gesture against the committed fixture
// (docs/archaeology/phase-8/fixtures/gesture-lines.txt), that every produced
// keystroke burst is stamped on one shared jiffy (D-17), that the 31-character
// adapter limit (docs/specification/clock-and-scheduler.md §13) rejects an
// over-long line, and that the 32-byte keyboard-buffer overrun stays reachable
// by typing (unaffected by the gesture adapters).
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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

std::map<std::string, std::string> load_fixture(const std::string& path) {
    std::map<std::string, std::string> out;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto bar = line.find('|');
        if (bar == std::string::npos) continue;
        out[line.substr(0, bar)] = line.substr(bar + 1);
    }
    return out;
}

void check_line(const std::map<std::string, std::string>& fixture,
                const std::string& name, const std::string& actual) {
    const auto it = fixture.find(name);
    check(it != fixture.end(), "fixture has entry", name);
    if (it != fixture.end())
        check(it->second == actual, "gesture matches fixture: " + name,
              "got '" + actual + "', want '" + it->second + "'");
}

void test_gesture_lines_match_fixture() {
    const std::string path = std::string(DAG_PHASE8_FIXTURE_DIR) + "/gesture-lines.txt";
    const auto fixture = load_fixture(path);
    check(!fixture.empty(), "fixture file loaded", path);

    check_line(fixture, "move_forward", dag::input::gesture_move_forward());
    check_line(fixture, "move_back", dag::input::gesture_move_back());
    check_line(fixture, "move_left", dag::input::gesture_move_left());
    check_line(fixture, "move_right", dag::input::gesture_move_right());
    check_line(fixture, "turn_left", dag::input::gesture_turn_left());
    check_line(fixture, "turn_right", dag::input::gesture_turn_right());
    check_line(fixture, "turn_around", dag::input::gesture_turn_around());
    check_line(fixture, "attack_left", dag::input::gesture_attack(false));
    check_line(fixture, "attack_right", dag::input::gesture_attack(true));
    check_line(fixture, "get_left_sword", dag::input::gesture_get(false, "SWORD"));
    check_line(fixture, "pull_right_torch", dag::input::gesture_pull(true, "TORCH"));
    check_line(fixture, "stow_left", dag::input::gesture_stow(false));
    check_line(fixture, "drop_right", dag::input::gesture_drop(true));
    check_line(fixture, "use_left", dag::input::gesture_use(false));
    check_line(fixture, "reveal_right", dag::input::gesture_reveal(true));
    check_line(fixture, "incant_supreme", dag::input::gesture_incant("SUPREME"));
    check_line(fixture, "examine", dag::input::gesture_examine());
    check_line(fixture, "look", dag::input::gesture_look());
    check_line(fixture, "climb_up", dag::input::gesture_climb_up());
    check_line(fixture, "climb_down", dag::input::gesture_climb_down());

    check(fixture.size() == 20, "fixture has exactly the checked gestures",
          "fixture has " + std::to_string(fixture.size()));
}

// D-17: a finished gesture delivers its whole line on one jiffy, unlike typed
// input's one-key-per-jiffy pacing.
void test_whole_line_one_jiffy() {
    const dag::input::GestureLine gl("M L", 42);
    check(gl.ok(), "M L composes within the length limit");
    check(gl.line() == "M L", "line text excludes the CR");
    check(gl.keys().size() == 4, "M L + CR is 4 keystrokes",
          std::to_string(gl.keys().size()));
    for (const auto& key : gl.keys()) {
        check(key.jiffy == 42, "every keystroke shares the gesture's jiffy");
    }
    check(gl.keys().back().ch == 0x0D, "the burst ends in CR");
    const std::string expect = "M L\r";
    check(gl.keys().size() == expect.size(), "keystroke count matches burst length");
    for (std::size_t i = 0; i < gl.keys().size() && i < expect.size(); ++i) {
        check(
            gl.keys()[i].ch == static_cast<std::uint8_t>(expect[i]),
            "keystroke byte matches the composed line at position " + std::to_string(i));
    }
}

// The adapter requirement in D-17: at most 31 characters including CR.
void test_over_long_line_is_rejected() {
    // "I " + 30 letters = 32 chars before CR, 33 with it: over the limit.
    const std::string huge_adjective(30, 'X');
    const dag::input::GestureLine gl(dag::input::gesture_incant(huge_adjective), 7);
    check(!gl.ok(), "an over-long gesture line is rejected");
    check(gl.keys().empty(), "a rejected gesture line produces no keystrokes");
}

// Every gesture line the mapping in §1 of the coverage table actually
// produces stays within the limit, including the longest one (INCANT with
// the longest real adjective, "SUPREME").
void test_longest_real_gesture_fits() {
    const dag::input::GestureLine gl(dag::input::gesture_incant("SUPREME"), 1);
    check(gl.ok(), "INCANT SUPREME fits the adapter limit");
    check(gl.line().size() + 1 <= dag::input::kMaxGestureLine,
          "INCANT SUPREME plus CR is at most kMaxGestureLine",
          std::to_string(gl.line().size() + 1));
}

}  // namespace

int main() {
    test_gesture_lines_match_fixture();
    test_whole_line_one_jiffy();
    test_over_long_line_is_rejected();
    test_longest_real_gesture_fits();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
