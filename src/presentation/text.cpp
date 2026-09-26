#include "daggorath/text.hpp"
#include "daggorath/lexicon_tables.hpp"
#include "daggorath/text_tables.hpp"

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

}  // namespace dag
