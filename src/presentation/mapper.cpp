#include "daggorath/mapper.hpp"

#include "daggorath/population.hpp"

#include <algorithm>
#include <sstream>

namespace dag {
namespace {

void emit_marks(std::ostringstream& os, const char* kind,
                std::vector<std::pair<int, int>> marks) {
    std::sort(marks.begin(), marks.end());
    for (const auto& p : marks) os << kind << ' ' << p.first << ' ' << p.second << '\n';
}

}  // namespace

MapProjection project_map(const MapSnapshot& snap) {
    MapProjection out;
    std::ostringstream os;
    os << "MAP features=" << (snap.features ? 1 : 0) << '\n';
    if (snap.cells != nullptr) {
        for (int i = 0; i < 1024; ++i) {
            if (snap.cells[i] == 0xFF)
                os << "SOLID " << (i / 32) << ' ' << (i % 32) << '\n';
        }
    }
    if (snap.features) {
        emit_marks(os, "OBJECT", snap.objects);
        emit_marks(os, "CREATURE", snap.creatures);
    }
    os << "PLAYER " << snap.player_row << ' ' << snap.player_col << '\n';
    emit_marks(os, "VFEATURE", snap.verticals);
    out.text = os.str();
    return out;
}

std::array<std::uint8_t, 6> mark4_rows(std::uint8_t a, std::uint8_t b) {
    return {0, a, b, b, a, 0};
}

std::array<std::uint8_t, 256 * 192> rasterize_map(const MapSnapshot& snap) {
    std::array<std::uint8_t, 256 * 192> pixels{};
    auto put_byte = [&](int row, int col, int line, std::uint8_t bits) {
        const int y = row * 6 + line;
        for (int b = 0; b < 8; ++b)
            pixels[static_cast<std::size_t>(y * 256 + col * 8 + b)] =
                (bits >> (7 - b)) & 1;
    };
    // MARK4: A on scanlines 1 and 4, B on 2 and 3.
    auto mark4 = [&](int row, int col, std::uint8_t a, std::uint8_t b) {
        const auto rows = mark4_rows(a, b);
        for (int line = 1; line <= 4; ++line)
            put_byte(row, col, line, rows[static_cast<std::size_t>(line)]);
    };
    // MAPP10-22: a solid wall ($FF) cell is white for all 6 lines, else black.
    if (snap.cells != nullptr) {
        for (int row = 0; row < 32; ++row)
            for (int col = 0; col < 32; ++col)
                if (snap.cells[row * 32 + col] == 0xFF)
                    for (int line = 0; line < 6; ++line) put_byte(row, col, line, 0xFF);
    }
    if (snap.features) {  // MAPFLG: objects ($00/$08), then creatures ($10/$54)
        for (const auto& p : snap.objects) mark4(p.first, p.second, 0x00, 0x08);
        for (const auto& p : snap.creatures) mark4(p.first, p.second, 0x10, 0x54);
    }
    mark4(snap.player_row, snap.player_col, 0x24, 0x18);  // MAPP50: "X" marks the spot
    for (const auto& p : snap.verticals) mark4(p.first, p.second, 0x3C, 0x24);  // MAPP60
    return pixels;
}

MapSnapshot map_snapshot_from(const Game& game) {
    MapSnapshot snap;
    snap.cells = game.maze().bytes().data();
    snap.player_row = game.player().row;
    snap.player_col = game.player().col;
    snap.features = game.player().map_features;
    for (const auto& o : game.objects())
        if (o.owner == 0 && o.level == game.level_index())
            snap.objects.push_back({o.row, o.col});
    for (const auto& c : game.creatures())
        if (c.in_use) snap.creatures.push_back({c.row, c.col});
    for (int r = 0; r < 32; ++r)
        for (int col = 0; col < 32; ++col)
            if (vfind(game.level_index(), r, col) >= 0)
                snap.verticals.push_back({r, col});
    return snap;
}

}  // namespace dag
