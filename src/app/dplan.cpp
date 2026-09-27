// dplan — closed-loop Original Mode planner that records a dcli script.
//
// Play style follows published community paths (Nemitz, "A Tour of Daggorath")
// as strategy only. Every combat decision is checked against this core:
// pickup-before-attack, darkness gate, scal16 exertion, and CDBTAB delays.
//
//   dplan --dump
//   dplan --script FILE [--max-jiffies N]
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "daggorath/combat.hpp"
#include "daggorath/game.hpp"
#include "daggorath/lexicon_tables.hpp"

namespace {

constexpr int kSupreme = 0, kJoule = 1, kElvish = 2, kMithril = 3, kSeer = 4,
              kThews = 5, kHoth = 6, kVision = 7, kAbye = 8, kHale = 9,
              kSolar = 10, kBronze = 11, kVulcan = 12, kIron = 13, kLunar = 14,
              kPine = 15, kLeather = 16, kWooden = 17, kEnergy = 19, kIce = 20,
              kFire = 21, kGold = 22, kEmpty = 23, kDeadTorch = 24;
constexpr int kClassSword = 4, kClassRing = 1, kClassTorch = 5, kClassShield = 3,
              kClassFlask = 0;

const char* type_name(int t) {
    static constexpr const char* k[] = {
        "spider", "viper", "giant1", "blob",    "knight1", "giant2",
        "scorp",  "knight2", "wraith", "balrog", "wiz0",    "wiz1"};
    if (t < 0 || t >= 12) return "?";
    return k[t];
}

const char* obj_name(int t) {
    if (t >= 0 && t < static_cast<int>(dag::kAdjTab.size()))
        return dag::kAdjTab[static_cast<std::size_t>(t)].word.data();
    return "?";
}

bool wizard(const dag::Ccb& c) { return c.in_use && (c.type == 10 || c.type == 11); }

bool scorpion(const dag::Ccb& c) { return c.in_use && c.type == 6; }

// Scorpions and worse: do not walk onto their cell. Their CMOVE can fire
// on the same jiffy as the landing MOVE.
bool stingy(const dag::Ccb& c) { return c.in_use && c.type >= 6 && c.type < 10; }

// Community "wimp": spider or viper. They miss often enough that a dark park
// is survivable. Creatures have no darkness gate; the miss is ATTACK's roll.
bool wimp(const dag::Ccb& c) { return c.in_use && c.type <= 1; }

// Anything above viper can drop a 160-power player in one or two hits.
// Type 2 (club giant) is ~81 through $8080; type 3 (blob) is a one-shot.
bool lethal(const dag::Ccb& c) {
    if (!c.in_use) return false;
    return c.type >= 2 || c.magic_offense != 0;
}

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

int find_obj(const dag::Game& g, int type) {
    const auto& o = g.objects();
    for (int i = 0; i < static_cast<int>(o.size()); ++i)
        if (o[static_cast<std::size_t>(i)].type == type) return i;
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

int live_count(const dag::Game& g) {
    int n = 0;
    for (int i = 0; i < dag::kCcbSlots; ++i)
        if (g.creatures()[static_cast<std::size_t>(i)].in_use) ++n;
    return n;
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

int find_owned(const dag::Game& g, int type) {
    const auto& o = g.objects();
    for (int i = 0; i < static_cast<int>(o.size()); ++i)
        if (o[static_cast<std::size_t>(i)].type == type && owned_by_player(g, i)) return i;
    return -1;
}


int obj_pho(const dag::Game& g, int index) {
    if (index < 0) return -1;
    return g.objects()[static_cast<std::size_t>(index)].physical_offense;
}

int obj_reveal(const dag::Game& g, int index) {
    if (index < 0) return -1;
    return g.objects()[static_cast<std::size_t>(index)].reveal;
}

// Sword/shield/torch rows start as the class generic (WOODEN/LEATHER/PINE).
// IRON's specific pho is 40 only after REVEAL; 13 * 25 = 325 power.
bool can_reveal(const dag::Game& g, int index) {
    if (index < 0) return false;
    const auto& o = g.objects()[static_cast<std::size_t>(index)];
    if (o.reveal == 0) return false;
    return static_cast<unsigned>(o.reveal) * 25u <= g.player().power;
}

int dump_world() {
    dag::Game g;
    const auto& p = g.player();
    std::cout << "player row=" << p.row << " col=" << p.col
              << " dir=" << static_cast<int>(p.dir) << " power=" << p.power
              << " damage=" << p.damage << "\n";
    std::cout << "ring_hit_vs_wiz mdef=6 pdef=0 at P=160: " << ring_damage(160, 6, 0)
              << "\n";
    std::cout << "ring_exertion at P=160: " << dag::scal16(160, 63) << "\n";
    std::cout << "abye_damage at P=160: " << dag::scal16(160, 102) << "\n";
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
        std::cout << "  slot " << i << " " << type_name(c.type)
                  << " r=" << static_cast<int>(c.row) << " c=" << static_cast<int>(c.col)
                  << " p=" << c.power << " obj=" << c.object_head << "\n";
    }
    return 0;
}

struct Runner {
    dag::Game game;
    std::vector<dag::KeyEvent> log;
    std::size_t seen = 0;
    int waits = 0;
    int camp_r = -1, camp_c = -1;
    int last_floor = -1;
    std::uint64_t hold_since = 0;
    std::uint64_t phase_since = 0;
    enum Phase {
        Opening,
        DarkPark,
        DarkHold,
        LightUp,
        Clear,
        Loot,
        Descend,
        WallWalk,
        PrepRings,
        KillImage,
        Survive3,
        KillWizard,
        TakeSupreme,
        Win
    } phase = Opening;

    std::uint64_t jiffy_limit = 4000000;
    unsigned recoveries = 0;
    std::size_t recovery_cursor = 0;
    std::string latest_save;
    struct PlannerCheckpoint {
        Phase phase; int camp_r, camp_c, last_floor;
        std::uint64_t hold_since, phase_since;
    };
    PlannerCheckpoint saved_plan{};
    std::string cache_dir = ".cache/playthrough";
    std::set<std::string> saved_stages;
    struct Anno {
        std::uint64_t jiffy = 0;
        std::string line;
    };
    std::vector<Anno> annos;

    std::uint8_t encode(char c) const {
        if (c == ' ') return 0x20;
        return static_cast<std::uint8_t>(c);
    }

    void append_cmd(std::vector<dag::KeyEvent>& keys, std::uint64_t j,
                    const std::string& cmd) {
        for (const char c : cmd) {
            keys.push_back({j, encode(c)});
            log.push_back({j, encode(c)});
        }
        keys.push_back({j, 0x0D});
        log.push_back({j, 0x0D});
    }

    void type(const std::vector<std::string>& cmds, std::uint64_t extra = 3) {
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
        game.advance_jiffies(std::min(span == 0 ? 1 : span,
            jiffy_limit > now ? jiffy_limit - now : 0));
        waits = 0;
    }

    void idle(std::uint64_t n) {
        game.load_script({});
        const auto now = game.counters().total_jiffies;
        game.advance_jiffies(std::min(n, jiffy_limit > now ? jiffy_limit - now : 0));
        ++waits;
    }

    void note(const std::string& line) {
        annos.push_back({game.counters().total_jiffies, line});
    }

    void recover() { idle(20); } // HSLOW; no direct damage edits.

    std::filesystem::path stage_path(const std::string& stage, const char* ext) const {
        return std::filesystem::path(cache_dir) / (stage + ext);
    }

    void write_script_to(const std::string& path) const {
        std::ofstream out(path);
        out << "# Original Mode power-on to WINNER. Authored by src/app/dplan.cpp.\n";
        out << "# Candidate contains legal keys only; checkpoints use cassette commands.\n";
        std::size_t ai = 0;
        for (const auto& k : log) {
            while (ai < annos.size() && annos[ai].jiffy < k.jiffy) {
                out << annos[ai].jiffy << ' ' << annos[ai].line << '\n';
                ++ai;
            }
            out << k.jiffy << ' ';
            if (k.ch == 0x20)
                out << "SPACE";
            else if (k.ch == 0x0D)
                out << "CR";
            else if (k.ch == 0x08)
                out << "BS";
            else
                out << static_cast<char>(k.ch);
            out << '\n';
        }
        while (ai < annos.size()) {
            out << annos[ai].jiffy << ' ' << annos[ai].line << '\n';
            ++ai;
        }
    }

    void checkpoint(const std::string& stage) {
        if (saved_stages.count(stage)) return;
        static const std::map<std::string, std::string> names = {
            {"power-on", "POWERON"}, {"cleared-0", "FLOORA"},
            {"cleared-1", "FLOORB"}, {"cleared-2", "FLOORC"},
            {"pre-image", "IMAGE"}, {"endgam", "ENDGAM"},
            {"cleared-3", "FLOORD"}, {"pre-wizard", "WIZARD"}};
        const std::string name = names.at(stage);
        const auto from = game.trace().size();
        type({"ZSAVE " + name}, 5);
        bool saved = false;
        for (std::size_t i = from; i < game.trace().size(); ++i)
            if (game.trace()[i].kind == "ZSAVE" &&
                game.trace()[i].detail.rfind(name + " bytes=", 0) == 0) saved = true;
        if (!saved) {
            std::cerr << "checkpoint failed " << name << "\n";
            return;
        }
        latest_save = name;
        saved_plan = {phase, camp_r, camp_c, last_floor, hold_since, phase_since};
        saved_stages.insert(stage);
        std::filesystem::create_directories(cache_dir);
        write_script_to(stage_path(stage, ".script").string());
        std::cerr << "checkpoint " << name << " j=" << game.counters().total_jiffies
                  << " p=" << game.player().power << " d=" << game.player().damage << "\n";
    }

    bool reload_after_death() {
        if (latest_save.empty() || ++recoveries > 12) return false;
        // One restart key, then a separately typed cassette command. Retain all
        // candidate history; no snapshot supplies state to this execution.
        if (game.player().dead) type({"X"}, 2);
        const auto from = game.trace().size();
        type({"ZLOAD " + latest_save}, 5);
        bool loaded = false;
        for (std::size_t i = from; i < game.trace().size(); ++i)
            if (game.trace()[i].kind == "ZLOAD" && game.trace()[i].detail == latest_save) {
                loaded = true;
                recovery_cursor = i + 1;
            }
        if (!loaded) return false;
        phase = saved_plan.phase;
        camp_r = saved_plan.camp_r; camp_c = saved_plan.camp_c;
        last_floor = saved_plan.last_floor;
        hold_since = game.counters().total_jiffies;
        phase_since = hold_since;
        seen = game.trace().size(); waits = 0;
        // Change relative timing after each failure, rather than repeating an
        // identical deterministic attempt at a different absolute timestamp.
        idle(recoveries * 7);
        std::cerr << "recovery " << recoveries << " save=" << latest_save << "\n";
        return true;
    }

    bool saw_kind(const std::string& kind) {
        bool hit = false;
        for (std::size_t i = seen; i < game.trace().size(); ++i)
            if (game.trace()[i].kind == kind) hit = true;
        seen = game.trace().size();
        return hit;
    }

    int here() const { return creature_at(game, game.player().row, game.player().col); }

    int clearable() const {
        int n = 0;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (!c.in_use) continue;
            if (wizard(c) && phase != KillImage && phase != KillWizard) continue;
            ++n;
        }
        return n;
    }

    int mobs() const {
        int n = 0;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (c.in_use && !wizard(c)) ++n;
        }
        return n;
    }

    bool relight() {
        if (torch_live()) return true;
        if (game.level_index() >= 2)
            return light_named(kLunar, "LUNAR TORCH") || light_named(kSolar, "SOLAR TORCH") ||
                   light_pine();
        return light_pine() || light_named(kLunar, "LUNAR TORCH");
    }

    int hand_type(bool right) const {
        const int h = right ? game.player().right_hand : game.player().left_hand;
        if (h < 0) return -1;
        return game.objects()[static_cast<std::size_t>(h)].type;
    }

    int cls_of(int index) const {
        if (index < 0) return -1;
        return game.objects()[static_cast<std::size_t>(index)].cls;
    }

    bool torch_live() const {
        const int t = game.player().torch;
        if (t < 0) return false;
        return game.objects()[static_cast<std::size_t>(t)].type != kDeadTorch;
    }

    bool charged_ring(int type) const {
        return type == kFire || type == kIce || type == kEnergy;
    }

    bool ring_ready() const {
        return charged_ring(hand_type(false)) || charged_ring(hand_type(true));
    }

    bool ring_safe() const {
        const auto& p = game.player();
        const std::uint16_t effort = dag::scal16(p.power, 63);
        return p.damage < p.power / 5 &&
               static_cast<unsigned>(p.damage) + effort + 32 < p.power;
    }

    bool dest_ok(int rel, bool empty = true) const {
        const dag::Dir d =
            static_cast<dag::Dir>((static_cast<int>(game.player().dir) + rel) & 3);
        int nr = 0, nc = 0;
        if (!dag::step_ok(game.maze(), game.player().row, game.player().col, d, nr, nc))
            return false;
        if (empty && creature_at(game, nr, nc) >= 0) return false;
        return true;
    }

    int cell_danger(int r, int c) const {
        int near = 0, dmin = 99;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& cr = game.creatures()[static_cast<std::size_t>(i)];
            if (!cr.in_use || cr.type < 6) continue;
            const int d = std::abs(cr.row - r) + std::abs(cr.col - c);
            if (d < dmin) dmin = d;
            if (d <= 2) ++near;
        }
        return near * 16 - dmin;
    }

    const char* leave_cmd() const {
        const char* cmds[] = {"MOVE BACK", "MOVE LEFT", "MOVE RIGHT", "MOVE"};
        const int rels[] = {2, 3, 1, 0};
        const int pr = game.player().row, pc = game.player().col;
        const char* best = nullptr;
        int best_d = 1e9;
        for (int i = 0; i < 4; ++i) {
            if (!dest_ok(rels[i])) continue;
            const dag::Dir d =
                static_cast<dag::Dir>((static_cast<int>(game.player().dir) + rels[i]) & 3);
            int nr = 0, nc = 0;
            dag::step_ok(game.maze(), pr, pc, d, nr, nc);
            if (creature_at(game, nr, nc) >= 0) continue;
            int dist = camp_r < 0 ? i : std::abs(nr - camp_r) + std::abs(nc - camp_c);
            if (game.level_index() >= 3) dist = cell_danger(nr, nc);
            if (best == nullptr || dist < best_d) {
                best = cmds[i];
                best_d = dist;
            }
        }
        if (best != nullptr) return best;
        for (int i = 0; i < 4; ++i)
            if (dest_ok(rels[i], false)) return cmds[i];
        return "MOVE BACK";
    }

    const char* attack_cmd(bool use_ring) const {
        if (use_ring) {
            if (charged_ring(hand_type(false))) return "ATTACK LEFT";
            if (charged_ring(hand_type(true))) return "ATTACK RIGHT";
        }
        if (cls_of(game.player().left_hand) == kClassSword) return "ATTACK LEFT";
        if (cls_of(game.player().right_hand) == kClassSword) return "ATTACK RIGHT";
        if (game.player().left_hand >= 0) return "ATTACK LEFT";
        return "ATTACK RIGHT";
    }

    void swing() { type({attack_cmd(false)}, 3); }

    void hit_run(bool use_ring) {
        type({attack_cmd(use_ring), leave_cmd()}, 3);
        if (here() >= 0) type({leave_cmd()}, 3);
    }

    int floor_here() const {
        int n = 0;
        const int r = game.player().row, c = game.player().col, lv = game.level_index();
        for (const auto& o : game.objects()) {
            if (o.owner == 0 && o.level == lv && o.row == r && o.col == c) ++n;
        }
        return n;
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
                if (!dag::step_ok(game.maze(), r, c, static_cast<dag::Dir>(d), nr, nc))
                    continue;
                const int ni = idx(nr, nc);
                if (parent[static_cast<std::size_t>(ni)] != -2) continue;
                const int sl = creature_at(game, nr, nc);
                if (sl >= 0) {
                    const dag::Ccb& cr = game.creatures()[static_cast<std::size_t>(sl)];
                    if (avoid || cr.type >= 6) continue;
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
        if (nr < sr)
            want = 0;
        else if (nc > sc)
            want = 1;
        else if (nr > sr)
            want = 2;
        else
            want = 3;
        const int face = static_cast<int>(game.player().dir);
        const int delta = (want - face) & 3;
        if (delta == 1)
            type({"TURN RIGHT"});
        else if (delta == 3)
            type({"TURN LEFT"});
        else if (delta == 2)
            type({"TURN AROUND"});
        const int step_sl = creature_at(game, nr, nc);
        if (step_sl >= 0) {
            const dag::Ccb& cr = game.creatures()[static_cast<std::size_t>(step_sl)];
            if (avoid || cr.type >= 6) return false;
        }
        type({"MOVE"});
        return true;
    }

    bool step_abs(int want) {
        const int face = static_cast<int>(game.player().dir);
        const int rel = (want - face) & 3;
        const char* cmd = "MOVE";
        if (rel == 1)
            cmd = "MOVE RIGHT";
        else if (rel == 2)
            cmd = "MOVE BACK";
        else if (rel == 3)
            cmd = "MOVE LEFT";
        if (!dest_ok(rel)) return false;
        const int r0 = game.player().row, c0 = game.player().col;
        type({cmd});
        return game.player().row != r0 || game.player().col != c0;
    }

    // Leave the row or column a lethal is walking. Sidestepping along the
    // corridor keeps the line and they still arrive.
    bool sidestep() {
        const int pr = game.player().row, pc = game.player().col;
        bool on_row = false, on_col = false;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (!lethal(c) || wizard(c)) continue;
            if (c.row == pr) on_row = true;
            if (c.col == pc) on_col = true;
        }
        if (on_row) {
            if (step_abs(0) || step_abs(2)) return true;
        }
        if (on_col) {
            if (step_abs(1) || step_abs(3)) return true;
        }
        idle(10);
        return false;
    }

    // A lethal on the same row or column with a clear walk will CMOVE toward us.
    bool lethal_aligned() const {
        const int pr = game.player().row, pc = game.player().col;
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (!lethal(c) || wizard(c)) continue;
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
            if (clear) return true;
        }
        return false;
    }

    bool rest_needed() const {
        if (game.player().damage > 63) return true;
        const int hr = static_cast<int>(static_cast<std::int8_t>(game.player().heart_rate));
        return hr <= 8;
    }

    void empty_hand(bool right) {
        const int h = right ? game.player().right_hand : game.player().left_hand;
        if (h < 0) return;
        type({right ? "STOW RIGHT" : "STOW LEFT"});
    }

    bool have_sword() const {
        return cls_of(game.player().left_hand) == kClassSword ||
               cls_of(game.player().right_hand) == kClassSword;
    }

    bool holding_iron() const {
        const int lt = hand_type(false), rt = hand_type(true);
        return lt == kIron || lt == kElvish || rt == kIron || rt == kElvish;
    }

    bool iron_ready() const {
        if (!holding_iron()) return false;
        const int idx = hand_type(false) == kIron || hand_type(false) == kElvish
                            ? game.player().left_hand
                            : game.player().right_hand;
        return obj_reveal(game, idx) == 0 && obj_pho(game, idx) >= 40;
    }

    void reveal_hand(bool right) {
        const int h = right ? game.player().right_hand : game.player().left_hand;
        if (!can_reveal(game, h)) return;
        type({right ? "REVEAL RIGHT" : "REVEAL LEFT"});
    }

    void reveal_held() {
        reveal_hand(false);
        reveal_hand(true);
    }

    // IRON is born with WOODEN stats (pho 16). Without REVEAL a scorpion
    // survives the swing and its 4-tenth sting connects.
    void ensure_sword() {
        const bool holding_elvish = hand_type(false) == kElvish || hand_type(true) == kElvish;
        // ENDGAM preserves both hands but drops the reachable bag chain.  Its
        // discarded objects retain owner=player, so do not try to PULL one in
        // preference to an iron weapon that survived in a hand.
        if (holding_iron()) {
            reveal_held();
            return;
        }
        if (find_owned(game, kElvish) >= 0 && !holding_elvish) {
            const bool left_ring = charged_ring(hand_type(false));
            empty_hand(!left_ring);
            type({left_ring ? "PULL RIGHT ELVISH SWORD" : "PULL LEFT ELVISH SWORD"});
            reveal_held();
            return;
        }
        if (find_owned(game, kElvish) >= 0) {
            empty_hand(false);
            type({"PULL LEFT ELVISH SWORD"});
            reveal_held();
            return;
        }
        if (find_owned(game, kIron) >= 0) {
            empty_hand(false);
            type({"PULL LEFT IRON SWORD"});
            reveal_held();
            return;
        }
        if (have_sword()) {
            reveal_held();
            return;
        }
        empty_hand(false);
        type({"PULL LEFT SWORD"});
        reveal_held();
    }

    void ensure_mithril() {
        const int lt = hand_type(false), rt = hand_type(true);
        if (lt == kMithril || rt == kMithril) {
            reveal_held();
            return;
        }
        if (find_owned(game, kMithril) < 0) return;
        empty_hand(true);
        type({"PULL RIGHT MITHRIL SHIELD"});
        if (hand_type(true) != kMithril) type({"PULL RIGHT SHIELD"});
        reveal_held();
    }

    bool hole_occupied_by_stingy(int r, int c) const {
        const int sl = creature_at(game, r, c);
        if (sl < 0) return false;
        const dag::Ccb& cr = game.creatures()[static_cast<std::size_t>(sl)];
        return cr.type >= 6;
    }

    void douse_torch() {
        if (game.player().torch < 0) return;
        empty_hand(true);
        type({"PULL RIGHT TORCH"});
        if (cls_of(game.player().right_hand) == kClassTorch) type({"STOW RIGHT"});
    }

    bool light_pine() {
        if (torch_live()) return true;
        empty_hand(true);
        type({"PULL RIGHT PINE TORCH"});
        if (cls_of(game.player().right_hand) != kClassTorch) type({"PULL RIGHT TORCH"});
        if (cls_of(game.player().right_hand) == kClassTorch) type({"USE RIGHT"});
        return torch_live();
    }

    bool light_named(int want, const char* pull) {
        if (torch_live()) {
            const int t = game.player().torch;
            if (t >= 0 && game.objects()[static_cast<std::size_t>(t)].type == want)
                return true;
        }
        empty_hand(true);
        type({std::string("PULL RIGHT ") + pull});
        if (hand_type(true) != want) type({"PULL RIGHT TORCH"});
        if (cls_of(game.player().right_hand) == kClassTorch) type({"USE RIGHT"});
        return torch_live();
    }

    int object_cell(int idx, int& tr, int& tc) const {
        if (idx < 0) return -2;
        const dag::Ocb& o = game.objects()[static_cast<std::size_t>(idx)];
        if (o.owner == 0) {
            tr = o.row;
            tc = o.col;
            return -1;
        }
        int slot = o.carrier;
        if (slot < 0 || !game.creatures()[static_cast<std::size_t>(slot)].in_use) slot = -1;
        if (slot < 0) {
            for (int i = 0; i < dag::kCcbSlots; ++i) {
                const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
                if (!c.in_use) continue;
                for (int obj = c.object_head; obj >= 0;
                     obj = game.objects()[static_cast<std::size_t>(obj)].next) {
                    if (obj == idx) {
                        slot = i;
                        break;
                    }
                }
                if (slot == i) break;
            }
        }
        if (slot < 0) return -2;
        const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
        tr = c.row;
        tc = c.col;
        return slot;
    }

    bool go_down() {
        int best_d = 1e9, br = -1, bc = -1;
        int any_d = 1e9, ar = -1, ac = -1;
        const int pr = game.player().row, pc = game.player().col;
        for (int r = 0; r < 32; ++r)
            for (int c = 0; c < 32; ++c) {
                const int f = dag::vfind(game.level_index(), r, c);
                if (f < 0 || (f & 2) == 0) continue;
                if (game.maze().at(r, c) == 0xFF) continue;
                const int d = std::abs(r - pr) + std::abs(c - pc);
                if (d < any_d) {
                    any_d = d;
                    ar = r;
                    ac = c;
                }
                if (hole_occupied_by_stingy(r, c)) continue;
                if (creature_at(game, r, c) >= 0) continue;
                if (d < best_d) {
                    best_d = d;
                    br = r;
                    bc = c;
                }
            }
        if (br < 0) {
            br = ar;
            bc = ac;
        }
        if (br < 0) return false;
        if (pr == br && pc == bc) {
            if (here() >= 0) {
                // Clear the hole; do not climb onto a balrog.
                hit_run(false);
                return false;
            }
            if (game.level_index() == 0) type({"TURN RIGHT"});
            type({"CLIMB DOWN", attack_cmd(false), "MOVE BACK"}, 1);
            return true;
        }
        if (hole_occupied_by_stingy(br, bc) || creature_at(game, br, bc) >= 0) {
            if (!adjacent_to(br, bc)) go_adjacent(br, bc);
            else
                idle(8);
            return false;
        }
        if (!path_step(br, bc, true)) idle(20);
        return false;
    }

    void mark_camp() {
        camp_r = game.player().row;
        camp_c = game.player().col;
    }

    bool at_camp() const {
        return camp_r >= 0 && game.player().row == camp_r && game.player().col == camp_c;
    }

    bool adjacent_to(int r, int c) const {
        if (std::abs(game.player().row - r) + std::abs(game.player().col - c) != 1)
            return false;
        const int pr = game.player().row, pc = game.player().col;
        dag::Dir face = dag::Dir::North;
        if (r < pr)
            face = dag::Dir::North;
        else if (c > pc)
            face = dag::Dir::East;
        else if (r > pr)
            face = dag::Dir::South;
        else
            face = dag::Dir::West;
        int nr = 0, nc = 0;
        return dag::step_ok(game.maze(), pr, pc, face, nr, nc) && nr == r && nc == c;
    }

    bool go_adjacent(int tr, int tc) {
        if (adjacent_to(tr, tc)) return true;
        const int pr = game.player().row, pc = game.player().col;
        int br = -1, bc = -1, bd = 1e9;
        for (int d = 0; d < 4; ++d) {
            int nr = 0, nc = 0;
            if (!dag::step_ok(game.maze(), tr, tc, static_cast<dag::Dir>(d), nr, nc))
                continue;
            if (creature_at(game, nr, nc) >= 0) continue;
            const int wz = slot_of_type(game, 11);
            if (wz >= 0 && phase == Clear) {
                const dag::Ccb& w = game.creatures()[static_cast<std::size_t>(wz)];
                if (nr == w.row && nc == w.col) continue;
                if (std::abs(nr - w.row) + std::abs(nc - w.col) == 0) continue;
            }
            const int dist = std::abs(nr - pr) + std::abs(nc - pc);
            if (dist < bd) {
                bd = dist;
                br = nr;
                bc = nc;
            }
        }
        if (br < 0) {
            idle(20);
            return false;
        }
        if (!path_step(br, bc, true)) {
            idle(20);
            return false;
        }
        return true;
    }

    bool return_camp() {
        if (camp_r < 0 || at_camp()) return true;
        const int occ = creature_at(game, camp_r, camp_c);
        if (occ >= 0) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(occ)];
            // Walking onto a lethal lets CMOVE attack the same jiffy. Stand
            // next to camp and let them step on us (PUPDAT, no swing).
            if (lethal(c) || rest_needed() || game.player().damage > 80) {
                if (!adjacent_to(camp_r, camp_c)) go_adjacent(camp_r, camp_c);
                else
                    idle(12);
                return false;
            }
        }
        if (!path_step(camp_r, camp_c, true)) {
            if (!path_step(camp_r, camp_c, false)) idle(20);
            return false;
        }
        return true;
    }

    void drop_junk_hand(bool right) {
        const int t = hand_type(right);
        if (t < 0) return;
        if (t == kIron || t == kElvish || t == kVulcan || t == kHoth || t == kJoule ||
            t == kFire || t == kIce || t == kEnergy || t == kSupreme || t == kThews ||
            t == kHale)
            return;
        type({right ? "DROP RIGHT" : "DROP LEFT"});
    }

    // Tour: on core level 1, drop everything except one pine and the iron
    // sword. Extra flasks and dead torches stay as pickup bait.
    void drop_nonessential() {
        drop_junk_hand(false);
        drop_junk_hand(true);
        for (int n = 0; n < 8; ++n) {
            empty_hand(true);
            const int before = game.player().right_hand;
            type({"PULL RIGHT TORCH"});
            const int t = hand_type(true);
            if (t == kLunar || t == kSolar) {
                type({"STOW RIGHT"});
            } else if (t == kPine || t == kDeadTorch) {
                if (t != kPine || torch_live())
                    type({"DROP RIGHT"});
                else
                    type({"STOW RIGHT"});
            } else if (t == kWooden || t == kLeather || t == kEmpty || t == kAbye) {
                type({"DROP RIGHT"});
            } else if (game.player().right_hand == before || game.player().right_hand < 0) {
                break;
            } else {
                type({"STOW RIGHT"});
                break;
            }
        }
        ensure_sword();
    }

    // Empty flasks and dead torches are bait. Never drink ABYE: scal16(P, 102).
    void seed_bait() {
        empty_hand(true);
        type({"PULL RIGHT EMPTY FLASK"});
        if (hand_type(true) == kEmpty) type({"DROP RIGHT"});
        empty_hand(true);
        type({"PULL RIGHT DEAD TORCH"});
        if (hand_type(true) == kDeadTorch) type({"DROP RIGHT"});
        empty_hand(true);
        type({"PULL RIGHT WOODEN SWORD"});
        if (hand_type(true) == kWooden) type({"DROP RIGHT"});
    }

    bool occupy_tick() {
        const int occ = here();
        if (occ < 0) {
            last_floor = -1;
            return false;
        }
        const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(occ)];
        if (wizard(c) && phase != KillImage && phase != KillWizard) {
            type({leave_cmd()});
            return true;
        }
        if (!torch_live() && !wimp(c) && !wizard(c)) {
            relight();
            if (!torch_live() && c.type >= 2) {
                hit_run(false);
                return true;
            }
            if (!torch_live()) {
                type({leave_cmd()});
                return true;
            }
        }
        const int floor = floor_here();
        const bool pickup = saw_kind("PICKUP") || (last_floor >= 0 && floor < last_floor);
        last_floor = floor;

        if (scorpion(c)) {
            // Do not PULL on this cell: that burns jiffies before the swing.
            // ATTACK has no SYNC, so ATTACK+MOVE leaves before CMOVE.
            if (!torch_live()) {
                type({leave_cmd()});
                return true;
            }
            hit_run(false);
            return true;
        }
        if (wizard(c)) {
            if (phase == KillImage || phase == KillWizard) {
                if (ring_ready() && ring_safe()) hit_run(true);
                else type({leave_cmd()}, 3);
                return true;
            }
            type({leave_cmd()});
            return true;
        }
        if ((phase == DarkPark || phase == DarkHold) && wimp(c) && !torch_live()) {
            // Faint eats keys. A spider hit is 32; fainting on the cell is death.
            if (game.player().damage > 80 ||
                static_cast<unsigned>(game.player().damage) + 40 >= game.player().power) {
                type({leave_cmd()});
                return true;
            }
            idle(20);
            return true;
        }
        if (!torch_live() && !wimp(c)) {
            type({leave_cmd()});
            return true;
        }
        if (phase == Clear || phase == LightUp || phase == Survive3) {
            // Do not PULL here: a shared cell with a ready CMOVE is fatal.
            // Club giants and worse: hit and leave before CMOVE's attack
            // delay (23 tenths for type 2). Sitting through EXAMINE lets them
            // swing. Wimps still wait on a pickup.
            if (c.type >= 2) {
                if (game.level_index() >= 3 && game.player().damage > game.player().power / 3) {
                    type({leave_cmd()}, 3);
                    return true;
                }
                if (!torch_live() && !wimp(c)) {
                    type({leave_cmd()});
                    return true;
                }
                hit_run(false);
                return true;
            }
            if (floor > 0 && !pickup) {
                type({"EXAMINE"});
                idle(8);
                return true;
            }
            swing();
            return true;
        }
        if (lethal(c)) {
            type({leave_cmd()});
            return true;
        }
        idle(10);
        return true;
    }

    void report_block(const std::string& why) const {
        const auto& p = game.player();
        std::cerr << "BLOCKED: " << why << " phase=" << static_cast<int>(phase)
                  << " level=" << game.level_index() << " pos=" << p.row << "," << p.col
                  << " power=" << p.power << " damage=" << p.damage
                  << " weight=" << p.carried_weight << " heart="
                  << static_cast<int>(static_cast<std::int8_t>(p.heart_rate))
                  << " fainted=" << p.fainted << " dead=" << p.dead
                  << " jiffy=" << game.counters().total_jiffies << " live=" << live_count(game)
                  << "\n";
        std::cerr << " hands L=" << p.left_hand << " t=" << hand_type(false)
                  << " pho=" << obj_pho(game, p.left_hand)
                  << " rev=" << obj_reveal(game, p.left_hand)
                  << " R=" << p.right_hand << " t=" << hand_type(true)
                  << " pho=" << obj_pho(game, p.right_hand)
                  << " torch=" << p.torch << " bag=" << p.bag_head
                  << " camp=" << camp_r << "," << camp_c
                  << " iron_ready=" << iron_ready() << "\n";
        std::cerr << " bag";
        for (int i = p.bag_head; i >= 0; i = game.objects()[static_cast<std::size_t>(i)].next)
            std::cerr << " " << i << ":" << obj_name(game.objects()[static_cast<std::size_t>(i)].type);
        std::cerr << "\n";
        const int sl = creature_at(game, p.row, p.col);
        if (sl >= 0) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(sl)];
            std::cerr << " occupant " << type_name(c.type) << " dmg=" << c.damage
                      << " p=" << c.power << "\n";
        }
        for (int i = 0; i < dag::kCcbSlots; ++i) {
            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
            if (!c.in_use) continue;
            std::cerr << " live " << type_name(c.type) << " r=" << static_cast<int>(c.row)
                      << " c=" << static_cast<int>(c.col) << " dmg=" << c.damage << "\n";
        }
        for (int t : {kVulcan, kHoth, kJoule, kElvish, kThews, kSupreme, kFire, kIce,
                      kEnergy, kBronze}) {
            const int owned = find_owned(game, t);
            const int i = owned >= 0 ? owned : find_obj(game, t);
            if (i < 0) continue;
            const dag::Ocb& o = game.objects()[static_cast<std::size_t>(i)];
            std::cerr << "  obj " << obj_name(t) << " owner=" << static_cast<int>(o.owner)
                      << " lv=" << static_cast<int>(o.level) << " r=" << static_cast<int>(o.row)
                      << " c=" << static_cast<int>(o.col) << " carrier=" << o.carrier
                      << " pho=" << static_cast<int>(o.physical_offense)
                      << " rev=" << static_cast<int>(o.reveal) << "\n";
        }
        const auto& objs = game.objects();
        for (int i = 0; i < static_cast<int>(objs.size()); ++i) {
            const dag::Ocb& o = objs[static_cast<std::size_t>(i)];
            if (o.type != kIron) continue;
            std::cerr << "  iron[" << i << "] owner=" << static_cast<int>(o.owner)
                      << " lv=" << static_cast<int>(o.level) << " r=" << static_cast<int>(o.row)
                      << " c=" << static_cast<int>(o.col) << " pho="
                      << static_cast<int>(o.physical_offense) << " rev="
                      << static_cast<int>(o.reveal) << " owned=" << owned_by_player(game, i)
                      << "\n";
        }
    }

    bool get_if_here(int obj_type, const char* specific) {
        if (find_owned(game, obj_type) >= 0) return true;
        int tr = 0, tc = 0;
        const int slot = object_cell(find_obj(game, obj_type), tr, tc);
        if (slot != -1) return false;
        if (game.player().row != tr || game.player().col != tc) return false;
        empty_hand(true);
        type({std::string("GET RIGHT ") + specific});
        return owned_by_player(game, find_obj(game, obj_type));
    }

    bool collect(int obj_type, const char* specific) {
        if (find_owned(game, obj_type) >= 0) return true;
        int tr = 0, tc = 0;
        const int slot = object_cell(find_obj(game, obj_type), tr, tc);
        if (slot == -2) return false;
        if (slot >= 0) return false;
        if (game.player().row != tr || game.player().col != tc) {
            if (!path_step(tr, tc, true)) {
                if (!path_step(tr, tc, false)) idle(20);
            }
            return false;
        }
        empty_hand(true);
        type({std::string("GET RIGHT ") + specific});
        if (owned_by_player(game, find_obj(game, obj_type))) empty_hand(true);
        return owned_by_player(game, find_obj(game, obj_type));
    }

    // Faithful scratch experiments only: candidate state is never restored.
    // Predict one legal action plus an eight-jiffy advisory horizon, then
    // execute only that action through the candidate's normal three-jiffy
    // timestamped-key path. The scratch horizon never supplies game state.
    void survival_search_tick() {
        const auto state = game.snapshot();
        const auto now = game.counters().total_jiffies;
        std::vector<std::vector<std::string>> choices{{}};
        const char* moves[] = {"MOVE", "MOVE RIGHT", "MOVE BACK", "MOVE LEFT"};
        for (int rel = 0; rel < 4; ++rel) {
            if (!dest_ok(rel)) continue;
            choices.push_back({moves[rel]});
            if (here() >= 0) {
                choices.push_back({attack_cmd(false), moves[rel]});
                if (ring_ready() && ring_safe() &&
                    game.creatures()[static_cast<std::size_t>(here())].type >= 8)
                    choices.push_back({attack_cmd(true), moves[rel]});
            }
        }
        if (here() >= 0) choices.push_back({attack_cmd(false)});
        if (here() < 0) {
            for (const auto& o : game.objects()) {
                if (o.owner != 0 || o.level != game.level_index() ||
                    o.row != game.player().row ||
                    o.col != game.player().col) continue;
                const char* name = o.type == kElvish ? "ELVISH SWORD" :
                    o.type == kJoule ? "JOULE RING" :
                    o.type == kMithril ? "MITHRIL SHIELD" :
                    o.type == kThews ? "THEWS FLASK" : nullptr;
                if (name) choices.push_back({"STOW RIGHT", std::string("GET RIGHT ") + name});
            }
            if (hand_type(true) == kThews) choices.push_back({"USE RIGHT"});
            if (hand_type(true) == kElvish || hand_type(false) == kElvish)
                choices.push_back({hand_type(true) == kElvish ? "REVEAL RIGHT" : "REVEAL LEFT"});
        }
        // Direct exploration toward the equipment carrier rather than merely
        // orbiting the safest corner when no creature is immediately present.
        int tr = -1, tc = -1;
        for (int t : {kElvish, kJoule}) {
            if (find_owned(game, t) >= 0) continue;
            const int obj = find_obj(game, t);
            if (obj >= 0 && object_cell(obj, tr, tc) != -2) break;
            tr = tc = -1;
        }
        if (tr < 0) {
            int nearest = 9999;
            for (const auto& c : game.creatures()) {
                if (!c.in_use) continue;
                int d = std::abs(c.row - game.player().row) + std::abs(c.col - game.player().col);
                if (d < nearest) { nearest = d; tr = c.row; tc = c.col; }
            }
        }
        std::array<int, 1024> distance;
        distance.fill(1000);
        if (tr >= 0) {
            std::queue<int> q;
            distance[tr * 32 + tc] = 0; q.push(tr * 32 + tc);
            while (!q.empty()) {
                int cell = q.front(); q.pop();
                for (int d = 0; d < 4; ++d) {
                    int nr = 0, nc = 0;
                    if (!dag::step_ok(game.maze(), cell / 32, cell % 32,
                                     static_cast<dag::Dir>(d), nr, nc)) continue;
                    if (distance[nr * 32 + nc] <= distance[cell] + 1) continue;
                    distance[nr * 32 + nc] = distance[cell] + 1; q.push(nr * 32 + nc);
                }
            }
        }
        double best = -1e100;
        std::size_t selected = 0;
        for (std::size_t ci = 0; ci < choices.size(); ++ci) {
            dag::Game scratch;
            scratch.restore_snapshot(state);
            std::vector<dag::KeyEvent> keys;
            std::uint64_t j = now;
            std::size_t used = 0;
            for (const auto& cmd : choices[ci]) {
                if (used + cmd.size() + 1 > 31) { ++j; used = 0; }
                for (char c : cmd) keys.push_back({j, encode(c)});
                keys.push_back({j, 0x0D}); used += cmd.size() + 1;
            }
            scratch.load_script(keys);
            scratch.advance_jiffies(8);
            // Predict an escape continuation if an idle horizon is dangerous.
            // This scratch rollout is advisory; execute only the first choice.
            for (int depth = 0; depth < 4 && !scratch.player().dead; ++depth) {
                const auto prefix = scratch.snapshot();
                double best_escape = -1e100;
                std::string next_state;
                for (int rel = -1; rel < 4; ++rel) {
                    dag::Game probe;
                    probe.restore_snapshot(prefix);
                    std::vector<dag::KeyEvent> next_keys;
                    if (rel >= 0) {
                        int nr = 0, nc = 0;
                        auto d = static_cast<dag::Dir>((static_cast<int>(probe.player().dir) + rel) & 3);
                        if (!dag::step_ok(probe.maze(), probe.player().row, probe.player().col, d, nr, nc) ||
                            creature_at(probe, nr, nc) >= 0) continue;
                        for (char ch : std::string(moves[rel]))
                            next_keys.push_back({probe.counters().total_jiffies, encode(ch)});
                        next_keys.push_back({probe.counters().total_jiffies, 0x0D});
                    }
                    probe.load_script(next_keys);
                    probe.advance_jiffies(8);
                    double value = -static_cast<double>(probe.player().damage);
                    if (probe.player().dead) value -= 1e9;
                    if (probe.player().fainted) value -= 1e6;
                    for (const auto& cr : probe.creatures()) {
                        if (!cr.in_use) continue;
                        int d = std::abs(cr.row - probe.player().row) + std::abs(cr.col - probe.player().col);
                        value -= 100.0 / (d + 1);
                    }
                    if (value > best_escape) { best_escape = value; next_state = probe.snapshot(); }
                }
                if (next_state.empty()) break;
                scratch.restore_snapshot(next_state);
            }
            double score = -1000.0 * scratch.player().damage / scratch.player().power;
            if (scratch.player().dead) score -= 1e8;
            if (scratch.player().fainted) score -= 10000;
            score += (scratch.player().power - game.player().power) * 10;
            for (int sl = 0; sl < dag::kCcbSlots; ++sl) {
                const auto& before = game.creatures()[static_cast<std::size_t>(sl)];
                const auto& after = scratch.creatures()[static_cast<std::size_t>(sl)];
                if (!before.in_use) continue;
                if (!after.in_use) score += 1500;
                else score += 2500.0 * (static_cast<int>(after.damage) - before.damage) / before.power;
            }
            for (int t : {kElvish, kJoule, kMithril, kThews}) {
                if (find_owned(scratch, t) >= 0 && find_owned(game, t) < 0) score += 2000;
            }
            const int elv = find_owned(scratch, kElvish);
            if (elv >= 0 && scratch.objects()[static_cast<std::size_t>(elv)].reveal == 0 &&
                game.objects()[static_cast<std::size_t>(elv)].reveal != 0) score += 1500;
            score -= 8.0 * distance[scratch.player().row * 32 + scratch.player().col];
            // Prefer separation from fast creatures when immediate outcomes tie.
            for (const auto& c : scratch.creatures()) {
                if (!c.in_use) continue;
                const int d = std::abs(c.row - scratch.player().row) +
                              std::abs(c.col - scratch.player().col);
                score -= 20.0 / (d + 1);
            }
            if (score > best) { best = score; selected = ci; }
        }
        if (choices[selected].empty()) idle(8);
        else type(choices[selected], 3);
    }

    int play(std::uint64_t max_jiffies) {
        jiffy_limit = max_jiffies;
        int ticks = 0;
        std::uint64_t last_j = 0;
        checkpoint("power-on");
        // Demonstrate natural death and legal recovery from the latest save.
        while (!game.player().dead && game.counters().total_jiffies < std::min<std::uint64_t>(10000, max_jiffies)) idle(20);
        if (!game.player().dead || !reload_after_death()) {
            report_block("initial death/recovery failed; last save=" + latest_save);
            return 1;
        }
        while (!game.player().won && game.counters().total_jiffies < max_jiffies) {
            bool pending_death = game.player().dead;
            while (recovery_cursor < game.trace().size())
                if (game.trace()[recovery_cursor++].kind == "DEATH") pending_death = true;
            if (pending_death) {
                if (reload_after_death()) continue;
                report_block("recovery bound/failure; last save=" + latest_save);
                return 1;
            }
            if ((++ticks % 200) == 0) {
                std::cerr << "tick " << ticks << " phase=" << static_cast<int>(phase)
                          << " lv=" << game.level_index() << " pos=" << game.player().row
                          << "," << game.player().col << " p=" << game.player().power
                          << " d=" << game.player().damage
                          << " j=" << game.counters().total_jiffies
                          << " live=" << live_count(game) << "\n";
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
            if (phase == DarkPark && here() >= 0) {
                const dag::Ccb& parked =
                    game.creatures()[static_cast<std::size_t>(here())];
                if (wimp(parked) && !torch_live()) {
                    hold_since = game.counters().total_jiffies;
                    phase = DarkHold;
                }
            }
            if (phase == Survive3 && game.level_index() == 3 && mobs() > 0) {
                survival_search_tick();
                continue;
            }
            if (phase == Clear && game.level_index() >= 4 && mobs() > 0) {
                if (!holding_iron()) {
                    ensure_sword();
                    continue;
                }
                if (!torch_live()) {
                    relight();
                    continue;
                }
                survival_search_tick();
                continue;
            }
            if (occupy_tick()) continue;

            switch (phase) {
                case Opening: {
                    // Nemitz: T A; M; M R; M; M; M; T L; M; M; M; M; M.
                    // Lands on the crossing of two long level-0 corridors.
                    // Hole at (20,17) is then two paces east.
                    type({"TURN AROUND"});
                    type({"MOVE"});
                    type({"MOVE RIGHT"});
                    type({"MOVE"});
                    type({"MOVE"});
                    type({"MOVE"});
                    type({"TURN LEFT"});
                    type({"MOVE"});
                    type({"MOVE"});
                    type({"MOVE"});
                    type({"MOVE"});
                    type({"MOVE"});
                    mark_camp();
                    empty_hand(false);
                    type({"PULL LEFT SWORD"});
                    phase_since = game.counters().total_jiffies;
                    phase = DarkPark;
                    break;
                }
                case DarkPark: {
                    if (game.level_index() != 0 && camp_r < 0) {
                        phase = WallWalk;
                        break;
                    }
                    if (rest_needed()) {
                        recover();
                        break;
                    }
                    if (game.counters().total_jiffies - phase_since > 8000 ||
                        lethal_aligned()) {
                        // A club giant or blob is on the corridor. Light and
                        // use floor bait rather than touring the map.
                        phase = LightUp;
                        break;
                    }
                    if (camp_r >= 0 && !at_camp()) {
                        return_camp();
                        break;
                    }
                    idle(30);
                    break;
                }
                case DarkHold: {
                    if (game.counters().total_jiffies - hold_since > 6000 ||
                        game.player().damage > 80 || lethal_aligned()) {
                        phase = LightUp;
                        break;
                    }
                    if (camp_r >= 0 && !at_camp() && here() < 0) {
                        return_camp();
                        break;
                    }
                    idle(30);
                    break;
                }
                case LightUp: {
                    if (rest_needed()) {
                        recover();
                        break;
                    }
                    ensure_sword();
                    if (game.level_index() >= 4) {
                        relight();
                        phase = Clear;
                        break;
                    }
                    seed_bait();
                    bool lit = torch_live();
                    if (!lit && game.level_index() >= 2)
                        lit = light_named(kLunar, "LUNAR TORCH") ||
                              light_named(kSolar, "SOLAR TORCH");
                    if (!lit) lit = light_pine();
                    if (!lit) {
                        empty_hand(true);
                        type({"GET RIGHT TORCH"});
                        if (cls_of(game.player().right_hand) == kClassTorch)
                            type({"USE RIGHT"});
                        lit = torch_live();
                    }
                    if (!lit) {
                        // Level 4 after ENDGAM may have no live torch. Hunt
                        // with the sword and GET one off the floor.
                        phase = Clear;
                        break;
                    }
                    last_floor = floor_here();
                    phase = Clear;
                    break;
                }
                case Clear: {
                    if (clearable() == 0) {
                        phase = Loot;
                        break;
                    }
                    if (game.level_index() >= 2 && !iron_ready()) {
                        ensure_sword();
                        break;
                    }
                    if (!torch_live() && game.level_index() < 4) {
                        relight();
                        break;
                    }
                    if (rest_needed() && here() < 0 && clearable() > 2) {
                        recover();
                        break;
                    }
                    if (game.level_index() >= 4) {
                        if (hand_type(false) != kElvish && find_owned(game, kElvish) >= 0)
                            ensure_sword();
                        if (hand_type(false) != kMithril && hand_type(true) != kMithril)
                            ensure_mithril();
                        if (!torch_live()) {
                            light_named(kLunar, "LUNAR TORCH") ||
                                light_named(kSolar, "SOLAR TORCH") || light_pine();
                        }
                        int br = -1, bc = -1, bd = 1e9, bt = -1;
                        const int pr = game.player().row, pc = game.player().col;
                        const int wiz = slot_of_type(game, 11);
                        int wr = -99, wc = -99;
                        if (wiz >= 0) {
                            wr = game.creatures()[static_cast<std::size_t>(wiz)].row;
                            wc = game.creatures()[static_cast<std::size_t>(wiz)].col;
                        }
                        for (int pass = 0; pass < 2 && br < 0; ++pass) {
                            bd = 1e9;
                            for (int i = 0; i < dag::kCcbSlots; ++i) {
                                const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
                                if (!c.in_use) continue;
                                if (wizard(c)) continue;
                                if (pass == 0 && wiz >= 0 &&
                                    std::abs(c.row - wr) + std::abs(c.col - wc) <= 2)
                                    continue;
                                const int d = std::abs(c.row - pr) + std::abs(c.col - pc);
                                if (d < bd) {
                                    bd = d;
                                    br = c.row;
                                    bc = c.col;
                                    bt = c.type;
                                }
                            }
                        }
                        if (br < 0) {
                            idle(8);
                            break;
                        }
                        if (bd == 0) {
                            hit_run(false);
                            break;
                        }
                        if (lethal_aligned() && bd > 1) {
                            sidestep();
                            break;
                        }
                        if (bd > 1 || bt >= 6) {
                            if (!adjacent_to(br, bc)) go_adjacent(br, bc);
                            else
                                idle(6);
                            break;
                        }
                        idle(8);
                        break;
                    }
                    if (clearable() <= 2) {
                        int br = -1, bc = -1, bd = 1e9, bt = -1;
                        const int pr = game.player().row, pc = game.player().col;
                        for (int i = 0; i < dag::kCcbSlots; ++i) {
                            const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
                            if (!c.in_use) continue;
                            if (wizard(c) && phase != KillImage && phase != KillWizard)
                                continue;
                            const int d = std::abs(c.row - pr) + std::abs(c.col - pc);
                            if (d < bd) {
                                bd = d;
                                br = c.row;
                                bc = c.col;
                                bt = c.type;
                            }
                        }
                        // Scorpions attack on a shared landing jiffy. Stand
                        // next to them and let CMOVE walk on (PUPDAT, no sting).
                        if (br >= 0 && bd > 0) {
                            const int occ = creature_at(game, br, bc);
                            const bool kite = occ >= 0 && stingy(
                                game.creatures()[static_cast<std::size_t>(occ)]);
                            if (kite || bt >= 6) {
                                if (!adjacent_to(br, bc)) go_adjacent(br, bc);
                                else
                                    idle(8);
                            } else if (!path_step(br, bc, false)) {
                                idle(20);
                            }
                        } else {
                            idle(8);
                        }
                        break;
                    }
                    if (camp_r >= 0 && !at_camp()) {
                        return_camp();
                        break;
                    }
                    if (lethal_aligned() && floor_here() == 0) seed_bait();
                    idle(20);
                    break;
                }
                case Loot: {
                    // Leave ABYE on the floor. Collect weapons, rings, Hale,
                    // Thews, bronze, and spare torches. Do not INCANT yet.
                    if (game.level_index() == 0) {
                        if (rest_needed()) {
                            recover();
                            break;
                        }
                        if (hand_type(false) == kVulcan) type({"INCANT FIRE"});
                        if (hand_type(true) == kVulcan) type({"INCANT FIRE"});
                        if (hand_type(false) == kFire) empty_hand(false);
                        if (hand_type(true) == kFire) empty_hand(true);
                        if (find_owned(game, kIron) < 0) {
                            collect(kIron, "IRON SWORD");
                            break;
                        }
                        if (!holding_iron() || !iron_ready()) {
                            ensure_sword();
                            break;
                        }
                        if (find_owned(game, kFire) < 0) {
                            if (find_owned(game, kVulcan) < 0) {
                                collect(kVulcan, "VULCAN RING");
                                break;
                            }
                            empty_hand(true);
                            type({"PULL RIGHT VULCAN RING"});
                            if (hand_type(true) != kVulcan) type({"PULL RIGHT RING"});
                            if (hand_type(true) == kVulcan) type({"INCANT FIRE"});
                            if (hand_type(true) == kFire) empty_hand(true);
                            break;
                        }
                        if (find_owned(game, kLunar) < 0) {
                            collect(kLunar, "LUNAR TORCH");
                            if (find_owned(game, kLunar) < 0 && waits < 400) break;
                        }
                        // Keep the revealed iron in the left hand for level 2.
                        ensure_sword();
                        checkpoint("cleared-0");
                        phase = Descend;
                        break;
                    }
                    if (rest_needed()) {
                        if (find_owned(game, kHale) >= 0) {
                            empty_hand(true);
                            type({"PULL RIGHT HALE FLASK"});
                            if (hand_type(true) != kHale) type({"GET RIGHT HALE FLASK"});
                            if (hand_type(true) == kHale) type({"USE RIGHT"});
                        } else if (find_obj(game, kHale) >= 0 &&
                                   game.objects()[static_cast<std::size_t>(find_obj(game, kHale))]
                                           .level == game.level_index()) {
                            collect(kHale, "HALE FLASK");
                        } else {
                            recover();
                        }
                        break;
                    }
                    if (game.level_index() == 1) {
                        if (find_owned(game, kHoth) < 0 && find_owned(game, kIce) < 0) {
                            collect(kHoth, "RIME RING");
                            break;
                        }
                        if (hand_type(false) == kHoth || hand_type(true) == kHoth)
                            type({"INCANT ICE"});
                        if (find_owned(game, kHoth) >= 0 &&
                            hand_type(false) != kIce && hand_type(true) != kIce &&
                            hand_type(false) != kHoth && hand_type(true) != kHoth) {
                            empty_hand(true);
                            type({"PULL RIGHT RIME RING"});
                            if (hand_type(true) != kHoth) type({"PULL RIGHT RING"});
                            if (hand_type(true) == kHoth) type({"INCANT ICE"});
                            if (hand_type(true) == kIce) empty_hand(true);
                            break;
                        }
                        if (find_owned(game, kSolar) < 0) {
                            collect(kSolar, "SOLAR TORCH");
                            if (find_owned(game, kSolar) < 0 && waits < 400)
                                break;
                        }
                        collect(kBronze, "BRONZE SHIELD");
                        if (find_owned(game, kBronze) < 0 && waits < 400)
                            break;
                        ensure_sword();
                        checkpoint("cleared-1");
                        phase = Descend;
                        break;
                    }
                    if (game.level_index() == 2) {
                        if (game.player().power < 4000) {
                            if (find_owned(game, kThews) < 0) {
                                collect(kThews, "THEWS FLASK");
                                break;
                            }
                            empty_hand(true);
                            type({"PULL RIGHT THEWS FLASK"});
                            if (hand_type(true) != kThews) type({"GET RIGHT THEWS FLASK"});
                            if (hand_type(true) == kThews) type({"USE RIGHT"});
                            break;
                        }
                        checkpoint("cleared-2");
                        phase = PrepRings;
                        break;
                    }
                    if (game.level_index() == 3) {
                        if (find_owned(game, kJoule) < 0 && find_owned(game, kEnergy) < 0) {
                            collect(kJoule, "JOULE RING");
                            break;
                        }
                        if (find_owned(game, kElvish) < 0) {
                            collect(kElvish, "ELVISH SWORD");
                            break;
                        }
                        collect(kMithril, "MITHRIL SHIELD");
                        if (find_owned(game, kJoule) >= 0 && hand_type(false) != kEnergy &&
                            hand_type(true) != kEnergy) {
                            empty_hand(true);
                            type({"PULL RIGHT JOULE RING"});
                            if (hand_type(true) != kJoule) type({"PULL RIGHT RING"});
                            if (hand_type(true) == kJoule) type({"INCANT ENERGY"});
                            break;
                        }
                        if (find_owned(game, kJoule) >= 0 || find_owned(game, kEnergy) >= 0 || waits > 800) {
                            checkpoint("cleared-3");
                            phase = Descend;
                        }
                        break;
                    }
                    if (game.level_index() >= 4) {
                        phase = mobs() == 0 ? KillWizard : Clear;
                        break;
                    }
                    phase = Descend;
                    break;
                }
                case Descend: {
                    if (game.level_index() >= 4) {
                        phase = mobs() == 0 ? KillWizard : Clear;
                        break;
                    }
                    const int before = game.level_index();
                    if (before == 3) {
                        ensure_sword();
                        ensure_mithril();
                    }
                    if (before == 2 && clearable() > 0 && slot_of_type(game, 10) >= 0) {
                        // Image is not last: do not fight it. Climb up to farm
                        // if a hole up exists, else keep clearing.
                        phase = Clear;
                        break;
                    }
                    // Level 2 scorpions: one revealed iron hit at ~1600 power.
                    if (before == 1 && !iron_ready()) {
                        ensure_sword();
                        if (!iron_ready()) {
                            if (waits > 400) {
                                report_block("no revealed IRON for level 2");
                                return 1;
                            }
                            break;
                        }
                    }
                    if (!go_down() && waits > 2000) {
                        report_block("cannot climb down");
                        return 1;
                    }
                    if (game.level_index() > before) {
                        if (game.level_index() >= 4) {
                            camp_r = -1;
                            camp_c = -1;
                        } else {
                            mark_camp();
                        }
                        // Do not PULL/DROP on the landing cell. L4 holes spawn
                        // next to balrogs; inventory burns the attack delay.
                        if (game.level_index() == 3) {
                            phase = Survive3;
                            break;
                        }
                        phase_since = game.counters().total_jiffies;
                        phase = game.level_index() == 1 ? DarkPark : LightUp;
                        break;
                    }
                    break;
                }
                case WallWalk: {
                    // Level 1: torch off, forward to a wall, turn right, repeat.
                    if (game.player().damage > 50) {
                        recover();
                        break;
                    }
                    const int f = dag::vfind(game.level_index(), game.player().row,
                                             game.player().col);
                    if (f >= 0 && (f & 2) != 0 && game.level_index() >= 1) {
                        mark_camp();
                        ensure_sword();
                        phase = DarkPark;
                        hold_since = game.counters().total_jiffies;
                        break;
                    }
                    if (dest_ok(0))
                        type({"MOVE"});
                    else
                        type({"TURN RIGHT"});
                    if (waits > 400) {
                        mark_camp();
                        phase = DarkPark;
                    }
                    break;
                }
                case PrepRings: {
                    // ENDGAM keeps hands and the torch. Hold a charged ring and
                    // the iron sword on the killing shot.
                    if (game.player().power < 1000 && find_owned(game, kThews) >= 0) {
                        empty_hand(true);
                        type({"PULL RIGHT THEWS FLASK", "USE RIGHT"});
                        break;
                    }
                    empty_hand(false);
                    type({"PULL LEFT RING"});
                    if (hand_type(false) == kVulcan) type({"INCANT FIRE"});
                    if (hand_type(false) == kHoth) type({"INCANT ICE"});
                    if (hand_type(false) == kJoule) type({"INCANT ENERGY"});
                    empty_hand(true);
                    type({"PULL RIGHT IRON SWORD"});
                    if (!have_sword()) type({"PULL RIGHT SWORD"});
                    if (!ring_ready()) {
                        report_block("rings not ready");
                        return 1;
                    }
                    relight();
                    checkpoint("pre-image");
                    phase = KillImage;
                    break;
                }
                case KillImage: {
                    if (game.level_index() == 3 || game.player().won) {
                        // ENDGAM relocates onto a new level-3 maze. The old
                        // hole camp is a different cell now.
                        mark_camp();
                        phase = Survive3;
                        checkpoint("endgam");
                        break;
                    }
                    const int held_ring = game.player().left_hand;
                    if (held_ring >= 0 && charged_ring(hand_type(false)) &&
                        game.objects()[static_cast<std::size_t>(held_ring)].spec[0] == 1 &&
                        find_owned(game, hand_type(false) == kIce ? kFire : kIce) >= 0) {
                        const bool was_ice = hand_type(false) == kIce;
                        empty_hand(false);
                        type({was_ice ? "PULL LEFT FIRE RING" : "PULL LEFT ICE RING"});
                        break;
                    }
                    if (!ring_ready()) {
                        empty_hand(false);
                        if (find_owned(game, kFire) >= 0) type({"PULL LEFT FIRE RING"});
                        else if (find_owned(game, kIce) >= 0) type({"PULL LEFT ICE RING"});
                        else {
                            report_block("image fight exhausted attack rings");
                            return 1;
                        }
                        break;
                    }
                    if (!ring_safe()) {
                        recover();
                        break;
                    }
                    const int sl = slot_of_type(game, 10);
                    if (sl < 0) {
                        report_block("no type 10");
                        return 1;
                    }
                    const dag::Ccb& w = game.creatures()[static_cast<std::size_t>(sl)];
                    if (!adjacent_to(w.row, w.col)) {
                        if (!go_adjacent(w.row, w.col)) idle(20);
                    } else idle(8);
                    if (waits > 4000) {
                        report_block("cannot reach type 10");
                        return 1;
                    }
                    break;
                }
                case Survive3: {
                    if (game.level_index() >= 4) {
                        phase = mobs() == 0 ? KillWizard : Clear;
                        break;
                    }
                    if (clearable() == 0) {
                        phase = Loot;
                        break;
                    }
                    mark_camp();
                    if (!torch_live()) light_named(kSolar, "SOLAR TORCH") || light_pine();
                    ensure_sword();
                    if (rest_needed()) {
                        recover();
                        break;
                    }
                    if (!at_camp()) {
                        return_camp();
                        break;
                    }
                    idle(8);
                    break;
                }
                case KillWizard: {
                    checkpoint("pre-wizard");
                    const int sl = slot_of_type(game, 11);
                    if (sl < 0) {
                        phase = TakeSupreme;
                        break;
                    }
                    if (mobs() > 0) {
                        phase = Clear;
                        break;
                    }
                    ensure_sword();
                    // Prepare the ring through typed commands after a cassette
                    // reload as well as during uninterrupted candidate progress.
                    if (!charged_ring(hand_type(false)) && !charged_ring(hand_type(true))) {
                        empty_hand(true);
                        type({"PULL RIGHT JOULE RING"});
                        type({"INCANT ENERGY"});
                        if (hand_type(true) != kEnergy) {
                            empty_hand(true);
                            type({"PULL RIGHT ENERGY RING"});
                        }
                    }
                    if (!ring_ready() && !have_sword()) {
                        report_block("no weapon for type 11");
                        return 1;
                    }
                    if (ring_ready() && !ring_safe()) {
                        recover();
                        break;
                    }
                    const dag::Ccb& w = game.creatures()[static_cast<std::size_t>(sl)];
                    const int adj = std::abs(game.player().row - w.row) +
                                    std::abs(game.player().col - w.col);
                    if (adj > 1) go_adjacent(w.row, w.col);
                    else if (adj == 1)
                        idle(8);
                    if (waits > 5000) {
                        report_block("cannot reach type 11");
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
                    empty_hand(true);
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
            report_block(std::string(game.player().dead ? "player died" : "jiffy budget exhausted") +
                         "; last save=" + latest_save);
            return 1;
        }
        return 0;
    }

    void write_script(const std::string& path) const { write_script_to(path); }
};

int usage() {
    std::cerr << "usage: dplan --dump\n"
                 "       dplan --script FILE [--max-jiffies N] [--cache DIR]\n"
                 "              (always starts from power-on)\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    std::string script;
    std::uint64_t max_jiffies = 4000000;
    bool dump = false;
    std::string cache = ".cache/playthrough";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() {
            if (i + 1 >= argc) std::exit(usage());
            return argv[++i];
        };
        if (a == "--dump")
            dump = true;
        else if (a == "--script")
            script = next();
        else if (a == "--max-jiffies")
            max_jiffies = std::strtoull(next(), nullptr, 10);
        else if (a == "--cache")
            cache = next();
        else
            return usage();
    }
    if (dump) return dump_world();
    if (script.empty()) return usage();
    Runner r;
    r.cache_dir = cache;
    const int rc = r.play(max_jiffies);
    r.write_script(script);
    std::cerr << "wrote " << script << " keys=" << r.log.size()
              << " jiffy=" << r.game.counters().total_jiffies
              << " won=" << r.game.player().won << "\n";
    return rc;
}
