#include "daggorath/touch_overlay.hpp"

#include <algorithm>

namespace dag::input {

namespace {

// Both layouts follow the Main (19.5:9) and Legacy (16:9) boards, which share
// one arrangement in board units: 48-unit buttons on a 54-unit pitch, 14 from
// the edges (docs/design/touch-controls/mockups/). Top corners: A, then G and
// P or "≡". Bottom left: ⇤ ↑ ⇥ / ↶ ↻ ↷ / ↓. Bottom right: C over the E/L
// toggle, ⌨ over the system menu. `unit` scales board units to pixels and
// `bottom` is the line the bottom clusters sit on.
std::vector<Button> layout_boards(double vw, double unit, double bottom, const OverlayState& state) {
    std::vector<Button> out;
    const double bs = 48 * unit, step = 54 * unit, m = 14 * unit;
    auto at = [&](ButtonId id, double x, double y) { out.push_back({id, {x, y, bs, bs}}); };
    const double right = vw - m - bs;

    at(ButtonId::AttackLeft, m, m);
    at(ButtonId::AttackRight, right, m);
    if (state.left_hand_empty) {
        at(ButtonId::GetLeft, m, m + step);
        at(ButtonId::PullLeft, m, m + 2 * step);
    } else {
        at(ButtonId::HandMenuLeft, m, m + step);
    }
    if (state.right_hand_empty) {
        at(ButtonId::GetRight, right, m + step);
        at(ButtonId::PullRight, right, m + 2 * step);
    } else {
        at(ButtonId::HandMenuRight, right, m + step);
    }

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
    at(ButtonId::Keyboard, right, row2);
    if (state.climb_available) at(ButtonId::Climb, right - step, row2);
    return out;
}

// Phone landscape: the Main board, scaled by height; controls sit in the
// side margins beside the 4:3 game.
std::vector<Button> layout_phone(double vw, double vh, const OverlayState& state) {
    return layout_boards(vw, vh / 390.0, vh, state);
}

// Tablet 4:3 (decision 2026-09-27): the same arrangement floating over the
// game's edges, at board size for a 576-high view, with the bottom clusters
// raised above the bottom quarter (the status/command band stays clear).
std::vector<Button> layout_tablet(double vw, double vh, const OverlayState& state) {
    return layout_boards(vw, vh / 576.0, vh * 0.75, state);
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
        case ButtonId::GetLeft:
            out.pending = PendingKind::FloorPicker;
            out.right_hand = false;
            break;
        case ButtonId::GetRight:
            out.pending = PendingKind::FloorPicker;
            out.right_hand = true;
            break;
        case ButtonId::PullLeft:
            out.pending = PendingKind::PackPicker;
            out.right_hand = false;
            break;
        case ButtonId::PullRight:
            out.pending = PendingKind::PackPicker;
            out.right_hand = true;
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
        case ButtonId::Keyboard:
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
            std::vector<std::string> out{"S", "D", "U", "R"};
            if (right_hand ? state.right_hand_ring : state.left_hand_ring) out.push_back("I");
            return out;
        }
        case PendingKind::ClimbChoice:
            return {"U", "D"};
        case PendingKind::IncantKeyboard:
        case PendingKind::FreeKeyboard:
        case PendingKind::None:
            return {};
    }
    return {};
}

namespace {
double layout_unit(OverlayLayout layout, double vh) {
    return layout == OverlayLayout::PhoneLandscape ? vh / 390.0 : vh / 576.0;
}
double layout_bottom(OverlayLayout layout, double vh) {
    return layout == OverlayLayout::PhoneLandscape ? vh : vh * 0.75;
}
}  // namespace

KeyboardLayout keyboard_layout(OverlayLayout layout, double vw, double vh, bool with_space) {
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
    if (with_space) out.keys.push_back({"SPACE", Rect{left + 24 * u, row3, key, key}});
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
        std::string line = pending == PendingKind::IncantKeyboard ? gesture_incant(typed) : typed;
        typed.clear();
        return line;
    }
    const std::size_t prefix = pending == PendingKind::IncantKeyboard ? 2 : 0;  // "I "
    if (prefix + typed.size() + 2 > kMaxGestureLine) return std::nullopt;  // room for CR
    if (key == "SPACE") typed.push_back(' ');
    else if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') typed.push_back(key[0]);
    return std::nullopt;
}

std::optional<ButtonId> picker_anchor(PendingKind pending, bool right_hand) {
    switch (pending) {
        case PendingKind::FloorPicker:
            return right_hand ? ButtonId::GetRight : ButtonId::GetLeft;
        case PendingKind::PackPicker:
            return right_hand ? ButtonId::PullRight : ButtonId::PullLeft;
        case PendingKind::HandMenu:
            return right_hand ? ButtonId::HandMenuRight : ButtonId::HandMenuLeft;
        case PendingKind::ClimbChoice:
            return ButtonId::Climb;
        case PendingKind::IncantKeyboard:
        case PendingKind::FreeKeyboard:
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
        case PendingKind::FreeKeyboard:
        case PendingKind::None:
            break;
    }
    return out;
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
            return std::nullopt;  // "I" opens the incant keyboard instead
        case PendingKind::ClimbChoice:
            if (choice == "U") return gesture_climb_up();
            if (choice == "D") return gesture_climb_down();
            return std::nullopt;
        case PendingKind::IncantKeyboard:
            return gesture_incant(choice);
        case PendingKind::FreeKeyboard:
            return choice;
        case PendingKind::None:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace dag::input
