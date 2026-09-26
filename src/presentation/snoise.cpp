#include "daggorath/snoise.hpp"

namespace dag {

std::uint16_t snoise(std::uint16_t& state) {
    const std::uint16_t original = state;
    std::uint16_t value = static_cast<std::uint16_t>(original << 1);
    value = static_cast<std::uint16_t>(value << 1);
    value = static_cast<std::uint16_t>(value + original);
    value = static_cast<std::uint16_t>((value & 0xFF00u) | ((value + 1u) & 0xFFu));
    state = value;
    return value;
}

}  // namespace dag
