// Phase 3 combat regression tests required by docs/prompts/phase-3-combat.md §4.
//
// Each case sets up a known state with the narrow test hooks (hold, wield_torch,
// place_player, set_player_damage, set_frozen) and asserts on the trace the core
// emits. Expected values follow PATTK.ASM and HUPDAT.ASM at the pinned listing.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "daggorath/game.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

std::vector<dag::KeyEvent> type_at(std::uint64_t start, const std::string& text) {
    std::vector<dag::KeyEvent> out;
    std::uint64_t j = start;
    for (const char ch : text) out.push_back({j++, static_cast<std::uint8_t>(ch)});
    out.push_back({j, 0x0D});
    return out;
}

int find_object(const dag::Game& game, int type, bool player_owned) {
    const auto& objects = game.objects();
    for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
        const dag::Ocb& o = objects[static_cast<std::size_t>(i)];
        if (o.type != type) continue;
        if (player_owned != (o.owner == 1)) continue;
        return i;
    }
    return -1;
}

constexpr int kTypeVulcan = 12, kTypePine = 15, kTypeWooden = 17;

int count(const dag::Game& game, const std::string& kind, std::uint64_t from = 0) {
    int n = 0;
    for (const auto& e : game.trace())
        if (e.kind == kind && e.jiffy >= from) ++n;
    return n;
}

int count_detail(const dag::Game& game, const std::string& kind, const std::string& detail) {
    int n = 0;
    for (const auto& e : game.trace())
        if (e.kind == kind && e.detail == detail) ++n;
    return n;
}

// Stand the player on creature `slot` with creatures frozen.
void stand_on(dag::Game& game, int slot) {
    game.set_frozen(true);
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
}

int first_live(const dag::Game& game) {
    for (int i = 0; i < dag::kCcbSlots; ++i)
        if (game.creatures()[static_cast<std::size_t>(i)].in_use) return i;
    return -1;
}

void test_attack_while_fainted() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    game.hold(false, find_object(game, kTypeWooden, true));
    // HUPDAX: 64*160/(160+2*155) = 21, +1 -19 = 3, which is the faint threshold.
    game.set_player_damage(155);
    check(game.player().fainted && !game.player().dead, "damage 155 of 160 faints the player");
    const std::size_t before = game.trace().size();
    game.load_script(type_at(1, "ATTACK LEFT"));
    game.advance_jiffies(20);
    bool line = false, exert = false;
    for (std::size_t i = before; i < game.trace().size(); ++i) {
        if (game.trace()[i].kind == "LINE") line = true;
        if (game.trace()[i].kind == "EXERT") exert = true;
    }
    check(!line && !exert, "a fainted player's ATTACK keys are eaten (HMAN: just eat chars)");
}

void test_attack_in_darkness() {
    // No torch: every connecting non-ring swing draws a second byte and hits
    // only when its low two bits are zero (PATT22).
    dag::Game dark(1, 0);
    const int slot = first_live(dark);
    stand_on(dark, slot);
    dark.hold(false, find_object(dark, kTypeWooden, true));
    std::string keys;
    for (int n = 0; n < 30; ++n) keys += "ATTACK LEFT\r";
    std::vector<dag::KeyEvent> script;
    std::uint64_t j = 1;
    for (const char ch : keys) script.push_back({j++, static_cast<std::uint8_t>(ch == '\r' ? 0x0D : ch)});
    dark.load_script(script);
    dark.advance_jiffies(j + 5);
    check(count(dark, "DARK") > 0, "unlit swings are rejected by the darkness gate",
          "dark=" + std::to_string(count(dark, "DARK")));
    bool gate_ok = true;
    for (const auto& e : dark.trace()) {
        if (e.kind != "DARK") continue;
        const int roll = std::stoi(e.detail.substr(e.detail.find('=') + 1));
        if ((roll & 3) == 0) gate_ok = false;
    }
    check(gate_ok, "every DARK rejection has a nonzero low-two-bit roll");

    dag::Game lit(1, 0);
    stand_on(lit, first_live(lit));
    lit.hold(false, find_object(lit, kTypeWooden, true));
    lit.wield_torch(find_object(lit, kTypePine, true));
    lit.load_script(script);
    lit.advance_jiffies(j + 5);
    check(count(lit, "DARK") == 0, "a live torch skips the darkness gate");
    check(count(lit, "HIT") > 0, "lit swings connect");
}

void test_empty_hand() {
    dag::Game game(1, 0);
    stand_on(game, first_live(game));
    game.wield_torch(find_object(game, kTypePine, true));
    game.load_script(type_at(1, "ATTACK RIGHT"));
    game.advance_jiffies(30);
    bool sound = false;
    for (const auto& e : game.trace())
        if (e.kind == "SOUND" && e.detail == "class=4") sound = true;
    check(sound, "an empty hand makes EMPHND's class-4 sword noise");
    // (5 + 0) >> 3 == 0, so SCAL16 adds no exertion.
    check(game.player().damage == 0, "an empty-hand swing costs no exertion",
          "damage=" + std::to_string(game.player().damage));
    check(count(game, "HIT") + count(game, "MISS") == 1, "an empty-hand swing still attacks");
}

void test_ring_bypass() {
    dag::Game game(1, 0);
    stand_on(game, first_live(game));
    game.hold(false, find_object(game, kTypeVulcan, false));
    game.load_script(type_at(1, "ATTACK LEFT"));
    game.advance_jiffies(30);
    check(count(game, "HIT") == 1 && count(game, "MISS") == 0 && count(game, "DARK") == 0,
          "a ring swing hits without ATTACK or the darkness gate");
    // Control: the same jiffies with a command that draws nothing. CREGEN and
    // CMOVE draw in both runs, so equal seeds mean the ring drew zero bytes.
    dag::Game control(1, 0);
    stand_on(control, first_live(control));
    control.load_script(type_at(1, "LOOK"));
    control.advance_jiffies(30);
    check(game.level().rng.seed() == control.level().rng.seed(),
          "a ring swing draws no RNG bytes");
}

void test_same_jiffy_attacks() {
    // Unfrozen creature on the player's cell: find the jiffy of its first
    // attack, then land the player's CR on that jiffy.
    dag::Game probe(1, 0);
    const int slot = first_live(probe);
    const dag::Ccb& c = probe.creatures()[static_cast<std::size_t>(slot)];
    probe.place_player(c.row, c.col);
    probe.advance_jiffies(600);
    const std::string mark = "slot=" + std::to_string(slot) + " type=";
    std::uint64_t at = 0;
    for (const auto& e : probe.trace()) {
        if (e.kind == "SOUND" && e.detail.rfind(mark, 0) == 0 &&
            e.detail.find("vol=255") != std::string::npos) {
            at = e.jiffy;
            break;
        }
    }
    check(at > 20, "the creature on the player's cell attacks", "jiffy=" + std::to_string(at));
    if (at <= 20) return;

    dag::Game game(1, 0);
    const dag::Ccb& g = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(g.row, g.col);
    game.wield_torch(find_object(game, kTypePine, true));
    // A key stamped on jiffy J is buffered by that interrupt and read on J + 1.
    const std::string text = "ATTACK RIGHT";
    game.load_script(type_at(at - 1 - text.size(), text));
    game.advance_jiffies(at + 1);
    int player_at = -1, creature_at = -1;
    int index = 0;
    for (const auto& e : game.trace()) {
        ++index;
        if (e.jiffy != at) continue;
        if (e.kind == "SOUND" && e.detail == "class=4" && player_at < 0) player_at = index;
        if (e.kind == "SOUND" && e.detail.rfind(mark, 0) == 0 &&
            e.detail.find("vol=255") != std::string::npos && creature_at < 0)
            creature_at = index;
    }
    check(player_at > 0 && creature_at > 0, "player and creature both attack on one jiffy",
          "player=" + std::to_string(player_at) + " creature=" + std::to_string(creature_at));
    check(player_at < creature_at, "PLAYER (Q.JIF) resolves before CMOVE (Q.TEN)");
}

void test_kill_then_reentry() {
    dag::Game game(1, 0);
    const int slot = first_live(game);
    stand_on(game, slot);
    game.hold(false, find_object(game, kTypeWooden, true));
    game.wield_torch(find_object(game, kTypePine, true));
    const std::uint8_t type = game.creatures()[static_cast<std::size_t>(slot)].type;
    const std::uint8_t before = game.matrix_row()[type];
    std::vector<dag::KeyEvent> script;
    std::uint64_t j = 1;
    for (int n = 0; n < 40; ++n) {
        for (const char ch : std::string("ATTACK LEFT\r"))
            script.push_back({j++, static_cast<std::uint8_t>(ch == '\r' ? 0x0D : ch)});
    }
    game.load_script(script);
    game.advance_jiffies(j + 5);
    check(count(game, "KILL") == 1, "a lit wooden sword kills the creature",
          "kills=" + std::to_string(count(game, "KILL")));
    check(count_detail(game, "SOUND", "A$KLK2") > 0, "a connecting swing emits ISOUND A$KLK2");
    check(count_detail(game, "DIALOGUE", "!!!") == count_detail(game, "SOUND", "A$KLK2"),
          "each hit prints OUTSTI !!!");
    check(count_detail(game, "SOUND", "A$EXP0") == 1, "the kill emits ISOUND A$EXP0");
    const dag::Ccb& dead = game.creatures()[static_cast<std::size_t>(slot)];
    check(!dead.in_use, "the kill clears P.CCUSE");
    check(dead.object_head >= 0, "the killed creature was carrying an object");
    if (dead.object_head >= 0) {
        const dag::Ocb& loot = game.objects()[static_cast<std::size_t>(dead.object_head)];
        check(loot.owner == 0 && loot.row == dead.row && loot.col == dead.col,
              "PATT30 leaves the creature's object unowned on its cell");
    }
    check(game.matrix_row()[type] == static_cast<std::uint8_t>(before - 1),
          "the kill decrements CMXLND");
    // CREGEN runs on the opening lap and then every five minutes.
    game.advance_jiffies(60 * 60 * 5 + 60);
    int total = 0;
    for (const std::uint8_t v : game.matrix_row()) total += v;
    game.enter_level(0);
    int live = 0, of_type = 0;
    for (const dag::Ccb& cc : game.creatures()) {
        if (!cc.in_use) continue;
        ++live;
        if (cc.type == type) ++of_type;
    }
    check(live == (total < dag::kCcbSlots ? total : dag::kCcbSlots),
          "re-entry births exactly the CMXLND row",
          "live=" + std::to_string(live) + " matrix=" + std::to_string(total));
    check(of_type == game.matrix_row()[type], "re-entry births the decremented type count");
}

void test_death_on_exact_jiffy() {
    // Unfrozen creature on the player's cell. Its first hit takes damage past
    // power (HUPD90 is BLO), so DEATH lands on the jiffy of that HIT.
    dag::Game game(1, 0);
    const int slot = first_live(game);
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
    game.advance_jiffies(600);
    std::uint64_t death = 0, hit = 0;
    for (const auto& e : game.trace()) {
        if (e.kind == "DEATH" && death == 0) death = e.jiffy;
        if (e.kind == "HIT" && e.detail.find("damage=") != std::string::npos && hit == 0)
            hit = e.jiffy;
    }
    check(game.player().dead && death > 0 && death == hit,
          "death is on the jiffy the creature's hit passes power",
          "death=" + std::to_string(death) + " hit=" + std::to_string(hit));
    check(game.player().damage > game.player().power, "death means damage above power");
    check(count(game, "TASK", death + 1) == 0, "no task runs after the death jiffy");
    const std::uint64_t frozen_at = game.counters().total_jiffies;
    game.load_script(type_at(frozen_at, "MOVE"));
    game.advance_jiffies(30);
    check(game.counters().total_jiffies == frozen_at && count(game, "LINE") == 0,
          "after DEATH's BRA * the clock and keyboard stop");
}

}  // namespace

int main() {
    test_attack_while_fainted();
    test_attack_in_darkness();
    test_empty_hand();
    test_ring_bypass();
    test_same_jiffy_attacks();
    test_kill_then_reentry();
    test_death_on_exact_jiffy();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
