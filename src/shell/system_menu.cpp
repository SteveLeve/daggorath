#include "daggorath/system_menu.hpp"

namespace dag::shell {

void MenuState::back_out(Shell& shell) {
    if (!shell.paused()) {
        shell.pause();
    } else if (shell.pending() != ConfirmKind::None) {
        shell.cancel();
    } else if (screen_ != MenuScreen::Top) {
        screen_ = MenuScreen::Top;
    } else {
        shell.resume();
    }
}

MenuEffect MenuState::press(Shell& shell, MenuKey key, std::size_t slot) {
    if (!shell.paused()) return MenuEffect::None;
    const ConfirmKind pending = shell.pending();
    if (pending != ConfirmKind::None) {
        if (key == MenuKey::No) {
            shell.cancel();
        } else if (key == MenuKey::Yes) {
            shell.confirm();
            if (pending == ConfirmKind::Restart || pending == ConfirmKind::Quit) {
                screen_ = MenuScreen::Top;
                return pending == ConfirmKind::Restart ? MenuEffect::Restart : MenuEffect::Quit;
            }
        }
        return MenuEffect::None;
    }
    if (screen_ == MenuScreen::Top) {
        if (key == MenuKey::Save) screen_ = MenuScreen::ChooseSave;
        else if (key == MenuKey::Load) screen_ = MenuScreen::ChooseLoad;
        else if (key == MenuKey::Restart) shell.request_restart();
        else if (key == MenuKey::Quit) shell.request_quit();
    } else if (key == MenuKey::Slot && slot < Shell::kSlotCount) {
        if (screen_ == MenuScreen::ChooseSave) shell.save_to_slot(slot);
        else shell.load_from_slot(slot);
        screen_ = MenuScreen::Top;
    }
    return MenuEffect::None;
}

}  // namespace dag::shell
