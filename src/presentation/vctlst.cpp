#include "daggorath/vctlst.hpp"

namespace dag {
namespace {

constexpr std::uint8_t kVectorEnd = 0xFE;

int scale_coord(std::uint8_t coord, std::uint8_t scale, int centroid) {
    const auto delta = static_cast<std::int8_t>(
        static_cast<std::uint8_t>(coord - static_cast<std::uint8_t>(centroid)));
    std::int16_t product;
    if (delta >= 0) {
        product = static_cast<std::int16_t>(static_cast<std::uint16_t>(delta) * scale);
    } else {
        product = static_cast<std::int16_t>(
            -static_cast<std::int16_t>(static_cast<std::uint16_t>(-delta) * scale));
    }
    product = static_cast<std::int16_t>(product >> 7);
    return centroid + product;
}

}  // namespace

std::vector<DrawSegment> decode_vectors(std::span<const std::uint8_t> list, std::uint8_t x_scale,
                                        std::uint8_t y_scale, int centroid_x, int centroid_y,
                                        std::uint8_t fade, std::size_t start) {
    std::vector<DrawSegment> out;
    if (static_cast<std::uint8_t>(fade + 1u) == 0) return out;
    bool have_start = false;
    int x0 = 0;
    int y0 = 0;
    std::uint8_t x_raw = 0;
    std::uint8_t y_raw = 0;
    std::size_t i = start;
    std::vector<std::size_t> returns;
    int steps = 0;
    while (i < list.size() && steps++ < 10000) {
        const std::uint8_t yb = list[i];
        if (yb >= 0xFA) {
            if (yb == kVectorEnd) break;
            if (yb == 0xFF) {
                ++i;
                have_start = false;
                continue;
            }
            if (yb == 0xFB || yb == 0xFD) {
                if (i + 2 >= list.size()) break;
                const unsigned addr = (static_cast<unsigned>(list[i + 1]) << 8) | list[i + 2];
                if (addr >= list.size()) break;
                if (yb == 0xFB) returns.push_back(i + 3);
                i = addr;
                have_start = false;
                continue;
            }
            if (yb == 0xFA) {
                if (returns.empty()) break;
                i = returns.back();
                returns.pop_back();
                have_start = false;
                continue;
            }
            if (yb == 0xFC) {
                ++i;
                while (i < list.size() && list[i] != 0) {
                    const std::uint8_t packed = list[i++];
                    const auto y_delta = static_cast<std::int8_t>(
                        static_cast<std::uint8_t>(static_cast<std::int8_t>(packed) >> 4) << 1);
                    std::uint8_t x_nibble = static_cast<std::uint8_t>(packed & 0x0F);
                    if ((x_nibble & 0x08) != 0) x_nibble = static_cast<std::uint8_t>(x_nibble | 0xF0);
                    const auto x_delta = static_cast<std::int8_t>(static_cast<std::uint8_t>(x_nibble << 1));
                    const auto raw_y = static_cast<std::uint8_t>(y_raw + y_delta);
                    const auto raw_x = static_cast<std::uint8_t>(x_raw + x_delta);
                    const int y = scale_coord(raw_y, y_scale, centroid_y);
                    const int x = scale_coord(raw_x, x_scale, centroid_x);
                    if (have_start) out.push_back(DrawSegment{x0, y0, x, y, 0, fade, "vector"});
                    x0 = x;
                    y0 = y;
                    y_raw = raw_y;
                    x_raw = raw_x;
                    have_start = true;
                }
                if (i < list.size() && list[i] == 0) ++i;
                have_start = false;
                continue;
            }
            break;
        }
        if (i + 1 >= list.size()) break;
        const int y = scale_coord(list[i], y_scale, centroid_y);
        const int x = scale_coord(list[i + 1], x_scale, centroid_x);
        i += 2;
        if (!have_start) {
            x0 = x;
            y0 = y;
            x_raw = list[i - 1];
            y_raw = list[i - 2];
            have_start = true;
            continue;
        }
        out.push_back(DrawSegment{x0, y0, x, y, 0, fade, "vector"});
        x0 = x;
        y0 = y;
        x_raw = list[i - 1];
        y_raw = list[i - 2];
    }
    return out;
}

}  // namespace dag
