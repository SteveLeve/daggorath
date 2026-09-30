// Phase 8.2 shell regressions (ADR-0009).
//
// Pause-invariance (ADR-0009 §4): a run paused at any point yields the same
// jiffy-stamped core trace as the same keystrokes delivered on the same
// jiffies without pause, because pause only withholds jiffy delivery and
// none is owed for the paused wall time (D-16). Slot round-trip (§5/§6):
// save/load through Game::snapshot()/restore_snapshot(), plus the shell's
// own validation of a bad or empty slot, and a slot put back from storage.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "daggorath/game.hpp"
#include "daggorath/shell.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

std::vector<dag::KeyEvent> move_turn_look_script() {
    // T SPACE R CR at jiffy 5 (matches the committed t3-burst-one-jiffy
    // scripted burst), then M CR later, well after the command dispatches.
    return {
        {5, 'T'}, {5, ' '}, {5, 'R'}, {5, 0x0D}, {40, 'M'}, {40, 0x0D},
    };
}

// The same trace, without any shell involved.
std::vector<dag::TraceEvent> run_unpaused(std::uint64_t jiffies) {
    dag::Game game;
    game.load_script(move_turn_look_script());
    game.advance_jiffies(jiffies);
    return game.trace();
}

void test_pause_invariance() {
    const auto baseline = run_unpaused(80);

    dag::Game game;
    game.load_script(move_turn_look_script());
    dag::shell::Shell shell(game);

    shell.tick(10);  // through the jiffy-5 burst
    shell.pause();
    // "Wall time" passes with no core progress: repeated ticks while
    // paused must be no-ops.
    shell.tick(1000);
    shell.tick(1000);
    check(game.counters().total_jiffies == 10, "tick() is a no-op while paused",
          std::to_string(game.counters().total_jiffies));
    shell.resume();
    shell.tick(70);  // 10 + 70 = 80, same total as the unpaused run

    check(game.counters().total_jiffies == 80, "resumed clock reaches the same total");
    check(game.trace().size() == baseline.size(), "paused run has the same trace length",
          std::to_string(game.trace().size()) + " vs " + std::to_string(baseline.size()));
    bool identical = game.trace().size() == baseline.size();
    for (std::size_t i = 0; identical && i < baseline.size(); ++i) {
        const auto& a = game.trace()[i];
        const auto& b = baseline[i];
        if (a.jiffy != b.jiffy || a.kind != b.kind || a.detail != b.detail ||
            a.counters != b.counters) {
            identical = false;
        }
    }
    check(identical, "the core trace is identical with and without the pause");

    // The shell's own PAUSE/RESUME lines are separate from Game::trace(),
    // per ADR-0009's trace-marker rule (they are shell lines, not
    // CoreEvents), so the identity check above needs no filtering.
    check(shell.trace().size() == 2, "shell records one PAUSE and one RESUME line",
          std::to_string(shell.trace().size()));
    check(shell.trace()[0].kind == "PAUSE" && shell.trace()[0].jiffy == 10,
          "PAUSE is marked at the jiffy delivery stopped");
    check(shell.trace()[1].kind == "RESUME" && shell.trace()[1].jiffy == 10,
          "RESUME carries the same jiffy as the matching PAUSE (ADR-0009)");
}

void test_save_and_load_round_trip() {
    dag::Game game;
    game.load_script(move_turn_look_script());
    dag::shell::Shell shell(game);
    shell.tick(40);
    const auto jiffies_at_save = game.counters().total_jiffies;

    check(shell.save_to_slot(0), "an empty slot saves immediately");
    check(shell.slots()[0].occupied(), "the slot is now occupied");
    check(!shell.slots()[0].name.empty(), "the slot has an auto-generated name");

    shell.tick(40);  // the clock moves on past the save point
    check(game.counters().total_jiffies == jiffies_at_save + 40,
          "the clock advanced past the saved point");

    check(shell.load_from_slot(0), "loading an occupied, valid slot succeeds");
    check(game.counters().total_jiffies == jiffies_at_save,
          "loading rolls the clock back to the saved point");
}

void test_overwrite_needs_confirmation() {
    dag::Game game;
    dag::shell::Shell shell(game);
    check(shell.save_to_slot(1), "first save to slot 1 is immediate");
    check(!shell.save_to_slot(1), "saving over an occupied slot is not immediate");
    check(shell.pending() == dag::shell::ConfirmKind::SaveOverwrite,
          "an overwrite attempt sets pending confirmation");
    check(shell.pending_slot() == 1, "pending confirmation names the right slot");
    shell.confirm();
    check(shell.pending() == dag::shell::ConfirmKind::None,
          "confirm() clears the pending state");
}

void test_restart_and_quit_confirmation() {
    dag::Game game;
    dag::shell::Shell shell(game);
    shell.request_restart();
    check(shell.pending() == dag::shell::ConfirmKind::Restart,
          "restart is not applied until confirmed");
    shell.cancel();
    check(shell.pending() == dag::shell::ConfirmKind::None, "cancel clears restart");

    shell.request_quit();
    check(shell.pending() == dag::shell::ConfirmKind::Quit,
          "quit is not applied until confirmed");
    shell.confirm();
    check(shell.pending() == dag::shell::ConfirmKind::None,
          "confirm clears quit; the caller performs the actual exit");
}

void test_empty_and_corrupt_slots_are_refused() {
    dag::Game game;
    dag::shell::Shell shell(game);
    check(!shell.load_from_slot(2), "an empty slot is refused, not aborted");
    check(!shell.has_hidden_slot(), "the hidden slot starts empty");
    check(!shell.restore_hidden_slot(), "an empty hidden slot is refused");
}

void test_hidden_slot_backgrounding() {
    dag::Game game;
    game.load_script(move_turn_look_script());
    dag::shell::Shell shell(game);
    shell.tick(40);
    shell.write_hidden_slot();
    check(shell.has_hidden_slot(), "backgrounding writes the hidden slot");
    check(shell.restore_hidden_slot(), "the hidden slot restores");
    check(shell.has_hidden_slot(), "restoring does not itself clear the hidden slot");
    shell.clear_hidden_slot();
    check(!shell.has_hidden_slot(), "clear_hidden_slot empties it once play continues");
}

// A slot kept outside the process (ADR-0011) goes back into a fresh Shell on
// a fresh Game and loads as if it had been saved there.
void test_put_slot_from_outside() {
    dag::Game first;
    first.load_script(move_turn_look_script());
    dag::shell::Shell saver(first);
    saver.tick(40);
    check(saver.save_to_slot(3), "slot 4 saves");
    const dag::shell::SnapshotSlot kept = saver.slots()[3];
    const auto jiffies_at_save = first.counters().total_jiffies;

    dag::Game second;
    dag::shell::Shell shell(second);
    check(shell.put_slot(3, kept), "a valid snapshot is accepted into a slot");
    check(shell.slots()[3].name == kept.name, "the slot keeps its display name");
    check(second.counters().total_jiffies == 0, "putting a slot does not touch the game");
    check(shell.load_from_slot(3), "the put slot loads");
    check(second.counters().total_jiffies == jiffies_at_save,
          "loading it restores the saved point");
    check(second.snapshot() == kept.bytes, "the restored game matches the snapshot exactly");

    check(!shell.put_slot(0, {"L1 00:00", "not a snapshot"}), "a corrupt snapshot is refused");
    check(!shell.put_slot(1, {}), "an empty slot is refused");
    check(!shell.put_slot(2, {kept.name, kept.bytes.substr(0, kept.bytes.size() / 2)}),
          "a truncated snapshot is refused");
    check(!shell.slots()[0].occupied() && !shell.slots()[1].occupied(),
          "refused slots stay empty");

    // A size field edited to an absurd value is refused, not allocated on
    // (read_bounded_count). A fresh game's snapshot has no tapes, so its
    // tape count is the "0" just before the retired D-12 field "100".
    const std::string fresh = dag::Game().snapshot();
    const auto at = fresh.find("\n0\n100\n");
    check(at != std::string::npos, "tape count located in a fresh snapshot");
    if (at != std::string::npos) {
        std::string huge = fresh;
        huge.replace(at, 3, "\n99999999999999999\n");
        check(!shell.put_slot(4, {"L1 00:00", huge}), "an absurd tape count is refused");
    }
}

}  // namespace

int main() {
    test_pause_invariance();
    test_save_and_load_round_trip();
    test_overwrite_needs_confirmation();
    test_restart_and_quit_confirmation();
    test_empty_and_corrupt_slots_are_refused();
    test_hidden_slot_backgrounding();
    test_put_slot_from_outside();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
