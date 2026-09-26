// Daggorath Core — the ordered CoreEvent stream (ADR-0004 rule 1).
// Source: SOUNDS.ASM (SNDTAB, SOUNDI, SOUNDX), COMSWI.ASM (ISOUND, SOUNDS),
//         CRETUR.ASM (CMOVE, CWALK), PATTK.ASM, PTURN.ASM, PUSE.ASM,
//         PINCAN.ASM, MISC.ASM (WIZIX, PREPAX), PLOOK.ASM (INIVUX),
//         PEXAM.ASM, PUPDAT.ASM, COMMON.ASM (CLOCK CLK30), CD.ASM.
//
// Only events the simulation itself produces, or that affect its timing,
// belong here. Presentation derives everything else. No presentation or
// platform header may be included.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace dag {

// DSPMOD. The stored routine address is the display mode.
enum class DisplayMode : std::uint8_t { Viewer = 0, Examine = 1, Mapper = 2 };

enum class CoreEventKind : std::uint8_t {
    Sound,        // SOUNDS / ISOUND request
    Text,         // OUTSTI string
    DisplayMode,  // a store to DSPMOD
    Block,        // foreground wait: SYNC, TURN/MOVE animation
    Heartbeat,    // CLOCK CLK30 toggle
};

// SNDTAB order: A$xxx is the entry's index (SOUNDS.ASM SND macro, FOO+1).
enum class SoundCue : std::uint8_t {
    SQK0 = 0, RTL0, ROR0, BEP0, KLK0, ROR1, RTL1, KLK1, PSHT, ROR2, SQK1, SQK2,
    FLAS = 12, RING, SCRO, SHIE, SWOR, TORC,
    KLK2 = 18, KLK3, THUD, EXP0, EXP1,
};
inline constexpr std::uint8_t kSoundCueCount = 23;
inline constexpr std::uint8_t kSndObj = 12;       // SOUNDS.ASM:80 SNDOBJ EQU 12

// ISOUND reads the cue byte after the SWI and forces B = $FF (SOUNDI).
// SOUNDS takes A = cue and B = volume from the caller (SOUNDX).
enum class SoundEntry : std::uint8_t { Isound, Sounds };

enum class BlockKind : std::uint8_t {
    Sync,           // DEC UPDATE / SYNC: the foreground waits for the next IRQ
    TurnAnimation,  // PTURN LRTURN/RLTURN line sweeps (VECTOR, no SYNC)
    MoveAnimation,  // PMOVE half-step PUPDAT or sidestep line sweep
    Wait,           // WAITX: 81 x SYNC
};

struct CoreEvent {
    std::uint64_t jiffy = 0;        // interrupt count when emitted
    std::uint32_t sequence = 0;     // index in the stream
    std::string position;           // running task name, "IRQ", or "FG"
    CoreEventKind kind = CoreEventKind::Text;

    // Sound. `range` is CWALK's T0 (larger row/column delta) when the volume
    // was range-attenuated, else -1. `source` is a CCB slot, or -1 for the
    // player's own action.
    std::uint8_t cue = 0;
    std::uint8_t volume = 0;
    SoundEntry entry = SoundEntry::Isound;
    int range = -1;
    int source = -1;

    // Text
    std::string text;

    // DisplayMode
    DisplayMode mode = DisplayMode::Viewer;

    // Block (also carried by Sound: the generator runs in the foreground).
    // `loop_count` is the source-derived iteration count where one applies.
    BlockKind block = BlockKind::Sync;
    std::uint32_t duration_jiffies = 0;
    bool duration_known = false;
    std::uint32_t loop_count = 0;

    // Heartbeat: PIA single-bit sound output after EORB #BIT1, and whether
    // the status-line heart was drawn (HEARTF) and at which size (HEARTS).
    bool audio_level = false;
    bool visual = false;
    bool large = false;

    std::string to_line() const;
};

std::string_view sound_cue_name(std::uint8_t cue);
std::string_view display_mode_name(DisplayMode mode);

}  // namespace dag
