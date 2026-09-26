#pragma once
#include <cstdint>
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

}  // namespace dag
