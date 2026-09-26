// dplan — closed-loop Original Mode planner that records a dcli script.
//
// It drives the real core from power-on. It does not change core behaviour.
// Combat numbers come from scal16 / apply_damage on the live state.
//
//   dplan --dump
//   dplan --script FILE [--max-jiffies N]
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "daggorath/combat.hpp"
#include "daggorath/game.hpp"
#include "daggorath/lexicon_tables.hpp"

namespace {

constexpr int kSupreme = 0, kJoule = 1, kElvish = 2, kThews = 5, kHoth = 6,
              kHale = 9, kSolar = 10, kVulcan = 12, kLunar = 14, kPine = 15,
              kWooden = 17, kEnergy = 19, kIce = 20, kFire = 21;
constexpr int kClassSword = 4, kClassRing = 1, kClassTorch = 5, kClassFlask = 0;

const char* type_name(int t) {
    static constexpr const char* k[] = {
        "spider", "viper", "giant1", "blob",  "knight1", "giant2",
        "scorp",  "knight2", "wraith", "balrog", "wiz0",  "wiz1"};
    if (t < 0 || t >= 12) return "?";
    return k[t];
}

const char* obj_name(int t) {
    if (t >= 0 && t < static_cast<int>(dag::kAdjTab.size())) return dag::kAdjTab[static_cast<std::size_t>(t)].word.data();
    return "?";
}

bool dangerous(const dag::Ccb& c) {
    if (!c.in_use) return false;
    // Blobs and up (and any magic user) one-shot or near-one-shot PPOW 160.
    return c.type >= 3 || c.magic_offense != 0;
}

bool wizard(const dag::Ccb& c) { return c.in_use && (c.type == 10 || c.type == 11); }

int ring_damage(std::uint16_t power, std::uint8_t mdef, std::uint8_t pdef) {
    dag::Fighter atk, def;
    atk.power = power;
    atk.magic_offense = 255;
    atk.physical_offense = 255;
    def.magic_defense = mdef;
    def.physical_defense = pdef;
    dag::apply_damage(atk, def);
    return def.damage;
}

int sword_damage(std::uint16_t power, std::uint8_t mgo, std::uint8_t pho,
                 std::uint8_t mdef, std::uint8_t pdef) {
    dag::Fighter atk, def;
    atk.power = power;
    atk.magic_offense = mgo;
    atk.physical_offense = pho;
    def.magic_defense = mdef;
    def.physical_defense = pdef;
    dag::apply_damage(atk, def);
    return def.damage;
}

int find_obj(const dag::Game& g, int type) {
    const auto& o = g.objects();
    for (int i = 0; i < static_cast<int>(o.size()); ++i)
        if (o[static_cast<std::size_t>(i)].type == type) return i;
    return -1;
}

int obj_of(const dag::Game& g, int type, int owner_mask) {
    const auto& o = g.objects();
    for (int i = 0; i < static_cast<int>(o.size()); ++i) {
        if (o[static_cast<std::size_t>(i)].type != type) continue;
        if (owner_mask < 0 || o[static_cast<std::size_t>(i)].owner == owner_mask) return i;
    }
    return -1;
}

int slot_of_type(const dag::Game& g, int type) {
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        const dag::Ccb& c = g.creatures()[static_cast<std::size_t>(i)];
        if (c.in_use && c.type == type) return i;
    }
    return -1;
}

int creature_at(const dag::Game& g, int row, int col) {
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        const dag::Ccb& c = g.creatures()[static_cast<std::size_t>(i)];
        if (c.in_use && c.row == row && c.col == col) return i;
    }
    return -1;
}

bool owned_by_player(const dag::Game& g, int index) {
    if (index < 0) return false;
    const auto& o = g.objects()[static_cast<std::size_t>(index)];
    if (o.owner != 1) return false;
    const auto& p = g.player();
    if (p.left_hand == index || p.right_hand == index || p.torch == index) return true;
    for (int i = p.bag_head; i >= 0; i = g.objects()[static_cast<std::size_t>(i)].next)
        if (i == index) return true;
    return false;
}

bool is_incanted(const dag::Game& g, int index, int become) {
    if (index < 0) return false;
    return g.objects()[static_cast<std::size_t>(index)].type == become;
}

int dump_world() {
    dag::Game g;
    const auto& p = g.player();
    std::cout << "player row=" << p.row << " col=" << p.col
              << " dir=" << static_cast<int>(p.dir) << " power=" << p.power
              << " damage=" << p.damage << " weight=" << p.carried_weight
              << " second=" << static_cast<int>(g.counters().second) << "\n";
    std::cout << "ring_hit_vs_wiz mdef=6 pdef=0 at P=160: "
              << ring_damage(160, 6, 0) << "\n";
    std::cout << "ring_exertion at P=160: " << dag::scal16(160, 63) << "\n";
    std::cout << "elvish_vs_wiz at P=160: " << sword_damage(160, 64, 64, 6, 0) << "\n";
    for (int lv = 0; lv < 5; ++lv) {
        std::cout << "stairs level " << lv << ":";
        for (int r = 0; r < 32; ++r)
            for (int c = 0; c < 32; ++c) {
                const int f = dag::vfind(lv, r, c);
                if (f >= 0) std::cout << " (" << r << "," << c << " feat=" << f << ")";
            }
        std::cout << "\n";
    }
    std::cout << "creatures level 0:\n";
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        const dag::Ccb& c = g.creatures()[static_cast<std::size_t>(i)];
        if (!c.in_use) continue;
        std::cout << "  slot " << i << " " << type_name(c.type) << " type=" << static_cast<int>(c.type)
                  << " r=" << static_cast<int>(c.row) << " c=" << static_cast<int>(c.col)
                  << " p=" << c.power << " obj=" << c.object_head << "\n";
    }
    std::cout << "objects:\n";
    for (int i = 0; i < static_cast<int>(g.objects().size()); ++i) {
        const dag::Ocb& o = g.objects()[static_cast<std::size_t>(i)];
        std::cout << "  [" << i << "] " << obj_name(o.type) << " type=" << static_cast<int>(o.type)
                  << " lv=" << static_cast<int>(o.level) << " owner=" << static_cast<int>(o.owner)
                  << " r=" << static_cast<int>(o.row) << " c=" << static_cast<int>(o.col)
                  << " carrier=" << o.carrier << " cls=" << static_cast<int>(o.cls)
                  << " mgo=" << static_cast<int>(o.magic_offense)
                  << " pho=" << static_cast<int>(o.physical_offense)
                  << " spec=" << static_cast<int>(o.spec[0]) << ","
                  << static_cast<int>(o.spec[1]) << "\n";
    }
    return 0;
}

struct Runner {
    dag::Game game;
    std::vector<dag::KeyEvent> log;
    std::size_t seen = 0;
    int waits = 0;
    enum Phase {
        Light,
        Arm,
        TakeVulcan,
        Down1,
        TakeHoth,
        Down2,
        TakeThews,
        KillImage,
        Down4,
        KillWizard,
        TakeSupreme,
        Win
    } phase = Light;

    std::uint8_t encode(char c) const {
        if (c == ' ') return 0x20;
        return static_cast<std::uint8_t>(c);
    }

    void append_cmd(std::vector<dag::KeyEvent>& keys, std::uint64_t j, const std::string& cmd) {
        for (const char c : cmd) {
            const dag::KeyEvent e{j, encode(c)};
            keys.push_back(e);
            log.push_back(e);
        }
        const dag::KeyEvent cr{j, 0x0D};
        keys.push_back(cr);
        log.push_back(cr);
    }

    void type(const std::vector<std::string>& cmds, std::uint64_t extra = 2) {
        std::vector<dag::KeyEvent> keys;
        std::uint64_t j = game.counters().total_jiffies;
        std::size_t used = 0;
        for (const auto& c : cmds) {
            if (used + c.size() + 1 > 31) {
                ++j;
                used = 0;
            }
            append_cmd(keys, j, c);
            used += c.size() + 1;
        }
        game.load_script(keys);
        const std::uint64_t now = game.counters().total_jiffies;
        std::uint64_t span = extra;
        if (j + 1 > now) span = (j - now) + extra;
        if (span < extra) span = extra;
        game.advance_jiffies(span == 0 ? 1 : span);
        waits = 0;
    }

    void idle(std::uint64_t n) {
        game.load_script({});
        game.advance_jiffies(n);
        ++waits;
    }

    bool has_kind(const std::string& kind) {
        for (std::size_t i = seen; i < game.trace().size(); ++i)
            if (game.trace()[i].kind == kind) {
                seen = game.trace().size();
                return true;
            }
        seen = game.trace().size();
        return false;
    }

    void mark() { seen = game.trace().size(); }

    int here() const { return creature_at(game, game.player().row, game.player().col); }

    bool holding(int index) const {
        return game.player().left_hand == index || game.player().right_hand == index;
    }

    int hand_type(bool right) const {
        const int h = right ? game.player().right_hand : game.player().left_hand;
        if (h < 0) return -1;
        return game.objects()[static_cast<std::size_t>(h)].type;
    }

    bool left_is_ring() const {
        const int h = game.player().left_hand;
        return h >= 0 && game.objects()[static_cast<std::size_t>(h)].cls == kClassRing;
    }

    bool path_step(int tr, int tc, bool avoid) {
        const int sr = game.player().row, sc = game.player().col;
        if (sr == tr && sc == tc) return true;
        auto idx = [](int r, int c) { return r * 32 + c; };
        std::array<int, 32 * 32> parent{};
        parent.fill(-2);
        std::queue<int> q;
        parent[static_cast<std::size_t>(idx(sr, sc))] = -1;
        q.push(idx(sr, sc));
        bool reached = false;
        while (!q.empty()) {
            const int cur = q.front();
            q.pop();
            const int r = cur / 32, c = cur % 32;
            if (r == tr && c == tc) {
                reached = true;
                break;
            }
            for (int d = 0; d < 4; ++d) {
                int nr = 0, nc = 0;
                if (!dag::step_ok(game.maze(), r, c, static_cast<dag::Dir>(d), nr, nc)) continue;
                const int ni = idx(nr, nc);
                if (parent[static_cast<std::size_t>(ni)] != -2) continue;
                if (nr >= 31 || nr <= 0) continue;
                if (avoid && !(nr == tr && nc == tc)) {
                    if (creature_at(game, nr, nc) >= 0) continue;
                }
                parent[static_cast<std::size_t>(ni)] = cur;
                q.push(ni);
            }
        }
        if (!reached) return false;
        int cell = idx(tr, tc);
        int next = cell;
        while (parent[static_cast<std::size_t>(cell)] >= 0) {
            next = cell;
            cell = parent[static_cast<std::size_t>(cell)];
        }
        const int nr = next / 32, nc = next % 32;
        int want = 0;
        if (nr < sr) want = 0;
        else if (nc > sc) want = 1;
        else if (nr > sr) want = 2;
        else want = 3;
        const int face = static_cast<int>(game.player().dir);
        const int delta = (want - face) & 3;
        if (delta == 1) type({"TURN RIGHT"});
        else if (delta == 3) type({"TURN LEFT"});
        else if (delta == 2) type({"TURN AROUND"}, 2);
        type({"MOVE"}, 2);
        return true;
    }

    bool break_alignment() {
        const int pr = game.player().row, pc = game.player().col;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (!dangerous(c) || wizard(c)) continue;
            if (c.row != pr && c.col != pc) continue;
            bool clear = true;
            if (c.row == pr) {
                const int step = c.col > pc ? 1 : -1;
                for (int col = pc + step; col != c.col; col += step) {
                    int nr = 0, nc = 0;
                    const dag::Dir d = step > 0 ? dag::Dir::East : dag::Dir::West;
                    if (!dag::step_ok(game.maze(), pr, col - step, d, nr, nc)) {
                        clear = false;
                        break;
                    }
                }
            } else {
                const int step = c.row > pr ? 1 : -1;
                for (int row = pr + step; row != c.row; row += step) {
                    int nr = 0, nc = 0;
                    const dag::Dir d = step > 0 ? dag::Dir::South : dag::Dir::North;
                    if (!dag::step_ok(game.maze(), row - step, pc, d, nr, nc)) {
                        clear = false;
                        break;
                    }
                }
            }
            if (!clear) continue;
            const int r0 = pr, c0 = pc;
            if (dest_ok(3)) type({"MOVE LEFT"}, 2);
            else if (dest_ok(1)) type({"MOVE RIGHT"}, 2);
            else return false;
            return game.player().row != r0 || game.player().col != c0;
        }
        return false;
    }

    bool rest_needed() const {
        const int d = game.player().damage;
        if (d > 63) return true;
        const int hr = static_cast<int>(static_cast<std::int8_t>(game.player().heart_rate));
        return hr <= 8;
    }

    bool dest_ok(int rel) const {
        const dag::Dir d =
            static_cast<dag::Dir>((static_cast<int>(game.player().dir) + rel) & 3);
        int nr = 0, nc = 0;
        if (!dag::step_ok(game.maze(), game.player().row, game.player().col, d, nr, nc))
            return false;
        return nr > 0 && nr < 31;
    }

    bool flee() {
        const int r0 = game.player().row, c0 = game.player().col;
        const char* cmds[] = {"MOVE BACK", "MOVE LEFT", "MOVE RIGHT"};
        const int rels[] = {2, 3, 1};
        for (int i = 0; i < 3; ++i) {
            if (!dest_ok(rels[i])) continue;
            type({cmds[i]}, 2);
            if (game.player().row != r0 || game.player().col != c0) return true;
        }
        for (int t = 0; t < 3; ++t) {
            const char* turn = t == 0 ? "TURN LEFT" : t == 1 ? "TURN RIGHT" : "TURN AROUND";
            type({turn}, 2);
            if (!dest_ok(0)) continue;
            type({"MOVE"}, 2);
            if (game.player().row != r0 || game.player().col != c0) return true;
        }
        return false;
    }

    bool ring_ready() const {
        const int l = hand_type(false);
        const int r = hand_type(true);
        return l == kFire || l == kIce || l == kEnergy || r == kFire || r == kIce || r == kEnergy;
    }

    void empty_hand(bool right) {
        const int h = right ? game.player().right_hand : game.player().left_hand;
        if (h < 0) return;
        type({right ? "STOW RIGHT" : "STOW LEFT"});
    }

    bool take_object(int obj_type, const char* specific) {
        if (owned_by_player(game, find_obj(game, obj_type))) return true;
        const int idx = find_obj(game, obj_type);
        if (idx < 0) return false;
        const dag::Ocb& o = game.objects()[static_cast<std::size_t>(idx)];
        if (o.level != game.level_index()) return false;
        int tr = o.row, tc = o.col;
        if (o.owner != 0) {
            int slot = o.carrier;
            if (slot < 0 || !game.creatures()[static_cast<std::size_t>(slot)].in_use)
                slot = creature_at(game, o.row, o.col);
            if (slot < 0) {
                for (int i = 0; i < dag::kCcbSlots; ++i) {
                    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
                    if (!c.in_use) continue;
                    int obj = c.object_head;
                    while (obj >= 0) {
                        if (obj == idx) {
                            slot = i;
                            break;
                        }
                        obj = game.objects()[static_cast<std::size_t>(obj)].next;
                    }
                    if (slot == i) break;
                }
            }
            if (slot < 0) return false;
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
            tr = c.row;
            tc = c.col;
            const int pr = game.player().row, pc = game.player().col;
            const int adj = std::abs(pr - tr) + std::abs(pc - tc);
            if (adj == 0) {
                mark();
                strike_and_clear();
                return false;
            }
            if (adj == 1) {
                idle(6);
                return false;
            }
            if (tr >= 31 || tr <= 0) {
                idle(20);
                return false;
            }
            if (!path_step(tr, tc, true)) {
                if (!path_step(tr, tc, false)) idle(20);
            }
            return false;
        }
        if (game.player().row == tr && game.player().col == tc) {
            empty_hand(true);
            type({std::string("GET RIGHT ") + specific});
            return owned_by_player(game, idx);
        }
        path_step(tr, tc, true);
        return false;
    }

    bool face_and_move(int want) {
        const int face = static_cast<int>(game.player().dir);
        const int delta = (want - face) & 3;
        if (delta == 1) type({"TURN RIGHT"}, 2);
        else if (delta == 3) type({"TURN LEFT"}, 2);
        else if (delta == 2) type({"TURN AROUND"}, 2);
        const int r0 = game.player().row, c0 = game.player().col;
        type({"MOVE"}, 2);
        return game.player().row != r0 || game.player().col != c0;
    }

    bool go_down() {
        int best_d = 1e9, br = -1, bc = -1;
        const int pr = game.player().row, pc = game.player().col;
        for (int r = 0; r < 32; ++r)
            for (int c = 0; c < 32; ++c) {
                const int f = dag::vfind(game.level_index(), r, c);
                if (f < 0 || (f & 2) == 0) continue;
                if (game.maze().at(r, c) == 0xFF) continue;
                const int d = std::abs(r - pr) + std::abs(c - pc);
                if (d < best_d) {
                    best_d = d;
                    br = r;
                    bc = c;
                }
            }
        if (br < 0) return false;
        if (pr == br && pc == bc) {
            type({"CLIMB DOWN"});
            return true;
        }
        if (!path_step(br, bc, true)) idle(30);
        return false;
    }

    const char* leave_cmd() const {
        if (dest_ok(2)) return "MOVE BACK";
        if (dest_ok(3)) return "MOVE LEFT";
        if (dest_ok(1)) return "MOVE RIGHT";
        if (dest_ok(0)) return "MOVE";
        return "MOVE BACK";
    }

    void hit_run() {
        const bool left = left_is_ring();
        mark();
        type({left ? "ATTACK LEFT" : "ATTACK RIGHT", leave_cmd()}, 2);
        if (here() >= 0) flee();
    }

    void strike_and_clear() {
        type({"ATTACK LEFT", leave_cmd()}, 2);
        if (here() >= 0) flee();
    }

    void report_block(const char* why) const {
        const auto& p = game.player();
        std::cerr << "BLOCKED: " << why << " phase=" << static_cast<int>(phase)
                  << " level=" << game.level_index() << " pos=" << p.row << "," << p.col
                  << " power=" << p.power << " damage=" << p.damage
                  << " weight=" << p.carried_weight << " heart="
                  << static_cast<int>(static_cast<std::int8_t>(p.heart_rate))
                  << " fainted=" << p.fainted << " dead=" << p.dead
                  << " jiffy=" << game.counters().total_jiffies << "\n";
        std::cerr << " hands L=" << p.left_hand << " R=" << p.right_hand
                  << " torch=" << p.torch << " bag=" << p.bag_head << "\n";
        const int sl = creature_at(game, p.row, p.col);
        if (sl >= 0) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(sl)];
            std::cerr << " occupant " << type_name(c.type) << " dmg=" << c.damage
                      << " p=" << c.power << "\n";
        }
        for (int t : {kVulcan, kHoth, kJoule, kElvish, kThews, kSupreme, kFire, kIce, kEnergy}) {
            const int i = find_obj(game, t);
            if (i < 0) continue;
            const dag::Ocb& o = game.objects()[static_cast<std::size_t>(i)];
            std::cerr << "  obj " << obj_name(t) << " owner=" << static_cast<int>(o.owner)
                      << " lv=" << static_cast<int>(o.level) << " r=" << static_cast<int>(o.row)
                      << " c=" << static_cast<int>(o.col) << " carrier=" << o.carrier << "\n";
        }
        int wiz = slot_of_type(game, game.level_index() == 2 ? 10 : 11);
        if (wiz >= 0) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(wiz)];
            std::cerr << " wizard slot=" << wiz << " r=" << static_cast<int>(c.row)
                      << " c=" << static_cast<int>(c.col) << " dmg=" << c.damage << "\n";
        }
    }

    int play(std::uint64_t max_jiffies) {
        int ticks = 0;
        std::uint64_t last_j = 0;
        while (!game.player().dead && !game.player().won &&
               game.counters().total_jiffies < max_jiffies) {
            if ((++ticks % 200) == 0) {
                std::cerr << "tick " << ticks << " phase=" << static_cast<int>(phase)
                          << " lv=" << game.level_index() << " pos=" << game.player().row << ","
                          << game.player().col << " p=" << game.player().power
                          << " d=" << game.player().damage
                          << " j=" << game.counters().total_jiffies << "\n";
                if (ticks > 50 && game.counters().total_jiffies == last_j) {
                    report_block("planner made no clock progress");
                    return 1;
                }
                last_j = game.counters().total_jiffies;
            }
            if (game.player().fainted) {
                idle(20);
                continue;
            }
            const auto p = game.player();
            const std::uint16_t ring_effort = dag::scal16(p.power, 63);
            const bool ring_safe = static_cast<unsigned>(p.damage) + ring_effort < p.power;
            const int occ = here();
            if (occ >= 0) {
                if (!ring_safe && ring_ready()) {
                    if (!flee()) idle(20);
                    continue;
                }
                if (hand_type(false) == kVulcan || hand_type(true) == kVulcan)
                    type({"INCANT FIRE"}, 2);
                if (hand_type(false) == kHoth || hand_type(true) == kHoth)
                    type({"INCANT ICE"}, 2);
                const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(occ)];
                if (wizard(c) && ring_ready() && ring_safe) {
                    hit_run();
                    continue;
                }
                if (phase == TakeVulcan || phase == TakeHoth || phase == Arm) {
                    if (ring_ready() && ring_safe) {
                        hit_run();
                        continue;
                    }
                    strike_and_clear();
                    continue;
                }
                if (!flee()) {
                    if (ring_ready() && ring_safe) hit_run();
                    else strike_and_clear();
                }
                continue;
            }
            if ((rest_needed() || (ring_ready() && !ring_safe)) && phase != Light) {
                idle(40);
                if (waits > 400) {
                    report_block("rest not reducing damage");
                    return 1;
                }
                continue;
            }
            if (phase != KillImage && phase != KillWizard && break_alignment()) continue;

            switch (phase) {
                case Light: {
                    if (game.player().torch >= 0) {
                        phase = Arm;
                        break;
                    }
                    type({"PULL LEFT TORCH", "USE LEFT"});
                    if (game.player().torch < 0) {
                        report_block("could not light torch");
                        return 1;
                    }
                    phase = Arm;
                    break;
                }
                case Arm: {
                    if (game.player().left_hand >= 0 &&
                        game.objects()[static_cast<std::size_t>(game.player().left_hand)].cls ==
                            kClassSword) {
                        phase = TakeVulcan;
                        break;
                    }
                    type({"PULL LEFT SWORD"});
                    phase = TakeVulcan;
                    break;
                }
                case TakeVulcan: {
                    const int v = find_obj(game, kVulcan);
                    const int fire = find_obj(game, kFire);
                    if (owned_by_player(game, v) || owned_by_player(game, fire) ||
                        hand_type(false) == kFire || hand_type(true) == kFire ||
                        hand_type(false) == kVulcan || hand_type(true) == kVulcan) {
                        if (hand_type(false) == kVulcan) type({"INCANT FIRE"}, 2);
                        if (hand_type(true) == kVulcan) type({"INCANT FIRE"}, 2);
                        if (hand_type(false) != kFire && hand_type(true) != kFire) {
                            empty_hand(false);
                            type({"PULL LEFT RING", "INCANT FIRE"}, 2);
                        }
                        phase = Down1;
                        break;
                    }
                    take_object(kVulcan, "VULCAN RING");
                    if (waits > 2000) {
                        report_block("cannot take VULCAN");
                        return 1;
                    }
                    break;
                }
                case Down1: {
                    if (game.level_index() >= 1) {
                        phase = TakeHoth;
                        break;
                    }
                    if (game.player().row >= 28) {
                        if (face_and_move(0)) break;
                    }
                    if (!go_down() && waits > 2000) {
                        report_block("cannot climb to 1");
                        return 1;
                    }
                    break;
                }
                case TakeHoth: {
                    if (owned_by_player(game, find_obj(game, kHoth)) ||
                        hand_type(true) == kIce || hand_type(false) == kIce) {
                        if (hand_type(true) != kIce && hand_type(true) != kHoth) {
                            empty_hand(true);
                            type({"PULL RIGHT RING"});
                            if (hand_type(true) == kHoth) type({"INCANT ICE"});
                            else {
                                type({"GET RIGHT HOTH RING"});
                                if (hand_type(true) == kHoth) type({"INCANT ICE"});
                            }
                        } else if (hand_type(true) == kHoth)
                            type({"INCANT ICE"});
                        else if (hand_type(false) == kHoth)
                            type({"INCANT ICE"});
                        phase = Down2;
                        break;
                    }
                    take_object(kHoth, "RIME RING");
                    if (waits > 3000) {
                        report_block("cannot take HOTH");
                        return 1;
                    }
                    break;
                }
                case Down2: {
                    if (game.level_index() >= 2) {
                        phase = TakeThews;
                        break;
                    }
                    if (game.level_index() < 1) {
                        report_block("dropped below level 1");
                        return 1;
                    }
                    if (!go_down() && waits > 2000) {
                        report_block("cannot climb to 2");
                        return 1;
                    }
                    break;
                }
                case TakeThews: {
                    phase = KillImage;
                    break;
                }
                case KillImage: {
                    if (game.level_index() == 3 || game.player().won) {
                        phase = Down4;
                        break;
                    }
                    if (hand_type(false) == kVulcan) type({"INCANT FIRE"});
                    if (hand_type(true) == kHoth) type({"INCANT ICE"});
                    if (hand_type(false) != kFire && hand_type(true) != kFire &&
                        hand_type(false) != kIce && hand_type(true) != kIce) {
                        empty_hand(false);
                        type({"PULL LEFT RING", "INCANT FIRE"});
                    }
                    const int sl = slot_of_type(game, 10);
                    if (sl < 0) {
                        report_block("no type 10 on level 2");
                        return 1;
                    }
                    const dag::Ccb& w = game.creatures()[static_cast<std::size_t>(sl)];
                    const int pr = game.player().row, pc = game.player().col;
                    const int wr = w.row, wc = w.col;
                    const int adj = std::abs(pr - wr) + std::abs(pc - wc);
                    if (adj == 1) {
                        idle(6);
                        break;
                    }
                    if (!path_step(wr, wc, true)) idle(20);
                    if (waits > 4000) {
                        report_block("cannot reach type 10");
                        return 1;
                    }
                    break;
                }
                case Down4: {
                    if (game.level_index() >= 4) {
                        phase = KillWizard;
                        break;
                    }
                    if (game.level_index() < 3) {
                        report_block("not on level 3 after ENDGAM");
                        return 1;
                    }
                    if (!go_down() && waits > 3000) {
                        report_block("cannot climb to 4");
                        return 1;
                    }
                    break;
                }
                case KillWizard: {
                    const int sl = slot_of_type(game, 11);
                    if (sl < 0) {
                        if (find_obj(game, kSupreme) >= 0 &&
                            game.objects()[static_cast<std::size_t>(find_obj(game, kSupreme))]
                                    .owner == 0) {
                            phase = TakeSupreme;
                            break;
                        }
                        report_block("no type 11");
                        return 1;
                    }
                    if (!ring_ready()) {
                        if (hand_type(false) == kVulcan || hand_type(false) == kHoth)
                            type({hand_type(false) == kVulcan ? "INCANT FIRE" : "INCANT ICE"});
                        else {
                            report_block("no ring for type 11");
                            return 1;
                        }
                    }
                    const dag::Ccb& w = game.creatures()[static_cast<std::size_t>(sl)];
                    const int adj =
                        std::abs(game.player().row - w.row) + std::abs(game.player().col - w.col);
                    if (adj == 1) {
                        idle(6);
                        break;
                    }
                    if (!path_step(w.row, w.col, true)) idle(20);
                    if (waits > 5000) {
                        report_block("cannot reach type 11");
                        const dag::Ccb& ww = game.creatures()[static_cast<std::size_t>(sl)];
                        std::cerr << " remaining wizard damage=" << ww.damage
                                  << " power=" << ww.power << " ring_hit="
                                  << ring_damage(game.player().power, ww.magic_defense,
                                                 ww.physical_defense)
                                  << "\n";
                        return 1;
                    }
                    break;
                }
                case TakeSupreme: {
                    const int idx = find_obj(game, kSupreme);
                    if (idx < 0) {
                        report_block("no SUPREME");
                        return 1;
                    }
                    const dag::Ocb& o = game.objects()[static_cast<std::size_t>(idx)];
                    if (game.player().row != o.row || game.player().col != o.col) {
                        path_step(o.row, o.col, false);
                        break;
                    }
                    type({"GET RIGHT RING"});
                    phase = Win;
                    break;
                }
                case Win: {
                    type({"INCANT FINAL"});
                    if (!game.player().won) {
                        report_block("INCANT FINAL did not win");
                        return 1;
                    }
                    break;
                }
            }
        }
        if (!game.player().won) {
            report_block(game.player().dead ? "player died" : "jiffy budget exhausted");
            return 1;
        }
        return 0;
    }

    void write_script(const std::string& path) const {
        std::ofstream out(path);
        out << "# Original Mode power-on to WINNER. Authored by src/app/dplan.cpp driving the core.\n";
        out << "# One key per line; see parse_script in game.hpp.\n";
        for (const auto& k : log) {
            out << k.jiffy << ' ';
            if (k.ch == 0x20) out << "SPACE";
            else if (k.ch == 0x0D) out << "CR";
            else if (k.ch == 0x08) out << "BS";
            else out << static_cast<char>(k.ch);
            out << '\n';
        }
    }
};

int usage() {
    std::cerr << "usage: dplan --dump\n"
                 "       dplan --script FILE [--max-jiffies N]\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    std::string script;
    std::uint64_t max_jiffies = 4000000;
    bool dump = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() {
            if (i + 1 >= argc) std::exit(usage());
            return argv[++i];
        };
        if (a == "--dump") dump = true;
        else if (a == "--script") script = next();
        else if (a == "--max-jiffies") max_jiffies = std::strtoull(next(), nullptr, 10);
        else return usage();
    }
    if (dump) return dump_world();
    if (script.empty()) return usage();
    Runner r;
    const int rc = r.play(max_jiffies);
    r.write_script(script);
    std::cerr << "wrote " << script << " keys=" << r.log.size()
              << " jiffy=" << r.game.counters().total_jiffies << " won=" << r.game.player().won
              << "\n";
    return rc;
}
