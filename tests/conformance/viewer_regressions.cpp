// Phase 6 VIEWER draw-list regressions.
// Expected text comes from tools/viewer_ref.py, which is an independent
// transliteration of VIEWER.ASM / VCTLST.ASM. The C++ projection must match
// those files; it must not be compared only to itself.
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "daggorath/raster.hpp"
#include "daggorath/game.hpp"
#include "daggorath/render_state.hpp"
#include "daggorath/snapshot.hpp"
#include "daggorath/vctlst.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;
int g_matches = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

std::string slurp(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

#ifndef DAG_PHASE6_DIR
#define DAG_PHASE6_DIR "docs/archaeology/phase-6/fixtures"
#endif

void test_draw_lists() {
    static const char* tags[] = {"dark", "regular", "magic"};
    static const int lights[][2] = {{0, 0}, {7, 0}, {0, 13}};
    for (int level = 0; level < 5; ++level) {
        dag::Game game(1, level);
        for (int i = 0; i < 3; ++i) {
            dag::ViewSnapshot view = dag::snapshot_from(game);
            view.regular_light = lights[i][0];
            view.magic_light = lights[i][1];
            const std::string got = dag::project(view).to_text();
            const std::string path = std::string(DAG_PHASE6_DIR) + "/draw-level-" +
                                     std::to_string(level) + "-start-" + tags[i] + ".txt";
            const std::string expected = slurp(path);
            const bool ok = got == expected && !expected.empty();
            check(ok, path);
            if (ok) ++g_matches;
        }
    }
}

void test_half_step_scale() {
    dag::ViewSnapshot standing;
    standing.regular_light = 7;
    dag::ViewSnapshot halfway = standing;
    halfway.scale = 1;
    const auto near = dag::project(standing);
    const auto mid = dag::project(halfway);
    check(!near.segments.empty() && near.segments.size() == mid.segments.size(),
          "a half-step keeps the same walls");
    if (!near.segments.empty() && near.segments.size() == mid.segments.size()) {
        check(near.segments[0].x0 != mid.segments[0].x0 || near.segments[0].y0 != mid.segments[0].y0,
              "HLFSCL draws the cell being left larger than the standing view");
    }
}

void test_decode_still_independent() {
    const std::uint8_t torch[] = {118, 60, 0xFC, 0xF7, 0xFF, 0x2A, 0x00, 0xFE};
    const auto torch_lines = dag::decode_vectors(torch, 128, 128, 128, 76, 0);
    check(torch_lines.size() == 3 && torch_lines.back().x1 == 60 && torch_lines.back().y1 == 118,
          "forward torch list closes on its tip");
}

}  // namespace

// NEWLVL.ASM:83-90 NLVL50 (source-proven): VDGINV = -(LEVEL & 1), so odd levels invert every dot.
void test_vdginv_polarity() {
    check(dag::vdginv(0) == 0x00 && dag::vdginv(2) == 0x00, "even levels are VDGINV $00");
    check(dag::vdginv(1) == 0xFF && dag::vdginv(3) == 0xFF, "odd levels are VDGINV $FF");
    std::array<std::uint8_t, dag::kScreenWidth * dag::kScreenHeight> frame{};
    frame[5] = 1;
    auto even = frame;
    dag::apply_vdginv(even, 4);
    check(even == frame, "an even level leaves the frame as composed");
    auto odd = frame;
    dag::apply_vdginv(odd, 1);
    check(odd[5] == 0 && odd[6] == 1 && odd.back() == 1, "an odd level inverts every dot");
}

int main() {
    test_vdginv_polarity();
    test_draw_lists();
    test_half_step_scale();
    test_decode_still_independent();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks
              << " checks, " << g_failures << " failures, " << g_matches
              << " draw-list fixtures matched\n";
    return g_failures == 0 ? 0 : 1;
}
