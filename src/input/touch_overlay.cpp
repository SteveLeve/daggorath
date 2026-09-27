#include "daggorath/touch_overlay.hpp"

namespace dag::input {

namespace {

// This prototype's own layout constants (design choice, not source-derived):
// button and gap sizes as a fraction of the shorter viewport dimension, so
// both layouts scale to any device size.
double button_size(double viewport_w, double viewport_h) {
    return 0.12 * (viewport_w < viewport_h ? viewport_w : viewport_h);
}
double gap(double viewport_w, double viewport_h) {
    return 0.03 * (viewport_w < viewport_h ? viewport_w : viewport_h);
}

// Phone landscape (docs/design/touch-controls/README.md "Layout"): hand
// controls in the top corners' margins, movement/turn at bottom left,
// climb/examine/look/keyboard/system-menu at bottom right.
std::vector<Button> layout_phone(double vw, double vh, const OverlayState& state) {
    std::vector<Button> out;
    const double bs = button_size(vw, vh);
    const double gp = gap(vw, vh);
    const double margin = gp;

    // Top corners: A (always), then G/P or the hand menu below it.
    out.push_back({ButtonId::AttackLeft, {margin, margin, bs, bs}});
    out.push_back({ButtonId::AttackRight, {vw - margin - bs, margin, bs, bs}});
    const double below_a = margin + bs + gp;
    if (state.left_hand_empty) {
        out.push_back({ButtonId::GetLeft, {margin, below_a, bs, bs}});
        out.push_back({ButtonId::PullLeft, {margin, below_a + bs + gp, bs, bs}});
    } else {
        out.push_back({ButtonId::HandMenuLeft, {margin, below_a, bs, bs}});
    }
    if (state.right_hand_empty) {
        out.push_back({ButtonId::GetRight, {vw - margin - bs, below_a, bs, bs}});
        out.push_back(
            {ButtonId::PullRight, {vw - margin - bs, below_a + bs + gp, bs, bs}});
    } else {
        out.push_back({ButtonId::HandMenuRight, {vw - margin - bs, below_a, bs, bs}});
    }

    // Bottom left: movement arrows (a 2x2 cross-ish block), then turn arrows
    // beside them.
    const double bl_y = vh - margin - bs;
    out.push_back({ButtonId::MoveForward, {margin + bs + gp, bl_y - bs - gp, bs, bs}});
    out.push_back({ButtonId::MoveBack, {margin + bs + gp, bl_y, bs, bs}});
    out.push_back({ButtonId::MoveLeft, {margin, bl_y, bs, bs}});
    out.push_back({ButtonId::MoveRight, {margin + 2 * (bs + gp), bl_y, bs, bs}});
    const double turn_x = margin + 3 * (bs + gp);
    out.push_back({ButtonId::TurnLeft, {turn_x, bl_y, bs, bs}});
    out.push_back({ButtonId::TurnRight, {turn_x + bs + gp, bl_y, bs, bs}});
    out.push_back(
        {ButtonId::TurnAround, {turn_x + 2 * (bs + gp), bl_y - bs - gp, bs, bs}});

    // Bottom right: climb (when available), examine, look, keyboard, then
    // the system menu, right-aligned.
    double x = vw - margin - bs;
    const double br_y = vh - margin - bs;
    out.push_back({ButtonId::SystemMenu, {x, br_y, bs, bs}});
    x -= bs + gp;
    out.push_back({ButtonId::Keyboard, {x, br_y, bs, bs}});
    x -= bs + gp;
    out.push_back({ButtonId::Look, {x, br_y, bs, bs}});
    x -= bs + gp;
    out.push_back({ButtonId::Examine, {x, br_y, bs, bs}});
    if (state.climb_available) {
        x -= bs + gp;
        out.push_back({ButtonId::Climb, {x, br_y, bs, bs}});
    }
    return out;
}

// Tablet 4:3 (decision 2026-09-27): controls float over the left/right
// edges, never the bottom band (status/command lines). Hand controls sit in
// the mostly-empty upper corners; everything else runs down the same edges
// in the lower two-thirds, well clear of the bottom.
std::vector<Button> layout_tablet(double vw, double vh, const OverlayState& state) {
    std::vector<Button> out;
    // A smaller fraction than the phone layout: with up to 7 secondary
    // controls stacked per edge, a 2-column grid (below) keeps every
    // cluster well clear of the bottom band even on a compact 4:3 tablet.
    const double bs = 0.08 * (vw < vh ? vw : vh);
    const double gp = 0.02 * (vw < vh ? vw : vh);
    const double margin = gp;
    // Everything stays above this line: the bottom quarter is the
    // status/command band this layout must leave clear.
    const double bottom_band = vh * 0.75;

    out.push_back({ButtonId::AttackLeft, {margin, margin, bs, bs}});
    out.push_back({ButtonId::AttackRight, {vw - margin - bs, margin, bs, bs}});
    const double below_a = margin + bs + gp;
    if (state.left_hand_empty) {
        out.push_back({ButtonId::GetLeft, {margin, below_a, bs, bs}});
        out.push_back({ButtonId::PullLeft, {margin, below_a + bs + gp, bs, bs}});
    } else {
        out.push_back({ButtonId::HandMenuLeft, {margin, below_a, bs, bs}});
    }
    if (state.right_hand_empty) {
        out.push_back({ButtonId::GetRight, {vw - margin - bs, below_a, bs, bs}});
        out.push_back(
            {ButtonId::PullRight, {vw - margin - bs, below_a + bs + gp, bs, bs}});
    } else {
        out.push_back({ButtonId::HandMenuRight, {vw - margin - bs, below_a, bs, bs}});
    }

    // Movement/turn as a 2-column grid hugging the left edge; the
    // climb/examine/look/keyboard/menu cluster as a 2-column grid hugging
    // the right edge. Both stay clear of `bottom_band` by construction: two
    // columns of up to 4 rows fit comfortably above it on any tablet-sized
    // viewport, which a single-column stack of 7 would not.
    const double grid_top = below_a + 2 * (bs + gp) + gp;
    auto place_grid = [&](const std::vector<ButtonId>& ids, bool right_edge) {
        for (std::size_t i = 0; i < ids.size(); ++i) {
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);
            const double y = grid_top + row * (bs + gp);
            if (y + bs > bottom_band) break;  // stays clear of the band
            const double x = right_edge ? (vw - margin - bs - col * (bs + gp))
                                        : (margin + col * (bs + gp));
            out.push_back({ids[i], {x, y, bs, bs}});
        }
    };
    place_grid({ButtonId::MoveForward, ButtonId::MoveBack, ButtonId::MoveLeft,
                ButtonId::MoveRight, ButtonId::TurnLeft, ButtonId::TurnRight,
                ButtonId::TurnAround},
               false);
    std::vector<ButtonId> right_cluster;
    if (state.climb_available) right_cluster.push_back(ButtonId::Climb);
    right_cluster.push_back(ButtonId::Examine);
    right_cluster.push_back(ButtonId::Look);
    right_cluster.push_back(ButtonId::Keyboard);
    right_cluster.push_back(ButtonId::SystemMenu);
    place_grid(right_cluster, true);
    return out;
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
        case PendingKind::None:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace dag::input
