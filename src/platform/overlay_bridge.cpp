#include "daggorath/overlay_bridge.hpp"

#include "daggorath/gesture.hpp"

namespace dag::platform {

using dag::input::ButtonId;
using dag::input::PendingKind;

const std::vector<dag::input::Button>& OverlayBridge::buttons(double viewport_w,
                                                               double viewport_h,
                                                               const dag::input::OverlayState& state) {
    buttons_ = dag::input::layout_buttons(layout_, viewport_w, viewport_h, state);
    return buttons_;
}

bool OverlayBridge::press_line(const std::string& line, dag::Game& game) {
    // Enforces D-17's 31-character limit the same way GestureLine does,
    // without constructing the KeyEvent vector GestureLine builds for the
    // scripted-replay path — this bridge presses characters directly on a
    // live Game, matching how typed input already reaches it in
    // sdl_app.cpp's main loop (see the header comment on why: not a burst).
    if (line.size() + 1 > dag::input::kMaxGestureLine) return false;
    for (const char ch : line) game.press(static_cast<std::uint8_t>(ch));
    game.press(0x0D);
    return true;
}

bool OverlayBridge::handle_tap(double x, double y, dag::Game& game) {
    const auto hit = dag::input::hit_test(buttons_, x, y);
    if (!hit) return false;
    if (*hit == ButtonId::Keyboard || *hit == ButtonId::SystemMenu) return false;

    const dag::input::OverlayState dummy_state{};  // resolve_tap ignores state today
    const dag::input::TapOutcome outcome = dag::input::resolve_tap(*hit, dummy_state);
    if (outcome.line) return press_line(*outcome.line, game);
    if (outcome.pending != PendingKind::None) {
        pending_ = outcome.pending;
        pending_right_hand_ = outcome.right_hand;
    }
    return false;
}

bool OverlayBridge::resolve_choice(const std::string& choice, dag::Game& game) {
    if (pending_ == PendingKind::None) return false;
    const auto line = dag::input::resolve_picker_choice(pending_, pending_right_hand_, choice);
    if (!line) return false;  // e.g. HandMenu's "I": caller must open_incant_keyboard()
    const PendingKind finished = pending_;
    pending_ = PendingKind::None;
    if (!press_line(*line, game)) {
        // Length limit tripped (unreachable for today's GENTAB names/letters,
        // kept for INCANT's longer adjectives): leave the picker closed
        // rather than silently drop the state the player already committed.
        pending_ = finished;
        return false;
    }
    return true;
}

}  // namespace dag::platform
