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
// PPOW 30000 is a test value: the ring has three charges (VULCAN's P.OCXXX
// survives OCBFIL, OBIRTH.ASM OFIL10), and 30000 makes three hits reach the
// 8000-power wizard. Each swing's damage is reset so the player survives.
bool kill_type(dag::Game& game, int type, int& ring) {
    const int slot = slot_of_type(game, type);
    if (slot < 0) return false;
    ring = find_object(game, kVulcan);
    game.hold(false, ring);
    run(game, {"INCANT FIRE"});
    const dag::Ccb& c = game.creatures()[static_cast<std::size_t>(slot)];
    game.place_player(c.row, c.col);
    game.set_player_power(30000);
    for (int n = 0; n < 20 && !has(game, "KILL"); ++n) {
        game.set_player_damage(0);
        run(game, {"ATTACK LEFT"});
    }
    return has(game, "KILL");
}

void test_spent_ring_becomes_a_plain_gold_ring() {
    // PATTK.ASM PATT10: the last charge turns the ring into T.RN20 through
    // PREV00 (PREVEA.ASM), so OCBFIL gives it the gold ring's own ODBTAB entry.
    dag::Game game(1, 0);
    game.set_frozen(true);
    const int ring = find_object(game, kVulcan);
    game.hold(false, ring);
    run(game, {"INCANT FIRE"});
    const dag::Ocb& o = game.objects()[static_cast<std::size_t>(ring)];
    check(o.type == 21 && o.magic_offense == 255, "INCANT FIRE gives the fire ring 255 offense");
    for (int n = 0; n < 3; ++n) run(game, {"ATTACK LEFT"});
    check(has(game, "RING", "spent"), "the fire ring's charges run out");
    check(o.type == 22 && o.cls == 1, "the spent ring is a gold ring (T.RN20), still class ring");
    check(o.magic_offense == 0 && o.physical_offense == 5,
          "OCBFIL gives the gold ring its own offense 0/5, not 255/255");
    check(o.reveal == 0, "PREV00 clears P.OCREV");
}

void test_luknew_pupdat_costs_a_sync() {
    // COMPLR.ASM LUKNEW calls PUPDAT once CWALK has set NEWLUK. PUPDAX's SYNC
    // costs the next jiffy's pass (D-15, inferred). Level 0 unfrozen: vipers
    // walk near the start cell within 200 jiffies (phase-0b t1 at jiffy 91).
    dag::Game game(1, 0);
    game.advance_jiffies(200);
    const auto& tr = game.trace();
    int charged = 0;
    bool each_followed = true;
    for (std::size_t i = 0; i < tr.size(); ++i) {
        if (tr[i].kind != "PUPDAT" || tr[i].detail != "luknew") continue;
        ++charged;
        bool sync = false;
        for (std::size_t k = i + 1; k < tr.size() && tr[k].jiffy <= tr[i].jiffy + 1; ++k)
            if (tr[k].kind == "SYNC" && tr[k].jiffy == tr[i].jiffy + 1) sync = true;
        if (!sync) each_followed = false;
    }
    check(charged > 0 && has(game, "LOOK"), "a nearby creature step makes LUKNEW call PUPDAT");
    check(each_followed, "each LUKNEW PUPDAT gives up the next jiffy to SYNC");
}

// ONCE.ASM GAME30: OBIRTH without loading B, so the starting objects keep
// GAME10's $0B (LDD #$100B) as P.OCLVL; COMSWI.ASM's SWI frame restores B.
void test_starting_bag_level_byte() {
    dag::Game game(1, 0);
    int n = 0;
    bool all = true;
    for (int i = game.player().bag_head; i >= 0; i = game.objects()[static_cast<std::size_t>(i)].next) {
        ++n;
        if (game.objects()[static_cast<std::size_t>(i)].level != 0x0B) all = false;
    }
    check(n == 2 && all, "the starting sword and torch carry P.OCLVL $0B");
    dag::Game deeper(1, 2);
    const auto& first = deeper.objects()[static_cast<std::size_t>(deeper.player().bag_head)];
    check(first.level == 0x0B, "the $0B does not depend on the starting level");
}

// COMPLR.ASM:43 BURN99: BURNER ends in DEC NEWLUK on every run, torch or not,
// so LUKNEW redraws even with the creatures frozen and no torch lit.
void test_burner_requests_redraw() {
    dag::Game game(1, 0);
    game.set_frozen(true);
    game.advance_jiffies(60);
    bool at_first_luknew = false;
    for (const auto& e : game.trace())
        if (e.kind == "PUPDAT" && e.detail == "luknew" && e.jiffy == 19) at_first_luknew = true;
    check(at_first_luknew,
          "the opening BURNER's NEWLUK makes the first LUKNEW (jiffy 19) redraw");
}

// HUPDAT.ASM:130-132 and :168: death is checked after a faint in the same
// HUPDAT, and DEATH does CLR FAINT.
void test_death_clears_faint() {
    dag::Game game(1, 0);
    game.set_player_damage(156);   // PPOW 160: heart rate 3, a faint and not a death
    for (int j = 0; j < 400 && !game.player().fainted; ++j) game.advance_jiffies(1);
    check(game.player().fainted, "near-fatal damage faints");
    game.set_player_damage(static_cast<std::uint16_t>(game.player().power + 1));
    game.advance_jiffies(200);
    check(game.player().dead && !game.player().fainted, "DEATH clears FAINT");
}

void test_no_pupdat_while_fainted() {
    // PUPDAT.ASM PUPDAX: TST FAINT / BNE PUPD99 - no redraw and no SYNC.
    dag::Game game(1, 0);
    game.set_player_damage(156);   // PPOW 160: heart rate 3, a faint and not a death
    game.advance_jiffies(400);
    bool fainted = false, revived = false;
    std::uint64_t faint = 0, revive = 0;
    for (const auto& e : game.trace()) {
        if (e.kind == "FAINT" && !fainted) { fainted = true; faint = e.jiffy; }
        if (e.kind == "REVIVE" && fainted && !revived) { revived = true; revive = e.jiffy; }
    }
    check(fainted && revived && revive > faint, "damage 156 of 160 faints, then HSLOW revives");
    bool charged = false;
    for (const auto& e : game.trace())
        if (e.jiffy > faint && e.jiffy < revive && (e.kind == "SYNC" || e.kind == "PUPDAT"))
            charged = true;
    check(!charged, "no PUPDAT or SYNC is charged while fainted");
}

void test_blocked_move_still_reports_its_half_step() {
    // PTURN.ASM PMOVE: HLFSTP/BAKSTP PUPDAT runs before PSTEP, so a step into
    // a wall still has a forward (or back) half-step to draw, then THUD.
    bool found = false;
    for (int turns = 0; turns < 4 && !found; ++turns) {
        dag::Game game(1, 0);
        game.set_frozen(true);
        std::vector<std::string> cmds(static_cast<std::size_t>(turns), "TURN RIGHT");
        cmds.push_back("MOVE");
        run(game, cmds);
        if (!has(game, "SOUND", "A$THUD")) continue;
        found = true;
        int step = -99;
        for (const auto& e : game.events())
            if (e.kind == dag::CoreEventKind::Block && e.block == dag::BlockKind::MoveAnimation)
                step = e.step_relative;
        check(step == 0, "a blocked forward MOVE carries step=0 on its MoveAnimation block");
    }
    check(found, "some facing at the level-0 start has a wall ahead");
}

void test_pull_costs_a_sync() {
    // PGET.ASM PPULL ends in COMUPD: STATUS then PUPDAT, whose SYNC costs the
    // next jiffy's pass (D-15).
    dag::Game game(1, 0);
    game.set_frozen(true);
    run(game, {"PULL RIGHT TORCH"});
    bool pupdat = false, sync_next = false;
    std::uint64_t at = 0;
    for (const auto& e : game.trace()) {
        if (e.kind == "PUPDAT" && e.detail == "comupd" && !pupdat) { pupdat = true; at = e.jiffy; }
        if (pupdat && e.kind == "SYNC" && e.jiffy == at + 1) sync_next = true;
    }
    check(has(game, "PULL") && pupdat && sync_next, "PULL's COMUPD redraw costs a SYNC jiffy");
}

int count_pupdat(const dag::Game& game, std::size_t from, const std::string& why) {
    int n = 0;
    for (std::size_t i = from; i < game.trace().size(); ++i)
        if (game.trace()[i].kind == "PUPDAT" && game.trace()[i].detail == why) ++n;
    return n;
}

void test_torch_use_redraws_twice_and_flask_not_at_all() {
    // PUSE.ASM PUSE12: PSTOW0 ends in COMUPD's PUPDAT, then A$TORC, then a
    // second PUPDAT. UFL900 (flasks) has ISOUND, STATUS and HUPDAT only.
    dag::Game game(1, 0);
    game.set_frozen(true);
    run(game, {"PULL RIGHT TORCH"});
    std::size_t from = game.trace().size();
    run(game, {"USE RIGHT"});
    check(count_pupdat(game, from, "comupd") == 1 && count_pupdat(game, from, "puse") == 1,
          "USE of a torch redraws twice: PSTOW0's COMUPD, then PUSE12");
    dag::Game flask(1, 0);
    flask.set_frozen(true);
    int index = -1;
    for (int i = 0; i < static_cast<int>(flask.objects().size()); ++i)
        if (flask.objects()[static_cast<std::size_t>(i)].cls == 0) { index = i; break; }
    check(index >= 0, "a flask object exists");
    if (index < 0) return;
    flask.hold(false, index);
    from = flask.trace().size();
    run(flask, {"USE LEFT"});
    check(has(flask, "USE") && count_pupdat(flask, from, "puse") == 0 &&
              count_pupdat(flask, from, "comupd") == 0,
          "USE of a flask does not redraw (UFL900)");
}

void test_inivu_returns_to_the_viewer() {
    // PLOOK.ASM INIVUX falls into PLOOK: DSPMOD = VIEWER, PUPDAT. HUMAN.ASM
    // HMAN10 runs INIVU on the first key after a map; PCLIMB.ASM PCLI20 runs it
    // after NEWLVL.
    dag::Game game(1, 0);
    game.set_frozen(true);
    int scroll = -1;
    for (int i = 0; i < static_cast<int>(game.objects().size()); ++i)
        if (game.objects()[static_cast<std::size_t>(i)].cls == 2) { scroll = i; break; }
    check(scroll >= 0, "a scroll object exists");
    if (scroll < 0) return;
    game.hold(false, scroll);
    game.set_player_power(10000);
    run(game, {"REVEAL LEFT", "USE LEFT"});
    check(game.display_mode() == dag::DisplayMode::Mapper, "USE of a revealed scroll shows the map");
    run(game, {"LOOK"});
    run(game, {"USE LEFT", "T"});
    check(game.display_mode() == dag::DisplayMode::Viewer,
          "the first key after the map runs INIVU and returns to the viewer");

    dag::Game climb(1, 0);
    climb.set_frozen(true);
    climb.place_player(0, 23);   // level 0 ladder down (VFTTAB)
    const std::size_t from = climb.trace().size();
    run(climb, {"CLIMB DOWN"});
    climb.advance_jiffies(500);
    bool inivu = false;
    std::uint64_t climbed = 0, built = 0, redrawn = 0;
    for (std::size_t i = from; i < climb.trace().size(); ++i) {
        const auto& e = climb.trace()[i];
        if (e.kind == "CLIMB") climbed = e.jiffy;
        if (e.kind == "NEWLVL") built = e.jiffy;
        if (e.kind == "PUPDAT" && e.detail == "inivu") { inivu = true; redrawn = e.jiffy; }
    }
    check(climb.level_index() == 1 && inivu, "CLIMB runs INIVU after NEWLVL");
    // [ROM] C-22 / descend-early: level 1's DGEN90 spin reads SECOND 326
    // interrupts after the command; the tail to PLAYER is 22-24 (+ spin).
    check(built - climbed == 326, "NEWLVL 1 reads SECOND 326 jiffies after CLIMB");
    // D-19: spin (draws + 5) / 10 [INF], then level 1's tail of 23 [ROM], whose
    // last jiffy is INIVU's SYNC.
    int second = -1;
    for (const auto& e : climb.trace())
        if (e.kind == "NEWLVL") second = std::stoi(e.detail.substr(e.detail.find("second=") + 7));
    const int draws = second == 0 ? 256 : second;
    check(second >= 0 && redrawn - built == static_cast<std::uint64_t>((draws + 5) / 10 + 22),
          "INIVU follows the spin and level 1's 23-jiffy tail");
    check(climb.polarity_level() == 1 && !climb.preparing(), "the build is over: NLVL50 ran, PREPARE! is gone");
}

void test_examine_costs_a_sync() {
    // PEXAM.ASM PEXAM: STX DSPMOD (EXAMIN), then PUPDAT.
    dag::Game game(1, 0);
    game.set_frozen(true);
    const std::size_t from = game.trace().size();
    run(game, {"EXAMINE"});
    check(game.display_mode() == dag::DisplayMode::Examine && count_pupdat(game, from, "pexam") == 1,
          "EXAMINE switches to the examine display and redraws once");
}

void test_turn_around_sweeps_twice() {
    // PTURN.ASM: TURN AROUND runs RLTURN twice, a single turn once.
    auto loops_for = [](const std::string& cmd) {
        dag::Game game(1, 0);
        game.set_frozen(true);
        run(game, {cmd});
        std::uint32_t loops = 0;
        for (const auto& e : game.events())
            if (e.kind == dag::CoreEventKind::Block && e.block == dag::BlockKind::TurnAnimation)
                loops = e.loop_count;
        return loops;
    };
    check(loops_for("TURN AROUND") == 16 && loops_for("TURN LEFT") == 8,
          "TURN AROUND reports two RLTURN sweeps (16 loops), a single turn one (8)");
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
    // MISC.ASM WIZIX0 CLRPRI wipes the swings' !!! before ENDGAM's messages, and
    // HMAN70's prompt waits until ENDGAM returns (HUMAN.ASM), so row 0 is blank
    // and only ENOUGH! and DOOM!!! carry bangs.
    const auto& text = game.primary_text();
    check(game.level_index() == 2 &&
              std::all_of(text.begin(), text.begin() + 32, [](std::uint8_t c) { return c == 0; }) &&
              std::count(text.begin(), text.end(), std::uint8_t{0x1B}) == 4,
          "WIZIN clears the text and no prompt precedes ENDGAM's messages");
    game.advance_jiffies(800);
    // PATTK.ASM ENDGAM / MISC.ASM: WIZIN's one WIZZES SYNC and WAITX's 81 SYNCs
    // come before NEWLVL; WIZOUT's 16 WIZZES SYNCs come before INIVU.
    std::uint64_t start = 0, relocate = 0, inivu_at = 0;
    for (const auto& e : game.trace()) {
        if (e.kind == "KILL") start = e.jiffy;
        if (e.kind == "RELOCATE") relocate = e.jiffy;
        if (e.kind == "PUPDAT" && e.detail == "inivu" && relocate != 0 && inivu_at == 0)
            inivu_at = e.jiffy;
    }
    // From the kill: PATT40's PUPDAT SYNC, WIZIN's WIZZES SYNC, WAITX's 81,
    // then NEWLVL 3's pre-spin build time (C-22: 377).
    std::uint64_t built = 0;
    for (const auto& e : game.trace())
        if (e.kind == "NEWLVL") built = e.jiffy;
    check(built - start == 83 + 377, "NEWLVL 3 reads SECOND after the SYNCs and its build time");
    check(relocate > built, "FNDCEL relocates after NEWLVL 3 is built");
    check(inivu_at - relocate == 16, "INIVU waits for WIZOUT's 16 WIZZES SYNCs");
    check(std::count(text.begin(), text.end(), std::uint8_t{0x1E}) == 1,
          "HMAN70 prompts once, after ENDGAM's closing INIVU");
    check(game.level_index() == 3, "ENDGAM rebuilds level 3");
    check(game.display_mode() == dag::DisplayMode::Viewer && game.heart().hbeatf == 0xFF,
          "ENDGAM's WIZIN clears HBEATF and its closing INIVU sets it to $FF, in the viewer");
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
    check(game.heart().hbeatf != 0, "the riddle's INIVU leaves the audio heartbeat on (no WIZIN)");
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
    check(game.counters().total_jiffies == at + 100 && game.player().won,
          "WINNER foreground loops while CLOCK continues");
}

// COMMON.ASM:136-141 LOAD90: after INIVU clears the text area, PROMPT prints
// I.CR, I.DOT (MISC.ASM M$PROM1), so a save or load leaves a "." prompt.
void test_tape_prompts_after_inivu() {
    for (const char* command : {"ZSAVE QUEST", "ZLOAD QUEST"}) {
        dag::Game game(1, 0);
        game.load_script(keys_for(10, {"ZSAVE QUEST", command}));
        game.advance_jiffies(200);
        const auto& text = game.primary_text();
        check(std::count(text.begin(), text.end(), std::uint8_t{0x1E}) == 1,
              std::string("LOAD90 prompts after ") + command);
    }
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
    const auto death_time = game.counters().total_jiffies;
    auto keys = keys_for(death_time + 35, {"ZLOAD QUEST", "TURN RIGHT"});
    keys.insert(keys.begin(), {death_time + 30, 'X'});
    game.load_script(keys);
    game.advance_jiffies(30);
    check(game.player().dead, "without a key, death remains in the foreground loop");
    check(game.counters().total_jiffies == death_time + 30,
          "CLOCK continues during death");
    game.advance_jiffies(1);
    check(!game.player().dead && has(game, "RESTART", "GAME after death"),
          "timestamped key restarts through GAME");
    check(game.cassette_image("QUEST") && *game.cassette_image("QUEST") == image,
          "cassette survives COMINI");
    check(game.line_buffer().empty(), "restart clears its triggering key");
    game.advance_jiffies(100);
    check(has(game, "ZLOAD", "QUEST"), "future typed ZLOAD restores the saved game");
    check(game.player().dir == dag::Dir::East, "future command runs after reload");
    check(has(game, "DEATH") && has(game, "ZSAVE"), "restart retains trace history");
    check(game.counters().total_jiffies == death_time + 131,
          "restart and ZLOAD preserve monotonic replay time");
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
    const auto first_hit_damage = [](dag::Game& g) {
        const auto hits = [&g] {
            std::size_t n = 0;
            for (const auto& e : g.trace()) n += e.kind == "HIT";
            return n;
        };
        const std::size_t seen = hits();
        const std::uint16_t before = g.player().damage;
        for (int i = 0; i < 400 && hits() == seen; ++i) g.advance_jiffies(1);
        return static_cast<unsigned>(g.player().damage) - before;
    };
    const unsigned full = first_hit_damage(a);
    dag::Game b;
    b.restore_snapshot(snap);
    check(b.incoming_damage_percent() == 100, "snapshot default incoming stays 100");
    b.set_incoming_damage_percent(25);
    const unsigned quarter = first_hit_damage(b);
    check(full > 0, "a creature hit the player at 100%", "added=" + std::to_string(full));
    check(quarter == full * 25u / 100u, "FUDGE incoming 25 scales creature-to-player damage",
          "full=" + std::to_string(full) + " quarter=" + std::to_string(quarter));
    check(fresh.incoming_damage_percent() == 100, "another Game() is still canonical 100");
}

}  // namespace

int main() {
    test_spent_ring_becomes_a_plain_gold_ring();
    test_luknew_pupdat_costs_a_sync();
    test_starting_bag_level_byte();
    test_burner_requests_redraw();
    test_death_clears_faint();
    test_no_pupdat_while_fainted();
    test_blocked_move_still_reports_its_half_step();
    test_pull_costs_a_sync();
    test_torch_use_redraws_twice_and_flask_not_at_all();
    test_inivu_returns_to_the_viewer();
    test_examine_costs_a_sync();
    test_turn_around_sweeps_twice();
    test_image_ending();
    test_wizard_ending();
    test_winner();
    test_death_line();
    test_tape_prompts_after_inivu();
    test_death_load_resumes();
    test_save_load_resumes_at_the_save();
    test_ram_image_is_the_whole_state();
    test_snapshot_round_trip_and_replay();
    test_fudge_harness_is_not_source_behaviour();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
