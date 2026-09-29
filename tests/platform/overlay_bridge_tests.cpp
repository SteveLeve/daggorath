// Phase 8.6.1: OverlayBridge closes the gap touch_overlay.hpp's own header
// comment names -- "the actual on-screen rendering... are not built or
// tested here" -- for the tap-to-keystroke half (sdl_app.cpp's rendering is
// verified separately, by screenshot). This proves a simulated tap sequence
// (hit-testing the same rects sdl_app.cpp draws) presses the identical
// characters, in the same order, that typing the equivalent command by hand
// would -- via the same live `Game::press()` mechanism sdl_app.cpp's typed
// keyboard path already uses (see overlay_bridge.hpp for why this bridge
// does not use GestureLine's scripted same-jiffy burst).
#include <iostream>
#include <sstream>
#include <string>

#include "daggorath/game.hpp"
#include "daggorath/overlay_bridge.hpp"
#include "daggorath/shell.hpp"

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

std::string render_trace(const dag::Game& game) {
    std::ostringstream os;
    os << "# jiffy\tclock\tevent\tdetail\n";
    for (const auto& e : game.trace()) os << e.to_line() << "\n";
    os << "# final\trow=" << game.player().row << "\tcol=" << game.player().col
       << "\tdir=" << static_cast<int>(game.player().dir)
       << "\tdamage=" << game.player().damage << "\n";
    return os.str();
}

// A fresh game with `line` typed by hand, one Game::press() per character
// plus the terminating CR -- the same live mechanism sdl_app.cpp's typed
// keyboard path already uses. This is the oracle every tap sequence below
// must match: "a tap composes and delivers the same keystrokes typing the
// command would."
dag::Game typed_reference(const std::string& line, std::uint64_t jiffies) {
    dag::Game game;
    for (const char ch : line) game.press(static_cast<std::uint8_t>(ch));
    game.press(0x0D);
    game.advance_jiffies(jiffies);
    return game;
}

// Center of the named button's hit rectangle, so tests tap wherever
// layout_buttons() actually places a control rather than a hardcoded pixel
// guess -- the same rects sdl_app.cpp will draw and hit-test against.
std::pair<double, double> center_of(const std::vector<dag::input::Button>& buttons,
                                   dag::input::ButtonId id) {
    for (const auto& b : buttons) {
        if (b.id == id) return {b.rect.x + b.rect.w / 2, b.rect.y + b.rect.h / 2};
    }
    return {-1, -1};
}

constexpr double kViewportW = 768.0;  // dod's fixed window: kScreenWidth * 3
constexpr double kViewportH = 576.0;  // kScreenHeight * 3

void test_move_forward_tap_matches_typed() {
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::MoveForward);
    check(x >= 0, "MoveForward has a hit rectangle in the tablet layout");

    dag::Game tapped;
    check(bridge.handle_tap(x, y, tapped),
          "tapping MoveForward's rect presses a finished line immediately");
    check(!bridge.picker_open(), "MoveForward never opens a picker");
    tapped.advance_jiffies(200);

    check(render_trace(tapped) == render_trace(typed_reference("M", 200)),
          "a MoveForward tap matches typing \"M\\r\" by hand, byte for byte");
}

void test_taps_after_system_menu_load() {
    for (const auto id : {dag::input::ButtonId::MoveForward,
                          dag::input::ButtonId::AttackLeft}) {
        dag::Game game;
        dag::shell::Shell shell(game);
        dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
        const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
        const auto [x, y] = center_of(buttons, id);
        shell.tick(20);
        shell.pause();
        check(shell.save_to_slot(0), "system menu saves before the clock advances");
        shell.resume();
        shell.tick(80);
        // Leave live keys queued on the later timeline when Load rewinds it.
        for (int i = 0; i < 100; ++i) game.press('X');
        shell.pause();
        check(shell.load_from_slot(0), "system menu restores the saved slot");
        shell.resume();
        check(game.counters().total_jiffies == 20, "load rewinds the clock");

        const std::size_t before = game.trace().size();
        check(bridge.handle_tap(x, y, game), "touch button queues a command after load");
        shell.tick(35);
        const auto expected = id == dag::input::ButtonId::MoveForward ? "MOVE" : "SOUND";
        bool observed = false;
        for (std::size_t i = before; i < game.trace().size(); ++i)
            if (game.trace()[i].kind == expected) observed = true;
        check(observed, "touch command executes promptly after load",
              id == dag::input::ButtonId::MoveForward ? "MoveForward" : "AttackLeft");
    }
}

void test_turn_right_tap_matches_typed() {
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::TurnRight);
    check(x >= 0, "TurnRight has a hit rectangle in the tablet layout");

    dag::Game tapped;
    check(bridge.handle_tap(x, y, tapped), "tapping TurnRight presses a finished line");
    tapped.advance_jiffies(200);

    check(render_trace(tapped) == render_trace(typed_reference("T R", 200)),
          "a TurnRight tap matches typing \"T R\\r\" by hand, byte for byte");
}

void test_examine_tap_matches_typed() {
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::Examine);
    check(x >= 0, "Examine has a hit rectangle in the tablet layout");

    dag::Game tapped;
    check(bridge.handle_tap(x, y, tapped), "tapping Examine presses a finished line");
    tapped.advance_jiffies(200);

    check(render_trace(tapped) == render_trace(typed_reference("E", 200)),
          "an Examine tap matches typing \"E\\r\" by hand, byte for byte");
}

void test_empty_hand_menu_does_not_open() {
    // Empty hand, nothing on the floor, empty pack: ≡ has nothing to offer.
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::HandMenuLeft);
    dag::Game game;
    check(!bridge.handle_tap(x, y, game) && !bridge.picker_open(),
          "an empty hand with nothing to get or pull opens no menu");
}

void test_get_left_through_hand_menu() {
    // G lives in the empty hand's "≡" menu (Steve, 2026-09-28): ≡, then G,
    // then the item finishes the line.
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    dag::input::OverlayState floor;
    floor.floor_items = {"PINE TORCH"};
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, floor);
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::HandMenuLeft);
    check(x >= 0, "the left hand's menu has a hit rectangle");

    dag::Game tapped;
    check(!bridge.handle_tap(x, y, tapped) && bridge.pending() == dag::input::PendingKind::HandMenu,
          "≡ opens the hand menu, pressing nothing");
    check(!bridge.resolve_choice("G", tapped) &&
              bridge.pending() == dag::input::PendingKind::FloorPicker && !bridge.pending_right_hand(),
          "G in the menu opens the left hand's floor picker");
    check(bridge.resolve_choice("TORCH", tapped) && !bridge.picker_open(),
          "choosing TORCH finishes the line and closes the picker");
    tapped.advance_jiffies(200);
    check(render_trace(tapped) == render_trace(typed_reference("G L TORCH", 200)),
          "≡ G TORCH matches typing \"G L TORCH\\r\" by hand, byte for byte");
}

void test_overlay_state_from_game() {
    dag::Game game;
    game.advance_jiffies(5);
    const auto state = dag::platform::overlay_state_from(game);
    check(state.left_hand_empty && state.right_hand_empty, "both hands start empty");
    check(state.pack_items.size() == 2, "the pack picker lists the starting sword and torch",
          std::to_string(state.pack_items.size()));
    check(state.floor_items.empty(), "nothing lies on the starting cell");
    check(!state.left_hand_ring && !state.right_hand_ring, "no ring is held at the start");

    // Right ≡ -> P -> the first listed name matches typing P R <name>.
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, state);
    const auto [x, y] = center_of(buttons, dag::input::ButtonId::HandMenuRight);
    dag::Game tapped;
    bridge.handle_tap(x, y, tapped);
    check(dag::input::picker_choices(bridge.pending(), true, state) == std::vector<std::string>{"P"},
          "with nothing on the floor the empty hand's menu offers only P");
    bridge.resolve_choice("P", tapped);
    const auto choices = dag::input::picker_choices(bridge.pending(), bridge.pending_right_hand(), state);
    check(!choices.empty() && bridge.resolve_choice(choices.front(), tapped),
          "the pack picker's first name finishes a PULL");
    tapped.advance_jiffies(200);
    const auto after = dag::platform::overlay_state_from(tapped);
    check(!after.right_hand_empty && after.pack_items.size() == 1,
          "the pulled item leaves the pack for the right hand");
    if (!choices.empty())
        check(render_trace(tapped) == render_trace(typed_reference("P R " + choices.front(), 200)),
              "≡ P -> first pack name matches typing it by hand");
}

void test_miss_and_system_menu_are_not_this_bridge() {
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    dag::Game game;
    const std::string baseline = render_trace(game);
    check(!bridge.handle_tap(-1000, -1000, game), "a tap off every button presses nothing");
    check(render_trace(game) == baseline, "a missed tap leaves the trace unchanged");

    const auto [mx, my] = center_of(buttons, dag::input::ButtonId::SystemMenu);
    check(!bridge.handle_tap(mx, my, game),
          "SystemMenu belongs to the shell (8.6.2), not this bridge");
    check(!bridge.picker_open(), "SystemMenu opens no picker here");
    check(render_trace(game) == baseline, "a SystemMenu tap presses nothing");
}

void test_incant_keyboard() {
    // Only INCANT opens the keyboard (Steve, 2026-09-28): ≡ I on a ring.
    dag::platform::OverlayBridge bridge(dag::input::OverlayLayout::Tablet4x3);
    const auto& buttons = bridge.buttons(kViewportW, kViewportH, {});
    dag::Game tapped;
    bridge.open_incant_keyboard();
    for (const char* key : {"F", "X", "BACK", "I"}) bridge.press_key(key, tapped);
    check(bridge.typed() == "FI", "letters and BACK edit the text box");

    dag::Game cancelled;
    const std::string before = render_trace(cancelled);
    bridge.press_key("CANCEL", cancelled);
    check(!bridge.keyboard_open() && bridge.typed().empty() && render_trace(cancelled) == before,
          "CANCEL closes the keyboard pressing nothing");

    const auto [fx, fy] = center_of(buttons, dag::input::ButtonId::MoveForward);
    dag::Game attacked;
    bridge.open_incant_keyboard();
    bridge.press_key("F", attacked);
    const auto [ax, ay] = center_of(buttons, dag::input::ButtonId::AttackLeft);
    check(bridge.handle_attack_tap(ax, ay, attacked) && bridge.keyboard_open() && bridge.typed() == "F",
          "A stays live over the keyboard and leaves the typed text alone");
    check(!bridge.handle_attack_tap(fx, fy, attacked), "other buttons stay dead over the keyboard");
    bridge.cancel_picker();
    attacked.advance_jiffies(200);
    check(render_trace(attacked) == render_trace(typed_reference("A L", 200)),
          "A over the keyboard matches typing A L");

    dag::Game incant;
    bridge.open_incant_keyboard();
    for (const char* key : {"F", "I", "R", "E"}) bridge.press_key(key, incant);
    bridge.press_key("ENTER", incant);
    incant.advance_jiffies(200);
    check(render_trace(incant) == render_trace(typed_reference("I FIRE", 200)),
          "the incant keyboard types I <word>");
}

}  // namespace

int main() {
    test_move_forward_tap_matches_typed();
    test_taps_after_system_menu_load();
    test_turn_right_tap_matches_typed();
    test_examine_tap_matches_typed();
    test_empty_hand_menu_does_not_open();
    test_get_left_through_hand_menu();
    test_miss_and_system_menu_are_not_this_bridge();
    test_overlay_state_from_game();
    test_incant_keyboard();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
