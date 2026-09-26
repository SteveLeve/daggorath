#include "daggorath/raster.hpp"

#include <cstdlib>

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
                  const DrawSegment& segment) {
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
    for (std::uint16_t i = 0; i < length; ++i) {
        const int high_x = (x >> 16) & 0xFF;
        const int px = (x >> 8) & 0xFF;
        const int py = (y >> 8) & 0xFF;
        if (high_x == 0 && py >= 0 && py < kScreenHeight) {
            pixels[static_cast<std::size_t>(py * kScreenWidth + px)] = 1;
        }
        x += x_step;
        y += y_step;
    }
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

}  // namespace dag
