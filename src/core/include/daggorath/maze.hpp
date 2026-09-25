// Daggorath Core — 32x32 maze and the original dungeon generator.
// Source: DGNGEN.ASM (DGNGEN, FRIEND, RNDCEL, MAP32, BORDER, MAKDOR, DGEN90,
//         LVLTAB, MSKTAB, DORTAB, SDRTAB); CRETUR.ASM (STEP, STPTAB, STEPOK);
//         CD.ASM (MAZLND, HF.DOR, HF.SDR).
#pragma once
#include <array>
#include <cstdint>

#include "daggorath/rng.hpp"

namespace dag {

enum class Edge : std::uint8_t { Passage = 0, RegularDoor = 1, SecretDoor = 2, Wall = 3 };
enum class Dir : std::uint8_t { North = 0, East = 1, South = 2, West = 3 };

// Serialization contract (must match fixtures/maze-level-N.bin):
//   1024 bytes, row-major, index = row * 32 + col, one byte per cell,
//   bit pairs low-to-high = North, East, South, West.
class Maze {
public:
    static constexpr int kSize = 32;
    static constexpr int kBytes = kSize * kSize;

    Maze() { cells_.fill(0xFF); }

    std::uint8_t at(int row, int col) const { return cells_[index(row, col)]; }
    void put(int row, int col, std::uint8_t v) { cells_[index(row, col)] = v; }

    Edge edge(int row, int col, Dir d) const {
        const int shift = 2 * static_cast<int>(d);
        return static_cast<Edge>((at(row, col) >> shift) & 0x3u);
    }

    const std::array<std::uint8_t, kBytes>& bytes() const { return cells_; }

    static int index(int row, int col) {
        return ((row & 31) * kSize) + (col & 31);
    }
    // BORDER: a position is legal only if MOD 32 leaves it unchanged.
    static bool in_bounds(int row, int col) {
        return (row & 31) == (row & 0xFF) && (col & 31) == (col & 0xFF);
    }

private:
    std::array<std::uint8_t, kBytes> cells_{};
};

// LVLTAB, read from DGNGEN.ASM. The three-byte seed windows overlap:
// seed(level) = { LVLTAB[level], LVLTAB[level+1], LVLTAB[level+2] }.
inline constexpr std::array<std::uint8_t, 7> kLvlTab{
    0x73, 0xC7, 0x5D, 0x97, 0xF3, 0x13, 0x87};

inline Rng::Seed level_seed(int level) {
    return {kLvlTab[static_cast<std::size_t>(level)],
            kLvlTab[static_cast<std::size_t>(level) + 1],
            kLvlTab[static_cast<std::size_t>(level) + 2]};
}

// One step in `d` from (row, col), wrapping in 8 bits exactly as STEP does.
inline void step(int& row, int& col, Dir d) {
    static constexpr int dr[4] = {-1, 0, 1, 0};
    static constexpr int dc[4] = {0, 1, 0, -1};
    row = (row + dr[static_cast<int>(d)]) & 0xFF;
    col = (col + dc[static_cast<int>(d)]) & 0xFF;
}

// STEPOK: a step is legal iff the destination is in bounds and the destination
// cell is not the never-carved pattern $FF. The original performs no per-edge
// wall test; Phase II of DGNGEN only raises a wall bit toward a $FF neighbour,
// so the two rules coincide. Doors do not block movement.
inline bool step_ok(const Maze& m, int row, int col, Dir d, int& out_row, int& out_col) {
    int r = row, c = col;
    step(r, c, d);
    if (!Maze::in_bounds(r, c)) return false;
    if (m.at(r, c) == 0xFF) return false;
    out_row = r;
    out_col = c;
    return true;
}

// Result of generating a level, including the RNG state around DGEN90.
struct GeneratedLevel {
    Maze maze;
    Rng::Seed rng_before_spin{};
    Rng::Seed rng_after_spin{};
    int spin_count = 0;
    Rng rng{};  // post-spin generator, for creature placement etc.
};

// DGNGEN. `second` is the SECOND counter at the moment the maze finishes;
// DGEN90 draws that many times (LDB SECOND / DEC B / BNE => 256 when SECOND is 0).
GeneratedLevel generate_level(int level, std::uint8_t second);

}  // namespace dag
