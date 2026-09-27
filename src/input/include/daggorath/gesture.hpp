// Touch gesture -> keystroke adapters (Phase 8.1).
//
// A gesture never talks to daggorath::core directly. It only builds the
// command-line text a typist would enter and hands it to the keyboard
// buffer as one timestamped burst, per deviation D-17
// (docs/specification/clock-and-scheduler.md §13): unlike typed input, which
// is timestamped one character per jiffy, a finished gesture delivers its
// whole line on a single jiffy. The mapping from gesture to command text
// follows docs/design/touch-controls/README.md and the coverage table in
// docs/architecture/touch-input.md §1.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/game.hpp"

namespace dag::input {

// D-17's adapter requirement: a touch line is at most 31 characters
// including the terminating CR, because a 32-character burst leaves
// KBDPUT's head equal to tail (reads as empty). Typed input is unaffected:
// it keeps its own per-key timing and can still produce the 32-byte
// overrun (docs/archaeology/phase-0b/traces/t5-keyboard-overrun.script).
inline constexpr std::size_t kMaxGestureLine = 31;

// One command line, ready to feed to `Game::load_script` (or appended to an
// existing script). `ok()` is false when the composed line would exceed
// `kMaxGestureLine`; in that case `keys()` is empty and the caller must not
// deliver it. Every character key shares the same jiffy stamp, matching the
// scripted `t3-burst-one-jiffy` fixture already used for typed same-jiffy
// bursts.
class GestureLine {
public:
    GestureLine(std::string line, std::uint64_t jiffy);

    bool ok() const { return ok_; }
    // The command text as a typist would enter it, without the trailing CR.
    const std::string& line() const { return line_; }
    const std::vector<dag::KeyEvent>& keys() const { return keys_; }

private:
    bool ok_ = false;
    std::string line_;
    std::vector<dag::KeyEvent> keys_;
};

// Command-line text for each control in the coverage table
// (docs/architecture/touch-input.md §1). These return the text a typist
// would enter, without the trailing CR; `GestureLine` appends it and
// enforces the length limit. `hand` selects `L`/`R`.
std::string gesture_move_forward();
std::string gesture_move_back();
std::string gesture_move_left();
std::string gesture_move_right();
std::string gesture_turn_left();
std::string gesture_turn_right();
std::string gesture_turn_around();
std::string gesture_attack(bool right_hand);
std::string gesture_get(bool right_hand, const std::string& object);
std::string gesture_pull(bool right_hand, const std::string& object);
std::string gesture_stow(bool right_hand);
std::string gesture_drop(bool right_hand);
std::string gesture_use(bool right_hand);
std::string gesture_reveal(bool right_hand);
// The in-game keyboard (design doc "INCANT") delivers `I <adjective>` as one
// gesture line once the player presses the incant keyboard's Return.
std::string gesture_incant(const std::string& adjective);
std::string gesture_examine();
std::string gesture_look();
std::string gesture_climb_up();
std::string gesture_climb_down();

}  // namespace dag::input
