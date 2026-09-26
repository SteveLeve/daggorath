// Phase 4 object, magic, and climb regression tests.
//
// Expected values come from DTABAS.ASM (OBJXXX / SPCXXX rows), PGET.ASM,
// PREVEA.ASM, PUSE.ASM, PINCAN.ASM, PCLIMB.ASM, and HUPDAT.ASM at the pinned
// listing. State is set with the narrow test hooks and asserted on the trace.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
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

// Types (DTABAS.ASM ODBTAB order).
constexpr int kSupreme = 0, kJoule = 1, kElvish = 2, kSeer = 4, kThews = 5, kHoth = 6,
              kVision = 7, kAbye = 8, kHale = 9, kVulcan = 12, kPine = 15, kLeather = 16,
              kWooden = 17, kFinal = 18, kEnergy = 19, kIce = 20, kFire = 21, kEmpty = 23,
              kDead = 24;
constexpr int kClassRing = 1, kClassSword = 4;

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

const dag::Ocb& obj(const dag::Game& game, int index) {
    return game.objects()[static_cast<std::size_t>(index)];
}

// Run each command after the previous one has had time to finish.
void run(dag::Game& game, const std::vector<std::string>& commands) {
    std::vector<dag::KeyEvent> keys;
    std::uint64_t j = game.counters().total_jiffies;
    for (const std::string& c : commands) {
        for (const char ch : c) keys.push_back({j++, static_cast<std::uint8_t>(ch)});
        keys.push_back({j, 0x0D});
        j += 20;
    }
    game.load_script(keys);
    game.advance_jiffies(j - game.counters().total_jiffies + 5);
}

int count(const dag::Game& game, const std::string& kind, const std::string& detail = "") {
    int n = 0;
    for (const auto& e : game.trace())
        if (e.kind == kind && (detail.empty() || e.detail == detail)) ++n;
    return n;
}

dag::Game frozen_game() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    return game;
}

void test_incant_fire() {
    dag::Game game = frozen_game();
    const int ring = find_object(game, kVulcan);
    game.hold(false, ring);
    run(game, {"INCANT ICE"});
    check(obj(game, ring).type == kVulcan, "VULCAN ignores the wrong word");
    run(game, {"INCANT FIRE"});
    const dag::Ocb& o = obj(game, ring);
    check(o.type == kFire, "INCANT FIRE turns VULCAN into FIRE", std::to_string(o.type));
    // SPCXXX FIRE,RN12,K.RING,0,255,255
    check(o.cls == kClassRing && o.magic_offense == 255 && o.physical_offense == 255,
          "FIRE takes the SPCXXX row: ring, 255 magic, 255 physical",
          "cls=" + std::to_string(o.cls) + " mgo=" + std::to_string(o.magic_offense) +
              " pho=" + std::to_string(o.physical_offense));
    check(o.spec[1] == 0, "the incantation word is consumed");
    check(count(game, "INCANT") == 1 && count(game, "SOUND", "A$RING") == 1,
          "one INCANT with the ring sound");
    run(game, {"INCANT FIRE"});
    check(count(game, "INCANT") == 1, "a spent word does not incant again");
}

void test_incant_final_winner() {
    dag::Game game = frozen_game();
    const int ring = find_object(game, kSupreme);
    game.hold(true, ring);
    run(game, {"INCANT FINAL"});
    const dag::Ocb& o = obj(game, ring);
    check(o.type == kFinal && o.cls == kClassRing, "INCANT FINAL makes the FINAL ring");
    // SPCXXX FINAL,RN15,K.RING,0,0,0
    check(o.magic_offense == 0 && o.physical_offense == 0, "FINAL has zero offense");
    check(count(game, "DEFER", "winner") == 1, "the final ring defers to WINNER");
}

void test_get_drop() {
    dag::Game game = frozen_game();
    const int sword = player_object(game, kWooden);
    const std::uint16_t base = game.player().carried_weight;
    run(game, {"PULL LEFT SWORD", "DROP LEFT"});
    check(game.player().left_hand < 0 && obj(game, sword).owner == 0, "DROP puts the sword down");
    check(game.player().carried_weight == base - 25, "DROP subtracts the sword class weight",
          std::to_string(game.player().carried_weight));
    check(obj(game, sword).row == game.player().row && obj(game, sword).col == game.player().col,
          "the dropped sword lies on the player's cell");

    run(game, {"GET LEFT SHIELD"});
    check(game.player().left_hand < 0, "GET of an absent class fails");
    run(game, {"GET LEFT IRON SWORD"});
    check(game.player().left_hand < 0, "GET with the wrong adjective fails");
    run(game, {"GET LEFT WOODEN SHIELD"});
    check(game.player().left_hand < 0, "an adjective of another class is rejected");
    run(game, {"GET RIGHT WOODEN SWORD"});
    check(game.player().right_hand == sword && obj(game, sword).owner == 1,
          "adjective plus generic GET takes the sword");
    check(game.player().carried_weight == base, "GET restores the weight");
    run(game, {"GET LEFT SWORD"});
    check(game.player().left_hand < 0, "a held object is not on the floor");
    run(game, {"DROP RIGHT", "GET LEFT SWORD"});
    check(game.player().left_hand == sword, "generic GET takes the sword");
    run(game, {"GET LEFT SWORD"});
    check(count(game, "OUTPUT", "???") >= 5, "failed GETs print ???");
}

void test_reveal() {
    dag::Game game = frozen_game();
    const int leather = find_object(game, kLeather);
    const int elvish = find_object(game, kElvish);
    game.hold(false, leather);
    game.hold(true, elvish);
    check(obj(game, elvish).physical_offense != 64, "an unrevealed ELVISH has generic stats");
    run(game, {"REVEAL RIGHT"});
    // PREVEA: reveal * 25 <= PPOW. ELVISH needs 150 * 25 = 3750 > 160.
    check(obj(game, elvish).reveal == 150 && count(game, "REVEAL") == 0,
          "REVEAL below the power threshold does nothing");
    run(game, {"REVEAL LEFT"});
    // LEATHER needs 5 * 25 = 125 <= 160.
    check(obj(game, leather).reveal == 0 && count(game, "REVEAL") == 1,
          "REVEAL at or above the threshold reveals");
}

void test_use_flask_and_scroll() {
    dag::Game game = frozen_game();
    const int hale = find_object(game, kHale);
    game.hold(false, hale);
    game.set_player_damage(50);
    run(game, {"USE LEFT"});
    check(game.player().damage == 0, "HALE clears damage");
    check(obj(game, hale).type == kEmpty, "a used flask becomes EMPTY");
    run(game, {"USE LEFT"});
    check(count(game, "USE") == 1, "an EMPTY flask does nothing");

    const int thews = find_object(game, kThews);
    const int vision = find_object(game, kVision);
    const int seer = find_object(game, kSeer);
    game.hold(false, thews);
    game.hold(true, seer);
    run(game, {"USE RIGHT"});
    check(game.display_mode() != dag::DisplayMode::Mapper, "an unrevealed SEER does not map");
    run(game, {"USE LEFT"});
    check(game.player().power == 1160, "THEWS adds 1000 power",
          std::to_string(game.player().power));
    game.hold(true, vision);
    // VISION needs 50 * 25 = 1250 <= 1160? No: still too weak.
    run(game, {"REVEAL RIGHT", "USE RIGHT"});
    check(count(game, "MAP") == 0, "VISION past the reveal threshold stays unusable");
    const int thews2 = [&] {
        const auto& objects = game.objects();
        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
            if (objects[static_cast<std::size_t>(i)].type == kThews && i != thews) return i;
        return -1;
    }();
    check(thews2 >= 0, "a second THEWS flask exists");
    game.hold(false, thews2);
    run(game, {"USE LEFT", "REVEAL RIGHT", "USE RIGHT"});
    check(game.player().power == 2160, "the second THEWS adds 1000 more");
    check(count(game, "MAP", "features=0") == 1 && game.display_mode() == dag::DisplayMode::Mapper,
          "a revealed VISION scroll maps without features");
}

void test_climb() {
    dag::Game game = frozen_game();
    game.place_player(15, 4);   // level-0 hole (VFTTAB feature 2)
    run(game, {"CLIMB UP"});
    check(game.level_index() == 0 && count(game, "OUTPUT", "???") == 1,
          "a hole cannot be climbed upward");
    game.place_player(0, 23);   // level-0 ladder down (feature 3)
    run(game, {"CLIMB DOWN"});
    check(game.level_index() == 1 && count(game, "CLIMB", "level=1") == 1,
          "CLIMB DOWN on the ladder enters level 1");
    check(game.player().row == 0 && game.player().col == 23, "the player keeps the cell");
    // Level 1 cell 0,23 is the ladder top (feature 1).
    run(game, {"CLIMB UP"});
    check(game.level_index() == 0 && count(game, "CLIMB", "level=0") == 1,
          "CLIMB UP returns to level 0");
    game.place_player(16, 11);
    run(game, {"CLIMB DOWN"});
    check(game.level_index() == 0, "no feature, no climb");
}

void test_death_stops_buffered_command() {
    // FIRE: 255 + 255 -> (510 >> 3) = 63 -> SCAL16(160, 63) = 78 exertion.
    // Damage 100 is heart rate 10; 100 + 78 = 178 > 160 without a faint first.
    dag::Game game = frozen_game();
    const int ring = find_object(game, kVulcan);
    game.hold(false, ring);
    run(game, {"INCANT FIRE"});
    check(obj(game, ring).type == kFire, "the ring is FIRE before the burst");
    const std::uint64_t at = game.counters().total_jiffies;
    game.set_player_damage(100);
    check(!game.player().fainted, "damage 100 leaves the player conscious");
    const int row = game.player().row;
    std::vector<dag::KeyEvent> keys;
    for (const char ch : std::string("ATTACK LEFT\rMOVE\r"))
        keys.push_back({at, static_cast<std::uint8_t>(ch == '\r' ? 0x0D : ch)});
    game.load_script(keys);
    game.advance_jiffies(60);
    std::uint64_t death = 0;
    for (const auto& e : game.trace())
        if (e.kind == "DEATH") death = e.jiffy;
    check(game.player().dead && death == at + 1, "ATTACK's exertion kills on the next jiffy",
          "death=" + std::to_string(death) + " at=" + std::to_string(at) +
              " damage=" + std::to_string(game.player().damage));
    check(count(game, "MOVE") == 0 && game.player().row == row,
          "DEATH's BRA * stops the MOVE buffered behind ATTACK (PATTK has no SYNC)");
}

std::string read_fixture(const std::string& name) {
    std::ifstream in(std::string(DAG_FIXTURE_DIR) + "/" + name);
    std::stringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

long json_number(const std::string& text, const std::string& key) {
    const auto at = text.find("\"" + key + "\"");
    if (at == std::string::npos) return -1;
    return std::strtol(text.c_str() + text.find(':', at) + 1, nullptr, 10);
}

void test_starting_inventory() {
    const std::string state = read_fixture("initial-state.json");
    check(!state.empty(), "initial-state.json is readable");
    dag::Game game(1, 0);
    const auto& p = game.player();
    check(p.row == json_number(state, "player_row") && p.col == json_number(state, "player_col"),
          "starting cell matches initial-state.json");
    check(static_cast<long>(p.dir) == json_number(state, "player_dir"), "starting facing");
    check(p.power == json_number(state, "ppow_initial"), "PPOW matches the fixture");
    check(p.damage == json_number(state, "pdam_initial"), "PDAM matches the fixture");
    check(p.carried_weight == json_number(state, "pobjwt_initial"), "POBJWT matches the fixture");
    check(p.heart_rate == json_number(state, "heartr_at_start"), "HEARTR matches the fixture");
    // initial_bag: WOODEN sword then PINE torch, both in the bag, hands empty.
    std::vector<int> bag;
    for (int i = p.bag_head; i >= 0; i = obj(game, i).next) bag.push_back(obj(game, i).type);
    check(state.find("WOODEN sword") != std::string::npos &&
              state.find("PINE torch") != std::string::npos,
          "the fixture lists the wooden sword and pine torch");
    check(bag.size() == 2 && std::count(bag.begin(), bag.end(), kWooden) == 1 &&
              std::count(bag.begin(), bag.end(), kPine) == 1,
          "the bag holds exactly WOODEN and PINE");
    check(p.left_hand < 0 && p.right_hand < 0 && p.torch < 0, "hands and PTORCH start empty");
}

void test_verb_coverage() {
    const std::string tokens = read_fixture("tokens.json");
    const auto begin = tokens.find("\"CMDTAB\"");
    const auto end = tokens.find("\"DIRTAB\"");
    check(begin != std::string::npos && end != std::string::npos, "tokens.json has CMDTAB");
    int verbs = 0;
    for (auto at = tokens.find("\"word\"", begin); at != std::string::npos && at < end;
         at = tokens.find("\"word\"", at + 1)) {
        const auto q1 = tokens.find('"', tokens.find(':', at) + 1);
        const std::string word = tokens.substr(q1 + 1, tokens.find('"', q1 + 1) - q1 - 1);
        ++verbs;
        dag::Game game = frozen_game();
        run(game, {word});
        bool unimplemented = count(game, "UNIMPLEMENTED") > 0;
        bool reached = false;
        for (const auto& e : game.trace())
            if (e.kind == "LINE" && e.detail == "\"" + word + "\"") reached = true;
        check(reached, word + " is dispatched");
        const bool allowed = word == "ZSAVE" || word == "ZLOAD";
        check(unimplemented == allowed,
              word + (allowed ? " still reports UNIMPLEMENTED" : " reaches its handler"));
    }
    check(verbs == 15, "CMDTAB has 15 verbs", std::to_string(verbs));
}

void test_torch_lifecycle() {
    // PINE: timer 15, regular light 7, magic light 0 (DTABAS.ASM OBJXXX).
    dag::Game game = frozen_game();
    const int pine = player_object(game, kPine);
    run(game, {"PULL LEFT TORCH", "USE LEFT"});
    check(game.player().torch == pine && game.player().regular_light == 7,
          "USE lights the pine torch at light 7");
    game.advance_jiffies(60 * 60 * 11);
    std::vector<std::uint64_t> at;
    std::vector<std::string> detail;
    for (const auto& e : game.trace()) {
        if (e.kind != "TORCH") continue;
        at.push_back(e.jiffy);
        detail.push_back(e.detail);
    }
    bool minute_apart = at.size() >= 2;
    for (std::size_t i = 1; i < at.size(); ++i)
        if (detail[i].rfind("timer=", 0) == 0 && detail[i - 1].rfind("timer=", 0) == 0 &&
            at[i] - at[i - 1] != 3600)
            minute_apart = false;
    check(minute_apart, "BURNER ticks once per minute boundary");
    bool clamp = false, dead = false;
    for (const std::string& d : detail) {
        if (d == "timer=6 light=6") clamp = true;
        if (d.rfind("dead timer=5", 0) == 0) dead = true;
    }
    check(clamp, "light is clamped to the timer below 7");
    check(dead && obj(game, pine).type == kDead, "timer 5 turns the torch DEAD (T.TOR5)");
    game.advance_jiffies(60 * 60 * 5);
    check(obj(game, pine).spec[0] == 0, "the timer runs down to 0");
    const auto torch_events = [&] {
        std::size_t n = 0;
        for (const auto& e : game.trace())
            if (e.kind == "TORCH") ++n;
        return n;
    };
    const std::size_t spent = torch_events();
    game.advance_jiffies(60 * 60 * 3);
    check(torch_events() == spent, "BURNER stops changing a torch at timer 0");
    check(game.player().regular_light == 0, "a spent torch gives no light");
}

void test_examine() {
    dag::Game game = frozen_game();
    run(game, {"EXAMINE"});
    check(game.display_mode() == dag::DisplayMode::Examine && count(game, "EXAMINE", "creature=-1") == 1,
          "EXAMINE on an empty cell selects the examine display");
    int slot = -1;
    for (int i = 0; i < dag::kCcbSlots && slot < 0; ++i)
        if (game.creatures()[static_cast<std::size_t>(i)].in_use) slot = i;
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
    run(game, {"LOOK", "EXAMINE"});
    check(count(game, "EXAMINE", "creature=" + std::to_string(slot)) == 1,
          "EXAMINE names the creature on the cell");
}

void test_each_flask() {
    {
        dag::Game game = frozen_game();
        game.hold(false, find_object(game, kThews));
        run(game, {"USE LEFT"});
        check(game.player().power == 1160, "THEWS adds 1000 power");
    }
    {
        dag::Game game = frozen_game();
        game.hold(false, find_object(game, kHale));
        game.set_player_damage(90);
        run(game, {"USE LEFT"});
        check(game.player().damage == 0, "HALE zeroes damage");
    }
    {
        // PUSE ABYE: PDAM += SCAL16(PPOW, 102) = 160 * 102 / 128 = 127.
        dag::Game game = frozen_game();
        const int abye = find_object(game, kAbye);
        game.hold(false, abye);
        run(game, {"USE LEFT"});
        check(game.player().damage == 127, "ABYE adds SCAL16(power, 102) damage",
              std::to_string(game.player().damage));
        check(obj(game, abye).type == kEmpty, "ABYE is emptied");
    }
}

void test_each_scroll() {
    for (const int type : {kVision, kSeer}) {
        const bool seer = type == kSeer;
        dag::Game game = frozen_game();
        const int scroll = find_object(game, type);
        game.hold(false, scroll);
        run(game, {"USE LEFT"});
        check(count(game, "MAP") == 0, std::string(seer ? "SEER" : "VISION") +
                                           " does nothing unrevealed");
        game.set_player_power(4000);
        run(game, {"REVEAL LEFT", "USE LEFT"});
        check(count(game, "MAP", seer ? "features=1" : "features=0") == 1 &&
                  game.display_mode() == dag::DisplayMode::Mapper,
              std::string(seer ? "SEER maps with features" : "VISION maps without features") +
                  " once revealed");
    }
}

void test_each_ring_word() {
    struct Ring { int type; const char* word; int becomes; };
    const Ring rings[] = {{kSupreme, "FINAL", kFinal}, {kJoule, "ENERGY", kEnergy},
                          {kHoth, "ICE", kIce}, {kVulcan, "FIRE", kFire}};
    for (const Ring& r : rings) {
        dag::Game held = frozen_game();
        const int index = find_object(held, r.type);
        held.hold(true, index);
        run(held, {std::string("INCANT ") + r.word});
        check(obj(held, index).type == r.becomes,
              std::string(r.word) + " transforms the held ring");
        dag::Game loose = frozen_game();
        run(loose, {std::string("INCANT ") + r.word});
        check(obj(loose, find_object(loose, r.type)).type == r.type &&
                  count(loose, "INCANT") == 0,
              std::string(r.word) + " without the ring in hand does nothing");
    }
}

void test_climb_every_feature() {
    struct Case { int level, row, col; const char* dir; int expect; };
    // VFTTAB: 3 = ladder down, 2 = hole down, 1 = ladder up, 0 = hole arrival.
    const Case cases[] = {
        {0, 0, 23, "DOWN", 1},  {0, 0, 23, "UP", 0},    // feature 3
        {0, 15, 4, "DOWN", 1},  {0, 15, 4, "UP", 0},    // feature 2
        {1, 0, 23, "UP", 0},    {1, 0, 23, "DOWN", 1},  // feature 1
        {1, 15, 4, "UP", 1},    {1, 15, 4, "DOWN", 1},  // feature 0
        {0, 16, 11, "UP", 0},   {0, 16, 11, "DOWN", 0}, // no feature
    };
    for (const Case& c : cases) {
        dag::Game game = frozen_game();
        if (c.level != 0) game.enter_level(c.level);
        game.place_player(c.row, c.col);
        run(game, {std::string("CLIMB ") + c.dir});
        check(game.level_index() == c.expect,
              "CLIMB " + std::string(c.dir) + " at level " + std::to_string(c.level) + " " +
                  std::to_string(c.row) + "," + std::to_string(c.col),
              "level=" + std::to_string(game.level_index()));
    }
    dag::Game bare = frozen_game();
    bare.place_player(0, 23);
    run(bare, {"CLIMB"});
    check(bare.level_index() == 0 && count(bare, "OUTPUT", "???") == 1,
          "bare CLIMB on a ladder is rejected");
}

void test_burden() {
    // POBJWT drives PMOV90: PDAM += POBJWT / 8 + 3.
    dag::Game game = frozen_game();
    run(game, {"MOVE"});
    std::uint16_t first = game.player().damage;
    check(first == 35 / 8 + 3, "MOVE at the starting burden costs 7", std::to_string(first));
    const int shield = find_object(game, kLeather);
    game.hold(false, -1);
    run(game, {"PULL LEFT SWORD", "DROP LEFT"});
    check(game.player().carried_weight == 10, "dropping the sword leaves 10");
    game.set_player_damage(0);
    run(game, {"MOVE"});
    check(game.player().damage == 10 / 8 + 3, "a lighter burden costs 4");
    (void)shield;
}

}  // namespace

int main() {
    test_starting_inventory();
    test_verb_coverage();
    test_torch_lifecycle();
    test_examine();
    test_each_flask();
    test_each_scroll();
    test_each_ring_word();
    test_climb_every_feature();
    test_burden();
    test_incant_fire();
    test_incant_final_winner();
    test_get_drop();
    test_reveal();
    test_use_flask_and_scroll();
    test_climb();
    test_death_stops_buffered_command();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
