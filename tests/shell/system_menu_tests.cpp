// Phase 8.6.6 system-menu navigation (ADR-0009 §6), headless: the key
// handling sdl_app.cpp used to hold inline, driven here against a real Shell.
#include <cstdint>
#include <iostream>
#include <string>

#include "daggorath/game.hpp"
#include "daggorath/shell.hpp"
#include "daggorath/system_menu.hpp"

namespace {

using dag::shell::ConfirmKind;
using dag::shell::MenuEffect;
using dag::shell::MenuKey;
using dag::shell::MenuScreen;
using dag::shell::MenuState;

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what << "\n";
    }
}

void test_back_out_one_level_at_a_time() {
    dag::Game game;
    dag::shell::Shell shell(game);
    MenuState menu;
    menu.back_out(shell);
    check(shell.paused(), "back-out while running pauses");
    menu.press(shell, MenuKey::Save);
    check(menu.screen() == MenuScreen::ChooseSave, "S opens the save slots");
    shell.tick(5);
    shell.save_to_slot(0);
    menu.press(shell, MenuKey::Slot, 0);
    check(shell.pending() == ConfirmKind::SaveOverwrite, "an occupied slot asks to overwrite");
    menu.back_out(shell);
    check(shell.pending() == ConfirmKind::None && shell.paused(),
          "first back-out cancels the confirmation only");
    menu.press(shell, MenuKey::Save);
    menu.back_out(shell);
    check(menu.screen() == MenuScreen::Top && shell.paused(),
          "next back-out leaves the sub-screen only");
    menu.back_out(shell);
    check(!shell.paused(), "back-out at the top screen resumes");
}

void test_keys_ignored_while_running() {
    dag::Game game;
    dag::shell::Shell shell(game);
    MenuState menu;
    check(menu.press(shell, MenuKey::Quit) == MenuEffect::None && shell.pending() == ConfirmKind::None,
          "menu keys do nothing while the game runs");
}

void test_save_load_round_trip_through_every_slot() {
    for (std::size_t slot = 0; slot < dag::shell::Shell::kSlotCount; ++slot) {
        dag::Game game;
        dag::shell::Shell shell(game);
        MenuState menu;
        shell.tick(20);
        const auto saved = game.snapshot();
        menu.back_out(shell);
        menu.press(shell, MenuKey::Save);
        menu.press(shell, MenuKey::Slot, slot);
        check(shell.slots()[slot].occupied() && menu.screen() == MenuScreen::Top,
              "saving to slot " + std::to_string(slot + 1) + " fills it and returns to the top");
        menu.back_out(shell);
        shell.tick(30);
        menu.back_out(shell);
        menu.press(shell, MenuKey::Load);
        menu.press(shell, MenuKey::Slot, slot);
        check(game.snapshot() == saved,
              "loading slot " + std::to_string(slot + 1) + " restores the saved game");
    }
}

void test_overwrite_confirmation() {
    dag::Game game;
    dag::shell::Shell shell(game);
    MenuState menu;
    menu.back_out(shell);
    menu.press(shell, MenuKey::Save);
    menu.press(shell, MenuKey::Slot, 2);
    const auto first = shell.slots()[2].bytes;
    menu.back_out(shell);
    shell.tick(25);
    menu.back_out(shell);
    menu.press(shell, MenuKey::Save);
    menu.press(shell, MenuKey::Slot, 2);
    check(shell.pending() == ConfirmKind::SaveOverwrite, "occupied slot asks first");
    menu.press(shell, MenuKey::No);
    check(shell.slots()[2].bytes == first, "N keeps the old save");
    menu.press(shell, MenuKey::Save);
    menu.press(shell, MenuKey::Slot, 2);
    check(menu.press(shell, MenuKey::Yes) == MenuEffect::None, "Y on overwrite is handled by the shell");
    check(shell.slots()[2].bytes != first, "Y overwrites the slot");
}

void test_restart_and_quit_confirmation() {
    dag::Game game;
    dag::shell::Shell shell(game);
    MenuState menu;
    menu.back_out(shell);
    menu.press(shell, MenuKey::Restart);
    check(shell.pending() == ConfirmKind::Restart, "X asks to confirm Restart");
    check(menu.press(shell, MenuKey::No) == MenuEffect::None && shell.pending() == ConfirmKind::None,
          "N cancels Restart");
    menu.press(shell, MenuKey::Restart);
    check(menu.press(shell, MenuKey::Yes) == MenuEffect::Restart, "Y confirms Restart");
    menu.press(shell, MenuKey::Quit);
    check(shell.pending() == ConfirmKind::Quit, "Q asks to confirm Quit");
    check(menu.press(shell, MenuKey::Save) == MenuEffect::None && shell.pending() == ConfirmKind::Quit &&
              menu.screen() == MenuScreen::Top,
          "other keys wait on the confirmation");
    check(menu.press(shell, MenuKey::Yes) == MenuEffect::Quit, "Y confirms Quit");
}

}  // namespace

int main() {
    test_back_out_one_level_at_a_time();
    test_keys_ignored_while_running();
    test_save_load_round_trip_through_every_slot();
    test_overwrite_confirmation();
    test_restart_and_quit_confirmation();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
