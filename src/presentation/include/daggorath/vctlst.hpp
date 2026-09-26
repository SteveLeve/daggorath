#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include "daggorath/render_state.hpp"

namespace dag {

// $FB and $FD read a big-endian index into this same buffer. An index past
// the buffer is a 6809 address this core cannot follow, and decoding stops.
// $FA returns from $FB. $FE ends the list.
std::vector<DrawSegment> decode_vectors(std::span<const std::uint8_t> list, std::uint8_t x_scale,
                                        std::uint8_t y_scale, int centroid_x, int centroid_y,
                                        std::uint8_t fade);

// Forward object shape from VOBJ.ASM for a class, at scale 128. Empty if unknown.
std::vector<DrawSegment> forward_object(int object_class);

}  // namespace dag
