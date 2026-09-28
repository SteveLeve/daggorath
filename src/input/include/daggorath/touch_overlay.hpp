// Touch overlay prototype (Phase 8.4): pure layout and hit-testing, no SDL.
//
// This is this project's own design choice, not a source-derived claim: the
// button positions come from docs/design/touch-controls/README.md's agreed
// layout table. Mouse-as-touch means a click is treated exactly like a
// touch tap — the same Rect/hit_test path serves both, so the desktop
// prototype and a real touch device exercise identical logic.
//
// **Recorded obstacle (2026-09-27):** this sandbox has no SDL3 installed and
// no network path to build it from source in this session, so the actual
// on-screen rendering of these buttons in `src/platform/sdl_app.cpp`, and
// wiring the shell's system-menu button (ADR-0009) and the `crisp` style
// (ADR-0010) into it, are not built or tested here. That SDL platform work
// stays open; this module is deliberately headless so its layout math and
// gesture dispatch can be tested regardless.
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "daggorath/gesture.hpp"

namespace dag::input {

// The two layouts evaluated (docs/design/touch-controls/README.md, decisions
// 2026-09-27): a 19.5:9 phone in landscape, controls in the side margins and
// bottom corners; a 4:3 tablet, controls floating over the left/right edges
// of the game view, off the bottom (status/command lines stay clear).
enum class OverlayLayout { PhoneLandscape, Tablet4x3 };

enum class ButtonId {
    AttackLeft,
    AttackRight,
    GetLeft,
    GetRight,
    PullLeft,
    PullRight,
    HandMenuLeft,
    HandMenuRight,  // "≡": S D U R I, holding hand only
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    TurnLeft,
    TurnRight,
    TurnAround,
    Climb,  // offers C U / C D when available
    Examine,
    Look,
    Keyboard,    // free command line
    SystemMenu,  // the one control that pauses (ADR-0009)
};

struct Rect {
    double x = 0, y = 0, w = 0, h = 0;
    bool contains(double px, double py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

struct Button {
    ButtonId id;
    Rect rect;
};

// Which controls the design doc's "shown when" column makes conditional:
// G/P show for an empty hand, the "≡" hand menu for a holding one; C U/D
// only appears on a ladder or hole. Everything else in ButtonId is always
// present.
struct OverlayState {
    bool left_hand_empty = true;
    bool right_hand_empty = true;
    bool climb_available = false;
    // EXAMINE's listing is up: the E/L slot shows L (back to the view).
    bool examining = false;
    // Hands holding a ring: only then does "≡" offer I (HandStates board).
    bool left_hand_ring = false;
    bool right_hand_ring = false;
    // Names as EXAMINE lists them: objects on the player's cell (G's
    // picker) and in the pack (P's picker). Filled by the platform from the
    // running game (overlay_state_from, overlay_bridge.hpp).
    std::vector<std::string> floor_items;
    std::vector<std::string> pack_items;
};

// Computes every visible button's hit rectangle for one viewport and hand
// state. Pure geometry: caller decides how (or whether) to draw it.
std::vector<Button> layout_buttons(OverlayLayout layout, double viewport_w,
                                   double viewport_h, const OverlayState& state);

// Mouse-as-touch: which button, if any, a tap/click at (x, y) lands on.
// Later entries in `buttons` win on overlap, matching draw order (topmost
// last); `layout_buttons` never overlaps its own output.
std::optional<ButtonId> hit_test(const std::vector<Button>& buttons, double x, double y);

// The picker/menu state a tap on GetLeft/GetRight, PullLeft/PullRight, or a
// HandMenu button opens, per the design doc's "sequential entry" rule: a tap
// types its letters immediately (the partial line, e.g. "G L", is already
// authoritative) and a picker/keyboard supplies the rest before the whole
// gesture line is delivered as one jiffy-stamped burst (D-17).
enum class PendingKind {
    None,
    FloorPicker,
    PackPicker,
    HandMenu,
    IncantKeyboard,
    FreeKeyboard,  // ⌨: the whole command line typed on the on-screen keyboard
    ClimbChoice
};

struct TapOutcome {
    // Set when the tap alone completed a whole command line ready to
    // deliver (e.g. MoveForward, TurnLeft, Examine, Look, Climb's confirm).
    std::optional<std::string> line;
    // Set when the tap instead opened a picker/menu/keyboard that needs a
    // second selection before a line is ready.
    PendingKind pending = PendingKind::None;
    bool right_hand = false;  // which hand `pending` refers to
};

// Resolves one button tap into either a finished command line or a pending
// picker/menu, without touching SDL, the keyboard buffer, or a `Game`. The
// caller feeds a finished `line` to a `GestureLine` (gesture.hpp) the same
// way for every source.
TapOutcome resolve_tap(ButtonId id, const OverlayState& state);

// The choices a pending picker offers, in display order: floor or pack item
// names (Picker board: "Get left: on floor"), the hand menu's S D U R plus I
// for a ring, or climb's U D. Empty when there is nothing to choose.
std::vector<std::string> picker_choices(PendingKind pending, bool right_hand,
                                        const OverlayState& state);

// The button a pending picker opened from: G or P for the floor or pack
// picker, "≡" for the hand menu, C for climb. nullopt for the keyboard.
std::optional<ButtonId> picker_anchor(PendingKind pending, bool right_hand);

struct Choice {
    std::string label;
    Rect rect;
};

// Places a picker's choices beside its anchor button, on the side facing the
// middle of the viewport, as the mockup boards draw them:
//  - floor/pack (Picker board): a 200-wide column of 44-high rows whose top
//    lines up with the anchor, moved up if it would run off the bottom;
//  - hand menu (Popup board): a row of 48-square letters level with "≡";
//  - climb (Climb board): a column of U over D ending level with C.
// The gap to the anchor is 6 (Picker, Climb boards) or 12 (Popup board).
// The mirrored cases (a left-hand menu, a picker opening leftward) and the
// upward shift for long lists are extrapolated; no board draws them.
std::vector<Choice> place_choices(PendingKind pending, const std::vector<std::string>& labels,
                                  const Rect& anchor, double viewport_w, double viewport_h);

// The on-screen keyboard (Incant board): a text box over three QWERTY rows of
// 44-unit keys on a 48-unit pitch, centred, the last row ending 14 above the
// bottom line; then ⌫ ("BACK") and ↵ ("ENTER") after M, and ✕ ("CANCEL") in
// the bottom-right corner. The free command line also needs a space
// ("SPACE", before Z); the Incant board has none. Same units as layout_buttons.
struct KeyboardLayout {
    Rect text_box;
    std::vector<Choice> keys;
};
KeyboardLayout keyboard_layout(OverlayLayout layout, double viewport_w, double viewport_h,
                               bool with_space);

// One key on the open keyboard, applied to `typed`. Returns the finished
// command line on ENTER (INCANT's "I <word>", or the free line as typed) and
// sets `closed` on ENTER or CANCEL. Letters stop at kMaxGestureLine.
std::optional<std::string> press_keyboard_key(PendingKind pending, std::string& typed,
                                              const std::string& key, bool& closed);

// Finishes a pending picker: the floor/pack picker's chosen object name, or
// the hand-menu's chosen verb letter ('S' stow, 'D' drop, 'U' use, 'R'
// reveal — 'I' opens the incant keyboard instead of finishing here).
std::optional<std::string> resolve_picker_choice(PendingKind pending, bool right_hand,
                                                 const std::string& choice);

}  // namespace dag::input
