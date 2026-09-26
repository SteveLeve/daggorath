#include "daggorath/parser.hpp"

namespace dag {

bool next_token(std::string_view line, std::size_t& pos, std::string& out) {
    out.clear();
    while (pos < line.size() && line[pos] == ' ') ++pos;   // GTOK10: eat spaces
    while (pos < line.size() && line[pos] != ' ') out.push_back(line[pos++]);
    return !out.empty();                                   // TST TOKEN / N flag
}

ParseResult parse(std::span<const TokenEntry> table, std::string_view line,
                  std::size_t& pos) {
    ParseResult r;
    if (!next_token(line, pos, r.token)) {
        r.status = ParseStatus::NoToken;                   // PARS92: null token
        return r;
    }
    bool seen = false;                                     // PARFLG
    for (const TokenEntry& e : table) {
        if (e.word.size() < r.token.size()) continue;
        if (e.word.compare(0, r.token.size(), r.token) != 0) continue;
        if (seen) {                                        // PARS90: two matches
            r.status = ParseStatus::NoMatch;
            return r;
        }
        seen = true;
        r.status = ParseStatus::Matched;
        r.type = e.index;
        r.token_class = e.token_class;
        r.full_word = e.word.size() == r.token.size();
    }
    if (!seen) r.status = ParseStatus::NoMatch;
    return r;
}

}  // namespace dag
