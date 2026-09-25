#include "daggorath/population.hpp"

namespace dag {
namespace {

enum Class : std::uint8_t { Flask = 0, Ring = 1, Scroll = 2, Shield = 3, Sword = 4, Torch = 5 };

// ODBTAB row plus the XXXTAB triple, when the object has one.
// Type index is the ADJTAB order (FOO reset before T.RN05 in TOKEN.ASM).
// spec_valid is false when XXXTAB has no entry; OCBFIL then leaves P.OCXXX.
struct ObjDef {
    std::uint8_t cls, reveal, mgo, pho;
    std::uint8_t spec[3];
    bool spec_valid;
    std::uint8_t initial_level, count;
};

// OBJXXX lines, DTABAS.ASM. Special-parameter symbols are the T.* indices:
// T.RN15=18, T.RN11=19, T.RN13=20, T.RN12=21.
constexpr ObjDef kObjects[] = {
    {Ring, 255, 0, 5, {3, 18, 0}, true, 4, 1},    // SUPREME
    {Ring, 170, 0, 5, {3, 19, 0}, true, 3, 1},    // JOULE
    {Sword, 150, 64, 64, {0, 0, 0}, false, 3, 1}, // ELVISH
    {Shield, 140, 13, 26, {64, 64, 0}, true, 3, 2}, // MITHRIL
    {Scroll, 130, 0, 5, {0, 0, 0}, false, 2, 3},  // SEER
    {Flask, 70, 0, 5, {0, 0, 0}, false, 2, 3},    // THEWS
    {Ring, 52, 0, 5, {3, 20, 0}, true, 1, 1},     // HOTH
    {Scroll, 50, 0, 5, {0, 0, 0}, false, 1, 3},   // VISION
    {Flask, 48, 0, 5, {0, 0, 0}, false, 1, 6},    // ABYE
    {Flask, 40, 0, 5, {0, 0, 0}, false, 1, 4},    // HALE
    {Torch, 70, 0, 5, {60, 13, 11}, true, 1, 4},  // SOLAR
    {Shield, 25, 0, 26, {96, 128, 0}, true, 1, 6}, // BRONZE
    {Ring, 13, 0, 5, {3, 21, 0}, true, 0, 1},     // VULCAN
    {Sword, 13, 0, 40, {0, 0, 0}, false, 0, 4},   // IRON
    {Torch, 25, 0, 5, {30, 10, 4}, true, 0, 8},   // LUNAR
    {Torch, 5, 0, 5, {15, 7, 0}, true, 0, 8},     // PINE  (T.TOR4 = 15)
    {Shield, 5, 0, 10, {108, 128, 0}, true, 0, 3}, // LEATHER (T.SHI4 = 16)
    {Sword, 5, 0, 16, {0, 0, 0}, false, 0, 4},    // WOODEN (T.SWO3 = 17)
};

constexpr int kPine = 15, kLeather = 16, kWooden = 17;
constexpr int kGeneric[6] = {-1, -1, -1, kLeather, kWooden, kPine};

// VFTTAB bytes, COMCRE.ASM. 0x80 is the negative terminator NLVL12 searches for.
constexpr std::uint8_t kVftTab[] = {
    0x80, 0x01, 0x00, 0x17, 0x00, 0x0F, 0x04, 0x00, 0x14, 0x11, 0x01, 0x1C, 0x1E, 0x80,
    0x01, 0x02, 0x03, 0x00, 0x03, 0x1F, 0x00, 0x13, 0x14, 0x00, 0x1F, 0x00, 0x80, 0x80,
    0x00, 0x00, 0x1F, 0x00, 0x05, 0x00, 0x00, 0x16, 0x1C, 0x00, 0x1F, 0x10, 0x80, 0x80,
};

struct Filled {
    std::uint8_t cls, reveal, mgo, pho;
    std::uint8_t spec[3];
    bool spec_written;
};

Filled ocbfil(int type) {
    const ObjDef& d = kObjects[type];
    Filled f{d.cls, d.reveal, d.mgo, d.pho, {d.spec[0], d.spec[1], d.spec[2]}, d.spec_valid};
    if (!d.spec_valid) {
        f.spec[0] = f.spec[1] = f.spec[2] = 0;
    }
    return f;
}

Ocb make_object(std::uint8_t type, std::uint8_t level) {
    Filled f = ocbfil(type);
    if (kGeneric[f.cls] >= 0) {
        const std::uint8_t reveal = f.reveal;
        const std::uint8_t prev[3] = {f.spec[0], f.spec[1], f.spec[2]};
        Filled g = ocbfil(kGeneric[f.cls]);
        if (!g.spec_written) {
            g.spec[0] = prev[0];
            g.spec[1] = prev[1];
            g.spec[2] = prev[2];
        }
        f = g;
        f.reveal = reveal;
    }
    Ocb o;
    o.level = level;
    o.owner = 0xFF;                 // DEC from the zeroed byte: creature-owned
    o.spec[0] = f.spec[0];
    o.spec[1] = f.spec[1];
    o.spec[2] = f.spec[2];
    o.type = type;
    o.cls = f.cls;
    o.reveal = f.reveal;
    o.magic_offense = f.mgo;
    o.physical_offense = f.pho;
    return o;
}

bool occupied(const std::array<Ccb, kCcbSlots>& ccbs, std::uint8_t row, std::uint8_t col) {
    for (const Ccb& c : ccbs) {
        if (c.in_use && c.row == row && c.col == col) return true;
    }
    return false;
}

}  // namespace

std::vector<Ocb> create_dungeon_objects() {
    std::vector<Ocb> out;
    for (const ObjDef& d : kObjects) {
        std::uint8_t level = d.initial_level;
        for (int n = 0; n < d.count; ++n) {
            out.push_back(make_object(static_cast<std::uint8_t>(&d - kObjects), level));
            ++level;
            if (level > 5) level = d.initial_level;   // CMPB #5 / BHI reset
        }
    }
    return out;
}

Ocb birth_player_object(std::uint8_t type, std::uint8_t level) {
    Ocb o = make_object(type, level);
    o.owner = 0;                    // caller INC's this to 1
    return o;
}

void birth_creatures(int level, const std::array<std::uint8_t, kCreatureTypes>& row,
                     Rng& rng, const Maze& maze, std::array<Ccb, kCcbSlots>& ccbs) {
    (void)level;
    ccbs = {};
    for (int type = kCreatureTypes - 1; type >= 0; --type) {
        for (int n = 0; n < row[static_cast<std::size_t>(type)]; ++n) {
            int slot = 0;
            while (ccbs[static_cast<std::size_t>(slot)].in_use) ++slot;
            Ccb& c = ccbs[static_cast<std::size_t>(slot)];
            const CreatureDef& d = kCreatureDefs[static_cast<std::size_t>(type)];
            c.in_use = 0xFF;        // DEC of a zeroed byte, before placement
            c.type = static_cast<std::uint8_t>(type);
            c.power = d.power;
            c.magic_offense = d.magic_offense;
            c.magic_defense = d.magic_defense;
            c.physical_offense = d.physical_offense;
            c.physical_defense = d.physical_defense;
            c.move_delay = d.move_delay;
            c.attack_delay = d.attack_delay;
            // row/col stay 0, so CFIND treats (0,0) as already occupied by this slot.
            for (;;) {
                const std::uint8_t col = static_cast<std::uint8_t>(rng.next() & 31);
                const std::uint8_t rowb = static_cast<std::uint8_t>(rng.next() & 31);
                if (maze.at(rowb, col) == 0xFF) continue;
                if (occupied(ccbs, rowb, col)) continue;
                c.row = rowb;
                c.col = col;
                break;
            }
        }
    }
}

void attach_objects(int level, std::array<Ccb, kCcbSlots>& ccbs, std::vector<Ocb>& objects) {
    for (Ocb& o : objects) {
        o.next = -1;
        o.carrier = -1;
    }
    for (Ccb& c : ccbs) c.object_head = -1;
    int u = -1;
    int idx = -1;
    bool started = false;
    for (;;) {
        if (!started) {
            idx = -1;
            started = true;
        }
        int found = -1;
        for (int j = idx + 1; j < static_cast<int>(objects.size()); ++j) {
            if (objects[static_cast<std::size_t>(j)].level == level) {
                found = j;
                break;
            }
        }
        if (found < 0) return;
        idx = found;
        if ((objects[static_cast<std::size_t>(idx)].owner & 0x80) == 0) continue;
        for (;;) {
            ++u;
            if (u >= kCcbSlots) u = 0;
            if (ccbs[static_cast<std::size_t>(u)].in_use) break;
        }
        Ocb& o = objects[static_cast<std::size_t>(idx)];
        o.next = ccbs[static_cast<std::size_t>(u)].object_head;
        ccbs[static_cast<std::size_t>(u)].object_head = idx;
        o.carrier = u;
    }
}

int cregen_increment(std::array<std::uint8_t, kCreatureTypes>& row, Rng& rng) {
    unsigned sum = 0;
    for (int t = kCreatureTypes - 1; t >= 0; --t) {
        sum = (sum + row[static_cast<std::size_t>(t)]) & 0xFFu;
    }
    if (sum >= 32) return -1;
    const int type = (rng.next() & 7) + 2;
    row[static_cast<std::size_t>(type)] =
        static_cast<std::uint8_t>(row[static_cast<std::size_t>(type)] + 1);
    return type;
}

int vft_pointer(int level) {
    int x = 0;
    int b = level;
    const int n = static_cast<int>(sizeof kVftTab);
    for (;;) {
        const int at = x;
        while (x < n && kVftTab[x] < 0x80) ++x;
        if (x < n) ++x;
        b = (b - 1) & 0xFF;
        if (b & 0x80) return at;
    }
}

}  // namespace dag
