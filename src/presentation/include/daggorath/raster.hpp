#pragma once
#include <array>
#include <cstdint>

#include "daggorath/render_state.hpp"

namespace dag {

inline constexpr int kScreenWidth = 256;
inline constexpr int kScreenHeight = 192;
inline constexpr int kScreenStride = 32;
inline constexpr int kPackedBytes = kScreenStride * kScreenHeight;

// BITMSK in VECTOR.ASM. Bit 7 of the byte is pixel column 0 within that byte.
void pack_bitmap(const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                 std::array<std::uint8_t, kPackedBytes>& bitmap);

// `fade` is VCTFAD before VECTOR's opening INC. Zero plots every step.
// A starting value of 0xFF draws nothing. Otherwise a dot is plotted every
// `fade + 1` steps.
void draw_segment(std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                  const DrawSegment& segment, std::uint8_t fade = 0);

// Host microseconds owed to the 60 Hz core. Every owed jiffy is returned.
// A stall catches up; it does not drop jiffies.
int jiffies_due(std::uint64_t elapsed_us, std::uint64_t& accumulator_us);

}  // namespace dag
