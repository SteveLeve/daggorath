// Touch overlay <-> Game bridge (Phase 8.6.1): the one piece of the on-screen
// touch UI's runtime state that stays headless-testable, per ADR-0009's
// architecture table (docs/adr/0009-layering-and-shell.md): src/shell earns
// its own target "once it has non-SDL logic worth testing headlessly" —
// true here too. This does not draw anything and does not touch SDL; sdl_app.cpp owns
// the viewport size, the button rects' pixels, and the picker's on-screen
// choices. It links daggorath::input (touch_overlay.hpp, gesture.hpp) and
// daggorath::core (Game::press), nothing else.
//
// What this adds beyond touch_overlay.hpp: touch_overlay's resolve_tap and
// resolve_picker_choice are pure functions of one tap; something has to hold
// the "a picker is open, waiting for its second tap" state across frames,
// and turn a finished command line into keystrokes on a real, running Game.
//
// This deliberately does NOT use dag::input::GestureLine's same-jiffy burst
// (D-17): that burst is built via Game::load_script, which replaces the
// game's whole pending-keystroke script and is meant for pre-loaded,
// scripted replay (tests/input/replay_equivalence_tests.cpp), not live
// append to a game already mid-run — overwriting `script_` here could drop
// any not-yet-consumed keystrokes the player already queued. Instead this
// presses each character with `Game::press()`, one keystroke per jiffy at
// or after the current clock — the exact mechanism sdl_app.cpp's typed-input
// path already uses. A real device's touch line therefore paces like fast
// typing, not an idealised single-jiffy burst; this is a platform choice for
// live interaction, recorded here rather than smoothed over, and distinct
// from GestureLine's scripted-replay contract.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/game.hpp"
#include "daggorath/touch_overlay.hpp"

namespace dag::platform {

// The overlay's view of the running game: hands, ring-in-hand, whether a
// ladder or hole is on this cell (VFIND, as CLIMB checks), and the floor and
// pack names EXAMINE would list (examine_snapshot_from). Read-only.
dag::input::OverlayState overlay_state_from(const dag::Game& game);

class OverlayBridge {
public:
    explicit OverlayBridge(dag::input::OverlayLayout layout) : layout_(layout) {}

    dag::input::OverlayLayout layout() const { return layout_; }
    void set_layout(dag::input::OverlayLayout layout) { layout_ = layout; }

    // Recomputes the current frame's button rects for a viewport size and
    // hand state. Call once per frame before hit-testing or drawing;
    // `state` changes with the player's hands and whether climb is offered.
    const std::vector<dag::input::Button>& buttons(double viewport_w, double viewport_h,
                                                     const dag::input::OverlayState& state);

    // True while a picker/menu/keyboard is open: the caller renders its
    // choices instead of the main button layout, and routes the next tap to
    // resolve_choice (or open_incant_keyboard/cancel_picker) instead of here.
    bool picker_open() const { return pending_ != dag::input::PendingKind::None; }
    dag::input::PendingKind pending() const { return pending_; }
    bool pending_right_hand() const { return pending_right_hand_; }

    // Hit-tests (x, y) against the last buttons() computed. A tap that
    // resolves to a finished line presses its keys on `game` (returns true),
    // one `Game::press()` call per character exactly as a physical
    // keystroke would (Game::press's own doc: "next free jiffy at or after
    // the current clock" — the same live pacing typed input already uses in
    // sdl_app.cpp, not GestureLine's scripted same-jiffy burst, which is
    // built for pre-loaded replay via Game::load_script, not live append to
    // a running Game). A tap that opens a picker updates pending()/
    // pending_right_hand() and returns false (nothing pressed yet). A tap
    // that misses every button, or lands on Keyboard/SystemMenu (not this
    // bridge's job — the free command line and the shell handle those
    // directly), returns false and changes nothing.
    bool handle_tap(double x, double y, dag::Game& game);

    // Finishes an open picker with `choice`: a GENTAB object name for
    // FloorPicker/PackPicker, a hand-menu letter ('S'/'D'/'U'/'R') for
    // HandMenu, or "U"/"D" for ClimbChoice. Presses the resulting gesture
    // line's keys and closes the picker (returns true). HandMenu's "I"
    // does not resolve here — the caller must see it and call
    // open_incant_keyboard() instead. Returns false, unchanged, if no
    // picker is open or `choice` doesn't resolve.
    bool resolve_choice(const std::string& choice, dag::Game& game);

    // HandMenu's "I": opens the incant keyboard pending state directly
    // (skipping resolve_choice, since "I" itself is not a finishable
    // choice — see resolve_picker_choice(HandMenu, ...) returning nullopt
    // for it).
    void open_incant_keyboard() { pending_ = dag::input::PendingKind::IncantKeyboard; }

    // Closes any open picker without pressing anything (a picker's "cancel").
    void cancel_picker() { pending_ = dag::input::PendingKind::None; }

private:
    bool press_line(const std::string& line, dag::Game& game);

    dag::input::OverlayLayout layout_;
    std::vector<dag::input::Button> buttons_;
    dag::input::PendingKind pending_ = dag::input::PendingKind::None;
    bool pending_right_hand_ = false;
};

}  // namespace dag::platform
