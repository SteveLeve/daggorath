// Phase 6 map, examine, and text projections. Expected dumps are produced by
// tools/extract_text.py from MAPPER.ASM, PEXAM.ASM, STATUS.ASM, TXTSER.ASM,
// COMDAT.ASM, and SWCHAR.ASM — an independent Python transliteration.
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "daggorath/examine.hpp"
#include "daggorath/game.hpp"
#include "daggorath/mapper.hpp"
#include "daggorath/text.hpp"
#include "daggorath/text_tables.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

std::string text_dir() {
#ifdef DAG_TEXT_FIXTURE_DIR
    return DAG_TEXT_FIXTURE_DIR;
#else
    return "docs/archaeology/phase-6/fixtures/text";
#endif
}

std::string read_all(const std::string& path) {
    std::ifstream in(path);
    check(static_cast<bool>(in), "readable " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void check_file(const std::string& name, const std::string& got) {
    const std::string want = read_all(text_dir() + "/" + name);
    check(got == want, name, got == want ? "" : "projection disagrees with the Python dump");
}

std::vector<std::pair<int, int>> verticals(int level) {
    std::vector<std::pair<int, int>> out;
    for (int r = 0; r < 32; ++r) {
        for (int c = 0; c < 32; ++c) {
            if (dag::vfind(level, r, c) >= 0) out.push_back({r, c});
        }
    }
    return out;
}

void test_regions() {
    check(dag::kCentroidX == 128 && dag::kCentroidY == 76, "centroid is COMDAT VCNTRX/VCNTRY");
    check(dag::kViewportScanlineEnd == 152, "STSVDB starts at scanline 152");
    check(dag::kStatusScanlineEnd == 160, "status band ends at scanline 160");
    check(dag::kCommandScanlineEnd == 192, "PRIVDB ends at scanline 192");
    check(dag::kEmptyHand == "EMPTY", "M$EMPT is EMPTY");
}

void test_names() {
    check(dag::object_name(std::optional<dag::Ocb>{}) + "\n" == "EMPTY\n", "empty hand");
    dag::Ocb hidden;
    hidden.type = 17;
    hidden.cls = 4;
    hidden.reveal = 1;
    dag::Ocb shown = hidden;
    shown.reveal = 0;
    check_file("object-names.txt",
               "EMPTY\n" + dag::object_name(hidden) + "\n" + dag::object_name(shown) + "\n");
}

void test_status_lines() {
    dag::TextSnapshot empty;
    empty.heart = dag::HeartGlyph::Small;
    const std::string empty_full = dag::project_text(empty).text;
    const auto nl = empty_full.find('\n');
    check_file("status-empty.txt", empty_full.substr(0, nl + 1));

    dag::Ocb sword;
    sword.type = 17;
    sword.cls = 4;
    sword.reveal = 0;
    dag::Ocb torch;
    torch.type = 15;
    torch.cls = 5;
    torch.reveal = 0;
    dag::TextSnapshot hands;
    hands.left = sword;
    hands.right = torch;
    hands.heart = dag::HeartGlyph::Large;
    const std::string hands_full = dag::project_text(hands).text;
    const auto hnl = hands_full.find('\n');
    check_file("status-hands.txt", hands_full.substr(0, hnl + 1));

    dag::TextSnapshot mapped;
    mapped.heart = dag::HeartGlyph::Off;
    const std::string map_full = dag::project_text(mapped).text;
    check_file("status-map.txt", map_full.substr(0, map_full.find('\n') + 1));

    check_file("command-prompt.txt", "COMMAND .\nLINE \n");
    check_file("command-partial.txt", "COMMAND .\nLINE MOVE\n");
}

void test_examine() {
    dag::ExamineSnapshot empty;
    check_file("examine-empty.txt", dag::project_examine(empty).text);
    dag::ExamineSnapshot creature;
    creature.creature = true;
    check_file("examine-creature.txt", dag::project_examine(creature).text);
    dag::ExamineSnapshot stuff;
    stuff.floor = {"WOODEN SWORD", "PINE TORCH"};
    stuff.bag = {"LEATHER SHIELD"};
    stuff.torch_index = 0;
    check_file("examine-objects.txt", dag::project_examine(stuff).text);
}

void test_maps() {
    for (int level = 0; level < 5; ++level) {
        dag::Game game(1, level);
        dag::MapSnapshot snap;
        snap.cells = game.maze().bytes().data();
        snap.player_row = 0x10;
        snap.player_col = 0x0B;
        snap.verticals = verticals(level);
        check_file("map-level-" + std::to_string(level) + "-plain.txt",
                   dag::project_map(snap).text);
        snap.features = true;
        for (const auto& c : game.creatures()) {
            if (c.in_use) snap.creatures.push_back({c.row, c.col});
        }
        check_file("map-level-" + std::to_string(level) + "-features.txt",
                   dag::project_map(snap).text);
    }
}

}  // namespace

void test_glyph_a() {
    std::uint8_t rows[7] = {};
    dag::glyph_rows(1, rows);
    const std::uint8_t expect[7] = {0x10, 0x28, 0x44, 0x44, 0x7C, 0x44, 0x44};
    for (int i = 0; i < 7; ++i) {
        check(rows[i] == expect[i], "SWCTAB A row " + std::to_string(i));
    }
}

int main() {
    test_glyph_a();
    test_regions();
    test_names();
    test_status_lines();
    test_examine();
    test_maps();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks
              << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
