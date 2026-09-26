// Phase 6a CoreEvent regressions (ADR-0004).
//
// Default traces keep their Phase 0b/2/3 SOUND and SYNC lines. This file checks
// the parallel CoreEvent stream: stamps, SOUNDS volume, D-4 blocking flags,
// heartbeat from CLOCK CLK30, and same-jiffy order.
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "daggorath/creature_move.hpp"
#include "daggorath/game.hpp"
#include "daggorath/sound_tables.hpp"

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

void test_volume_formula() {
    check(dag::creature_sound_volume(0) == 0xFF, "range 0 is full volume");
    check(dag::creature_sound_volume(1) == static_cast<std::uint8_t>(~31),
          "range 1 is ~(T0*31)");
    check(dag::kExtractedSndObj == 12, "SNDOBJ is 12");
}

void test_thud_and_turn_blocks() {
    dag::Game game;
    game.load_script(type_at(2, "TURN LEFT"));
    game.advance_jiffies(40);
    bool turn_block = false, sync_block = false;
    for (const auto& e : game.events()) {
        if (e.kind == dag::CoreEventKind::Block && e.block == dag::BlockKind::TurnAnimation) {
            turn_block = true;
            check(!e.duration_known && e.duration_jiffies == 0 && e.loop_count == 8,
                  "TURN animation is D-4a: 8 loops, duration unknown");
        }
        if (e.kind == dag::CoreEventKind::Block && e.block == dag::BlockKind::Sync) {
            sync_block = true;
            check(e.duration_known && e.duration_jiffies == 1,
                  "DEC UPDATE / SYNC is one known jiffy");
        }
    }
    check(turn_block && sync_block, "a TURN emits both the sweep and the SYNC");

    dag::Game wall;
    wall.load_script({
        {5, 'T'}, {6, ' '}, {7, 'R'}, {8, 0x0D},
        {30, 'M'}, {31, 0x0D},
    });
    wall.advance_jiffies(50);
    bool thud = false;
    for (const auto& e : wall.events()) {
        if (e.kind == dag::CoreEventKind::Sound && e.cue == static_cast<std::uint8_t>(dag::SoundCue::THUD)) {
            thud = true;
            check(e.entry == dag::SoundEntry::Isound && e.volume == 0xFF && !e.duration_known,
                  "blocked MOVE is ISOUND A$THUD at full volume, D-4b");
        }
    }
    check(thud, "a blocked MOVE emits A$THUD");
}

void test_heartbeat_flags() {
    dag::Game game;
    check(game.heart().hbeatf != 0 && game.heart().heartf != 0,
          "GAME50 INIVU turns the audio and visual heartbeat on");
    game.advance_jiffies(120);
    int beats = 0;
    bool saw_visual = false;
    for (const auto& e : game.events()) {
        if (e.kind != dag::CoreEventKind::Heartbeat) continue;
        ++beats;
        check(e.position == "IRQ", "CLK30 events are stamped IRQ");
        if (e.visual) saw_visual = true;
    }
    check(beats > 0 && saw_visual, "CLK30 emits HEART events with the glyph flag");
}

void test_same_jiffy_order() {
    dag::Game probe(1, 0);
    probe.advance_jiffies(400);
    std::uint64_t at = 0;
    for (const auto& ev : probe.trace()) {
        if (ev.kind == "TASK" && ev.detail == "run CMOVE-6") {
            at = ev.jiffy;
            break;
        }
    }
    check(at > 0, "CMOVE-6 runs inside 400 jiffies");
    dag::Game game(1, 0);
    game.load_script({{at, static_cast<std::uint8_t>('M')}});
    game.advance_jiffies(at + 1);
    bool player = false, cmove = false;
    for (const auto& ev : game.trace()) {
        if (ev.jiffy != at) continue;
        if (ev.kind == "TASK" && ev.detail == "run PLAYER") player = true;
        if (ev.kind == "TASK" && ev.detail == "run CMOVE-6") cmove = true;
    }
    check(player && cmove, "PLAYER and CMOVE-6 both run on the keystroke jiffy");
    bool irq_before_fg = true;
    for (const auto& e : game.events()) {
        if (e.position == "IRQ") continue;
        for (const auto& irq : game.events()) {
            if (irq.position != "IRQ" || irq.jiffy != e.jiffy) continue;
            if (irq.sequence > e.sequence) irq_before_fg = false;
        }
    }
    check(irq_before_fg, "IRQ CoreEvents on a jiffy precede that jiffy's task events");
}

void test_creature_sound_payload() {
    dag::Game game(1, 0);
    game.advance_jiffies(200);
    bool ranged = false;
    for (const auto& e : game.events()) {
        if (e.kind != dag::CoreEventKind::Sound) continue;
        if (e.range < 0) continue;
        ranged = true;
        check(e.entry == dag::SoundEntry::Sounds, "CWALK uses SOUNDS");
        check(e.volume == dag::creature_sound_volume(e.range),
              "volume is ~(T0*31)",
              "vol=" + std::to_string(e.volume) + " range=" + std::to_string(e.range));
        check(e.source >= 0, "the source is a CCB slot");
        check(!e.duration_known, "creature sound duration is D-4b");
    }
    check(ranged, "a ranged creature sound appears in 200 harness jiffies");
}

void test_sounds_fixture() {
#ifdef DAG_FIXTURE_DIR
    const std::string path = std::string(DAG_FIXTURE_DIR) + "/sounds.json";
#else
    const std::string path = "sounds.json";
#endif
    std::ifstream in(path);
    check(static_cast<bool>(in), "sounds.json is readable");
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    check(text.find("\"A$THUD\"") != std::string::npos, "SNDTAB includes A$THUD");
    check(text.find("\"reads_seed\": false") != std::string::npos, "SNOISE does not read SEED");
    check(dag::kSoundCueNames[20] == "A$THUD", "generated header index 20 is A$THUD");
}

}  // namespace

int main() {
    test_volume_formula();
    test_thud_and_turn_blocks();
    test_heartbeat_flags();
    test_same_jiffy_order();
    test_creature_sound_payload();
    test_sounds_fixture();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks
              << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
