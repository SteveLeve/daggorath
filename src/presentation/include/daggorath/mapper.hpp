// MAPPER.ASM top-view projection. Pure: reads a snapshot, mutates nothing.
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace dag {

struct MapSnapshot {
    const std::uint8_t* cells = nullptr;  // 1024 row-major maze bytes
    int player_row = 0;
    int player_col = 0;
    bool features = false;
    std::vector<std::pair<int, int>> objects;    // unowned, this level
    std::vector<std::pair<int, int>> creatures;  // live CCBs
    std::vector<std::pair<int, int>> verticals;  // VFTTAB from VFTPTR, both groups
};

struct MapProjection {
    std::string text;
};

MapProjection project_map(const MapSnapshot& snap);

}  // namespace dag
