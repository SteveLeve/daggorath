// Prints the `crisp` draw list (ADR-0010) for the same golden states
// `tools/viewer_ref.py` fixes in docs/archaeology/phase-6/fixtures, plus one
// map snapshot. `tests/CMakeLists.txt` regenerates this output and requires
// it to match the committed fixture byte for byte (the phase-3 trace
// pattern) — proof that `crisp` and `pixel` project the same draw list
// (ADR-0010 §2), not a second geometry source.
#include <iomanip>
#include <iostream>
#include <string>

#include "daggorath/crisp.hpp"
#include "daggorath/game.hpp"
#include "daggorath/mapper.hpp"
#include "daggorath/snapshot.hpp"

namespace {

void print_frame(const std::string& label, const dag::CrispFrame& frame) {
    std::cout << "FRAME " << label << " lines=" << frame.lines.size() << '\n';
    for (const auto& line : frame.lines) {
        std::cout << "LINE " << line.kind << ' ' << line.x0 << ' ' << line.y0 << ' '
                  << line.x1 << ' ' << line.y1 << ' ' << static_cast<int>(line.fade) << '\n';
    }
}

}  // namespace

int main() {
    static const char* tags[] = {"dark", "regular", "magic"};
    static const int lights[][2] = {{0, 0}, {7, 0}, {0, 13}};
    for (int level = 0; level < 5; ++level) {
        dag::Game game(1, level);
        for (int i = 0; i < 3; ++i) {
            dag::ViewSnapshot view = dag::snapshot_from(game);
            view.regular_light = lights[i][0];
            view.magic_light = lights[i][1];
            const dag::RenderState state = dag::project(view);
            const dag::CrispFrame frame = dag::build_crisp_frame(state);
            print_frame("level-" + std::to_string(level) + "-" + tags[i], frame);
        }
    }

    dag::Game game(1, 0);
    dag::MapSnapshot snap = dag::map_snapshot_from(game);
    snap.features = true;
    const dag::CrispMap map = dag::build_crisp_map(snap);
    std::cout << "MAP walls=" << map.walls.size() << " marks=" << map.marks.size()
              << '\n';
    for (const auto& wall : map.walls) {
        std::cout << "WALL " << wall.x << ' ' << wall.y << ' ' << wall.w << ' ' << wall.h
                  << '\n';
    }
    for (const auto& mark : map.marks) {
        std::cout << "MARK " << mark.x << ' ' << mark.y;
        for (const auto row : mark.rows) std::cout << ' ' << static_cast<int>(row);
        std::cout << '\n';
    }
    return 0;
}
