// Phase 3 combat regression tests required by docs/prompts/phase-3-combat.md §4.
//
// Each case sets up a known state with the narrow test hooks (hold, wield_torch,
// place_player, set_player_damage, set_frozen) and asserts on the trace the core
// emits. Expected values follow PATTK.ASM and HUPDAT.ASM at the pinned listing.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "daggorath/combat.hpp"
#include "daggorath/game.hpp"

#include <cstdlib>

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

std::string text_row(const dag::Game& game, int row) {
    std::string out(32, ' ');
    const auto& page = game.primary_text();
    for (int col = 0; col < 32; ++col) {
        const std::uint8_t code = page[static_cast<std::size_t>(row * 32 + col)];
        if (code >= 1 && code <= 26) out[static_cast<std::size_t>(col)] = static_cast<char>('A' + code - 1);
        else if (code == 0x1B) out[static_cast<std::size_t>(col)] = '!';
        else if (code == 0x1C) out[static_cast<std::size_t>(col)] = '_';
        else if (code == 0x1E) out[static_cast<std::size_t>(col)] = '.';
    }
    return out;
}

bool page_has(const dag::Game& game, const std::string& needle) {
    for (int row = 0; row < 4; ++row)
        if (text_row(game, row).find(needle) != std::string::npos) return true;
    return false;
}

void test_abbreviated_attack_mark() {
    dag::Game dark(1, 0);
    const int slot = first_live(dark);
    stand_on(dark, slot);
    dark.hold(true, find_object(dark, kTypeWooden, true));
    dark.load_script(type_at(1, "A R"));
    dark.advance_jiffies(20);
    check(page_has(dark, "A R") && !page_has(dark, "!!!"),
          "A R in the dark is a swing with no in-line exclamation");

    dag::Game game(1, 0);
    stand_on(game, first_live(game));
    game.hold(true, find_object(game, kTypeWooden, true));
    game.wield_torch(find_object(game, kTypePine, true));
    bool marked = false;
    for (int n = 0; n < 30 && !marked; ++n) {
        const auto at = game.counters().total_jiffies + 1;
        game.load_script(type_at(at, "A R"));
        game.advance_jiffies(8);
        if (count_detail(game, "DIALOGUE", "!!!") > 0) marked = page_has(game, "A R !!!");
    }
    check(marked, "a connecting A R prints A R !!! on that line");
}

void test_hit_mark_follows_the_command() {
    dag::Game game(1, 0);
    const int slot = first_live(game);
    stand_on(game, slot);
    game.hold(false, find_object(game, kTypeWooden, true));
    game.wield_torch(find_object(game, kTypePine, true));
    game.load_script(type_at(1, "ATTACK LEFT"));
    game.advance_jiffies(20);
    const bool hit = count_detail(game, "DIALOGUE", "!!!") > 0;
    check(page_has(game, hit ? "ATTACK LEFT !!!" : "ATTACK LEFT"),
          hit ? "a hit appends !!! to the typed line" : "a miss leaves the typed line without !!!");
    if (!hit) {
        // The erased cursor is a space; the bangs are absent.
        bool bangs = false;
        for (int row = 0; row < 4; ++row)
            if (text_row(game, row).find("!!!") != std::string::npos) bangs = true;
        check(!bangs, "a miss shows no in-line exclamation");
    }
}

void test_viper_damage() {
    dag::Fighter attacker;
    attacker.power = 56;
    attacker.physical_offense = 80;
    attacker.magic_offense = 0;
    dag::Fighter defender;
    defender.power = 160;
    defender.magic_defense = 0x80;
    defender.physical_defense = 0x80;
    dag::apply_damage(attacker, defender);
    check(defender.damage == 35, "an unshielded viper hit is 35 damage",
          "damage=" + std::to_string(defender.damage));
    // Listing DAMAGE with unshielded $8080. The second SCAL16 by 128 is an
    // identity, so each channel is SCAL16(power, offense). These are the
    // source amounts; a viper's 35 against 160 power is not a multiplier bug.
    static constexpr std::uint16_t kUnshielded[] = {
        32, 35, 81, 228, 378, 704, 1592, 1593, 2400, 3984, 3984, 31874};
    for (int type = 0; type < dag::kCreatureTypes; ++type) {
        const dag::CreatureDef& def = dag::kCreatureDefs[static_cast<std::size_t>(type)];
        dag::Fighter atk;
        atk.power = def.power;
        atk.magic_offense = def.magic_offense;
        atk.physical_offense = def.physical_offense;
        dag::Fighter ply;
        ply.power = 160;
        ply.magic_defense = 0x80;
        ply.physical_defense = 0x80;
        dag::apply_damage(atk, ply);
        check(ply.damage == kUnshielded[type],
              "unshielded damage for creature " + std::to_string(type),
              "damage=" + std::to_string(ply.damage));
    }

    dag::Game game(1, 0);
    int slot = -1;
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
        if (c.in_use && c.type == 1) {
            slot = i;
            break;
        }
    }
    check(slot >= 0, "level 0 births a viper");
    if (slot < 0) return;
    game.place_player(game.creatures()[static_cast<std::size_t>(slot)].row,
                      game.creatures()[static_cast<std::size_t>(slot)].col);
    game.set_player_power(4000);
    std::vector<std::uint64_t> bites;
    std::uint64_t seen = 0;
    for (int n = 0; n < 400 && bites.size() < 4; ++n) {
        game.advance_jiffies(1);
        for (const auto& e : game.trace()) {
            if (e.jiffy < seen) continue;
            if (e.kind != "HIT" && e.kind != "MISS") continue;
            const auto mark = e.detail.find("slot=");
            if (mark == std::string::npos) continue;
            if (std::atoi(e.detail.c_str() + mark + 5) != slot) continue;
            bites.push_back(e.jiffy);
        }
        if (!game.trace().empty()) seen = game.trace().back().jiffy + 1;
    }
    check(bites.size() >= 3, "a viper sharing the cell keeps attacking");
    if (bites.size() >= 3) {
        check(bites[1] - bites[0] == 42 && bites[2] - bites[1] == 42,
              "viper attacks every 7 tenths (42 jiffies), the listing attack delay",
              "gaps=" + std::to_string(bites[1] - bites[0]) + "," +
                  std::to_string(bites[2] - bites[1]));
    }
    bool saw = false;
    for (int n = 0; n < 800 && !game.player().dead; ++n) {
        const std::uint16_t before = game.player().damage;
        const std::size_t trace_at = game.trace().size();
        game.advance_jiffies(1);
        if (game.player().damage <= before) continue;
        int hits = 0;
        int viper_slot = -1;
        for (std::size_t i = trace_at; i < game.trace().size(); ++i) {
            const auto& e = game.trace()[i];
            if (e.kind != "HIT") continue;
            ++hits;
            const auto mark = e.detail.find("slot=");
            if (mark == std::string::npos) continue;
            const int who = std::atoi(e.detail.c_str() + mark + 5);
            if (who >= 0 && game.creatures()[static_cast<std::size_t>(who)].type == 1)
                viper_slot = who;
        }
        if (hits == 1 && viper_slot >= 0 && before < 64) {
            saw = true;
            check(game.player().damage - before == 35,
                  "a live viper hit adds 35, the unshielded DAMAGE result",
                  "before=" + std::to_string(before) +
                      " after=" + std::to_string(game.player().damage));
            break;
        }
    }
    check(saw, "a viper on the player's cell connects within 800 jiffies");
}

void test_leather_shield_does_not_soften_a_viper() {
    // DTABAS: leather and bronze physical filters are 128, the same as the
    // unshielded $8080 pair. Mithril is 64. The port's ShieldFix swaps those
    // bytes; this core does not.
    dag::Fighter bite;
    bite.power = 56;
    bite.physical_offense = 80;
    dag::Fighter leather;
    leather.magic_defense = 108;
    leather.physical_defense = 128;
    dag::apply_damage(bite, leather);
    dag::Fighter mithril;
    mithril.magic_defense = 64;
    mithril.physical_defense = 64;
    dag::apply_damage(bite, mithril);
    check(leather.damage == 35 && mithril.damage == 17,
          "leather leaves a viper bite at 35; revealed mithril cuts it to 17",
          "leather=" + std::to_string(leather.damage) +
              " mithril=" + std::to_string(mithril.damage));

    dag::Game game(1, 0);
    int shield = -1;
    int viper = -1;
    for (int i = 0; i < static_cast<int>(game.objects().size()); ++i) {
        if (game.objects()[static_cast<std::size_t>(i)].type == 16) {
            shield = i;
            break;
        }
    }
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        if (game.creatures()[static_cast<std::size_t>(i)].in_use &&
            game.creatures()[static_cast<std::size_t>(i)].type == 1) {
            viper = i;
            break;
        }
    }
    check(shield >= 0 && viper >= 0, "a leather shield and a viper both exist");
    if (shield < 0 || viper < 0) return;
    game.hold(false, shield);
    const auto& snake = game.creatures()[static_cast<std::size_t>(viper)];
    game.place_player(snake.row, snake.col);
    game.set_player_power(4000);
    bool saw = false;
    for (int n = 0; n < 800 && !saw; ++n) {
        const std::uint16_t before = game.player().damage;
        const std::size_t trace_at = game.trace().size();
        game.advance_jiffies(1);
        if (game.player().damage <= before || before >= 64) continue;
        int hits = 0;
        bool from_viper = false;
        for (std::size_t i = trace_at; i < game.trace().size(); ++i) {
            const auto& e = game.trace()[i];
            if (e.kind != "HIT") continue;
            ++hits;
            const auto mark = e.detail.find("slot=");
            if (mark == std::string::npos) continue;
            const int who = std::atoi(e.detail.c_str() + mark + 5);
            if (who == viper) from_viper = true;
        }
        if (hits == 1 && from_viper) {
            saw = true;
            check(static_cast<int>(game.player().damage - before) == 35,
                  "holding the leather shield does not reduce the viper bite",
                  "added=" + std::to_string(game.player().damage - before));
        }
    }
    check(saw, "the shielded player is bitten by the viper");
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
    test_abbreviated_attack_mark();
    test_hit_mark_follows_the_command();
    test_viper_damage();
    test_leather_shield_does_not_soften_a_viper();
    test_death_on_exact_jiffy();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
