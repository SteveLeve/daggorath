// Player preferences the platform remembers between launches: the Video
// (pixel/crisp, ADR-0010) and Controls (phone/tablet) system-menu entries.
// Platform state only; nothing here reaches the core. Headless so the text
// format is unit-tested (tests/platform/storage_tests.cpp).
#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "daggorath/touch_overlay.hpp"

namespace dag::platform {

struct Prefs {
    bool crisp = false;
    // Unset until the player picks a layout; the caller's default applies.
    std::optional<dag::input::OverlayLayout> layout;
};

// "DODPREFS 1\nvideo=crisp\nlayout=phone\n". Line-based so a later field can
// be added without a version bump.
std::string serialize_prefs(const Prefs& prefs);

// Tolerant: a missing header, an unknown key or a bad value leaves that
// field at its default rather than failing the whole read.
Prefs parse_prefs(std::string_view text);

}  // namespace dag::platform
