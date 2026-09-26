#pragma once
#include <cstdint>

namespace dag {

// SOUNDS.ASM SNOISE. Updates the 16-bit SNDRND word: multiply by five, then
// increment the low byte without a carry into the high byte. Does not touch SEED.
std::uint16_t snoise(std::uint16_t& state);

}  // namespace dag
