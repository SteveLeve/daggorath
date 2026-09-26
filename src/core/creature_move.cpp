#include "daggorath/creature_move.hpp"

#include <cstdlib>

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
            const std::uint8_t volume =
                static_cast<std::uint8_t>(~static_cast<std::uint8_t>(big * 31));
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

void defer_attack(const Ccb& self, int slot, std::vector<std::string>& events) {
    // Loud attack sound, then SHIELD into 0x8080. Hands are empty at game
    // start (PLHAND/PRHAND are not written by ONCE), so the upgrade does not
    // run. The ATTACK call itself is Phase 3.
    events.push_back("SOUND slot=" + std::to_string(slot) +
                     " type=" + std::to_string(self.type) + " vol=255");
    events.push_back("DEFER creature-attack " + std::to_string(slot));
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
                 const Maze& maze, Rng& rng, const CmoveView& view,
                 std::vector<std::string>& events) {
    Ccb& self = ccbs[static_cast<std::size_t>(slot)];
    const auto movement = TaskResult{Queue::Tenth, self.move_delay};
    const auto attack = TaskResult{Queue::Tenth, self.attack_delay};

    // Frozen is tested before the dead check. A frozen creature, live or not,
    // takes the movement-delay return (CMOV12 -> CMOV90).
    if (view.frozen) return movement;

    if (self.in_use == 0) {
        // RTS with B still holding P.CCUSE, which is 0. That is not Q.SCD, so
        // SCHED unlinks the task and QUEADDs it on queue 0, which CLOCK never
        // scans.
        return {Queue::Null, 0};
    }

    const bool skips_pickup = self.type == 6 || self.type >= 10;
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
            return movement;
        }
    }

    if (self.row == view.player_row && self.col == view.player_col) {
        defer_attack(self, slot, events);
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
        if (try_line(face)) return movement;
    } else if (self.col == view.player_col) {
        const Dir face = self.row > view.player_row ? Dir::North : Dir::South;
        if (try_line(face)) return movement;
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
    if (self.row == view.player_row && self.col == view.player_col) {
        events.push_back("PUPDAT slot=" + std::to_string(slot));
    }
    return movement;
}

}  // namespace dag
