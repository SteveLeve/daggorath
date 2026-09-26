#include "daggorath/text.hpp"
#include "daggorath/lexicon_tables.hpp"
#include "daggorath/raster.hpp"
#include "daggorath/text_tables.hpp"

#include <algorithm>

namespace dag {

std::string object_name(const std::optional<Ocb>& object) {
    if (!object) return std::string(kEmptyHand);
    const Ocb& o = *object;
    if (o.cls >= kGenTab.size()) return std::string(kEmptyHand);
    const std::string generic(kGenTab[o.cls].word);
    if (o.reveal != 0) return generic;
    if (o.type >= kAdjTab.size()) return generic;
    return std::string(kAdjTab[o.type].word) + " " + generic;
}

TextProjection project_text(const TextSnapshot& snap) {
    const std::string left = object_name(snap.left);
    const std::string right = object_name(snap.right);
    std::string row(32, ' ');
    const std::string l = left.substr(0, 15);
    row.replace(0, l.size(), l);
    std::string r = right.substr(0, 15);
    int start = 32 - static_cast<int>(r.size());
    if (start < 17) start = 17;
    row.replace(static_cast<std::size_t>(start), r.size(), r);
    if (snap.heart == HeartGlyph::Small) {
        row[15] = 's';
        row[16] = 's';
    } else if (snap.heart == HeartGlyph::Large) {
        row[15] = 'L';
        row[16] = 'L';
    }
    TextProjection out;
    out.text = "STATUS " + row + "\nCOMMAND .\nLINE " + snap.line + "\n";
    return out;
}

namespace {

int next5(const std::uint8_t* bytes, int& bit) {
    int value = 0;
    for (int i = 0; i < 5; ++i) {
        const int byte = bit / 8;
        const int shift = 7 - (bit % 8);
        value = (value << 1) | ((bytes[byte] >> shift) & 1);
        ++bit;
    }
    return value;
}

std::uint8_t code_for(char ch) {
    if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 32);
    if (ch == ' ') return 0;
    if (ch >= 'A' && ch <= 'Z') return static_cast<std::uint8_t>(ch - 'A' + 1);
    if (ch == '!') return 0x1B;
    if (ch == '_') return 0x1C;
    if (ch == '?') return 0x1D;
    if (ch == '.') return 0x1E;
    return 0;
}

void plot(std::uint8_t* pixels, int width, int col, int y, std::uint8_t code) {
    if (col < 0 || col >= 32 || y < 0) return;
    std::uint8_t rows[7] = {};
    glyph_rows(code, rows);
    for (int row = 0; row < 7; ++row) {
        const int py = y + row;
        if (py < 0 || py >= kScreenHeight) continue;
        for (int bit = 0; bit < 8; ++bit) {
            if ((rows[row] & (0x80 >> bit)) == 0) continue;
            pixels[static_cast<std::size_t>(py * width + col * 8 + bit)] = 1;
        }
    }
}

void plot_string(std::uint8_t* pixels, int width, int col, int y, std::string_view text) {
    for (char ch : text) {
        if (col >= 32) break;
        plot(pixels, width, col, y, code_for(ch));
        ++col;
    }
}

}  // namespace

void glyph_rows(std::uint8_t code, std::uint8_t rows[7]) {
    std::fill(rows, rows + 7, 0);
    if (code >= 0x20) {
        const int index = static_cast<int>(code - 0x20) * 7;
        if (index < 0 || index + 7 > static_cast<int>(kSpcTab.size())) return;
        for (int i = 0; i < 7; ++i) rows[i] = kSpcTab[static_cast<std::size_t>(index + i)];
        return;
    }
    if (static_cast<std::size_t>(code) * 5 + 5 > kSwcTab.size()) return;
    const std::uint8_t* packed = &kSwcTab[static_cast<std::size_t>(code) * 5];
    int bit = 0;
    next5(packed, bit);  // leading quintet; seven row values fill the rest of the five bytes
    for (int i = 0; i < 7; ++i) {
        rows[i] = static_cast<std::uint8_t>(next5(packed, bit) << 2);
    }
}

void paint_text_bands(std::uint8_t* pixels, int width,
                      const TextSnapshot& snap, std::string_view message) {
    for (int y = kViewportScanlineEnd; y < kScreenHeight; ++y) {
        std::fill(pixels + static_cast<std::size_t>(y * width),
                  pixels + static_cast<std::size_t>(y * width + kScreenWidth), 0);
    }
    const std::string status = project_text(snap).text.substr(7, 32);
    plot_string(pixels, width, 0, kViewportScanlineEnd, status);
    if (snap.heart == HeartGlyph::Small || snap.heart == HeartGlyph::Large) {
        const std::uint8_t base = snap.heart == HeartGlyph::Large ? 0x22 : 0x20;
        plot(pixels, width, 15, kViewportScanlineEnd, base);
        plot(pixels, width, 16, kViewportScanlineEnd, static_cast<std::uint8_t>(base + 1));
    }
    std::string command = ".";
    command += snap.line;
    if (command.size() < 32) command.push_back('_');
    plot_string(pixels, width, 0, kStatusScanlineEnd, command);
    plot_string(pixels, width, 0, kStatusScanlineEnd + 8, message.substr(0, 32));
}

}  // namespace dag
