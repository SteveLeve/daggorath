// Presentation projection. Depends on core values passed in; core does not
// include this header.
#pragma once
#include <string>
#include <vector>

namespace dag {

// NORSCL ratios from VIEWER.ASM, 128ths. Source-proven table, not a draw.
inline constexpr int kNormalScale[10] = {128, 128, 80, 50, 31, 20, 12, 8, 4, 2};

struct DrawSegment {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    std::string kind;
};

struct RenderState {
    std::vector<DrawSegment> segments;
    std::string text;
    std::string to_text() const;
};

struct ViewSnapshot {
    int row = 0;
    int col = 0;
    int dir = 0;  // 0 north, 1 east, 2 south, 3 west
    int regular_light = 0;
    int magic_light = 0;
    int mode = 0;  // 0 viewer, 1 examine, 2 mapper
    bool map_features = false;
    int foreground = -1;  // lit torch class, or -1
    int left_class = -1;
    int right_class = -1;
    // Cell underfoot and the next four cells along the facing, 0xFF = solid.
    int ahead[5] = {0, 0, 0, 0, 0};
};

// Logical 256-wide surface. Centroid (128, 76) is the Phase 0 report figure;
// the scale bytes are from VIEWER.ASM. Segment positions are the scale applied
// to a facing ray. They are not a VCTLST decode.
RenderState project(const ViewSnapshot& view);

}  // namespace dag
