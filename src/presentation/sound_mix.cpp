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

void SoundMix::consume_events(const std::vector<CoreEvent>& events) {
    while (next_ < events.size()) {
        const CoreEvent& event = events[next_++];
        if (event.kind != CoreEventKind::Sound) continue;
        const auto samples = samples_for_cue(event.cue, event.volume, state_);
        pending_.insert(pending_.end(), samples.begin(), samples.end());
    }
}

}  // namespace dag
