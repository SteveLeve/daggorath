#include "daggorath/touch_overlay.hpp"

#include <algorithm>

namespace dag::input {

namespace {

// Both layouts follow the Main (19.5:9) and Legacy (16:9) boards, which share
// one arrangement in board units: 48-unit buttons on a 54-unit pitch, 14 from
// the edges (docs/design/touch-controls/mockups/). Top corners: A over "≡"
// for each hand (Steve, 2026-09-28: G and P moved into "≡", so each hand
// shows only Attack and Menu). Bottom left: ⇤ ↑ ⇥ / ↶ ↻ ↷ / ↓. Bottom
// right: C over the E/L toggle, beside the system menu. There is no ⌨
// button (Steve, 2026-09-28): the keyboard opens only for INCANT. `unit`
// scales board units to pixels, `bottom` is the line the bottom clusters sit
// on, and [game_left, game_right) is where the game's 4:3 picture is drawn.
std::vector<Button> layout_boards(double vw, double unit, double bottom, double game_left,
                                  double game_right, const OverlayState& state) {
    std::vector<Button> out;
    const double bs = 48 * unit, step = 54 * unit, m = 14 * unit;
    const double right = vw - m - bs;
    auto at = [&](ButtonId id, double x, double y) { out.push_back({id, {x, y, bs, bs}}); };
    // While EXAMINE's listing is up, a top-corner button that would cover
    // the picture is left out (Steve, 2026-09-28); in the phone's margins
    // nothing is.
    auto top = [&](ButtonId id, double x, double y) {
        const bool over_picture = x < game_right && x + bs > game_left;
        if (!(state.examining && over_picture)) at(id, x, y);
    };

    top(ButtonId::AttackLeft, m, m);
    top(ButtonId::AttackRight, right, m);
    top(ButtonId::HandMenuLeft, m, m + step);
    top(ButtonId::HandMenuRight, right, m + step);

    const double row3 = bottom - m - bs, row2 = row3 - step, row1 = row2 - step;
    at(ButtonId::MoveLeft, m, row1);
    at(ButtonId::MoveForward, m + step, row1);
    at(ButtonId::MoveRight, m + 2 * step, row1);
    at(ButtonId::TurnLeft, m, row2);
    at(ButtonId::TurnAround, m + step, row2);
    at(ButtonId::TurnRight, m + 2 * step, row2);
    at(ButtonId::MoveBack, m + step, row3);

    // E and L toggle (Steve, 2026-09-28): E while the view shows, L while
    // the EXAMINE listing shows, in one slot. The boards draw both side by
    // side; the system menu, which no board draws, takes the other slot.
    at(state.examining ? ButtonId::Look : ButtonId::Examine, right - step, row3);
    at(ButtonId::SystemMenu, right, row3);
    if (state.climb_available) at(ButtonId::Climb, right - step, row2);
    return out;
}

// Phone landscape: the Main board, scaled by height but shrunk if needed so
// the three-button move cluster (176 units with its inner gap) fits the side
// margin beside the 4:3 game (Steve, 2026-09-28; the board itself overlaps
// by 8 units).
std::vector<Button> layout_phone(double vw, double vh, const OverlayState& state) {
    const double game_w = vh * 4.0 / 3.0;
    const double margin = (vw - game_w) / 2;
    const double unit = std::min(vh / 390.0, margin / 176.0);
    return layout_boards(vw, unit, vh, margin, margin + game_w, state);
}

// Tablet 4:3 (decision 2026-09-27): the same arrangement floating over the
// game's edges, at board size for a 576-high view, with the bottom clusters
// raised above the bottom quarter (the status/command band stays clear).
std::vector<Button> layout_tablet(double vw, double vh, const OverlayState& state) {
    return layout_boards(vw, vh / 576.0, vh * 0.75, 0, vw, state);
}

}  // namespace

std::vector<Button> layout_buttons(OverlayLayout layout, double viewport_w,
                                   double viewport_h, const OverlayState& state) {
    switch (layout) {
        case OverlayLayout::PhoneLandscape:
            return layout_phone(viewport_w, viewport_h, state);
        case OverlayLayout::Tablet4x3:
            return layout_tablet(viewport_w, viewport_h, state);
    }
    return {};
}

std::optional<ButtonId> hit_test(const std::vector<Button>& buttons, double x, double y) {
    std::optional<ButtonId> hit;
    for (const auto& button : buttons) {
        if (button.rect.contains(x, y)) hit = button.id;
    }
    return hit;
}

TapOutcome resolve_tap(ButtonId id, const OverlayState& state) {
    TapOutcome out;
    switch (id) {
        case ButtonId::AttackLeft:
            out.line = gesture_attack(false);
            break;
        case ButtonId::AttackRight:
            out.line = gesture_attack(true);
            break;
        case ButtonId::HandMenuLeft:
            out.pending = PendingKind::HandMenu;
            out.right_hand = false;
            break;
        case ButtonId::HandMenuRight:
            out.pending = PendingKind::HandMenu;
            out.right_hand = true;
            break;
        case ButtonId::MoveForward:
            out.line = gesture_move_forward();
            break;
        case ButtonId::MoveBack:
            out.line = gesture_move_back();
            break;
        case ButtonId::MoveLeft:
            out.line = gesture_move_left();
            break;
        case ButtonId::MoveRight:
            out.line = gesture_move_right();
            break;
        case ButtonId::TurnLeft:
            out.line = gesture_turn_left();
            break;
        case ButtonId::TurnRight:
            out.line = gesture_turn_right();
            break;
        case ButtonId::TurnAround:
            out.line = gesture_turn_around();
            break;
        case ButtonId::Climb:
            // Both directions are always offered as confirmation (design
            // doc); the caller shows a two-way choice next.
            out.pending = PendingKind::ClimbChoice;
            break;
        case ButtonId::Examine:
            out.line = gesture_examine();
            break;
        case ButtonId::Look:
            out.line = gesture_look();
            break;
        case ButtonId::SystemMenu:
            // The free command line and the system menu are not gesture
            // lines: the former hands control to the typed line, the latter
            // to the shell (ADR-0009), never through GestureLine.
            break;
    }
    (void)state;
    return out;
}

std::vector<std::string> picker_choices(PendingKind pending, bool right_hand,
                                        const OverlayState& state) {
    switch (pending) {
        case PendingKind::FloorPicker:
            return state.floor_items;
        case PendingKind::PackPicker:
            return state.pack_items;
        case PendingKind::HandMenu: {
            // An empty hand offers G when something is on the floor and P
            // when the pack holds something (Steve, 2026-09-28); a full hand
            // offers S D U R, plus I for a ring (HandStates board).
            const bool empty = right_hand ? state.right_hand_empty : state.left_hand_empty;
            if (empty) {
                std::vector<std::string> out;
                if (!state.floor_items.empty()) out.push_back("G");
                if (!state.pack_items.empty()) out.push_back("P");
                return out;
            }
            std::vector<std::string> out{"S", "D", "U", "R"};
            if (right_hand ? state.right_hand_ring : state.left_hand_ring) out.push_back("I");
            return out;
        }
        case PendingKind::ClimbChoice:
            return {"U", "D"};
        case PendingKind::IncantKeyboard:
        case PendingKind::None:
            return {};
    }
    return {};
}

namespace {
double layout_unit(OverlayLayout layout, double vh) {
    return layout == OverlayLayout::PhoneLandscape ? vh / 390.0 : vh / 576.0;
}  // the keyboard spans the middle, so the phone's margin fit doesn't apply
double layout_bottom(OverlayLayout layout, double vh) {
    return layout == OverlayLayout::PhoneLandscape ? vh : vh * 0.75;
}
}  // namespace

KeyboardLayout keyboard_layout(OverlayLayout layout, double vw, double vh) {
    const double u = layout_unit(layout, vh);
    const double key = 44 * u, pitch = 48 * u;
    const double row3 = layout_bottom(layout, vh) - 14 * u - key;
    const double row2 = row3 - pitch, row1 = row2 - pitch;
    const double left = vw / 2 - 238 * u;  // the board's 476-wide box, centred
    KeyboardLayout out;
    out.text_box = Rect{left, row1 - 58 * u, 476 * u, 44 * u};
    auto row = [&](const std::string& letters, double x, double y) {
        for (std::size_t i = 0; i < letters.size(); ++i)
            out.keys.push_back({std::string(1, letters[i]),
                                Rect{x + static_cast<double>(i) * pitch, y, key, key}});
    };
    row("QWERTYUIOP", left, row1);
    row("ASDFGHJKL", left + 24 * u, row2);
    row("ZXCVBNM", left + 72 * u, row3);
    out.keys.push_back({"BACK", Rect{left + 72 * u + 7 * pitch, row3, key, key}});
    out.keys.push_back({"ENTER", Rect{left + 72 * u + 8 * pitch, row3, key, key}});
    const double bs = 48 * u;
    out.keys.push_back({"CANCEL", Rect{vw - 14 * u - bs, layout_bottom(layout, vh) - 14 * u - bs, bs, bs}});
    return out;
}

std::optional<std::string> press_keyboard_key(PendingKind pending, std::string& typed,
                                              const std::string& key, bool& closed) {
    closed = false;
    if (key == "CANCEL") {
        typed.clear();
        closed = true;
        return std::nullopt;
    }
    if (key == "BACK") {
        if (!typed.empty()) typed.pop_back();
        return std::nullopt;
    }
    if (key == "ENTER") {
        closed = true;
        std::string line = gesture_incant(typed);
        typed.clear();
        return line;
    }
    (void)pending;
    if (typed.size() + 2 + 2 > kMaxGestureLine) return std::nullopt;  // "I " and CR
    if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') typed.push_back(key[0]);
    return std::nullopt;
}

std::string hand_verb_caption(const std::string& letter) {
    if (letter == "S") return "STOW";
    if (letter == "D") return "DROP";
    if (letter == "U") return "USE";
    if (letter == "R") return "REVEAL";
    if (letter == "I") return "INCANT";
    if (letter == "G") return "GET";
    if (letter == "P") return "PULL";
    return {};
}

std::optional<ButtonId> picker_anchor(PendingKind pending, bool right_hand) {
    switch (pending) {
        case PendingKind::FloorPicker:  // opened from "≡" (G/P live there now)
        case PendingKind::PackPicker:
        case PendingKind::HandMenu:
            return right_hand ? ButtonId::HandMenuRight : ButtonId::HandMenuLeft;
        case PendingKind::ClimbChoice:
            return ButtonId::Climb;
        case PendingKind::IncantKeyboard:
        case PendingKind::None:
            return std::nullopt;
    }
    return std::nullopt;
}

std::vector<Choice> place_choices(PendingKind pending, const std::vector<std::string>& labels,
                                  const Rect& anchor, double viewport_w, double viewport_h) {
    constexpr double kGap = 6, kSquare = 48, kStep = 54;
    constexpr double kMenuW = 200, kRowH = 44;
    const bool opens_right = anchor.x + anchor.w / 2 < viewport_w / 2;
    std::vector<Choice> out;
    const double n = static_cast<double>(labels.size());
    switch (pending) {
        case PendingKind::FloorPicker:
        case PendingKind::PackPicker: {
            const double x = opens_right ? anchor.x + anchor.w + kGap : anchor.x - kGap - kMenuW;
            double top = anchor.y;
            if (top + n * kRowH > viewport_h) top = std::max(0.0, viewport_h - n * kRowH);
            for (std::size_t i = 0; i < labels.size(); ++i)
                out.push_back({labels[i], Rect{x, top + static_cast<double>(i) * kRowH, kMenuW, kRowH}});
            break;
        }
        case PendingKind::HandMenu: {
            // Left hand: S D U R I reading outward from "≡". Right hand: the
            // same order, the row ending beside "≡" (Popup board).
            constexpr double kPopupGap = 12;  // Popup board: I ends at 770, "≡" at 782
            const double first = opens_right ? anchor.x + anchor.w + kPopupGap
                                             : anchor.x - kPopupGap - (n - 1) * kStep - kSquare;
            for (std::size_t i = 0; i < labels.size(); ++i)
                out.push_back({labels[i], Rect{first + static_cast<double>(i) * kStep, anchor.y,
                                               kSquare, kSquare}});
            break;
        }
        case PendingKind::ClimbChoice: {
            const double x = opens_right ? anchor.x + anchor.w + kGap : anchor.x - kGap - kSquare;
            const double top = anchor.y - (n - 1) * kStep;
            for (std::size_t i = 0; i < labels.size(); ++i)
                out.push_back({labels[i], Rect{x, top + static_cast<double>(i) * kStep, kSquare, kSquare}});
            break;
        }
        case PendingKind::IncantKeyboard:
        case PendingKind::None:
            break;
    }
    return out;
}

std::optional<PendingKind> hand_menu_opens(const std::string& choice) {
    if (choice == "G") return PendingKind::FloorPicker;
    if (choice == "P") return PendingKind::PackPicker;
    if (choice == "I") return PendingKind::IncantKeyboard;
    return std::nullopt;
}

std::optional<std::string> resolve_picker_choice(PendingKind pending, bool right_hand,
                                                 const std::string& choice) {
    switch (pending) {
        case PendingKind::FloorPicker:
            return gesture_get(right_hand, choice);
        case PendingKind::PackPicker:
            return gesture_pull(right_hand, choice);
        case PendingKind::HandMenu:
            if (choice == "S") return gesture_stow(right_hand);
            if (choice == "D") return gesture_drop(right_hand);
            if (choice == "U") return gesture_use(right_hand);
            if (choice == "R") return gesture_reveal(right_hand);
            return std::nullopt;  // G, P, I open the next picker (hand_menu_opens)
        case PendingKind::ClimbChoice:
            if (choice == "U") return gesture_climb_up();
            if (choice == "D") return gesture_climb_down();
            return std::nullopt;
        case PendingKind::IncantKeyboard:
            return gesture_incant(choice);
        case PendingKind::None:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace dag::input
