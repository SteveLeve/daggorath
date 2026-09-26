// Daggorath Core — attack resolution at the original byte widths.
// Source: PATTK.ASM (ATTACK, DAMAGE, SCAL16, ASRD3).
#pragma once
#include <cstdint>

namespace dag {

struct Fighter {
    std::uint16_t power = 0;
    std::uint16_t damage = 0;
    std::uint8_t magic_offense = 0;
    std::uint8_t magic_defense = 0;
    std::uint8_t physical_offense = 0;
    std::uint8_t physical_defense = 0;
};

// SCAL16: (value * radix) >> 7, low 16 bits. The 6809 form drops the bit
// that shifts out of the high byte.
std::uint16_t scal16(std::uint16_t value, std::uint8_t radix);

// ATTACK reward/penalty before the RNG byte is added. Signed, from the
// quarter-step index.
std::int16_t attack_adjustment(std::uint16_t attacker_power, std::uint16_t defender_power,
                               std::uint16_t defender_damage);

// True when SUBD #127 leaves N clear (the swing connects).
bool attack_hits(std::uint16_t attacker_power, std::uint16_t defender_power,
                 std::uint16_t defender_damage, std::uint8_t roll);

// DAMAGE: magical channel, then physical. Each is scal16(scal16(power, offense), defense).
void apply_damage(const Fighter& attacker, Fighter& defender);

// BHI after DAMAGE: still alive when power > damage (unsigned).
bool damage_survived(const Fighter& defender);

}  // namespace dag
