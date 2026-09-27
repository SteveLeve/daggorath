#include "daggorath/gesture.hpp"

namespace dag::input {

namespace {

char hand_letter(bool right_hand) {
    return right_hand ? 'R' : 'L';
}

}  // namespace

GestureLine::GestureLine(std::string line, std::uint64_t jiffy) : line_(std::move(line)) {
    std::string burst = line_;
    burst.push_back('\r');
    if (burst.size() > kMaxGestureLine) {
        ok_ = false;
        return;
    }
    ok_ = true;
    keys_.reserve(burst.size());
    for (const char ch : burst) {
        const std::uint8_t raw =
            (ch == '\r') ? std::uint8_t{0x0D} : static_cast<std::uint8_t>(ch);
        keys_.push_back(dag::KeyEvent{jiffy, raw});
    }
}

std::string gesture_move_forward() {
    return "M";
}
std::string gesture_move_back() {
    return "M B";
}
std::string gesture_move_left() {
    return "M L";
}
std::string gesture_move_right() {
    return "M R";
}

std::string gesture_turn_left() {
    return "T L";
}
std::string gesture_turn_right() {
    return "T R";
}
std::string gesture_turn_around() {
    return "T A";
}

std::string gesture_attack(bool right_hand) {
    return std::string("A ") + hand_letter(right_hand);
}

std::string gesture_get(bool right_hand, const std::string& object) {
    return std::string("G ") + hand_letter(right_hand) + " " + object;
}

std::string gesture_pull(bool right_hand, const std::string& object) {
    return std::string("P ") + hand_letter(right_hand) + " " + object;
}

std::string gesture_stow(bool right_hand) {
    return std::string("S ") + hand_letter(right_hand);
}

std::string gesture_drop(bool right_hand) {
    return std::string("D ") + hand_letter(right_hand);
}

std::string gesture_use(bool right_hand) {
    return std::string("U ") + hand_letter(right_hand);
}

std::string gesture_reveal(bool right_hand) {
    return std::string("R ") + hand_letter(right_hand);
}

std::string gesture_incant(const std::string& adjective) {
    return "I " + adjective;
}

std::string gesture_examine() {
    return "E";
}
std::string gesture_look() {
    return "L";
}

std::string gesture_climb_up() {
    return "C U";
}
std::string gesture_climb_down() {
    return "C D";
}

}  // namespace dag::input
