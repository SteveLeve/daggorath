// Daggorath Core — token matching, exactly PARSER's rule set.
// Source: PARSER.ASM (GETTOK, PARSER/PARSE0, PARS10-PARS30), EXPAND.ASM.
#pragma once
#include <span>
#include <string>
#include <string_view>

#include "daggorath/lexicon_tables.hpp"

namespace dag {

enum class ParseStatus { Matched, NoToken, NoMatch };

struct ParseResult {
    ParseStatus status = ParseStatus::NoMatch;
    std::uint8_t type = 0;          // index within the table (A on return)
    std::uint8_t token_class = 0;   // STRING+1 (B on return)
    bool full_word = false;         // FULFLG: token consumed the whole table word
    std::string token;              // the raw token that was matched against
};

// GETTOK: skip spaces, copy non-space characters until a space or terminator.
// Returns false when no token remains on the line.
bool next_token(std::string_view line, std::size_t& pos, std::string& out);

// PARSER: a token matches a table entry when it is a prefix of that entry.
// Two matching entries is a failure, even if one of them is an exact match.
ParseResult parse(std::span<const TokenEntry> table, std::string_view line,
                  std::size_t& pos);

}  // namespace dag
