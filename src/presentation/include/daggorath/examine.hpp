// PEXAM.ASM EXAMIN projection. Logical character cells, not pixels.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/game.hpp"

namespace dag {

struct ExamineSnapshot {
    bool creature = false;
    std::vector<std::string> floor;
    std::vector<std::string> bag;
    int torch_index = -1;  // inverse name in the bag, or -1
};

struct ExamineProjection {
    std::string text;   // text dump; an inverse name starts with '*'
    // The 19 x 32 TXTEXA cells as printed, and which cells print inverse
    // (the lit torch's whole name: COM P.TXINV until PRTOBJ restores VDGINV).
    std::array<std::array<char, 32>, 19> cells{};
    std::array<std::array<bool, 32>, 19> inverse{};
};

ExamineProjection project_examine(const ExamineSnapshot& snap);

// EXAMIN's inputs: CFIND at PROW, OFIND's unowned objects here, the bag chain.
ExamineSnapshot examine_snapshot_from(const Game& game);

// EXAMIO: ZFLOP, then TXTEXA text (32 x 19) from the video base: 19 rows of 8
// scanlines over the viewport (scanlines 0-151). Leaving the bands alone is
// inferred (phase-7 reconciliation).
void paint_examine(std::uint8_t* pixels, int width, const ExamineProjection& page);

// MISC.ASM PREPAX: EXAMIO's ZFLOP blanks the viewport, then PREPARE! at TXTEXA
// cursor 32*9+12, shown while PCLIMB's NEWLVL builds.
void paint_prepare(std::uint8_t* pixels, int width);

}  // namespace dag
