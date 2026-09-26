#include "daggorath/sound_mix.hpp"

#include "daggorath/snoise.hpp"

namespace dag {

void SoundMix::consume(const std::vector<TraceEvent>& trace) {
    while (next_ < trace.size()) {
        const TraceEvent& event = trace[next_++];
        const auto samples = samples_for(event.kind, event.detail, state_);
        pending_.insert(pending_.end(), samples.begin(), samples.end());
    }
}

}  // namespace dag
