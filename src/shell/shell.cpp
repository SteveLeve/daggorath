#include "daggorath/shell.hpp"

#include <sstream>
#include <utility>

namespace dag::shell {

void Shell::pause() {
    if (paused_) return;
    paused_ = true;
    trace_.push_back({game_.counters().total_jiffies, "PAUSE"});
}

void Shell::resume() {
    if (!paused_) return;
    paused_ = false;
    trace_.push_back({game_.counters().total_jiffies, "RESUME"});
}

void Shell::tick(std::uint64_t jiffies) {
    if (paused_) return;  // D-16: no jiffy delivered, none owed later.
    game_.advance_jiffies(jiffies);
}

std::string Shell::auto_name(int level_index, std::uint64_t total_jiffies) {
    constexpr std::uint64_t kJiffiesPerSecond = 60;
    const std::uint64_t seconds = total_jiffies / kJiffiesPerSecond;
    const std::uint64_t minutes = seconds / 60;
    const std::uint64_t secs = seconds % 60;
    std::string out = "L" + std::to_string(level_index) + " ";
    if (minutes < 10) out += "0";
    out += std::to_string(minutes) + ":";
    if (secs < 10) out += "0";
    out += std::to_string(secs);
    return out;
}

bool Shell::save_to_slot(std::size_t slot) {
    SnapshotSlot& target = slots_.at(slot);
    if (target.occupied()) {
        pending_ = ConfirmKind::SaveOverwrite;
        pending_slot_ = slot;
        return false;
    }
    target.bytes = game_.snapshot();
    target.name = auto_name(game_.level_index(), game_.counters().total_jiffies);
    return true;
}

bool Shell::valid_snapshot(const std::string& bytes) {
    // Game::restore_snapshot's own format: "DAGSNAP 1\n..." (game.cpp). The
    // shell checks the same two tokens restore_snapshot reads, so a bad slot
    // is refused here instead of reaching Game::restore_snapshot's abort()
    // (ADR-0009 §5).
    std::istringstream in(bytes);
    std::string magic;
    int version = 0;
    in >> magic >> version;
    return magic == "DAGSNAP" && version == 1;
}

bool Shell::put_slot(std::size_t slot, SnapshotSlot contents) {
    if (!contents.occupied() || !valid_snapshot(contents.bytes)) return false;
    Game trial;
    trial.restore_snapshot(contents.bytes);
    if (trial.snapshot() != contents.bytes) return false;  // truncated or altered
    slots_.at(slot) = std::move(contents);
    return true;
}

bool Shell::load_from_slot(std::size_t slot) const {
    const SnapshotSlot& source = slots_.at(slot);
    if (!source.occupied() || !valid_snapshot(source.bytes)) return false;
    game_.restore_snapshot(source.bytes);
    return true;
}

void Shell::confirm() {
    if (pending_ == ConfirmKind::SaveOverwrite) {
        SnapshotSlot& target = slots_.at(pending_slot_);
        target.bytes = game_.snapshot();
        target.name = auto_name(game_.level_index(), game_.counters().total_jiffies);
    }
    // Restart/Quit: the caller performs the action; this only clears the
    // pending state so a second confirm() is not a repeat.
    pending_ = ConfirmKind::None;
}

void Shell::write_hidden_slot() {
    hidden_.bytes = game_.snapshot();
    hidden_.name = auto_name(game_.level_index(), game_.counters().total_jiffies);
}

bool Shell::restore_hidden_slot() const {
    if (!hidden_.occupied() || !valid_snapshot(hidden_.bytes)) return false;
    game_.restore_snapshot(hidden_.bytes);
    return true;
}

}  // namespace dag::shell
