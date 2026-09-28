// The system-menu shell (ADR-0009): pause/resume, five save/load slots plus
// one hidden slot, confirmations, and PAUSE/RESUME trace markers.
//
// Headless by design (module-boundaries.md: "its own target `src/shell` once
// it has non-SDL logic worth testing headlessly"): this module links only
// daggorath::core, so the pause-invariance test and slot round-trip run
// without SDL3. The SDL desktop window (`src/platform/sdl_app.cpp`) wires
// this to the Esc key and the menu UI in a later Phase 8 workstream.
//
// What the shell may do to a running game (ADR-0009 §2) — nothing else:
// withhold/resume jiffy delivery; take/restore a suspend snapshot through
// Game::snapshot()/restore_snapshot(); hand the caller a confirmed Restart
// or Quit request, since constructing a new Game or exiting the process is
// not this module's decision to make; nothing here writes core state
// directly or reaches a rule.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/game.hpp"

namespace dag::shell {

// ADR-0009 "Trace markers": PAUSE/RESUME are shell lines, not CoreEvents
// (ADR-0004 rule 1 admits only events the simulation produces), so they are
// kept separately from Game::trace() and merged only for debugging output.
// Both carry the jiffy at which delivery stopped or resumed.
struct ShellTraceLine {
    std::uint64_t jiffy = 0;
    std::string kind;  // "PAUSE" or "RESUME"
    std::string to_line() const { return std::to_string(jiffy) + " " + kind; }
};

// One suspend-snapshot slot (ADR-0009 §5/§6). `bytes` is the exact string
// Game::snapshot()/restore_snapshot() exchange; empty means unused. `name`
// is auto-generated (see Shell::auto_name), read-only display text.
struct SnapshotSlot {
    std::string name;
    std::string bytes;
    bool occupied() const { return !bytes.empty(); }
};

// What a caller must do next after Shell::confirm(): SaveOverwrite is
// executed by the shell itself; Restart and Quit are handed back because
// constructing a new Game or exiting the process is outside what ADR-0009
// §2 lets the shell do.
enum class ConfirmKind { None, SaveOverwrite, Restart, Quit };

class Shell {
public:
    static constexpr std::size_t kSlotCount = 5;

    explicit Shell(Game& game) : game_(game) {}

    // Pause withholds jiffy delivery entirely: tick() becomes a no-op while
    // paused. D-16: no jiffy is owed for wall time spent paused, so the
    // resumed clock restarts exactly where it left off (ADR-0009 §3/§4).
    // Both are idempotent and each writes one trace line.
    void pause();
    void resume();
    bool paused() const { return paused_; }

    // The shell's own driving step, replacing a direct advance_jiffies call:
    // does nothing while paused. In-play overlays never call this only the
    // system menu and OS backgrounding pause (ADR-0009 §3).
    void tick(std::uint64_t jiffies);

    // "<level> <mm:ss>", from the core's own read-only accessors
    // (Game::level_index(), Game::counters().total_jiffies), never written
    // by the shell. 60 jiffies per second (the original's jiffy rate).
    static std::string auto_name(int level_index, std::uint64_t total_jiffies);

    const std::array<SnapshotSlot, kSlotCount>& slots() const { return slots_; }

    // true: the slot was empty and is now saved. false: the slot was
    // occupied; nothing changed yet and pending() is now SaveOverwrite for
    // `slot` until confirm() or cancel().
    bool save_to_slot(std::size_t slot);

    // Loads and restores `slot`'s snapshot. Refuses (returns false, no
    // change to `game_`) an empty slot or one whose stored bytes do not
    // start with the snapshot magic/version Game::restore_snapshot expects
    // (ADR-0009 §5: restore_snapshot aborts on a bad magic, so the shell
    // validates first rather than letting it abort).
    bool load_from_slot(std::size_t slot) const;

    void request_restart() { pending_ = ConfirmKind::Restart; }
    void request_quit() { pending_ = ConfirmKind::Quit; }
    ConfirmKind pending() const { return pending_; }
    std::size_t pending_slot() const { return pending_slot_; }
    void cancel() { pending_ = ConfirmKind::None; }
    // Executes a pending SaveOverwrite (writes and renames the slot);
    // clears Restart/Quit so the caller performs them (see class comment).
    void confirm();

    // ADR-0009 §6: written when the OS backgrounds the app, read at the
    // next launch. Not listed among slots(); never touched by Save/Load.
    void write_hidden_slot();
    bool has_hidden_slot() const { return hidden_.occupied(); }
    // Restores the hidden slot's snapshot (same validation as
    // load_from_slot). Does not clear it: per ADR-0009 §6 the hidden slot
    // is "cleared once play continues", which is clear_hidden_slot(),
    // called by the caller once the resumed, paused game is un-paused.
    bool restore_hidden_slot() const;
    void clear_hidden_slot() { hidden_ = SnapshotSlot{}; }

    const std::vector<ShellTraceLine>& trace() const { return trace_; }

private:
    static bool valid_snapshot(const std::string& bytes);

    Game& game_;
    bool paused_ = false;
    std::array<SnapshotSlot, kSlotCount> slots_{};
    SnapshotSlot hidden_{};
    ConfirmKind pending_ = ConfirmKind::None;
    std::size_t pending_slot_ = 0;
    std::vector<ShellTraceLine> trace_;
};

}  // namespace dag::shell
