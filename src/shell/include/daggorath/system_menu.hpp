// System-menu input state (Phase 8.6.6, ADR-0009 §6): which sub-screen is
// showing and how a menu key or the back-out gesture moves between them, on
// top of Shell's own confirmation state. Headless, like Shell, so the menu's
// navigation is tested without a window (tests/shell/system_menu_tests.cpp);
// sdl_app.cpp maps SDL keys and the SystemMenu tap to these calls and draws
// the screen. The Shell is passed per call, not held, because a confirmed
// Restart replaces it.
#pragma once
#include <cstddef>

#include "daggorath/shell.hpp"

namespace dag::shell {

enum class MenuScreen { Top, ChooseSave, ChooseLoad };

enum class MenuKey { Save, Load, Restart, Quit, Yes, No, Slot };

// What the caller must do after a key: a confirmed Restart or Quit leaves the
// shell's hands (the caller rebuilds the game or ends the loop).
enum class MenuEffect { None, Restart, Quit };

class MenuState {
public:
    MenuScreen screen() const { return screen_; }

    // Esc and the SystemMenu tap: pause if running; otherwise cancel a pending
    // confirmation, then leave a sub-screen, then resume -- one level per call,
    // so reopening the menu never shows a stale confirmation.
    void back_out(Shell& shell);

    // A key while paused. `slot` (0-based) is read only for MenuKey::Slot.
    MenuEffect press(Shell& shell, MenuKey key, std::size_t slot = 0);

private:
    MenuScreen screen_ = MenuScreen::Top;
};

}  // namespace dag::shell
