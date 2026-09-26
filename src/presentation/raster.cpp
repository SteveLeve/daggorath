#include "daggorath/raster.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace dag {
namespace {

std::uint16_t divide88(std::uint16_t dividend, std::uint16_t divisor) {
    // VECTOR.ASM DIVIDE. The quotient is an 8.8 value.
    if (dividend == 0) return 0;
    if (dividend == divisor) return 0x0100;
    std::uint32_t remainder = dividend;
    std::uint16_t quotient = 0;
    for (int bit = 0; bit < 16; ++bit) {
        remainder = (remainder << 1) & 0xFFFFFFu;
        quotient = static_cast<std::uint16_t>(quotient << 1);
        std::uint16_t top = static_cast<std::uint16_t>((remainder >> 8) & 0xFFFFu);
        if (top >= divisor) {
            top = static_cast<std::uint16_t>(top - divisor);
            remainder = (static_cast<std::uint32_t>(top) << 8) | (remainder & 0xFFu);
            quotient = static_cast<std::uint16_t>(quotient + 1);
        }
    }
    return quotient;
}

std::uint16_t negate16(std::uint16_t value) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(~value) + 1u);
}

std::int32_t increment_for(std::int16_t delta, std::uint16_t length) {
    const bool negative = delta < 0;
    const std::uint16_t magnitude = negative ? negate16(static_cast<std::uint16_t>(delta))
                                             : static_cast<std::uint16_t>(delta);
    std::uint16_t step = divide88(magnitude, length);
    if (negative) step = negate16(step);
    const std::int32_t sign_byte = (step & 0x8000u) != 0 ? 0xFF : 0;
    return static_cast<std::int32_t>((sign_byte << 16) | step);
}

}  // namespace

void draw_segment(std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                  const DrawSegment& segment, std::uint8_t fade) {
    const std::uint8_t fade_now = static_cast<std::uint8_t>(fade + 1u);
    if (fade_now == 0) return;
    // VECTOR.ASM. Length is the larger absolute delta. The walk starts at
    // coordinate + 1/2 and repeats `length` times. A zero-length line is skipped.
    const std::int16_t dx = static_cast<std::int16_t>(segment.x1 - segment.x0);
    const std::int16_t dy = static_cast<std::int16_t>(segment.y1 - segment.y0);
    const int adx = std::abs(dx);
    const int ady = std::abs(dy);
    const std::uint16_t length = static_cast<std::uint16_t>(adx > ady ? adx : ady);
    if (length == 0) return;
    const std::int32_t x_step = increment_for(dx, length);
    const std::int32_t y_step = increment_for(dy, length);
    std::int32_t x = (static_cast<std::int32_t>(segment.x0) << 8) | 0x80;
    std::int32_t y = (static_cast<std::int32_t>(segment.y0) << 8) | 0x80;
    std::uint8_t countdown = fade_now;
    for (std::uint16_t i = 0; i < length; ++i) {
        const int high_x = (x >> 16) & 0xFF;
        const int px = (x >> 8) & 0xFF;
        const int py = (y >> 8) & 0xFF;
        countdown = static_cast<std::uint8_t>(countdown - 1u);
        if (countdown == 0) {
            countdown = fade_now;
            if (high_x == 0 && py >= 0 && py < kScreenHeight) {
                pixels[static_cast<std::size_t>(py * kScreenWidth + px)] = 1;
            }
        }
        x += x_step;
        y += y_step;
    }
}

void pack_bitmap(const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                 std::array<std::uint8_t, kPackedBytes>& bitmap) {
    bitmap.fill(0);
    for (int y = 0; y < kScreenHeight; ++y) {
        for (int x = 0; x < kScreenWidth; ++x) {
            if (pixels[static_cast<std::size_t>(y * kScreenWidth + x)] == 0) continue;
            const int byte = y * kScreenStride + (x >> 3);
            bitmap[static_cast<std::size_t>(byte)] = static_cast<std::uint8_t>(
                bitmap[static_cast<std::size_t>(byte)] | (0x80u >> (x & 7)));
        }
    }
}

std::uint8_t set_fade(std::uint8_t light, std::uint8_t range) {
    const std::uint8_t adjusted = static_cast<std::uint8_t>(static_cast<std::uint8_t>(light - 7u) - range);
    const auto signed_level = static_cast<std::int8_t>(adjusted);
    if (signed_level >= 0) return 0;
    if (signed_level <= -7) return 0xFF;
    // Reachable signed levels are -6..-1, so only BITMSK entries 2..7 are used.
    static constexpr std::uint8_t kBitMask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    return kBitMask[8 + signed_level];
}

int jiffies_due(std::uint64_t elapsed_us, std::uint64_t& accumulator_us) {
    accumulator_us += elapsed_us;
    int count = 0;
    constexpr std::uint64_t kJiffyUs = 1000000 / 60;
    while (accumulator_us >= kJiffyUs) {
        accumulator_us -= kJiffyUs;
        ++count;
    }
    return count;
}

std::string bitmap_pbm(const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels) {
    std::ostringstream out;
    out << "P1\n" << kScreenWidth << ' ' << kScreenHeight << '\n';
    for (int y = 0; y < kScreenHeight; ++y) {
        for (int x = 0; x < kScreenWidth; ++x) {
            out << (pixels[static_cast<std::size_t>(y * kScreenWidth + x)] ? '1' : '0');
            if (x + 1 != kScreenWidth) out << ' ';
        }
        out << '\n';
    }
    return out.str();
}

std::array<std::uint8_t, kScreenWidth * kScreenHeight> rasterize(const ViewSnapshot& view) {
    std::array<std::uint8_t, kScreenWidth * kScreenHeight> pixels{};
    const std::uint8_t light = static_cast<std::uint8_t>(
        std::min(255, view.regular_light + view.magic_light));
    const std::uint8_t fade = set_fade(light, 0);
    // VIEWER draws OFIND's unowned objects only. Carried objects are the
    // status-line names, not a second copy of the floor picture.
    for (const DrawSegment& segment : project(view).segments) draw_segment(pixels, segment, fade);
    return pixels;
}

std::vector<std::uint8_t> scale_frame(
    const std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels, int factor) {
    if (factor < 1) factor = 1;
    const int width = kScreenWidth * factor;
    const int height = kScreenHeight * factor;
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width * height), 0);
    for (int y = 0; y < kScreenHeight; ++y) {
        for (int x = 0; x < kScreenWidth; ++x) {
            if (pixels[static_cast<std::size_t>(y * kScreenWidth + x)] == 0) continue;
            for (int dy = 0; dy < factor; ++dy) {
                for (int dx = 0; dx < factor; ++dx) {
                    const int ox = x * factor + dx;
                    const int oy = y * factor + dy;
                    out[static_cast<std::size_t>(oy * width + ox)] = 1;
                }
            }
        }
    }
    return out;
}

}  // namespace dag
