#include "daggorath/raster.hpp"

#include <cstdlib>

namespace dag {

void draw_segment(std::array<std::uint8_t, kScreenWidth * kScreenHeight>& pixels,
                  const DrawSegment& segment) {
    const int dx = segment.x1 - segment.x0;
    const int dy = segment.y1 - segment.y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    auto plot = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= kScreenWidth || y >= kScreenHeight) return;
        pixels[static_cast<std::size_t>(y * kScreenWidth + x)] = 1;
    };
    if (steps == 0) {
        plot(segment.x0, segment.y0);
        return;
    }
    for (int i = 0; i <= steps; ++i) {
        plot(segment.x0 + (dx * i) / steps, segment.y0 + (dy * i) / steps);
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
