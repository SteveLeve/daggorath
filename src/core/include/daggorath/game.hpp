// Daggorath Core — clock, PLAYER/HSLOW, MOVE/TURN/LOOK, and NEWLVL population.
// Source: HUMAN.ASM, PTURN.ASM, PLOOK.ASM, COMPLR.ASM, HUPDAT.ASM, COMMON.ASM,
//         ONCE.ASM, COMDAT.ASM, NEWLVL.ASM, COMCRE.ASM, OBIRTH.ASM.
//
// Deliberately out of scope: combat damage, magic, rendering. CMOVE runs;
// the attack branch is D-7. CREGEN updates the matrix only.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "daggorath/combat.hpp"
#include "daggorath/maze.hpp"
#include "daggorath/population.hpp"
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
    bool dead = false;
    int left_hand = -1;
    int right_hand = -1;
    int torch = -1;
    int bag_head = -1;
    bool map_features = false;
    std::uint8_t regular_light = 0;
    std::uint8_t magic_light = 0;
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

// Interrupts from GAME10's IRQSYN to the fetch of GAME50, measured on
// MAME 0.264 coco2b running catalog 26-3093. Isolated so a later cycle-cost
// derivation can replace the count. See clock-and-scheduler.md §13.
inline constexpr std::uint32_t kLevel0BuildInterrupts = 377;

class Game {
public:
    // Original Mode. Counts `kLevel0BuildInterrupts` before DGEN90, so the
    // scheduler entry clock is 0:0:6.2.5 and SECOND is 6.
    Game();

    // Harness clock: set SECOND and leave the other counters at 0. This is the
    // source-derived population comparison and the harness-modified spins. It
    // is not the ROM level-0 entry.
    explicit Game(std::uint8_t second_at_entry, int level = 0);

    void load_script(std::vector<KeyEvent> keys) {
        script_ = std::move(keys);
        script_pos_ = 0;
    }

    // Advance exactly n discrete 1/60 s boundaries. A large delta never skips
    // intervening boundaries.
    void advance_jiffies(std::uint64_t n);

    const PlayerState& player() const { return player_; }
    const Maze& maze() const { return level_.maze; }
    const GeneratedLevel& level() const { return level_; }
    int level_index() const { return level_index_; }
    const std::vector<TraceEvent>& trace() const { return trace_; }
    const Counters& counters() const { return sched_.counters(); }
    DisplayMode display_mode() const { return mode_; }

    const std::array<Ccb, kCcbSlots>& creatures() const { return ccbs_; }
    const std::vector<Ocb>& objects() const { return objects_; }
    const std::array<std::uint8_t, kCreatureTypes>& matrix_row() const {
        return matrix_[static_cast<std::size_t>(level_index_)];
    }
    const std::array<std::array<std::uint8_t, kCreatureTypes>, 5>& matrix() const {
        return matrix_;
    }

    // NEWLVL for `level`, using the clock's current SECOND. Does not move the
    // player. SYSTCB rebuilds the system tasks and drops every CMOVE task, then
    // the new level's creatures are queued.
    void enter_level(int level);

    // FRZFLG. Frozen creatures take the movement-delay return and do not act.
    void set_frozen(bool frozen) { frozen_ = frozen; }
    bool frozen() const { return frozen_; }

    // PLHAND / PRHAND / PTORCH. Phase 3 has no GET; tests and later phases
    // write the same slots the object commands will.
    void hold(bool right, int object_index);
    void wield_torch(int object_index);
    // Test hooks: write PROW/PCOL and PDAM directly, then run HUPDAT.
    void place_player(int row, int col) { player_.row = row; player_.col = col; }
    void set_player_power(std::uint16_t power) {
        player_.power = power;
        update_heart_rate();
    }
    void set_player_damage(std::uint16_t damage) {
        player_.damage = damage;
        update_heart_rate();
    }

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
    void cmd_attack(const std::string& line, std::size_t& pos);
    void cmd_get(const std::string& line, std::size_t& pos);
    void cmd_drop(const std::string& line, std::size_t& pos);
    void cmd_stow(const std::string& line, std::size_t& pos);
    void cmd_pull(const std::string& line, std::size_t& pos);
    void cmd_use(const std::string& line, std::size_t& pos);
    void cmd_reveal(const std::string& line, std::size_t& pos);
    void cmd_incant(const std::string& line, std::size_t& pos);
    void cmd_examine();
    void cmd_climb(const std::string& line, std::size_t& pos);
    bool parse_hand(const std::string& line, std::size_t& pos, bool& right, int& held);
    bool parse_object(const std::string& line, std::size_t& pos, bool& specific, std::uint8_t& kind);
    void add_weight(int delta);
    void stow_index(bool right, int index);
    void refresh_light();
    bool incant_hand(int index, std::uint8_t word);
    TaskResult task_burner();
    int find_creature(int row, int col) const;
    void kill_creature(int slot);
    Fighter player_fighter() const;
    void store_player_fighter(const Fighter& fighter);
    void step_player(int relative_dir);    // PSTEP
    void movement_exertion();              // PMOV90
    void emit(const std::string& kind, const std::string& detail);
    TaskResult task_cregen();
    TaskResult task_cmove(int slot);
    void queue_creatures();
    void systcb();
    void build_level(int level, std::uint8_t second);
    void start(bool rom_build, std::uint8_t second_at_entry, int level);

    std::array<std::array<std::uint8_t, kCreatureTypes>, 5> matrix_{};
    std::array<Ccb, kCcbSlots> ccbs_{};
    std::vector<Ocb> objects_;
    int level_index_ = 0;

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
    std::vector<int> creature_tasks_;
    bool frozen_ = false;
    // A command that ends in DEC UPDATE / SYNC blocks until the next interrupt.
    bool sync_pending_ = false;
};

// Input script format: one event per line, "<jiffy> <KEY>" where KEY is a single
// character, or the words SPACE, CR or BS. '#' starts a comment.
std::vector<KeyEvent> parse_script(const std::string& text, std::string& error);

}  // namespace dag
