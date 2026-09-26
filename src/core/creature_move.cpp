#include "daggorath/creature_move.hpp"

#include <cstdlib>
#include <string>

namespace dag {
namespace {

int find_unowned(const std::vector<Ocb>& objects, int level, int row, int col) {
    // OFIND with OFINDF cleared: first unowned object on this level at (row, col).
    // Ownership is non-zero for the player (1) and for a creature (0xFF).
    for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
        const Ocb& o = objects[static_cast<std::size_t>(i)];
        if (o.level != level) continue;
        if (o.row != row || o.col != col) continue;
        if (o.owner != 0) continue;
        return i;
    }
    return -1;
}

bool cell_occupied(const std::array<Ccb, kCcbSlots>& ccbs, int row, int col) {
    for (const Ccb& c : ccbs) {
        if (c.in_use && c.row == row && c.col == col) return true;
    }
    return false;
}

// CWALK. Relative turn is added to the creature's facing. Z is success.
bool cwalk(Ccb& self, int slot, std::array<Ccb, kCcbSlots>& ccbs, const Maze& maze,
           Rng& rng, const CmoveView& view, std::uint8_t relative,
           std::vector<std::string>& events) {
    const Dir dir = static_cast<Dir>((self.dir + relative) & 3);
    int nr = 0, nc = 0;
    if (!step_ok(maze, self.row, self.col, dir, nr, nc)) return false;
    if (cell_occupied(ccbs, nr, nc)) return false;
    self.row = static_cast<std::uint8_t>(nr);
    self.col = static_cast<std::uint8_t>(nc);
    self.dir = static_cast<std::uint8_t>(dir);

    const int dr = std::abs(nr - view.player_row);
    const int dc = std::abs(nc - view.player_col);
    const int big = std::max(dr, dc);
    const int little = std::min(dr, dc);
    // CWLK20: update and maybe sound only when the larger delta is <= 8 and
    // the smaller is <= 2.
    if (big <= 8 && little <= 2) {
        const std::uint8_t roll = rng.next();
        if ((roll & 1) != 0) {
            const std::uint8_t volume = creature_sound_volume(big);
            if (view.sounds != nullptr)
                view.sounds->push_back({self.type, volume, SoundEntry::Sounds, big});
            events.push_back("SOUND slot=" + std::to_string(slot) +
                             " type=" + std::to_string(self.type) +
                             " vol=" + std::to_string(volume));
        } else {
            events.push_back("SOUND slot=" + std::to_string(slot) + " silent");
        }
        events.push_back("LOOK slot=" + std::to_string(slot));
    }
    return true;
}

void apply_shield(HeldShield hand, std::uint8_t& magic, std::uint8_t& physical) {
    // SHIELD: empty or non-shield leaves the pair. A lower (better) pair replaces it.
    if (!hand.present || hand.cls != 3) return;   // CD.ASM K.SHIE, CRETUR.ASM:116
    const unsigned current = (static_cast<unsigned>(magic) << 8) | physical;
    const unsigned offered =
        (static_cast<unsigned>(hand.magic_defense) << 8) | hand.physical_defense;
    if (offered >= current) return;
    magic = hand.magic_defense;
    physical = hand.physical_defense;
}

void creature_attack(Ccb& self, int slot, Rng& rng, CmoveView& view,
                     std::vector<std::string>& events) {
    if (view.sounds != nullptr)
        view.sounds->push_back({self.type, 0xFF, SoundEntry::Sounds, -1});
    events.push_back("SOUND slot=" + std::to_string(slot) +
                     " type=" + std::to_string(self.type) + " vol=255");
    std::uint8_t magic = 0x80;
    std::uint8_t physical = 0x80;
    apply_shield(view.left, magic, physical);
    apply_shield(view.right, magic, physical);
    if (view.player == nullptr) return;
    view.player->magic_defense = magic;
    view.player->physical_defense = physical;
    Fighter attacker;
    attacker.power = self.power;
    attacker.damage = self.damage;
    attacker.magic_offense = self.magic_offense;
    attacker.magic_defense = self.magic_defense;
    attacker.physical_offense = self.physical_offense;
    attacker.physical_defense = self.physical_defense;
    const std::uint8_t roll = rng.next();
    if (!attack_hits(attacker.power, view.player->power, view.player->damage, roll)) {
        events.push_back("MISS slot=" + std::to_string(slot) + " roll=" + std::to_string(roll));
    } else {
        if (view.sounds != nullptr)   // CRETUR.ASM:101-103 ISOUND A$KLK3
            view.sounds->push_back({static_cast<std::uint8_t>(SoundCue::KLK3), 0xFF,
                                    SoundEntry::Isound, -1});
        const std::uint16_t before = view.player->damage;
        apply_damage(attacker, *view.player);
        if (view.incoming_damage_percent != 100) {
            const unsigned added =
                static_cast<unsigned>(view.player->damage) +
                (view.player->damage < before ? 65536u : 0u) - before;
            const unsigned scaled =
                added * static_cast<unsigned>(view.incoming_damage_percent) / 100u;
            view.player->damage = static_cast<std::uint16_t>(before + scaled);
        }
        events.push_back("HIT slot=" + std::to_string(slot) +
                         " damage=" + std::to_string(view.player->damage));
    }
    if (view.heart_update != nullptr) *view.heart_update = true;
}

}  // namespace

Preference movement_preference(std::uint8_t random_byte) {
    // CMOV70: bit 7 clear selects MOVTAB+3 (forward, right, left).
    // CMOV72: the low two bits == 0 skips the forward entry (side first).
    const bool right_first = (random_byte & 0x80) == 0;
    const bool side_first = (random_byte & 3) == 0;
    Preference p;
    p.side_first = side_first;
    const std::uint8_t fwd = 0, left = 3, right = 1;
    std::uint8_t seq[3] = {fwd, right_first ? right : left, right_first ? left : right};
    if (side_first) {
        p.relative[0] = seq[1];
        p.relative[1] = seq[2];
        p.relative[2] = seq[0];
    } else {
        p.relative[0] = seq[0];
        p.relative[1] = seq[1];
        p.relative[2] = seq[2];
    }
    return p;
}

TaskResult cmove(int slot, std::array<Ccb, kCcbSlots>& ccbs, std::vector<Ocb>& objects,
                 const Maze& maze, Rng& rng, CmoveView& view,
                 std::vector<std::string>& events) {
    Ccb& self = ccbs[static_cast<std::size_t>(slot)];
    const auto movement = TaskResult{Queue::Tenth, self.move_delay};
    const auto attack = TaskResult{Queue::Tenth, self.attack_delay};

    // Frozen is tested before the dead check (CMOV12 -> CMOV90).
    if (view.frozen) {
        if (self.row == view.player_row && self.col == view.player_col) return attack;
        return movement;
    }

    if (self.in_use == 0) {
        // RTS with B still holding P.CCUSE, which is 0. That is not Q.SCD, so
        // SCHED unlinks the task and QUEADDs it on queue 0, which CLOCK never
        // scans.
        return {Queue::Null, 0};
    }

    const bool skips_pickup = self.type == 6 || self.type >= 10;
    // CMOV90: movement delay, unless the creature is now on the player's cell,
    // in which case PUPDAT and the attack delay (CMOV92).
    const auto finish = [&]() -> TaskResult {
        if (self.row == view.player_row && self.col == view.player_col) {
            events.push_back("PUPDAT slot=" + std::to_string(slot));
            return attack;
        }
        return movement;
    };

    if (!skips_pickup) {
        const int obj = find_unowned(objects, view.level, self.row, self.col);
        if (obj >= 0) {
            Ocb& o = objects[static_cast<std::size_t>(obj)];
            o.next = self.object_head;
            self.object_head = obj;
            o.owner = static_cast<std::uint8_t>(o.owner - 1);  // DEC; 0 -> 0xFF
            o.carrier = slot;
            events.push_back("PICKUP slot=" + std::to_string(slot) +
                             " object=" + std::to_string(obj));
            return finish();
        }
    }

    if (self.row == view.player_row && self.col == view.player_col) {
        creature_attack(self, slot, rng, view, events);
        return attack;
    }

    // In line with the player: walk intervening cells. A wall sends us to the
    // random preference. Reaching the player cell faces us and tries one step.
    auto try_line = [&](Dir face) -> bool {
        int r = self.row, c = self.col;
        for (;;) {
            int nr = 0, nc = 0;
            if (!step_ok(maze, r, c, face, nr, nc)) return false;
            r = nr;
            c = nc;
            if (r == view.player_row && c == view.player_col) {
                self.dir = static_cast<std::uint8_t>(face);
                cwalk(self, slot, ccbs, maze, rng, view, 0, events);
                return true;
            }
        }
    };

    if (self.row == view.player_row) {
        const Dir face = self.col < view.player_col ? Dir::East : Dir::West;
        // SUBA PCOL / BMI is west. Equal columns already left via the attack
        // test, so col != player col here.
        if (try_line(face)) return finish();
    } else if (self.col == view.player_col) {
        const Dir face = self.row > view.player_row ? Dir::North : Dir::South;
        if (try_line(face)) return finish();
    }

    const Preference pref = movement_preference(rng.next());
    bool moved = false;
    for (const std::uint8_t rel : pref.relative) {
        if (cwalk(self, slot, ccbs, maze, rng, view, rel, events)) {
            moved = true;
            break;
        }
    }
    if (!moved) cwalk(self, slot, ccbs, maze, rng, view, 2, events);
    return finish();
}

}  // namespace dag
