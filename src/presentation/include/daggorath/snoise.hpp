#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace dag {

// SOUNDS.ASM SNOISE. Updates the 16-bit SNDRND word: multiply by five, then
// increment the low byte without a carry into the high byte. Does not touch SEED.
std::uint16_t snoise(std::uint16_t& state);

// SNOUT. The high byte of the noise word times the volume, with the low two
// bits of that product cleared, is the DAC byte written to $FF20.
std::uint8_t dac_sample(std::uint8_t noise_high, std::uint8_t volume);

// PSSHT, PSSST, and RATTLE. The loaded count is the number of 192-sample pulses.
std::vector<std::uint8_t> noise_pulses(std::uint16_t& state, std::uint8_t volume, int pulses);

// THUD via BOOMER. Pitch starts at $0080 and climbs by 2 to $0150.
// Each pitch writes one noise sample. The waits between samples are not stored.
std::vector<std::uint8_t> thud(std::uint16_t& state, std::uint8_t volume);

// WHOOSH then CHUCK (SOUNDS.ASM). Attack envelope starts at BIGZER and adds
// $80 until that add carries. Decay starts at NEGONE and subtracts $A0 until
// the subtract borrows or lands on zero. One DAC byte per SNOUT. The wait
// loops are not stored, and this does not advance the scheduler (D-4b).
std::vector<std::uint8_t> whoosh(std::uint16_t& state, std::uint8_t volume);

// SNDTAB generator for one cue at SNVOL. Creature types 0–11 are cues 0–11.
// Volume is the caller's B register: $FF on the same cell, quieter with range.
std::vector<std::uint8_t> samples_for_cue(std::uint8_t cue, std::uint8_t volume,
                                          std::uint16_t& state);

// Maps a trace event onto DAC samples. A$THUD, a sword swing (class 4 /
// A$SWOR), a connecting hit (A$KLK2 / KLINK), a creature clank (A$KLK3),
// and a kill (A$EXP0 / BANG) use full volume. A creature "slot=" line uses
// the volume in that detail.
std::vector<std::uint8_t> samples_for(const std::string& kind, const std::string& detail,
                                      std::uint16_t& state);

}  // namespace dag
