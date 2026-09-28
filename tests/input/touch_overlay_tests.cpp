// Phase 8.4 touch overlay regressions: pure layout, hit-testing and gesture
// dispatch (no SDL). Mouse-as-touch: a click and a tap are the same
// (x, y) -> hit_test path.
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "daggorath/touch_overlay.hpp"

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

bool has(const std::vector<dag::input::Button>& buttons, dag::input::ButtonId id) {
    for (const auto& b : buttons)
        if (b.id == id) return true;
    return false;
}

const dag::input::Rect& rect_of(const std::vector<dag::input::Button>& buttons,
                                dag::input::ButtonId id) {
    for (const auto& b : buttons)
        if (b.id == id) return b.rect;
    static dag::input::Rect empty{};
    return empty;
}

bool overlaps(const dag::input::Rect& a, const dag::input::Rect& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

void test_always_present_controls() {
    using namespace dag::input;
    OverlayState state;
    for (auto layout : {OverlayLayout::PhoneLandscape, OverlayLayout::Tablet4x3}) {
        const double vw = layout == OverlayLayout::PhoneLandscape ? 1280 : 1024;
        const double vh = layout == OverlayLayout::PhoneLandscape ? 720 : 768;
        const auto buttons = layout_buttons(layout, vw, vh, state);
        for (ButtonId id : {ButtonId::AttackLeft, ButtonId::AttackRight,
                            ButtonId::MoveForward, ButtonId::MoveBack, ButtonId::MoveLeft,
                            ButtonId::MoveRight, ButtonId::TurnLeft, ButtonId::TurnRight,
                            ButtonId::TurnAround, ButtonId::Examine, ButtonId::Look,
                            ButtonId::Keyboard, ButtonId::SystemMenu}) {
            check(has(buttons, id), "control is always present in both layouts");
        }
        check(!has(buttons, ButtonId::Climb), "Climb is absent when not available");
    }
}

void test_hand_state_swaps_controls() {
    using namespace dag::input;
    OverlayState empty_hands;
    OverlayState holding;
    holding.left_hand_empty = false;
    holding.right_hand_empty = false;

    const auto with_empty =
        layout_buttons(OverlayLayout::PhoneLandscape, 1280, 720, empty_hands);
    check(has(with_empty, ButtonId::GetLeft) && has(with_empty, ButtonId::PullLeft),
          "empty left hand shows G and P");
    check(!has(with_empty, ButtonId::HandMenuLeft), "empty hand has no hand menu");

    const auto with_holding =
        layout_buttons(OverlayLayout::PhoneLandscape, 1280, 720, holding);
    check(has(with_holding, ButtonId::HandMenuLeft) &&
              has(with_holding, ButtonId::HandMenuRight),
          "holding hands show the hand menu");
    check(!has(with_holding, ButtonId::GetLeft) && !has(with_holding, ButtonId::PullLeft),
          "holding hand has no G or P");
}

void test_climb_conditional() {
    using namespace dag::input;
    OverlayState state;
    state.climb_available = true;
    const auto buttons = layout_buttons(OverlayLayout::Tablet4x3, 1024, 768, state);
    check(has(buttons, ButtonId::Climb), "Climb appears when available");
}

void test_no_overlaps_within_a_layout() {
    using namespace dag::input;
    OverlayState state;
    state.left_hand_empty = false;
    state.right_hand_empty = true;
    state.climb_available = true;
    for (auto layout : {OverlayLayout::PhoneLandscape, OverlayLayout::Tablet4x3}) {
        const double vw = layout == OverlayLayout::PhoneLandscape ? 1280 : 1024;
        const double vh = layout == OverlayLayout::PhoneLandscape ? 720 : 768;
        const auto buttons = layout_buttons(layout, vw, vh, state);
        bool clean = true;
        for (std::size_t i = 0; i < buttons.size() && clean; ++i)
            for (std::size_t j = i + 1; j < buttons.size(); ++j)
                if (overlaps(buttons[i].rect, buttons[j].rect)) clean = false;
        check(clean, "no two buttons overlap in this layout/state combination");
    }
}

void test_tablet_stays_clear_of_bottom_band() {
    using namespace dag::input;
    OverlayState state;
    state.climb_available = true;
    const auto buttons = layout_buttons(OverlayLayout::Tablet4x3, 1024, 768, state);
    const double bottom_band = 768 * 0.75;
    bool clear = true;
    for (const auto& b : buttons)
        if (b.rect.y + b.rect.h > bottom_band) clear = false;
    check(clear, "tablet 4:3 controls stay out of the bottom status/command band");
}

void test_mouse_as_touch_hit_testing() {
    using namespace dag::input;
    OverlayState state;
    const auto buttons = layout_buttons(OverlayLayout::PhoneLandscape, 1280, 720, state);
    const Rect& forward = rect_of(buttons, ButtonId::MoveForward);
    const auto hit = hit_test(buttons, forward.x + 1, forward.y + 1);
    check(hit.has_value() && *hit == ButtonId::MoveForward,
          "a click inside a button's rect hits that button (mouse-as-touch)");

    const auto miss = hit_test(buttons, -100, -100);
    check(!miss.has_value(), "a click outside every rect hits nothing");
}

void test_tap_dispatch_for_simple_commands() {
    using namespace dag::input;
    OverlayState state;
    auto outcome = resolve_tap(ButtonId::MoveForward, state);
    check(outcome.line.has_value() && *outcome.line == "M", "MoveForward types M");
    check(outcome.pending == PendingKind::None, "a finished line has no pending picker");

    outcome = resolve_tap(ButtonId::TurnAround, state);
    check(outcome.line == std::string("T A"), "TurnAround types T A");

    outcome = resolve_tap(ButtonId::AttackRight, state);
    check(outcome.line == std::string("A R"), "AttackRight types A R");
}

void test_tap_dispatch_for_pickers_and_menus() {
    using namespace dag::input;
    OverlayState state;
    auto outcome = resolve_tap(ButtonId::GetLeft, state);
    check(!outcome.line.has_value(), "GetLeft alone has no finished line yet");
    check(outcome.pending == PendingKind::FloorPicker && !outcome.right_hand,
          "GetLeft opens the floor picker for the left hand");

    const auto chosen =
        resolve_picker_choice(outcome.pending, outcome.right_hand, "SWORD");
    check(chosen.has_value() && *chosen == "G L SWORD",
          "choosing SWORD from the floor picker types G L SWORD");

    outcome = resolve_tap(ButtonId::HandMenuRight, state);
    check(outcome.pending == PendingKind::HandMenu && outcome.right_hand,
          "the hand menu opens for the tapped hand");
    const auto stow = resolve_picker_choice(outcome.pending, outcome.right_hand, "S");
    check(stow.has_value() && *stow == "S R", "choosing S from the hand menu types S R");
    const auto incant_escape =
        resolve_picker_choice(outcome.pending, outcome.right_hand, "I");
    check(!incant_escape.has_value(),
          "choosing I from the hand menu has no line yet (opens the incant keyboard)");
}

void test_climb_confirmation() {
    using namespace dag::input;
    OverlayState state;
    const auto outcome = resolve_tap(ButtonId::Climb, state);
    check(outcome.pending == PendingKind::ClimbChoice,
          "Climb opens a two-way confirmation");
    const auto up = resolve_picker_choice(outcome.pending, false, "U");
    const auto down = resolve_picker_choice(outcome.pending, false, "D");
    check(up.has_value() && *up == "C U", "confirming U types C U");
    check(down.has_value() && *down == "C D", "confirming D types C D");
}

void test_incant_keyboard_finish() {
    using namespace dag::input;
    const auto line =
        resolve_picker_choice(PendingKind::IncantKeyboard, false, "SUPREME");
    check(line.has_value() && *line == "I SUPREME",
          "finishing the incant keyboard types I <adjective>");
}

}  // namespace

int main() {
    test_always_present_controls();
    test_hand_state_swaps_controls();
    test_climb_conditional();
    test_no_overlaps_within_a_layout();
    test_tablet_stays_clear_of_bottom_band();
    test_mouse_as_touch_hit_testing();
    test_tap_dispatch_for_simple_commands();
    test_tap_dispatch_for_pickers_and_menus();
    test_climb_confirmation();
    test_incant_keyboard_finish();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
