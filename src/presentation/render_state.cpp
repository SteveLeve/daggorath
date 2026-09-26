#include "daggorath/render_state.hpp"

#include <sstream>

namespace dag {

std::string RenderState::to_text() const {
    std::ostringstream os;
    os << text << '\n';
    for (const DrawSegment& segment : segments) {
        os << segment.kind << ' ' << segment.x0 << ' ' << segment.y0 << ' ' << segment.x1
           << ' ' << segment.y1 << '\n';
    }
    return os.str();
}

RenderState project(const ViewSnapshot& view) {
    RenderState state;
    if (view.mode == 2) {
        state.text = view.map_features ? "MAP features" : "MAP plain";
        return state;
    }
    if (view.mode == 1) {
        state.text = "EXAMINE";
        return state;
    }
    const int light = view.regular_light + view.magic_light;
    state.text = "VIEW light=" + std::to_string(light);
    if (light == 0) return state;
    static constexpr int dr[4] = {-1, 0, 1, 0};
    static constexpr int dc[4] = {0, 1, 0, -1};
    (void)dr;
    (void)dc;
    for (int distance = 1; distance <= 4; ++distance) {
        if (view.ahead[distance] == 0xFF) break;
        const int scale = kNormalScale[distance];
        const int half = scale / 2;
        DrawSegment wall;
        wall.kind = "wall";
        wall.x0 = 128 - half;
        wall.y0 = 76 - half / 2;
        wall.x1 = 128 + half;
        wall.y1 = 76 + half / 2;
        state.segments.push_back(wall);
    }
    return state;
}

}  // namespace dag
