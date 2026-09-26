// Daggorath Core — CMOVE, including the attack branch.
// Source: CRETUR.ASM (CMOVE, STEPOK, CWALK, MOVTAB, SHIELD), COMCRE.ASM (OFIND).
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/combat.hpp"
#include "daggorath/maze.hpp"
#include "daggorath/population.hpp"
#include "daggorath/rng.hpp"
#include "daggorath/scheduler.hpp"

namespace dag {

struct HeldShield {
    bool present = false;
    std::uint8_t cls = 0;
    std::uint8_t magic_defense = 0;
    std::uint8_t physical_defense = 0;
};

struct CmoveView {
    bool frozen = false;
    int player_row = 0;
    int player_col = 0;
    int level = 0;
    Fighter* player = nullptr;
    HeldShield left;
    HeldShield right;
    bool* heart_update = nullptr;  // set when CMOVE calls HUPDAT
};

// One creature action. `events` are trace details (sound selection, pickup,
// deferred attack). A Null queue means the task left SCDQUE for queue 0.
TaskResult cmove(int slot, std::array<Ccb, kCcbSlots>& ccbs, std::vector<Ocb>& objects,
                 const Maze& maze, Rng& rng, CmoveView& view,
                 std::vector<std::string>& events);

// MOVTAB order for one RANDOM byte. Three relative turns, then the back-off
// relative (2) the listing tries after those three fail.
struct Preference {
    std::uint8_t relative[3];
    bool side_first;
};

Preference movement_preference(std::uint8_t random_byte);

}  // namespace dag
