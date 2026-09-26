#include "daggorath/render_state.hpp"
#include "daggorath/viewer.hpp"

#include <sstream>

namespace dag {

std::string RenderState::to_text() const {
    std::ostringstream os;
    os << text << '\n';
    for (const DrawSegment& segment : segments) {
        os << segment.kind << ' ' << segment.x0 << ' ' << segment.y0 << ' ' << segment.x1
           << ' ' << segment.y1 << ' ' << segment.range << ' ' << segment.fade << '\n';
    }
    return os.str();
}

RenderState project(const ViewSnapshot& view) { return project_viewer(view); }

}  // namespace dag
