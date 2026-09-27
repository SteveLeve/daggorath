#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

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

// The same VECTOR.ASM walk `draw_segment` plots, exposed so a second
// consumer (the `crisp` render style, ADR-0010) can find the exact dots a
// dim segment would plot without a second geometry source (ADR-0010 §2).
// Calls `emit(x, y)` for each in-bounds source-pixel position `draw_segment`
// would set at this `fade`; a zero-length segment or `fade == 0xFF` calls it
// zero times.
void walk_segment(const DrawSegment& segment, std::uint8_t fade,
                  const std::function<void(int x, int y)>& emit);

// Host microseconds owed to the 60 Hz core. Every owed jiffy is returned.
// A stall catches up; it does not drop jiffies.
// SETFAX. Light minus 7 minus range. Non-negative is full brightness (fade 0).
// -7 or below is darkness (fade 0xFF). The values in between are BITMSK entries.
std::uint8_t set_fade(std::uint8_t light, std::uint8_t range);

// Host microseconds owed to the 60 Hz core. Every owed jiffy is returned.
// A stall catches up; it does not drop jiffies.
int jiffies_due(std::uint64_t elapsed_us, std::uint64_t& accumulator_us);

// P1 portable bitmap, one character per pixel. Used by the offscreen tests.
std::string bitmap_pbm(
    const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels);

// Project the view and plot every segment with the fade SETFAX stored on it.
std::array<std::uint8_t, kScreenWidth * kScreenHeight> rasterize(
    const ViewSnapshot& view);

// MISC.ASM WIZZES: the crescent wizard (WIZ1) at scale $80 with VCTFAD = `fade`.
std::array<std::uint8_t, kScreenWidth * kScreenHeight> rasterize_wizard(
    std::uint8_t fade);

// Integer scale. Each source dot becomes a factor-by-factor block. Factor 1 copies.
std::vector<std::uint8_t> scale_frame(
    const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels, int factor);

}  // namespace dag
