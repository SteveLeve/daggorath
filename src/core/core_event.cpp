#include "daggorath/core_event.hpp"
#include "daggorath/sound_tables.hpp"

#include <array>

namespace dag {

static_assert(kSndObj == kExtractedSndObj);
static_assert(kSoundCueCount == kSoundCueNames.size());

std::string_view sound_cue_name(std::uint8_t cue) {
    return cue < kSoundCueNames.size() ? kSoundCueNames[cue] : std::string_view{"A$????"};
}

std::string_view display_mode_name(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::Viewer: return "VIEWER";
        case DisplayMode::Examine: return "EXAMIN";
        case DisplayMode::Mapper: return "MAPPER";
    }
    return "?";
}

namespace {

std::string_view block_name(BlockKind b) {
    switch (b) {
        case BlockKind::Sync: return "SYNC";
        case BlockKind::TurnAnimation: return "TURN";
        case BlockKind::MoveAnimation: return "MOVE";
        case BlockKind::Wait: return "WAIT";
    }
    return "?";
}

std::string duration(const CoreEvent& e) {
    return e.duration_known ? std::to_string(e.duration_jiffies) : std::string("?");
}

}  // namespace

std::string CoreEvent::to_line() const {
    std::string head = std::to_string(jiffy) + "\t" + position + "\t";
    switch (kind) {
        case CoreEventKind::Sound:
            return head + "SOUND\tcue=" + std::string(sound_cue_name(cue)) +
                   " id=" + std::to_string(cue) +
                   " via=" + (entry == SoundEntry::Isound ? "ISOUND" : "SOUNDS") +
                   " vol=" + std::to_string(volume) +
                   " range=" + (range < 0 ? std::string("-") : std::to_string(range)) +
                   " source=" + (source < 0 ? std::string("player") : std::to_string(source)) +
                   " block=" + duration(*this);
        case CoreEventKind::Text:
            return head + "TEXT\t\"" + text + "\"";
        case CoreEventKind::DisplayMode:
            return head + "MODE\t" + std::string(display_mode_name(mode));
        case CoreEventKind::Block:
            return head + "BLOCK\t" + std::string(block_name(block)) +
                   " jiffies=" + duration(*this) +
                   " loops=" + std::to_string(loop_count);
        case CoreEventKind::Heartbeat:
            return head + "HEART\taudio=" + (audio_level ? "1" : "0") +
                   " visual=" + (visual ? (large ? "large" : "small") : "off");
    }
    return head + "?";
}

}  // namespace dag
