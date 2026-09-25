// Daggorath Core — 24-bit polynomial RNG.
// Source: RANDOM.ASM RANDOX (reconstructed 1983 listing, commit a94326f).
// Independent implementation; no port code copied.
#pragma once
#include <array>
#include <cstdint>

namespace dag {

class Rng {
public:
    using Seed = std::array<std::uint8_t, 3>;   // SEED, SEED+1, SEED+2

    constexpr Rng() = default;
    constexpr explicit Rng(Seed s) : seed_(s) {}

    // RANDOX: eight rounds; each masks SEED+2 with $E1, counts the set bits and
    // rotates the count's LSB into a 24-bit rotate-left of the three seed bytes.
    // Returns SEED[0].
    std::uint8_t next() {
        for (int round = 0; round < 8; ++round) {
            std::uint8_t b = 0;
            std::uint8_t a = static_cast<std::uint8_t>(seed_[2] & 0xE1u);
            for (int bit = 0; bit < 8; ++bit) {
                const bool carry = (a & 0x80u) != 0;
                a = static_cast<std::uint8_t>(a << 1);
                if (carry) ++b;                       // INCB
            }
            std::uint8_t carry = static_cast<std::uint8_t>(b & 1u);  // LSRB
            for (int i = 0; i < 3; ++i) {                            // ROL chain
                const std::uint8_t next_carry = (seed_[i] & 0x80u) ? 1u : 0u;
                seed_[i] = static_cast<std::uint8_t>((seed_[i] << 1) | carry);
                carry = next_carry;
            }
        }
        ++calls_;
        return seed_[0];
    }

    const Seed& seed() const { return seed_; }
    void set_seed(Seed s) { seed_ = s; }
    std::uint64_t calls() const { return calls_; }

private:
    Seed seed_{0, 0, 0};
    std::uint64_t calls_ = 0;
};

}  // namespace dag
