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

std::uint8_t dac_sample(std::uint8_t noise_high, std::uint8_t volume) {
    const std::uint16_t product = static_cast<std::uint16_t>(noise_high) * volume;
    return static_cast<std::uint8_t>((product >> 8) & 0xFCu);
}

std::vector<std::uint8_t> rattle(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    samples.reserve(10 * 0xC0);
    for (int pulse = 0; pulse < 10; ++pulse) {
        for (int i = 0; i < 0xC0; ++i) {
            const std::uint16_t word = snoise(state);
            samples.push_back(dac_sample(static_cast<std::uint8_t>(word >> 8), volume));
        }
    }
    return samples;
}

}  // namespace dag
