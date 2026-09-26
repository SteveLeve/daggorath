#include "daggorath/combat.hpp"

namespace dag {

std::uint16_t scal16(std::uint16_t value, std::uint8_t radix) {
    const std::uint32_t product = static_cast<std::uint32_t>(value) * radix;
    return static_cast<std::uint16_t>((product >> 7) & 0xFFFFu);
}

std::int16_t attack_adjustment(std::uint16_t attacker_power, std::uint16_t defender_power,
                               std::uint16_t defender_damage) {
    std::uint16_t scaled =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(defender_power - defender_damage) << 2);
    int index = 15;
    while (index != 0) {
        if (scaled < attacker_power) break;
        scaled = static_cast<std::uint16_t>(scaled - attacker_power);
        --index;
    }
    const int delta = index - 3;
    if (delta >= 0) return static_cast<std::int16_t>(delta * 10);
    const int mag = -delta;
    return static_cast<std::int16_t>(-(mag * 25));
}

bool attack_hits(std::uint16_t attacker_power, std::uint16_t defender_power,
                 std::uint16_t defender_damage, std::uint8_t roll) {
    const std::int16_t sum = static_cast<std::int16_t>(
        attack_adjustment(attacker_power, defender_power, defender_damage) +
        static_cast<std::int16_t>(roll));
    const std::int16_t judged = static_cast<std::int16_t>(sum - 127);
    return judged >= 0;
}

void apply_damage(const Fighter& attacker, Fighter& defender) {
    const auto channel = [&](std::uint8_t offense, std::uint8_t defense) {
        const std::uint16_t through = scal16(attacker.power, offense);
        return scal16(through, defense);
    };
    defender.damage = static_cast<std::uint16_t>(
        defender.damage + channel(attacker.magic_offense, defender.magic_defense));
    defender.damage = static_cast<std::uint16_t>(
        defender.damage + channel(attacker.physical_offense, defender.physical_defense));
}

bool damage_survived(const Fighter& defender) {
    return defender.power > defender.damage;
}

}  // namespace dag
