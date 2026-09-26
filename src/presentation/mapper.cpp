#include "daggorath/mapper.hpp"

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
            if (snap.cells[i] == 0xFF) os << "SOLID " << (i / 32) << ' ' << (i % 32) << '\n';
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

}  // namespace dag
