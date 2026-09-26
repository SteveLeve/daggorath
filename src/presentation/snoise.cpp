#include "daggorath/snoise.hpp"

#include <cstdlib>

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

namespace {

// SNWAIT's LEAX -1,X / BNE is 8 cycles. The CoCo CPU clock is 0.895 MHz and the
// window plays at 6000 Hz. The jiffy cost of a cue is still D-4b; this only
// holds the DAC sample for the wait so a squeak has a pitch and a rattle has
// gaps. Inferred from the instruction timings, not from a ROM capture.
int audio_holds(int loops) {
    constexpr int kCpuHz = 894886;
    const int n = static_cast<int>((static_cast<long long>(loops) * 8 * 6000 + kCpuHz / 2) / kCpuHz);
    return n < 1 ? 1 : n;
}

void hold_last(std::vector<std::uint8_t>& samples, int loops) {
    if (samples.empty() || loops <= 0) return;
    const std::uint8_t held = samples.back();
    const int extra = audio_holds(loops);
    samples.insert(samples.end(), static_cast<std::size_t>(extra), held);
}

std::vector<std::uint8_t> tone_sweep(std::uint8_t volume, std::uint16_t pitch) {
    // SNSQK2 writes $FF then 0. Each SNOUT is followed by SNWAIT of the
    // current pitch, which is what makes the rising chirp.
    std::vector<std::uint8_t> samples;
    for (std::uint16_t x = pitch; x != 0; --x) {
        samples.push_back(dac_sample(0xFF, volume));
        hold_last(samples, x);
        samples.push_back(dac_sample(0x00, volume));
        hold_last(samples, x);
    }
    return samples;
}

std::vector<std::uint8_t> rattled(std::uint16_t& state, std::uint8_t volume, int pulses) {
    // Each pulse is 192 SNOUT writes, then SNWT1K ($1000) before the next.
    std::vector<std::uint8_t> samples;
    for (int i = 0; i < pulses; ++i) {
        const auto burst = noise_pulses(state, volume, 1);
        samples.insert(samples.end(), burst.begin(), burst.end());
        hold_last(samples, 0x1000);
    }
    return samples;
}

std::vector<std::uint8_t> beoop(std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    for (std::uint16_t x = 0x0500; x < 0x0800; x = static_cast<std::uint16_t>(x + 48)) {
        samples.push_back(dac_sample(0xFF, volume));
        hold_last(samples, x);
        samples.push_back(dac_sample(0x00, volume));
        hold_last(samples, x);
    }
    return samples;
}

std::vector<std::uint8_t> growl(std::uint16_t& state, std::uint8_t volume, std::uint16_t attack) {
    std::vector<std::uint8_t> samples;
    std::uint16_t level = 0;
    for (;;) {
        const auto noise_high = static_cast<std::uint8_t>(snoise(state) >> 8);
        const std::uint32_t sum = static_cast<std::uint32_t>(level) + attack;
        const bool carry = sum > 0xFFFFu;
        level = static_cast<std::uint16_t>(sum);
        if (carry) break;
        emit_enveloped(samples, level, noise_high, volume);
        hold_last(samples, 0x00F0);   // SNGRL1 SNWAIT #$00F0
    }
    level = 0xFFFF;
    for (;;) {
        const auto noise_high = static_cast<std::uint8_t>(snoise(state) >> 8);
        const bool borrow = level < 0x40u;
        level = static_cast<std::uint16_t>(level - 0x40u);
        if (borrow || level == 0) break;
        emit_enveloped(samples, level, noise_high, volume);
        hold_last(samples, 0x0060);   // SNGRL3 SNWAIT #$0060
    }
    return samples;
}

bool decay_tone(std::vector<std::uint8_t>& samples, std::uint16_t& level, std::uint8_t sample,
                std::uint8_t volume) {
    const bool borrow = level < 0x60u;
    level = static_cast<std::uint16_t>(level - 0x60u);
    if (borrow || level == 0) return false;
    emit_enveloped(samples, level, sample, volume);
    return true;
}

std::vector<std::uint8_t> klank(std::uint8_t volume, std::uint8_t freq1, std::uint8_t freq2) {
    // KLANK/CLANK/CLANG/KKLANK. Two detuned squares through a $60 decay.
    std::vector<std::uint8_t> samples;
    std::uint16_t level = 0xFFFF;
    std::uint16_t x = freq1;
    std::uint16_t y = freq2;
    std::uint8_t sample = 0;
    while (samples.size() < 8000) {
        if (--x == 0) {
            x = freq1;
            sample = static_cast<std::uint8_t>(sample ^ 0x7F);
            if (!decay_tone(samples, level, sample, volume)) break;
            // One pass of SNCLK2 is LEAX/BNE plus LEAY/BNE, about 16 cycles,
            // and audio_holds counts an 8-cycle wait. The gap is freq1 passes.
            hold_last(samples, static_cast<int>(freq1) * 2);
        }
        if (--y == 0) {
            y = freq2;
            sample = static_cast<std::uint8_t>(sample ^ 0x80);
            if (!decay_tone(samples, level, sample, volume)) break;
            hold_last(samples, static_cast<int>(freq2) * 2);
        }
    }
    return samples;
}

std::vector<std::uint8_t> boomer(std::uint16_t& state, std::uint8_t volume, std::uint16_t pitch,
                                 int repeats, bool hold_pitch) {
    // BOOM2 calls SNSUB3: one SNOUT, then SNWAIT with X still the pitch.
    std::vector<std::uint8_t> samples;
    for (; pitch != 0x0150; pitch = static_cast<std::uint16_t>(pitch + 2)) {
        for (int i = 0; i < repeats; ++i) {
            const std::uint16_t word = snoise(state);
            samples.push_back(dac_sample(static_cast<std::uint8_t>(word >> 8), volume));
            if (hold_pitch) hold_last(samples, pitch);
        }
    }
    return samples;
}

std::vector<std::uint8_t> bdlbdl(std::uint16_t& state, std::uint8_t volume) {
    std::vector<std::uint8_t> samples;
    for (int n = 0; n < 8; ++n) {
        const std::uint16_t word = snoise(state);
        std::uint8_t low = static_cast<std::uint8_t>(word & 0xFF);
        low = static_cast<std::uint8_t>(low >> 1);
        if (low == 0) low = 1;
        auto squeak = tone_sweep(volume, low);
        samples.insert(samples.end(), squeak.begin(), squeak.end());
    }
    // BDLBDL falls into KABOOM: THUDD, then the $0050/$0004 pair ahead of BANGD.
    auto first = boomer(state, volume, 0x0080, 1, true);
    auto second = boomer(state, volume, 0x0050, 4, true);
    samples.insert(samples.end(), first.begin(), first.end());
    samples.insert(samples.end(), second.begin(), second.end());
    return samples;
}

}  // namespace

std::vector<std::uint8_t> samples_for_cue(std::uint8_t cue, std::uint8_t volume,
                                          std::uint16_t& state) {
    switch (cue) {
        case 0: return tone_sweep(volume, 0x0020);                 // SQUEAK
        case 1: return rattled(state, volume, 10);                 // RATTLE (viper)
        case 2: return growl(state, volume, 0x0300);               // GROWL
        case 3: return beoop(volume);                              // BEOOP
        case 4: return klank(volume, 0xAF, 0x36);                  // KLANK
        case 5: return growl(state, volume, 0x0200);               // GRAWL
        case 6: return rattled(state, volume, 2);                  // PSSST
        case 7: return klank(volume, 0x32, 0x12);                  // KKLANK
        case 8: return rattled(state, volume, 1);                  // PSSHT
        case 9: return growl(state, volume, 0x0100);               // SNARL
        case 10:
        case 11: return bdlbdl(state, volume);                     // BDLBDL
        case 12: {                                                  // GLUGLG: 4× MSQUEQ
            std::vector<std::uint8_t> samples;
            for (int i = 0; i < 4; ++i) {
                auto one = tone_sweep(volume, 0x0080);
                samples.insert(samples.end(), one.begin(), one.end());
            }
            return samples;
        }
        case 13: {                                                  // PHASER: 10× MSQUEK
            std::vector<std::uint8_t> samples;
            for (int i = 0; i < 10; ++i) {
                auto one = tone_sweep(volume, 0x0040);
                samples.insert(samples.end(), one.begin(), one.end());
            }
            return samples;
        }
        case 14: return tone_sweep(volume, 0x0100);                // WHOOP
        case 15: return klank(volume, 0x64, 0x24);                 // CLANG
        case 16: return whoosh(state, volume);                     // WHOOSH
        case 17: {                                                  // CHUCK is WHOOSH's decay
            auto both = whoosh(state, volume);
            return both.size() > 511 ? std::vector<std::uint8_t>(both.begin() + 511, both.end())
                                     : both;
        }
        case 18: return klink(state, volume);                      // KLINK
        case 19: return klank(volume, 0x19, 0x09);                 // CLANK
        case 20: return boomer(state, volume, 0x0080, 1, true);   // THUD
        case 21: return boomer(state, volume, 0x0050, 5, true);   // BANG
        case 22: {                                                  // KABOOM
            auto first = boomer(state, volume, 0x0080, 1, true);
            auto second = boomer(state, volume, 0x0050, 4, true);
            first.insert(first.end(), second.begin(), second.end());
            return first;
        }
        default: return {};
    }
}

std::vector<std::uint8_t> samples_for(const std::string& kind, const std::string& detail,
                                      std::uint16_t& state) {
    if (kind != "SOUND") return {};
    if (detail == "A$THUD") return thud(state, 0xFF);
    // PATTK emits SOUND class=<hand class>. EMPHND and a sword are class 4,
    // which is A$SWOR / WHOOSH.
    if (detail == "class=4" || detail == "A$SWOR") return whoosh(state, 0xFF);
    if (detail == "A$KLK2") return klink(state, 0xFF);
    if (detail == "A$KLK3") return samples_for_cue(19, 0xFF, state);
    if (detail == "A$EXP0") return bang(state, 0xFF);
    if (detail.rfind("slot=", 0) == 0 && detail.find("silent") == std::string::npos) {
        const auto type_at = detail.find("type=");
        const auto vol_at = detail.find("vol=");
        if (type_at == std::string::npos || vol_at == std::string::npos) return {};
        const int type = std::atoi(detail.c_str() + type_at + 5);
        const int vol = std::atoi(detail.c_str() + vol_at + 4);
        if (type < 0 || type > 22 || vol < 0 || vol > 255) return {};
        return samples_for_cue(static_cast<std::uint8_t>(type), static_cast<std::uint8_t>(vol),
                               state);
    }
    return {};
}

}  // namespace dag
