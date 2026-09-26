#pragma once
#include <cstdint>
#include <vector>

#include "daggorath/game.hpp"

namespace dag {

// Pulls SOUND events that have a sample generator into one playback buffer.
class SoundMix {
public:
    void consume(const std::vector<TraceEvent>& trace);
    const std::vector<std::uint8_t>& pending() const { return pending_; }
    void clear() { pending_.clear(); }

private:
    std::uint16_t state_ = 1;
    std::size_t next_ = 0;
    std::vector<std::uint8_t> pending_;
};

}  // namespace dag
