#include "daggorath/examine.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/text.hpp"
#include "daggorath/text_tables.hpp"

#include <algorithm>

#include <array>
#include <string>
#include <string_view>

namespace dag {
namespace {

struct Pad {
    std::array<std::array<char, 32>, 19> grid{};
    std::array<std::array<char, 32>, 19> real{};
    std::array<std::array<bool, 32>, 19> inv{};
    int cur = 0;
    bool inverse_next = false;
    bool inverse_run = false;

    Pad() {
        for (auto& row : grid) row.fill(' ');
        for (auto& row : real) row.fill(' ');
    }

    void put(char ch) {
        if (ch == '\n') {
            cur = (cur + 32) & ~31;
            return;
        }
        const int r = cur >> 5;
        const int c = cur & 31;
        if (r >= 0 && r < kExamineRows && c >= 0 && c < kExamineCols) {
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] =
                inverse_next ? '*' : ch;
            real[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = ch;
            inv[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = inverse_run;
        }
        ++cur;
        inverse_next = false;
    }

    void write(std::string_view s) {
        for (char ch : s) put(ch);
    }

    std::string to_text() const {
        int last = 0;
        for (int i = 0; i < kExamineRows; ++i) {
            bool used = false;
            for (char ch : grid[static_cast<std::size_t>(i)]) {
                if (ch != ' ') {
                    used = true;
                    break;
                }
            }
            if (used) last = i;
        }
        std::string out;
        for (int i = 0; i <= last; ++i) {
            out.append(grid[static_cast<std::size_t>(i)].data(), 32);
            out.push_back('\n');
        }
        return out;
    }
};

void print_names(Pad& pad, const std::vector<std::string>& names, int torch, bool close_line) {
    bool newline = false;
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (i == torch) pad.inverse_next = pad.inverse_run = true;
        pad.write(names[static_cast<std::size_t>(i)]);
        pad.inverse_run = false;   // PRTOBJ: LDA VDGINV / STA P.TXINV
        newline = !newline;
        if (newline) pad.cur = (pad.cur + 16) & ~15;
        else pad.put('\n');
    }
    // EXAM20 closes an odd floor list; the bag list (EXAM30 -> EXAM99) is left open.
    if (newline && close_line) pad.put('\n');
}

}  // namespace

ExamineProjection project_examine(const ExamineSnapshot& snap) {
    Pad pad;
    pad.cur = 10;
    pad.write(kExamRoom);
    pad.put('\n');
    if (snap.creature) {
        pad.cur += 11;
        pad.write(kExamCreature);
        pad.put('\n');
    }
    print_names(pad, snap.floor, -1, true);
    pad.write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    pad.cur += 12;
    pad.write(kExamBackpack);
    pad.put('\n');
    print_names(pad, snap.bag, snap.torch_index, false);
    ExamineProjection out;
    out.text = pad.to_text();
    out.cells = pad.real;
    out.inverse = pad.inv;
    return out;
}

void paint_examine(std::uint8_t* pixels, int width, const ExamineProjection& page) {
    for (int y = 0; y < kExamineRows * 8; ++y)
        std::fill(pixels + static_cast<std::size_t>(y * width),
                  pixels + static_cast<std::size_t>(y * width + 256), 0);
    for (int row = 0; row < kExamineRows; ++row)
        for (int col = 0; col < kExamineCols; ++col)
            plot_cell(pixels, width, col, row * 8,
                      page.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)],
                      page.inverse[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]);
}

ExamineSnapshot examine_snapshot_from(const Game& game) {
    ExamineSnapshot exam;
    for (const auto& c : game.creatures())
        if (c.in_use && c.row == game.player().row && c.col == game.player().col) exam.creature = true;
    for (const auto& o : game.objects())
        if (o.owner == 0 && o.level == game.level_index() && o.row == game.player().row &&
            o.col == game.player().col)
            exam.floor.push_back(object_name(o));
    int bag_i = 0;
    for (int i = game.player().bag_head; i >= 0; i = game.objects()[static_cast<std::size_t>(i)].next) {
        exam.bag.push_back(object_name(game.objects()[static_cast<std::size_t>(i)]));
        if (i == game.player().torch) exam.torch_index = bag_i;
        ++bag_i;
    }
    return exam;
}

void paint_prepare(std::uint8_t* pixels, int width) {
    for (int y = 0; y < kViewportScanlineEnd; ++y)
        std::fill(pixels + static_cast<std::size_t>(y * width),
                  pixels + static_cast<std::size_t>(y * width + kScreenWidth), 0);
    const std::string_view word = "PREPARE!";
    for (std::size_t i = 0; i < word.size(); ++i)
        plot_cell(pixels, width, 12 + static_cast<int>(i), 9 * 8, word[i], false);
}

}  // namespace dag
