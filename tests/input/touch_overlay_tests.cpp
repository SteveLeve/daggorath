// Phase 8.4 touch overlay regressions: pure layout, hit-testing and gesture
// dispatch (no SDL). Mouse-as-touch: a click and a tap are the same
// (x, y) -> hit_test path.
#include <cmath>
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
                            ButtonId::TurnAround, ButtonId::SystemMenu}) {
            check(has(buttons, id), "control is always present in both layouts");
        }
        check(has(buttons, ButtonId::Examine) && !has(buttons, ButtonId::Look),
              "the view offers E, not L");
        OverlayState examining;
        examining.examining = true;
        const auto listing = layout_buttons(layout, vw, vh, examining);
        check(has(listing, ButtonId::Look) && !has(listing, ButtonId::Examine),
              "the EXAMINE listing offers L, not E");
        check(rect_of(listing, ButtonId::Look).x == rect_of(buttons, ButtonId::Examine).x &&
                  rect_of(listing, ButtonId::Look).y == rect_of(buttons, ButtonId::Examine).y,
              "E and L share one slot");
        check(!has(buttons, ButtonId::Climb), "Climb is absent when not available");
    }
}

void test_each_hand_shows_attack_and_menu() {
    using namespace dag::input;
    for (bool empty : {true, false}) {
        OverlayState state;
        state.left_hand_empty = state.right_hand_empty = empty;
        const auto b = layout_buttons(OverlayLayout::PhoneLandscape, 1280, 720, state);
        check(has(b, ButtonId::AttackLeft) && has(b, ButtonId::HandMenuLeft) &&
                  has(b, ButtonId::AttackRight) && has(b, ButtonId::HandMenuRight),
              "each hand shows A and its menu, full or empty");
        check(b.size() == 13, "no separate G, P or keyboard buttons (Steve, 2026-09-28)",
              std::to_string(b.size()));
    }
}

void test_phone_follows_main_board() {
    using namespace dag::input;
    // At 872x390 the move cluster just fits the margin, so a board unit is
    // one pixel and the left side matches the Main board exactly.
    OverlayState state;
    state.climb_available = true;
    const double vw = 872;
    const auto b = layout_buttons(OverlayLayout::PhoneLandscape, vw, 390, state);
    auto at = [&](ButtonId id, double x, double y) {
        const Rect& r = rect_of(b, id);
        return std::abs(r.x - x) < 1e-9 && std::abs(r.y - y) < 1e-9 && std::abs(r.w - 48) < 1e-9;
    };
    check(at(ButtonId::AttackLeft, 14, 14) && at(ButtonId::HandMenuLeft, 14, 68) &&
              at(ButtonId::AttackRight, vw - 62, 14) && at(ButtonId::HandMenuRight, vw - 62, 68),
          "A over the hand menu, as the Main board stacks A over its hand button");
    check(at(ButtonId::MoveLeft, 14, 220) && at(ButtonId::MoveForward, 68, 220) &&
              at(ButtonId::MoveRight, 122, 220) && at(ButtonId::TurnLeft, 14, 274) &&
              at(ButtonId::TurnAround, 68, 274) && at(ButtonId::TurnRight, 122, 274) &&
              at(ButtonId::MoveBack, 68, 328),
          "movement is ⇤ ↑ ⇥ / ↶ ↻ ↷ / ↓ as on the Main board");
    check(at(ButtonId::Examine, vw - 116, 328) && at(ButtonId::Climb, vw - 116, 274),
          "E where the Main board puts it, C in the Legacy board's slot");

    // At the Main board's own 844 wide the controls shrink to the margin.
    const auto narrow = layout_buttons(OverlayLayout::PhoneLandscape, 844, 390, state);
    const double game_left = (844 - 520) / 2.0, game_right = game_left + 520;
    bool clear = true;
    for (const auto& btn : narrow)
        if (btn.rect.x < game_right && btn.rect.x + btn.rect.w > game_left) clear = false;
    check(clear, "no phone control reaches into the game picture (T22)");
}

void test_examining_hides_top_buttons_over_the_picture() {
    using namespace dag::input;
    OverlayState state;
    state.examining = true;
    const auto tablet = layout_buttons(OverlayLayout::Tablet4x3, 768, 576, state);
    check(!has(tablet, ButtonId::AttackLeft) && !has(tablet, ButtonId::HandMenuRight),
          "on the tablet the top buttons leave the EXAMINE listing clear");
    const auto phone = layout_buttons(OverlayLayout::PhoneLandscape, 1248, 576, state);
    check(has(phone, ButtonId::AttackLeft) && has(phone, ButtonId::HandMenuRight),
          "in the phone's margins they stay");
    check(has(tablet, ButtonId::Look), "L stays so the player can leave the listing");
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
    auto outcome = resolve_tap(ButtonId::HandMenuLeft, state);
    check(!outcome.line.has_value(), "the hand menu alone has no finished line yet");
    check(hand_menu_opens("G") == PendingKind::FloorPicker && hand_menu_opens("P") == PendingKind::PackPicker &&
              hand_menu_opens("I") == PendingKind::IncantKeyboard && !hand_menu_opens("S"),
          "G, P and I lead on to the floor picker, pack picker and keyboard");

    const auto chosen = resolve_picker_choice(PendingKind::FloorPicker, false, "SWORD");
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

void test_picker_choices_follow_game_state() {
    using namespace dag::input;
    OverlayState state;
    state.floor_items = {"LEATHER SHIELD", "PINE TORCH", "FLASK"};
    state.pack_items = {"WOODEN SWORD"};
    check(picker_choices(PendingKind::FloorPicker, false, state) == state.floor_items,
          "the floor picker lists the floor names in order (Picker board)");
    check(picker_choices(PendingKind::PackPicker, true, state) == state.pack_items,
          "the pack picker lists the pack names");
    check(picker_choices(PendingKind::FloorPicker, false, OverlayState{}).empty(),
          "an empty floor gives an empty picker, not generic names");
    const std::vector<std::string> sdur{"S", "D", "U", "R"};
    const std::vector<std::string> gp{"G", "P"};
    check(picker_choices(PendingKind::HandMenu, true, state) == gp,
          "an empty hand's menu offers G and P when floor and pack hold something");
    check(picker_choices(PendingKind::HandMenu, true, OverlayState{}).empty(),
          "nothing to get or pull, nothing offered");
    state.right_hand_empty = state.left_hand_empty = false;
    check(picker_choices(PendingKind::HandMenu, true, state) == sdur,
          "the hand menu offers no I without a ring (HandStates board)");
    state.right_hand_ring = true;
    check(picker_choices(PendingKind::HandMenu, true, state).back() == "I",
          "a ring in that hand adds I");
    check(picker_choices(PendingKind::HandMenu, false, state) == sdur,
          "a ring in the other hand does not");
}

void test_choices_sit_beside_their_anchor() {
    using namespace dag::input;
    constexpr double vw = 844, vh = 390;  // the boards' 19.5:9 canvas
    // Picker board: G at (14,68) -> menu at left 68, top 68, width 200, rows 44.
    const auto floor = place_choices(PendingKind::FloorPicker,
                                     {"LEATHER SHIELD", "PINE TORCH", "FLASK"},
                                     Rect{14, 68, 48, 48}, vw, vh);
    check(floor.size() == 3 && floor[0].rect.x == 68 && floor[0].rect.y == 68 &&
              floor[0].rect.w == 200 && floor[2].rect.y == 68 + 2 * 44,
          "the floor picker is a column beside G, as on the Picker board");
    // Popup board: right "≡" at (782,68) -> S D U R I at 506..722, top 68.
    const auto hand = place_choices(PendingKind::HandMenu, {"S", "D", "U", "R", "I"},
                                    Rect{782, 68, 48, 48}, vw, vh);
    check(hand.size() == 5 && hand[0].rect.x == 506 && hand[4].rect.x == 722 &&
              hand[0].rect.y == 68,
          "the right hand menu is a row ending beside its ≡, as on the Popup board");
    // Climb board: C at (728,274) -> U at (674,220), D at (674,274).
    const auto climb = place_choices(PendingKind::ClimbChoice, {"U", "D"}, Rect{728, 274, 48, 48}, vw, vh);
    check(climb.size() == 2 && climb[0].rect.x == 674 && climb[0].rect.y == 220 &&
              climb[1].rect.y == 274,
          "climb's U over D sits beside C, as on the Climb board");
    // A long pack list near the bottom moves up to stay on screen.
    const auto pack = place_choices(PendingKind::PackPicker,
                                    {"A", "B", "C", "D", "E", "F", "G", "H"},
                                    Rect{14, 122, 48, 48}, vw, vh);
    check(pack.back().rect.y + pack.back().rect.h <= vh, "a long picker stays on screen");
    check(picker_anchor(PendingKind::PackPicker, true) == ButtonId::HandMenuRight &&
              picker_anchor(PendingKind::HandMenu, false) == ButtonId::HandMenuLeft &&
              picker_anchor(PendingKind::ClimbChoice, false) == ButtonId::Climb,
          "each picker is anchored to the button that opens it");
}

void test_keyboard_matches_incant_board() {
    using namespace dag::input;
    // The Incant board is 844x390; at that size a board unit is one pixel.
    const auto kb = keyboard_layout(OverlayLayout::PhoneLandscape, 844, 390);
    auto key = [&](const std::string& label) {
        for (const auto& k : kb.keys)
            if (k.label == label) return k.rect;
        return Rect{-1, -1, 0, 0};
    };
    check(kb.text_box.x == 184 && kb.text_box.y == 178 && kb.text_box.w == 476 && kb.text_box.h == 44,
          "the text box sits where the Incant board puts it");
    check(key("Q").x == 184 && key("Q").y == 236 && key("P").x == 616 && key("A").x == 208 &&
              key("A").y == 284 && key("Z").x == 256 && key("Z").y == 332 && key("M").x == 544,
          "the QWERTY rows match the Incant board");
    check(key("BACK").x == 592 && key("ENTER").x == 640 && key("CANCEL").x == 782 &&
              key("CANCEL").y == 328,
          "⌫ ↵ and ✕ match the Incant board");
    check(key("SPACE").x < 0, "the incant keyboard has no space");
    std::string typed;
    bool closed = false;
    for (int i = 0; i < 40; ++i) press_keyboard_key(PendingKind::IncantKeyboard, typed, "A", closed);
    check(typed.size() + 2 + 1 <= kMaxGestureLine, "typing stops at the gesture-line limit");
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
    test_each_hand_shows_attack_and_menu();
    test_examining_hides_top_buttons_over_the_picture();
    test_phone_follows_main_board();
    test_keyboard_matches_incant_board();
    test_climb_conditional();
    test_no_overlaps_within_a_layout();
    test_tablet_stays_clear_of_bottom_band();
    test_mouse_as_touch_hit_testing();
    test_tap_dispatch_for_simple_commands();
    test_tap_dispatch_for_pickers_and_menus();
    test_picker_choices_follow_game_state();
    test_choices_sit_beside_their_anchor();
    test_climb_confirmation();
    test_incant_keyboard_finish();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
