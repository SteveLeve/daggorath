#include "daggorath/overlay_bridge.hpp"

#include "daggorath/examine.hpp"
#include "daggorath/gesture.hpp"
#include "daggorath/population.hpp"

namespace dag::platform {

using dag::input::ButtonId;
using dag::input::PendingKind;

namespace {
constexpr std::uint8_t kClassRing = 1;  // CD.ASM K.RING

bool holds_ring(const dag::Game& game, int index) {
    return index >= 0 && game.objects()[static_cast<std::size_t>(index)].cls == kClassRing;
}
}  // namespace

dag::input::OverlayState overlay_state_from(const dag::Game& game) {
    const auto& player = game.player();
    dag::input::OverlayState state;
    state.left_hand_empty = player.left_hand < 0;
    state.right_hand_empty = player.right_hand < 0;
    state.left_hand_ring = holds_ring(game, player.left_hand);
    state.right_hand_ring = holds_ring(game, player.right_hand);
    state.examining = game.display_mode() == dag::DisplayMode::Examine;
    state.climb_available = dag::vfind(game.level_index(), player.row, player.col) >= 0;
    const dag::ExamineSnapshot exam = dag::examine_snapshot_from(game);
    state.floor_items = exam.floor;
    state.pack_items = exam.bag;
    return state;
}

const std::vector<dag::input::Button>& OverlayBridge::buttons(double viewport_w,
                                                               double viewport_h,
                                                               const dag::input::OverlayState& state) {
    state_ = state;
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
    if (*hit == ButtonId::SystemMenu) return false;
    if (*hit == ButtonId::Keyboard) {  // ⌨ opens the on-screen keyboard
        pending_ = PendingKind::FreeKeyboard;
        typed_.clear();
        return false;
    }

    const dag::input::TapOutcome outcome = dag::input::resolve_tap(*hit, state_);
    if (outcome.line) return press_line(*outcome.line, game);
    if (outcome.pending != PendingKind::None) {
        pending_ = outcome.pending;
        pending_right_hand_ = outcome.right_hand;
    }
    return false;
}

bool OverlayBridge::press_key(const std::string& key, dag::Game& game) {
    if (!keyboard_open()) return false;
    bool closed = false;
    const auto line = dag::input::press_keyboard_key(pending_, typed_, key, closed);
    if (closed) pending_ = PendingKind::None;
    return line && !line->empty() && press_line(*line, game);
}

bool OverlayBridge::handle_attack_tap(double x, double y, dag::Game& game) {
    const auto hit = dag::input::hit_test(buttons_, x, y);
    if (hit != ButtonId::AttackLeft && hit != ButtonId::AttackRight) return false;
    return press_line(dag::input::gesture_attack(hit == ButtonId::AttackRight), game);
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
