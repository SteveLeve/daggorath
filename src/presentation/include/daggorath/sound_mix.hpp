#pragma once
#include <cstdint>
#include <vector>

#include "daggorath/core_event.hpp"
#include "daggorath/game.hpp"

namespace dag {

// Pulls SOUND events that have a sample generator into one playback buffer.
class SoundMix {
public:
    void consume(const std::vector<TraceEvent>& trace);
    // Core sound requests, including creature cues at their SNVOL. Do not also
    // consume the trace: both streams carry the same swings.
    void consume_events(const std::vector<CoreEvent>& events);
    // Ignore events already played before a restore. The next consume starts here.
    void discard_through(std::size_t count) { next_ = count; }
    const std::vector<std::uint8_t>& pending() const { return pending_; }
    void clear() { pending_.clear(); }

private:
    std::uint16_t state_ = 1;
    std::size_t next_ = 0;
    std::vector<std::uint8_t> pending_;
};

// The heartbeat already fills the 6000 Hz clock. Effects replace that stream
// from the front and the leftover stays in `effect`, so a rattle starts now
// instead of after every heartbeat sample queued so far.
inline void overlay_dac(std::vector<std::uint8_t>& timeline, std::vector<std::uint8_t>& effect) {
    const std::size_t n = timeline.size() < effect.size() ? timeline.size() : effect.size();
    for (std::size_t i = 0; i < n; ++i) timeline[i] = effect[i];
    effect.erase(effect.begin(), effect.begin() + static_cast<std::ptrdiff_t>(n));
}

// A new generator takes the DAC now. The original plays one sound at a time in
// the foreground, so queueing this behind a rattle that has not finished yet
// would make the next creature silent until that rattle drained. One second at
// 6000 Hz is the longest tail kept.
inline void start_dac(std::vector<std::uint8_t>& carry, const std::vector<std::uint8_t>& incoming) {
    if (incoming.empty()) return;
    if (carry.size() < incoming.size()) carry.resize(incoming.size());
    for (std::size_t i = 0; i < incoming.size(); ++i) carry[i] = incoming[i];
    constexpr std::size_t kMaxSamples = 6000;
    if (carry.size() > kMaxSamples) carry.resize(kMaxSamples);
}

}  // namespace dag
