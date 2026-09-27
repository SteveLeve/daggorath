#include "daggorath/crisp.hpp"

#include "daggorath/raster.hpp"

namespace dag {

CrispFrame build_crisp_frame(const RenderState& state) {
    CrispFrame frame;
    for (const DrawSegment& segment : state.segments) {
        const auto fade = static_cast<std::uint8_t>(segment.fade);
        const auto fade_now = static_cast<std::uint8_t>(fade + 1u);
        if (fade_now == 0) continue;  // fade 0xFF: draw_segment draws nothing.
        if (fade == 0) {
            frame.lines.push_back({static_cast<double>(segment.x0),
                                   static_cast<double>(segment.y0),
                                   static_cast<double>(segment.x1),
                                   static_cast<double>(segment.y1), segment.kind});
            continue;
        }
        walk_segment(segment, fade, [&frame, &segment](int x, int y) {
            frame.dots.push_back(
                {static_cast<double>(x), static_cast<double>(y), segment.kind});
        });
    }
    return frame;
}

CrispMap build_crisp_map(const MapSnapshot& snap) {
    CrispMap out;
    if (snap.cells != nullptr) {
        for (int row = 0; row < 32; ++row) {
            for (int col = 0; col < 32; ++col) {
                if (snap.cells[static_cast<std::size_t>(row * 32 + col)] == 0xFF) {
                    out.walls.push_back({static_cast<double>(col * 8),
                                         static_cast<double>(row * 6), 8, 6});
                }
            }
        }
    }
    auto push_mark = [&](int row, int col, std::uint8_t a, std::uint8_t b) {
        out.marks.push_back({static_cast<double>(col * 8), static_cast<double>(row * 6),
                             mark4_rows(a, b)});
    };
    if (snap.features) {
        for (const auto& p : snap.objects) push_mark(p.first, p.second, 0x00, 0x08);
        for (const auto& p : snap.creatures) push_mark(p.first, p.second, 0x10, 0x54);
    }
    push_mark(snap.player_row, snap.player_col, 0x24, 0x18);
    for (const auto& p : snap.verticals) push_mark(p.first, p.second, 0x3C, 0x24);
    return out;
}

}  // namespace dag
