// Daggorath Core — the Phase 0b slice: clock + PLAYER/HSLOW tasks + the line
// editor and command dispatch for MOVE, TURN and LOOK.
// Source: HUMAN.ASM (PLAYER, HUMAN), PTURN.ASM (PTURN, PMOVE, PSTEP, PREVU),
//         PLOOK.ASM, COMPLR.ASM (HSLOW), HUPDAT.ASM (HUPDAX), COMMON.ASM,
//         ONCE.ASM (GAME10), COMDAT.ASM (TCBDAT, GAMDAT).
//
// Deliberately out of scope: combat, creatures, items, magic, rendering.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/maze.hpp"
#include "daggorath/scheduler.hpp"

namespace dag {

enum class DisplayMode : std::uint8_t { Viewer = 0, Examine = 1, Mapper = 2 };

struct PlayerState {
    int row = 0x10;                 // ONCE.ASM GAME10: LDD #$100B / STD PROW
    int col = 0x0B;
    Dir dir = Dir::North;           // inferred: RAM is zeroed before GAME10
    // RAMDAT initialises PPOW to $17A0; GAME10's `CLR PPOW` clears only the
    // HIGH byte of the two-byte field, so game-mode play starts at $00A0 = 160.
    std::uint16_t power = 160;      // PPOW
    std::uint16_t damage = 0;       // PDAM
    std::uint16_t carried_weight = 35;  // POBJWT: wooden sword 25 + pine torch 10
    std::uint8_t heart_rate = 0;    // HEARTR, in jiffies
    bool fainted = false;
};

struct TraceEvent {
    std::uint64_t jiffy = 0;
    std::string counters;
    std::string kind;
    std::string detail;
    std::string to_line() const;
};

// A timestamped keystroke, as read from an input script.
struct KeyEvent {
    std::uint64_t jiffy = 0;
    std::uint8_t ch = 0;
};

class Game {
public:
    // `second_at_entry` feeds DGEN90 for the level-0 maze.
    explicit Game(std::uint8_t second_at_entry = 1, int level = 0);

    void load_script(std::vector<KeyEvent> keys) { script_ = std::move(keys); }

    // Advance exactly n discrete 1/60 s boundaries. A large delta never skips
    // intervening boundaries.
    void advance_jiffies(std::uint64_t n);

    const PlayerState& player() const { return player_; }
    const Maze& maze() const { return level_.maze; }
    const GeneratedLevel& level() const { return level_; }
    const std::vector<TraceEvent>& trace() const { return trace_; }
    const Counters& counters() const { return sched_.counters(); }
    DisplayMode display_mode() const { return mode_; }

    // HUPDAX: heart rate = (P*64)/(P+2D) - 19, by repeated subtraction, stored
    // in one signed byte. Faint at <= 3, recover above 4.
    void update_heart_rate();

private:
    TaskResult task_player();
    TaskResult task_hslow();
    void feed_char(std::uint8_t ch);       // HUMAN
    void dispatch_line();                  // HMAN50
    void cmd_move(const std::string& line, std::size_t& pos);
    void cmd_turn(const std::string& line, std::size_t& pos);
    void cmd_look();
    void step_player(int relative_dir);    // PSTEP
    void movement_exertion();              // PMOV90
    void emit(const std::string& kind, const std::string& detail);

    Scheduler sched_;
    GeneratedLevel level_;
    PlayerState player_;
    DisplayMode mode_ = DisplayMode::Viewer;
    std::string line_;                     // LINBUF (32 bytes)
    std::vector<KeyEvent> script_;
    std::size_t script_pos_ = 0;
    std::vector<TraceEvent> trace_;
    int player_task_ = -1;
    int hslow_task_ = -1;
    // A command that ends in DEC UPDATE / SYNC blocks until the next interrupt.
    bool sync_pending_ = false;
};

// Input script format: one event per line, "<jiffy> <KEY>" where KEY is a single
// character, or the words SPACE, CR or BS. '#' starts a comment.
std::vector<KeyEvent> parse_script(const std::string& text, std::string& error);

}  // namespace dag
