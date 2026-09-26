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

std::vector<std::uint8_t> noise_pulses(std::uint16_t& state, std::uint8_t volume, int pulses) {
    std::vector<std::uint8_t> samples;
    samples.reserve(static_cast<std::size_t>(pulses) * 0xC0);
    for (int pulse = 0; pulse < pulses; ++pulse) {
        for (int i = 0; i < 0xC0; ++i) {
            const std::uint16_t word = snoise(state);
            samples.push_back(dac_sample(static_cast<std::uint8_t>(word >> 8), volume));
        }
    }
    return samples;
}

std::vector<std::uint8_t> thud(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    for (std::uint16_t pitch = 0x0080; pitch != 0x0150; pitch = static_cast<std::uint16_t>(pitch + 2)) {
        const std::uint16_t word = snoise(state);
        samples.push_back(dac_sample(static_cast<std::uint8_t>(word >> 8), volume));
    }
    return samples;
}

namespace {

void emit_enveloped(std::vector<std::uint8_t>& samples, std::uint16_t level,
                    std::uint8_t noise_high, std::uint8_t volume) {
    // SNENV's MUL is the envelope high byte times the noise high byte.
    // SNOUT then scales that product's high byte by SNVOL.
    const auto env_high = static_cast<std::uint8_t>(level >> 8);
    const std::uint16_t mixed = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(env_high) * noise_high);
    samples.push_back(dac_sample(static_cast<std::uint8_t>(mixed >> 8), volume));
}

}  // namespace

std::vector<std::uint8_t> whoosh(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    // SETNVA loads BIGZER. The carrying add is not written to the DAC.
    std::uint16_t level = 0;
    for (;;) {
        const auto noise_high = static_cast<std::uint8_t>(snoise(state) >> 8);
        const std::uint32_t sum = static_cast<std::uint32_t>(level) + 0x80u;
        const bool carry = sum > 0xFFFFu;
        level = static_cast<std::uint16_t>(sum);
        if (carry) break;
        emit_enveloped(samples, level, noise_high, volume);
    }
    // CHUCK: SETNVD loads NEGONE ($FFFF). BLS (borrow or zero) ends the sound
    // without a sample for that step.
    level = 0xFFFF;
    for (;;) {
        const auto noise_high = static_cast<std::uint8_t>(snoise(state) >> 8);
        const bool borrow = level < 0xA0u;
        level = static_cast<std::uint16_t>(level - 0xA0u);
        if (borrow || level == 0) break;
        emit_enveloped(samples, level, noise_high, volume);
    }
    return samples;
}

// KLINK. SETNVD loads NEGONE and the $60 decay. Each pass feeds SNOISE's high
// byte, shifted right, then a second noise byte with bit 7 set. SNENVT writes
// one sample unless the subtract borrows or lands on zero.
std::vector<std::uint8_t> klink(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    std::uint16_t level = 0xFFFF;
    auto step = [&](std::uint8_t sample) -> bool {
        const bool borrow = level < 0x60u;
        level = static_cast<std::uint16_t>(level - 0x60u);
        if (borrow || level == 0) return false;
        emit_enveloped(samples, level, sample, volume);
        return true;
    };
    for (;;) {
        const auto first = static_cast<std::uint8_t>(snoise(state) >> 8);
        if (!step(static_cast<std::uint8_t>(first >> 1))) break;
        const auto second = static_cast<std::uint8_t>(snoise(state) >> 8);
        if (!step(static_cast<std::uint8_t>(second | 0x80u))) break;
    }
    return samples;
}

// BANG via BOOMER and SWCHAR.ASM BANGD ($0050, 5). Pitch climbs by 2 to $0150.
// Each pitch writes `repeats` noise samples. The waits are not stored.
std::vector<std::uint8_t> bang(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    for (std::uint16_t pitch = 0x0050; pitch != 0x0150; pitch = static_cast<std::uint16_t>(pitch + 2)) {
        for (int i = 0; i < 5; ++i) {
            const std::uint16_t word = snoise(state);
            samples.push_back(dac_sample(static_cast<std::uint8_t>(word >> 8), volume));
        }
    }
    return samples;
}

std::vector<std::uint8_t> samples_for(const std::string& kind, const std::string& detail,
                                      std::uint16_t& state) {
    if (kind != "SOUND") return {};
    if (detail == "A$THUD") return thud(state, 0xFF);
    // PATTK emits SOUND class=<hand class>. EMPHND and a sword are class 4,
    // which is A$SWOR / WHOOSH.
    if (detail == "class=4" || detail == "A$SWOR") return whoosh(state, 0xFF);
    if (detail == "A$KLK2") return klink(state, 0xFF);
    if (detail == "A$EXP0") return bang(state, 0xFF);
    return {};
}

}  // namespace dag
