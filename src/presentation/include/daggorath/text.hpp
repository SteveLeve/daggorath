// STATUX / PROMPT / OBJNAM projection. Character cells, not a raster.
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "daggorath/population.hpp"

namespace dag {

enum class HeartGlyph { Off, Small, Large };

struct TextSnapshot {
    std::optional<Ocb> left;
    std::optional<Ocb> right;
    HeartGlyph heart = HeartGlyph::Off;
    std::string line;
    // When set, the four TXTPRI rows replace the synthesized ".line" row.
    bool has_page = false;
    std::array<std::uint8_t, 128> page{};
};

struct TextProjection {
    std::string text;
};

std::string object_name(const std::optional<Ocb>& object);
inline std::string object_name(const Ocb& object) {
    return object_name(std::optional<Ocb>(object));
}
TextProjection project_text(const TextSnapshot& snap);

// COMTXT.ASM TXTDPB. Normal codes expand SWCTAB (EXPAN0 count, then 5-bit
// rows, shifted left twice). Codes $20+ are SPCTAB rows already positioned.
void glyph_rows(std::uint8_t code, std::uint8_t rows[7]);

// Paint the status band and the command band over `pixels`. The viewer
// occupies scanlines 0–151; these bands are 152–191.
// `command_override`, when non-empty, replaces the ".line_" command row.
// The desktop window uses that for the post-death prompt, which is not a
// line the halted core can edit.
void paint_text_bands(std::uint8_t* pixels, int width,
                      const TextSnapshot& snap, std::string_view message,
                      std::string_view command_override = {});

}  // namespace dag
