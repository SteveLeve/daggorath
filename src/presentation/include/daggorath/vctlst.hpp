#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include "daggorath/render_state.hpp"

namespace dag {

// Absolute vector lists. A byte below $FA is a coordinate. $FE ends the list.
// The first pair sets the pen; each later pair is a line. Scale is radix-7.
// $FF fade draws nothing. JSR, JMP, and relative mode are not decoded.
std::vector<DrawSegment> decode_vectors(std::span<const std::uint8_t> list, std::uint8_t x_scale,
                                        std::uint8_t y_scale, int centroid_x, int centroid_y,
                                        std::uint8_t fade);

}  // namespace dag
