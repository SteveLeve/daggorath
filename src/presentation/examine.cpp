#include "daggorath/examine.hpp"
#include "daggorath/text_tables.hpp"

#include <array>
#include <string>
#include <string_view>

namespace dag {
namespace {

struct Pad {
    std::array<std::array<char, 32>, 19> grid{};
    int cur = 0;
    bool inverse_next = false;

    Pad() {
        for (auto& row : grid) row.fill(' ');
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

void print_names(Pad& pad, const std::vector<std::string>& names, int torch) {
    bool newline = false;
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (i == torch) pad.inverse_next = true;
        pad.write(names[static_cast<std::size_t>(i)]);
        newline = !newline;
        if (newline) pad.cur = (pad.cur + 16) & ~15;
        else pad.put('\n');
    }
    if (newline) pad.put('\n');
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
    print_names(pad, snap.floor, -1);
    pad.write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    pad.cur += 12;
    pad.write(kExamBackpack);
    pad.put('\n');
    print_names(pad, snap.bag, snap.torch_index);
    ExamineProjection out;
    out.text = pad.to_text();
    return out;
}

}  // namespace dag
