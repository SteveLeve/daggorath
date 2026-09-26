// STATUX / PROMPT / OBJNAM projection. Character cells, not a raster.
#pragma once
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
};

struct TextProjection {
    std::string text;
};

std::string object_name(const std::optional<Ocb>& object);
inline std::string object_name(const Ocb& object) {
    return object_name(std::optional<Ocb>(object));
}
TextProjection project_text(const TextSnapshot& snap);

}  // namespace dag
