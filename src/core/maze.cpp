#include "daggorath/maze.hpp"

#include <array>

namespace dag {
namespace {

constexpr std::uint8_t kNWall = 0x03, kEWall = 0x0C, kSWall = 0x30, kWWall = 0xC0;
constexpr std::uint8_t kMskTab[4] = {0x03, 0x0C, 0x30, 0xC0};              // MSKTAB
constexpr std::uint8_t kDorTab[4] = {1 * 1, 1 * 4, 1 * 16, 1 * 64};        // DORTAB
constexpr std::uint8_t kSdrTab[4] = {2 * 1, 2 * 4, 2 * 16, 2 * 64};        // SDRTAB

// FRIEND: 3x3 neighbourhood centred on (row,col); out-of-bounds reads as $FF.
void friends(const Maze& m, int row, int col, std::array<std::uint8_t, 9>& out) {
    int k = 0;
    for (int r = row - 1; r <= row + 1; ++r) {
        for (int c = col - 1; c <= col + 1; ++c) {
            const int rr = r & 0xFF, cc = c & 0xFF;
            out[static_cast<std::size_t>(k++)] =
                Maze::in_bounds(rr, cc) ? m.at(rr, cc) : 0xFF;
        }
    }
}

// RNDCEL: the FIRST draw is the column, the SECOND is the row.
void rnd_cel(Rng& rng, int& row, int& col) {
    col = rng.next() & 31;
    row = rng.next() & 31;
}

void make_door(Rng& rng, Maze& m, const std::uint8_t (&table)[4]) {
    for (;;) {
        int r = 0, c = 0;
        rnd_cel(rng, r, c);
        const std::uint8_t cell = m.at(r, c);
        if (cell == 0xFF) continue;                       // never-carved cell
        const int d = rng.next() & 3;
        if (cell & kMskTab[d]) continue;                  // edge is not a passage
        m.put(r, c, static_cast<std::uint8_t>(cell | table[d]));
        int nr = r, nc = c;
        step(nr, nc, static_cast<Dir>(d));
        const int opp = (d + 2) & 3;                      // fix the adjoining cell
        m.put(nr, nc, static_cast<std::uint8_t>(m.at(nr, nc) | table[opp]));
        return;
    }
}

}  // namespace

GeneratedLevel generate_level(int level, std::uint8_t second) {
    GeneratedLevel out;
    Rng rng(level_seed(level));
    Maze& m = out.maze;   // constructed as all $FF (NEGRAM MAZLND..MAZEND)

    // ---- Phase I: carve 500 cells -------------------------------------
    int cells_left = 500;
    int drow = 0, dcol = 0;
    rnd_cel(rng, drow, dcol);

    int dir = 0, dst = 0, row = 0, col = 0;
    enum State { Pick, Advance, Tentative } state = Pick;
    for (;;) {
        if (state == Pick) {                              // DGEN10
            dir = rng.next() & 3;
            dst = (rng.next() & 7) + 1;
            state = Tentative;
            continue;
        }
        if (state == Advance) {                           // DGEN20
            drow = row;
            dcol = col;
            dst = (dst - 1) & 0xFF;
            state = (dst == 0) ? Pick : Tentative;
            continue;
        }
        // DGEN30: tentative step
        int nr = drow, nc = dcol;
        step(nr, nc, static_cast<Dir>(dir));
        if (!Maze::in_bounds(nr, nc)) { state = Pick; continue; }
        row = nr;
        col = nc;
        if (m.at(row, col) == 0) { state = Advance; continue; }   // already carved

        std::array<std::uint8_t, 9> n{};
        friends(m, row, col, n);
        static constexpr int kCorners[4][3] = {{3, 0, 1}, {1, 2, 5}, {5, 8, 7}, {7, 6, 3}};
        bool rejected = false;
        for (const auto& trio : kCorners) {
            const std::uint8_t sum = static_cast<std::uint8_t>(
                n[static_cast<std::size_t>(trio[0])] + n[static_cast<std::size_t>(trio[1])] +
                n[static_cast<std::size_t>(trio[2])]);
            if (sum == 0) { rejected = true; break; }
        }
        if (rejected) { state = Pick; continue; }

        m.put(row, col, 0);
        if (--cells_left == 0) break;
        state = Advance;
    }

    // ---- Phase II: raise walls toward never-carved neighbours ----------
    for (int r = 0; r < Maze::kSize; ++r) {
        for (int c = 0; c < Maze::kSize; ++c) {
            const std::uint8_t cell = m.at(r, c);
            if (cell == 0xFF) continue;
            std::array<std::uint8_t, 9> n{};
            friends(m, r, c, n);
            std::uint8_t v = cell;
            if (n[1] == 0xFF) v |= kNWall;
            if (n[3] == 0xFF) v |= kWWall;
            if (n[5] == 0xFF) v |= kEWall;
            if (n[7] == 0xFF) v |= kSWall;
            m.put(r, c, v);
        }
    }

    // ---- Doors --------------------------------------------------------
    for (int i = 0; i < 70; ++i) make_door(rng, m, kDorTab);
    for (int i = 0; i < 45; ++i) make_door(rng, m, kSdrTab);

    out.rng_before_spin = rng.seed();

    // ---- DGEN90: clock-dependent spin, AFTER the maze is complete ------
    out.spin_count = (second == 0) ? 256 : static_cast<int>(second);
    for (int i = 0; i < out.spin_count; ++i) rng.next();
    out.rng_after_spin = rng.seed();
    out.rng = rng;
    return out;
}

}  // namespace dag
