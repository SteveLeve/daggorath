// MAPPER.ASM top-view projection. Pure: reads a snapshot, mutates nothing.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "daggorath/game.hpp"

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

// MARK4: row 0 and 5 are blank, rows 1 and 4 are `a`, rows 2 and 3 are `b`.
// Shared by `rasterize_map`'s pixel plot and the `crisp` render style's
// bitmap marks (ADR-0010 §7), so there is one source for the pattern.
std::array<std::uint8_t, 6> mark4_rows(std::uint8_t a, std::uint8_t b);

// MAPPER.ASM on the 256x192 screen, one byte per pixel (0 or 1). Each maze cell
// is one byte (8 pixels) wide and 6 scanlines tall (DSP32), so the map fills
// the whole screen.
std::array<std::uint8_t, 256 * 192> rasterize_map(const MapSnapshot& snap);

MapSnapshot map_snapshot_from(const Game& game);

}  // namespace dag
