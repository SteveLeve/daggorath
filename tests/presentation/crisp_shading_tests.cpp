#include <cassert>
#include <cstdint>

#include "daggorath/crisp.hpp"

int main() {
    dag::RenderState state;
    state.segments.push_back({0, 1, 10, 11, 0, 0, "lit"});
    state.segments.push_back({20, 21, 30, 31, 0, 2, "dim"});
    state.segments.push_back({40, 41, 50, 51, 0, 0xFF, "hidden"});
    const auto frame = dag::build_crisp_frame(state);
    assert(frame.lines.size() == 2);
    assert(frame.lines[0].x0 == 0 && frame.lines[0].y1 == 11 && frame.lines[0].fade == 0);
    assert(frame.lines[1].x0 == 20 && frame.lines[1].y1 == 31 && frame.lines[1].fade == 2);
    assert(dag::crisp_shade(0, 255, 0) == 255);
    assert(dag::crisp_shade(2, 255, 0) == 127);
    assert(dag::crisp_shade(0, 0, 255) == 0);
    assert(dag::crisp_shade(2, 0, 255) == 127);
}
