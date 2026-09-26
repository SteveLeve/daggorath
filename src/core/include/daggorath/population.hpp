// Daggorath Core — level population and the creature matrix.
// Source: NEWLVL.ASM (NLVL30, NLVL40), COMCRE.ASM (CBIRTH, FNDCEL, CFIND,
//         CREGEN), ONCE.ASM (CINI40), OBIRTH.ASM (OBIRTX, GENVAL, OCBFIX),
//         COMDAT.ASM (CMTTAB), DTABAS.ASM (CREXXX, OBJXXX, CDBTAB, ODBTAB),
//         CD.ASM (CCB and OCB layouts).
//
// CMOVE is queued by Game::queue_creatures (COMCRE.ASM CBIRTH). Attacks are
// deferred; see deviation D-7.
#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "daggorath/maze.hpp"
#include "daggorath/rng.hpp"

namespace dag {

inline constexpr int kCreatureTypes = 12;
inline constexpr int kCcbSlots = 32;

struct CreatureDef {
    std::uint16_t power;
    std::uint8_t magic_offense, magic_defense;
    std::uint8_t physical_offense, physical_defense;
    std::uint8_t move_delay, attack_delay;
};

// CDBTAB, CREXXX argument order transcribed from DTABAS.ASM.
inline constexpr std::array<CreatureDef, kCreatureTypes> kCreatureDefs{{
    {32, 0, 255, 128, 255, 23, 11},      // SPIDER
    {56, 0, 255, 80, 128, 15, 7},        // VIPER
    {200, 0, 255, 52, 192, 29, 23},      // SGINT1
    {304, 0, 255, 96, 167, 31, 31},      // BLOB
    {504, 0, 128, 96, 60, 13, 7},        // KNIGT1
    {704, 0, 128, 128, 48, 17, 13},      // SGINT2
    {400, 255, 128, 255, 128, 5, 4},     // SCORP
    {800, 0, 64, 255, 8, 13, 7},         // KNIGT2
    {800, 192, 16, 192, 8, 3, 3},        // WRAITH
    {1000, 255, 5, 255, 3, 4, 3},        // BALROG
    {1000, 255, 6, 255, 0, 13, 7},       // WIZ0
    {8000, 255, 6, 255, 0, 13, 7},       // WIZ1
}};

// CMTTAB, level-major, 12 types. The RAM image CMXLND is initialised from
// these bytes (COMDAT.ASM RAMDAT) and is not recopied by NEWLVL.
inline constexpr std::array<std::array<std::uint8_t, kCreatureTypes>, 5> kCmtTab{{
    {{9, 9, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{2, 4, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0}},
    {{0, 0, 0, 4, 0, 6, 8, 4, 0, 0, 1, 0}},
    {{0, 0, 0, 0, 0, 0, 8, 6, 6, 4, 0, 0}},
    {{2, 2, 2, 2, 2, 2, 2, 4, 4, 8, 0, 1}},
}};

// One creature control block. Offsets match CD.ASM P.CC*.
struct Ccb {
    std::uint16_t power = 0;
    std::uint8_t magic_offense = 0, magic_defense = 0;
    std::uint8_t physical_offense = 0, physical_defense = 0;
    std::uint8_t move_delay = 0, attack_delay = 0;
    int object_head = -1;                 // P.CCOBJ, index into the object pool
    std::uint16_t damage = 0;
    std::uint8_t in_use = 0;               // P.CCUSE, 0xFF while live
    std::uint8_t type = 0;
    std::uint8_t dir = 0;                  // left 0; CBIRTH does not set it
    std::uint8_t row = 0, col = 0;
};

// One object control block. Offsets match CD.ASM P.OC*.
struct Ocb {
    int next = -1;                         // P.OCPTR
    std::uint8_t row = 0, col = 0;
    std::uint8_t level = 0;
    std::uint8_t owner = 0;                // 0xFF creature, 1 player, 0 none
    std::uint8_t spec[3] = {0, 0, 0};      // P.OCXXX
    std::uint8_t type = 0;
    std::uint8_t cls = 0;
    std::uint8_t reveal = 0;
    std::uint8_t magic_offense = 0, physical_offense = 0;
    int carrier = -1;                      // creature slot, or -1
};

// Dungeon objects created once by ONCE CINI40, before the first NEWLVL.
std::vector<Ocb> create_dungeon_objects();

// OBIRTH for the two game-mode bag objects. Owner is left 0; the caller
// applies INC (player) — GAME30 does that after NEWLVL returns.
Ocb birth_player_object(std::uint8_t type, std::uint8_t level);

// NEWLVL's birth loop: type 11 down to 0, CMXLND count times. `rng` is the
// generator DGNGEN left behind after DGEN90.
void birth_creatures(int level, const std::array<std::uint8_t, kCreatureTypes>& row,
                     Rng& rng, const Maze& maze, std::array<Ccb, kCcbSlots>& ccbs);

// NEWLVL NLVL40: creature-owned objects on this level, round-robin onto live
// creatures, each prepended to P.CCOBJ.
void attach_objects(int level, std::array<Ccb, kCcbSlots>& ccbs, std::vector<Ocb>& objects);

// CREGEN. Returns the type incremented, or -1 when the row sum is already >= 32.
// Does not create a control block.
int cregen_increment(std::array<std::uint8_t, kCreatureTypes>& row, Rng& rng);

// Byte index NEWLVL leaves in VFTPTR for `level` (NLVL10/NLVL12).
int vft_pointer(int level);

}  // namespace dag
