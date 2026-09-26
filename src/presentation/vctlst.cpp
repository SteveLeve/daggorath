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
                                        std::uint8_t fade) {
    std::vector<DrawSegment> out;
    if (static_cast<std::uint8_t>(fade + 1u) == 0) return out;
    bool have_start = false;
    int x0 = 0;
    int y0 = 0;
    std::size_t i = 0;
    while (i < list.size()) {
        const std::uint8_t yb = list[i];
        if (yb >= 0xFA) {
            if (yb == kVectorEnd) break;
            break;
        }
        if (i + 1 >= list.size()) break;
        const int y = scale_coord(list[i], y_scale, centroid_y);
        const int x = scale_coord(list[i + 1], x_scale, centroid_x);
        i += 2;
        if (!have_start) {
            x0 = x;
            y0 = y;
            have_start = true;
            continue;
        }
        out.push_back(DrawSegment{x0, y0, x, y, "vector"});
        x0 = x;
        y0 = y;
    }
    return out;
}

}  // namespace dag
