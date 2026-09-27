// The `crisp` render style (ADR-0010): the same logical draw list `pixel`
// rasterises, mapped to line segments and dots instead of a fixed-scale
// bitmap. No second geometry source — every coordinate here comes from
// `RenderState::segments` (the listing's VECTOR/VCTLST data, ADR-0006) or
// `raster.hpp`'s `walk_segment`, the same walk `pixel`'s `draw_segment`
// uses. Coordinates stay in source 256x192 space; the platform (SDL3) scales
// to real device pixels and sets line thickness there (ADR-0010 §2), which
// this headless module does not do.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/mapper.hpp"
#include "daggorath/render_state.hpp"

namespace dag {

// A fully-lit segment (fade == 0): draw_segment plots every step, so crisp
// draws one continuous line instead of individually dotting it.
struct CrispLine {
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    std::string kind;  // DrawSegment::kind, carried through for the platform
};

// A dim segment (0 < fade < 0xFF): the same dot draw_segment would plot,
// carried at source resolution (ADR-0010 §8 — dotted, not a colour blend).
struct CrispDot {
    double x = 0, y = 0;
    std::string kind;
};

struct CrispFrame {
    std::vector<CrispLine> lines;
    std::vector<CrispDot> dots;
};

// Builds the crisp draw list from `state.segments`. A segment with
// fade == 0xFF (draw_segment's "draw nothing") contributes nothing, matching
// `pixel`.
CrispFrame build_crisp_frame(const RenderState& state);

// ADR-0010 §7: a solid-wall maze cell (MAPPER.ASM's $FF cell,
// `rasterize_map`'s `MAPP10-22`) is one filled 8x6 source-pixel square.
struct CrispMapSquare {
    double x = 0, y = 0, w = 8, h = 6;
};

// The player, object, creature and vertical-feature marks `rasterize_map`'s
// `MARK4` plots are small bitmaps, not solid squares (two distinct 8-bit rows
// repeated per `mapper.cpp`'s `mark4`); crisp keeps them as nearest-neighbour
// bitmap cells, the same choice ADR-0010 §6 makes for text.
struct CrispMapMark {
    double x = 0, y = 0;
    std::array<std::uint8_t, 6> rows{};  // one bitmap row per scanline, 8 bits each
};

struct CrispMap {
    std::vector<CrispMapSquare> walls;
    std::vector<CrispMapMark> marks;
};

// Builds the crisp map draw list from the same `MapSnapshot` projection
// `rasterize_map` (mapper.hpp) rasterises: no second geometry source.
CrispMap build_crisp_map(const MapSnapshot& snap);

}  // namespace dag
