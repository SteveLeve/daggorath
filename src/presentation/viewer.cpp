#include "daggorath/viewer.hpp"

#include "daggorath/population.hpp"
#include "daggorath/vctlst.hpp"
#include "daggorath/vector_tables.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace dag {
namespace {

int set_fade(int light, int range) {
    const int a = light - 7 - range;
    if (a >= 0) return 0;
    if (a <= -7) return 0xFF;
    return kBitmsk[static_cast<std::size_t>(8 + a)];
}

const SeenCreature* cfind(const ViewSnapshot& view, int row, int col) {
    for (const SeenCreature& c : view.creatures) {
        if (c.row == row && c.col == col) return &c;
    }
    return nullptr;
}

void append_list(RenderState& state, std::size_t offset, int range, int fade,
                 const std::string& kind, const std::array<int, 10>& scales) {
    if (fade == 0xFF) return;
    const int scale = scales[static_cast<std::size_t>(range)];
    auto lines = decode_vectors(kVectorBlob, static_cast<std::uint8_t>(scale),
                                static_cast<std::uint8_t>(scale), kCentroidX, kCentroidY,
                                static_cast<std::uint8_t>(fade), offset);
    for (DrawSegment& line : lines) {
        line.kind = kind;
        line.range = range;
        line.fade = fade;
        state.segments.push_back(line);
    }
}

void drawit(RenderState& state, const ViewSnapshot& view, std::size_t offset, int range,
            bool magic, const std::string& kind) {
    const int light = magic ? view.magic_light : view.regular_light;
    const std::array<int, 10>& scales =
        view.scale == 1 ? kHlfscl : view.scale == 2 ? kBakscl : kNorscl;
    append_list(state, offset, range, set_fade(light, range), kind, scales);
}

}  // namespace

RenderState project_viewer(const ViewSnapshot& view) {
    RenderState state;
    if (view.mode == 2) {
        state.text = view.map_features ? "MAP features" : "MAP plain";
        return state;
    }
    if (view.mode == 1) {
        state.text = "EXAMINE";
        return state;
    }
    state.text = "VIEW rlight=" + std::to_string(view.regular_light) +
                 " mlight=" + std::to_string(view.magic_light);
    static constexpr int dr[4] = {-1, 0, 1, 0};
    static constexpr int dc[4] = {0, 1, 0, -1};
    const int pdir = view.dir & 3;
    int row = view.row;
    int col = view.col;
    for (int range = 0; range <= 9; ++range) {
        std::uint8_t cell = 0xFF;
        if (row >= 0 && col >= 0 && row < 32 && col < 32) cell = view.maze.at(row, col);
        const int north = cell & 3;
        const int east = (cell >> 2) & 3;
        const int south = (cell >> 4) & 3;
        const int west = (cell >> 6) & 3;
        const int pairs[4] = {north, east, south, west};
        int rel[4];
        for (int i = 0; i < 4; ++i) rel[i] = pairs[(pdir + i) & 3];
        for (const FlaEntry& entry : kFlaTab) {
            int feat = rel[entry.rel & 3];
            if (feat == 2) {
                drawit(state, view, entry.lists[2], range, true, "architecture");
                feat = 3;
            }
            drawit(state, view, entry.lists[static_cast<std::size_t>(feat)], range, false,
                   "architecture");
        }
        if (const SeenCreature* cre = cfind(view, row, col)) {
            drawit(state, view, kFwdCre[static_cast<std::size_t>(cre->type)], range,
                   cre->magic_offense != 0, "creature");
        }
        for (const auto& peek : {std::pair<int, std::size_t>{3, kLpeek}, {1, kRpeek}}) {
            if (rel[peek.first] != 0) continue;
            const int d = (pdir + peek.first) & 3;
            const int nr = row + dr[d];
            const int nc = col + dc[d];
            if (const SeenCreature* seen = cfind(view, nr, nc)) {
                drawit(state, view, peek.second, range, seen->magic_offense != 0, "peek");
            }
        }
        const int vf = vfind(view.level, row, col);
        if (vf < 0) drawit(state, view, kCeline, range, false, "vertical");
        else drawit(state, view, kFwdVer[static_cast<std::size_t>(vf)], range, false, "vertical");
        for (const SeenObject& obj : view.objects) {
            if (obj.level != view.level || obj.row != row || obj.col != col) continue;
            if (obj.owner != 0) continue;
            drawit(state, view, kFwdObj[static_cast<std::size_t>(obj.cls)], range, true, "object");
            drawit(state, view, kFwdObj[static_cast<std::size_t>(obj.cls)], range, false, "object");
        }
        if (rel[0] != 0) break;
        row = (row + dr[pdir]) & 0xFF;
        col = (col + dc[pdir]) & 0xFF;
        if (row > 31 || col > 31) break;
    }
    return state;
}

}  // namespace dag
