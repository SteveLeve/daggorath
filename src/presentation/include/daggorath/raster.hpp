#pragma once
#include <array>
#include <cstdint>

#include "daggorath/render_state.hpp"

namespace dag {

inline constexpr int kScreenWidth = 256;
inline constexpr int kScreenHeight = 192;

// VECTOR.ASM plot walk on a 256×192 byte surface. One byte per pixel, 0 or 1.
// The step count and the 8.8 increment follow DIVIDE and INCRE. The bitmap
// bit masks and the fade counter are not applied; every step is plotted.
void draw_segment(std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                  const DrawSegment& segment);

// Host microseconds owed to the 60 Hz core. Every owed jiffy is returned.
// A stall catches up; it does not drop jiffies.
int jiffies_due(std::uint64_t elapsed_us, std::uint64_t& accumulator_us);

}  // namespace dag
