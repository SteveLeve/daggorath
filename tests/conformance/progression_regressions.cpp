// Phase 5 endings, save/load, and suspend-snapshot regression tests.
//
// Endings follow PATTK.ASM (ENDGAM and the ring riddle), PINCAN.ASM (WINNER),
// and HUPDAT.ASM (DEATH). Dialogue strings are the OUTSTI bytes decoded by
// tools/decode_outsti.py. Save/load follows PZTAPE.ASM and COMMON.ASM SAVE,
// LOAD, and LOAD90.
#include <algorithm>
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

constexpr int kSupreme = 0, kVulcan = 12, kPine = 15, kFire = 21;

int find_object(const dag::Game& game, int type) {
    const auto& objects = game.objects();
    for (int i = 0; i < static_cast<int>(objects.size()); ++i)
        if (objects[static_cast<std::size_t>(i)].type == type) return i;
    return -1;
}

int player_object(const dag::Game& game, int type) {
    const auto& objects = game.objects();
    for (int i = 0; i < static_cast<int>(objects.size()); ++i)
        if (objects[static_cast<std::size_t>(i)].type == type &&
            objects[static_cast<std::size_t>(i)].owner == 1)
            return i;
    return -1;
}

std::vector<dag::KeyEvent> keys_for(std::uint64_t start, const std::vector<std::string>& cmds,
                                    std::uint64_t gap = 20) {
    std::vector<dag::KeyEvent> keys;
    std::uint64_t j = start;
    for (const std::string& c : cmds) {
        for (const char ch : c) keys.push_back({j++, static_cast<std::uint8_t>(ch)});
        keys.push_back({j, 0x0D});
        j += gap;
    }
    return keys;
}

void run(dag::Game& game, const std::vector<std::string>& cmds) {
    const std::uint64_t start = game.counters().total_jiffies;
    game.load_script(keys_for(start, cmds));
    std::uint64_t len = 0;
    for (const std::string& c : cmds) len += c.size() + 20;
    game.advance_jiffies(len + 5);
}

std::vector<std::string> dialogue(const dag::Game& game) {
    std::vector<std::string> out;
    for (const auto& e : game.trace())
        if (e.kind == "DIALOGUE") out.push_back(e.detail);
    return out;
}

bool has(const dag::Game& game, const std::string& kind, const std::string& detail = "") {
    for (const auto& e : game.trace())
        if (e.kind == kind && (detail.empty() || e.detail == detail)) return true;
    return false;
}

int slot_of_type(const dag::Game& game, int type) {
    for (int i = 0; i < dag::kCcbSlots; ++i) {
        const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(i)];
        if (c.in_use && c.type == type) return i;
    }
    return -1;
}

// Kill the creature of `type` on the current level with an incanted FIRE ring.
// PPOW 8000 is a test value: it makes each hit large and each swing survivable.
bool kill_type(dag::Game& game, int type, int& ring) {
    const int slot = slot_of_type(game, type);
    if (slot < 0) return false;
    ring = find_object(game, kVulcan);
    game.hold(false, ring);
    run(game, {"INCANT FIRE"});
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
    game.set_player_power(8000);
    for (int n = 0; n < 20 && !has(game, "KILL"); ++n) {
        game.set_player_damage(0);
        run(game, {"ATTACK LEFT"});
    }
    return has(game, "KILL");
}

void test_image_ending() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    game.enter_level(2);
    const int torch = player_object(game, kPine);
    run(game, {"PULL RIGHT TORCH", "USE RIGHT"});
    check(game.player().torch == torch, "the pine torch is lit");
    int ring = -1;
    check(kill_type(game, 10, ring), "the wizard's image (type 10) dies");
    check(has(game, "ENDGAM", "image"), "killing type 10 runs ENDGAM");
    const auto lines = dialogue(game);
    check(lines.size() >= 2 && lines[lines.size() - 2] == "^ ENOUGH! I TIRE OF THIS PLAY..." &&
              lines.back() == "   PREPARE TO MEET THY DOOM!!!",
          "ENDGAM prints PATTK.ASM's two OUTSTI strings");
    bool hits_marked = !lines.empty();
    for (std::size_t i = 0; i + 2 < lines.size(); ++i)
        if (lines[i] != "!!!") hits_marked = false;
    check(hits_marked, "each connecting swing prints OUTSTI !!! before ENDGAM");
    check(game.level_index() == 3, "ENDGAM rebuilds level 3");
    check(game.player().carried_weight == 200, "ENDGAM sets POBJWT to 200");
    check(game.player().bag_head == torch &&
              game.objects()[static_cast<std::size_t>(torch)].next == -1,
          "the torch in PTORCH is the only bag object");
    check(game.player().left_hand == ring && game.player().torch == torch,
          "ENDGAM keeps PLHAND and PTORCH");
    check(game.maze().at(game.player().row, game.player().col) != 0xFF,
          "FNDCEL relocates onto an open cell");
}

void test_wizard_ending() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    game.enter_level(4);
    int ring = -1;
    check(kill_type(game, 11, ring), "the wizard (type 11) dies");
    check(has(game, "ENDGAM", "wizard"), "killing type 11 runs the ring riddle");
    check(game.frozen(), "DEC FRZFLG freezes the creatures");
    check(game.player().regular_light == 0x07 && game.player().magic_light == 0x13,
          "PRLITE / PMLITE become $07 / $13");
    check(game.player().bag_head < 0 && game.player().torch < 0 &&
              game.player().left_hand < 0 && game.player().right_hand < 0,
          "bag, torch, and both hands are cleared");
    check(!game.player().dead && !game.player().won, "the riddle does not end the game");
}

void test_winner() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    game.hold(true, find_object(game, kSupreme));
    run(game, {"INCANT FINAL"});
    check(has(game, "WINNER") && game.player().won, "INCANT FINAL runs WINNER");
    const auto lines = dialogue(game);
    check(lines.size() == 2 && lines[0] == "^BEHOLD! DESTINY AWAITS THE HAND" &&
              lines[1] == "        OF A NEW WIZARD...",
          "WINNER prints PINCAN.ASM's two OUTSTI strings");
    const std::uint64_t at = game.counters().total_jiffies;
    game.advance_jiffies(100);
    check(game.counters().total_jiffies == at, "WINNER ends in BRA *");
}

void test_death_load_resumes() {
    dag::Game game(1, 0);
    game.load_script(keys_for(10, {"ZSAVE QUEST"}));
    game.advance_jiffies(80);
    const std::string* saved = game.cassette_image("QUEST");
    check(saved != nullptr && saved->rfind("DAGRAM 1", 0) == 0,
          "ZSAVE keeps a named cassette image");
    if (saved == nullptr) return;
    const std::string image = *saved;
    game.set_player_damage(static_cast<std::uint16_t>(game.player().power + 1));
    game.advance_jiffies(2);
    check(game.player().dead, "damage past power is death");
    const auto frozen = game.counters().total_jiffies;
    game.advance_jiffies(30);
    check(game.counters().total_jiffies == frozen, "DEATH's BRA * takes no further interrupts");
    game.restore_ram_image(image);
    check(!game.player().dead, "the cassette image is the living game");
    game.advance_jiffies(30);
    check(game.counters().total_jiffies == frozen + 30,
          "restoring a living image returns to SCHED");
}

void test_death_line() {
    dag::Game game(1, 0);
    const int slot = slot_of_type(game, 3);
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
    game.advance_jiffies(1200);
    check(game.player().dead, "a type-3 creature kills the idle player");
    const auto lines = dialogue(game);
    check(lines.size() == 1 && lines[0] == "^ YET ANOTHER DOES NOT RETURN...",
          "DEATH prints HUPDAT.ASM's OUTSTI string", lines.empty() ? "" : lines[0]);
}

// Trace lines from index `from`, without the harness jiffy column.
std::vector<std::string> events_from(const dag::Game& game, std::size_t from) {
    std::vector<std::string> out;
    for (std::size_t i = from; i < game.trace().size(); ++i) {
        const auto& e = game.trace()[i];
        out.push_back(e.counters + "\t" + e.kind + "\t" + e.detail);
    }
    return out;
}

std::size_t index_after(const dag::Game& game, const std::string& kind) {
    for (std::size_t i = 0; i < game.trace().size(); ++i)
        if (game.trace()[i].kind == kind) return i + 1;
    return game.trace().size();
}

void test_save_load_resumes_at_the_save() {
    // Unfrozen level 0 so creatures, CREGEN, HSLOW, and the RNG all move on.
    dag::Game straight;
    straight.load_script(keys_for(10, {"TURN RIGHT", "ZSAVE QUEST"}));
    straight.advance_jiffies(80);
    const std::size_t after_save = index_after(straight, "ZSAVE");
    straight.advance_jiffies(900);

    dag::Game detour;
    detour.load_script(keys_for(10, {"TURN RIGHT", "ZSAVE QUEST", "MOVE", "TURN LEFT", "MOVE"}));
    detour.advance_jiffies(400);
    detour.load_script(keys_for(detour.counters().total_jiffies, {"ZLOAD QUEST"}));
    detour.advance_jiffies(40);
    check(has(detour, "ZLOAD", "QUEST"), "ZLOAD finds the saved name");
    const std::size_t after_load = index_after(detour, "ZLOAD");
    detour.advance_jiffies(900);
    const auto a = events_from(straight, after_save);
    const auto b = events_from(detour, after_load);
    const std::size_t n = std::min(a.size(), b.size());
    bool same = n > 100;
    std::size_t diverge = 0;
    for (std::size_t i = 0; i < n && same; ++i)
        if (a[i] != b[i]) { same = false; diverge = i; }
    check(same, "after ZLOAD the game continues exactly as it did after ZSAVE",
          "n=" + std::to_string(n) + " diverge=" + std::to_string(diverge) +
              (same || diverge >= n ? "" : " a=" + a[diverge] + " b=" + b[diverge]));
    check(detour.player().dir == dag::Dir::East, "ZLOAD restores the saved facing");

    dag::Game missing(1, 0);
    run(missing, {"ZLOAD NOPE"});
    check(has(missing, "OUTPUT", "???") && !has(missing, "ZLOAD"),
          "ZLOAD of an absent name reports ??? (D-11)");
}

void test_ram_image_is_the_whole_state() {
    dag::Game game;
    game.load_script(keys_for(10, {"PULL LEFT TORCH", "USE LEFT", "MOVE"}));
    game.advance_jiffies(700);
    const std::string image = game.ram_image();
    dag::Game other(9, 3);   // different clock, level, and creatures
    other.restore_ram_image(image);
    check(other.ram_image() == image, "a RAM image restores byte for byte into another game");
    check(other.level_index() == game.level_index() &&
              other.creatures()[0].row == game.creatures()[0].row &&
              other.objects().size() == game.objects().size(),
          "level, creatures, and objects come from the image");
}

void test_snapshot_round_trip_and_replay() {
    for (const int variant : {0, 1}) {
        dag::Game a;
        a.load_script(keys_for(10, {"PULL LEFT TORCH", "USE LEFT", "TURN RIGHT", "MOVE",
                                    "ZSAVE ONE", "MOVE"}));
        a.advance_jiffies(500);
        const std::string snap = a.snapshot();
        dag::Game b = variant == 0 ? dag::Game() : dag::Game(40, 2);
        if (variant == 1) {
            b.load_script(keys_for(1, {"MOVE", "TURN LEFT"}));
            b.advance_jiffies(300);
        }
        b.restore_snapshot(snap);
        check(b.snapshot() == snap, "snapshot round trip is byte-identical",
              variant == 0 ? "fresh target" : "diverged target");
        const std::size_t ta = a.trace().size(), tb = b.trace().size();
        const std::vector<std::string> more = {"TURN AROUND", "MOVE", "MOVE", "ATTACK LEFT",
                                               "ZLOAD ONE", "MOVE"};
        a.load_script(keys_for(a.counters().total_jiffies + 1, more));
        b.load_script(keys_for(b.counters().total_jiffies + 1, more));
        a.advance_jiffies(4000);
        b.advance_jiffies(4000);
        bool same = a.trace().size() - ta == b.trace().size() - tb;
        for (std::size_t i = 0; same && i < a.trace().size() - ta; ++i)
            same = a.trace()[ta + i].to_line() == b.trace()[tb + i].to_line();
        check(same, "replay after restore matches the original run line for line",
              variant == 0 ? "fresh target" : "diverged target");
        check(a.snapshot() == b.snapshot(), "final snapshots match");
    }
}

void test_fudge_harness_is_not_source_behaviour() {
    dag::Game fresh;
    check(fresh.incoming_damage_percent() == 100, "default Game is Original Mode incoming (100)");

    std::string err;
    const std::string body = "0 A\nFUDGE incoming 25\n10 FUDGE rest\n10 B\n";
    const auto keys = dag::parse_script(body, err);
    check(err.empty() && keys.size() == 2, "parse_script ignores FUDGE lines",
          "n=" + std::to_string(keys.size()) + " err=" + err);
    const auto ev = dag::parse_harness(body, err);
    check(err.empty() && ev.size() == 2, "parse_harness collects FUDGE lines",
          "n=" + std::to_string(ev.size()) + " err=" + err);

    dag::Game rest;
    rest.set_player_damage(200);
    rest.load_harness(dag::parse_harness("0 FUDGE rest\n", err));
    rest.advance_jiffies(1);
    check(rest.player().damage == 63, "FUDGE rest writes the HSLOW floor",
          std::to_string(rest.player().damage));

    dag::Game a;
    int sl = -1;
    for (int i = 0; i < dag::kCcbSlots; ++i)
        if (a.creatures()[static_cast<std::size_t>(i)].in_use) {
            sl = i;
            break;
        }
    check(sl >= 0, "Original Mode births at least one creature");
    if (sl < 0) return;
    const dag::Ccb& c = a.creatures()[static_cast<std::size_t>(sl)];
    a.place_player(c.row, c.col);
    const std::string snap = a.snapshot();
    const std::uint16_t before = a.player().damage;
    a.advance_jiffies(400);
    const unsigned full =
        static_cast<unsigned>(a.player().damage) + (a.player().damage < before ? 65536u : 0u) -
        before;
    dag::Game b;
    b.restore_snapshot(snap);
    check(b.incoming_damage_percent() == 100, "snapshot default incoming stays 100");
    b.set_incoming_damage_percent(25);
    const std::uint16_t b0 = b.player().damage;
    b.advance_jiffies(400);
    const unsigned quarter =
        static_cast<unsigned>(b.player().damage) + (b.player().damage < b0 ? 65536u : 0u) - b0;
    check(full > 0, "a creature hit the player at 100%", "added=" + std::to_string(full));
    check(quarter == full * 25u / 100u, "FUDGE incoming 25 scales creature-to-player damage",
          "full=" + std::to_string(full) + " quarter=" + std::to_string(quarter));
    check(fresh.incoming_damage_percent() == 100, "another Game() is still canonical 100");
}

}  // namespace

int main() {
    test_image_ending();
    test_wizard_ending();
    test_winner();
    test_death_line();
    test_death_load_resumes();
    test_save_load_resumes_at_the_save();
    test_ram_image_is_the_whole_state();
    test_snapshot_round_trip_and_replay();
    test_fudge_harness_is_not_source_behaviour();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
