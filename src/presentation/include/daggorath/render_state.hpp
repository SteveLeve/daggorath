// Presentation projection. Depends on core values passed in; core does not
// include this header.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/maze.hpp"

namespace dag {

struct DrawSegment {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    int range = 0;
    int fade = 0;
    std::string kind;
};

struct SeenCreature {
    int row = 0;
    int col = 0;
    int type = 0;
    int magic_offense = 0;
};

struct SeenObject {
    int row = 0;
    int col = 0;
    int level = 0;
    int owner = 0;
    int cls = 0;
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
    int level = 0;
    Maze maze;
    std::vector<SeenCreature> creatures;
    std::vector<SeenObject> objects;
    // Cell underfoot and the next four cells along the facing, 0xFF = solid.
    int ahead[5] = {0, 0, 0, 0, 0};
};

// Logical 256-wide surface. VIEWER.ASM range walk, VCTLST decode, VARC/VERT/VOBJ/D3/D4 lists.
RenderState project(const ViewSnapshot& view);

}  // namespace dag
