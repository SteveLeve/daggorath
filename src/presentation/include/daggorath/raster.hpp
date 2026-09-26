#pragma once
#include <array>
#include <cstdint>

#include "daggorath/render_state.hpp"

namespace dag {

inline constexpr int kScreenWidth = 256;
inline constexpr int kScreenHeight = 192;

// Integer DDA onto a 256×192 byte surface. One byte per pixel, 0 or 1.
// This is not yet the fractional accumulator in VECTOR.ASM.
void draw_segment(std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                  const DrawSegment& segment);

// Host microseconds owed to the 60 Hz core. Every owed jiffy is returned.
// A stall catches up; it does not drop jiffies.
int jiffies_due(std::uint64_t elapsed_us, std::uint64_t& accumulator_us);

}  // namespace dag
