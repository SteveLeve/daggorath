#include "daggorath/game.hpp"

#include <cctype>
#include <sstream>

#include "daggorath/creature_move.hpp"
#include "daggorath/parser.hpp"
#include "daggorath/population.hpp"

namespace dag {
namespace {

// Internal character codes (CD.ASM).
constexpr std::uint8_t kISp = 0x00, kICr = 0x1F, kIBs = 0x24;
// ASCII codes as delivered by POLCAT (CD.ASM).
constexpr std::uint8_t kCBs = 0x08, kCCr = 0x0D, kCSp = 0x20;

constexpr std::size_t kLineBufSize = 32;   // CD.ASM:584 LINBUF RMB 32

// Command indices within CMDTAB, i.e. the order of the CMDXXX macro.
constexpr std::uint8_t kCmdLook = 6, kCmdMove = 7, kCmdTurn = 11;
// Direction indices within DIRTAB.
constexpr std::uint8_t kDirLeft = 0, kDirRight = 1, kDirBack = 2, kDirAround = 3;

std::string dir_name(Dir d) {
    switch (d) {
        case Dir::North: return "N";
        case Dir::East:  return "E";
        case Dir::South: return "S";
        default:         return "W";
    }
}

}  // namespace

std::string TraceEvent::to_line() const {
    std::ostringstream os;
    os << jiffy << '\t' << counters << '\t' << kind << '\t' << detail;
    return os.str();
}

Game::Game() { start(true, 0, 0); }

Game::Game(std::uint8_t second_at_entry, int level) {
    start(false, second_at_entry, level);
}

void Game::start(bool rom_build, std::uint8_t second_at_entry, int level) {
    // The clock is already running when NEWLVL builds the maze, so the SECOND
    // counter at level entry is an input to DGEN90, not to the maze itself.
    // Original Mode applies the measured build interrupts first. SECOND is
    // already 6 at DGEN90 and still 6 at GAME50, so the whole count lands
    // before the maze spin. Foreground tasks are added after that, because
    // SCHED has not started and the capture's matrix is still the CMTTAB row.
    matrix_ = kCmtTab;
    objects_ = create_dungeon_objects();
    if (rom_build) {
        sched_.advance_clock_counters(kLevel0BuildInterrupts);
    } else {
        sched_.counters().second = second_at_entry;
    }
    const std::uint8_t second_now = sched_.counters().second;
    build_level(level, second_now);
    // GAME30 runs after NEWLVL, so these two are absent from the first attachment.
    for (const std::uint8_t type : {std::uint8_t{17}, std::uint8_t{15}}) {  // WOODEN, PINE
        Ocb bag = birth_player_object(type, 0);
        bag.owner = 1;                       // INC of the zeroed ownership byte
        objects_.push_back(bag);
    }

    update_heart_rate();

    // ONCE.ASM SYSTCB adds the TCBDAT tasks to SCDQUE in this order. LUKNEW and
    // BURNER stay inert. CREGEN performs the matrix increment. CMOVE was queued
    // on Q.TEN during birth, ahead of LUKNEW's later QUEADD onto that queue.
    player_task_ = sched_.add({"PLAYER", [this] { return task_player(); },
                              Queue::Sched, 0, true});
    sched_.add({"LUKNEW", [] { return TaskResult{Queue::Tenth, 3}; },
                Queue::Sched, 0, true});
    hslow_task_ = sched_.add({"HSLOW", [this] { return task_hslow(); },
                              Queue::Sched, 0, true});
    sched_.add({"BURNER", [] { return TaskResult{Queue::Minute, 1}; },
                Queue::Sched, 0, true});
    sched_.add({"CREGEN", [this] { return task_cregen(); },
                Queue::Sched, 0, true});

    sched_.set_trace([this](const std::string& msg) {
        const auto sp = msg.find(' ');
        emit(msg.substr(0, sp), msg.substr(sp + 1));
    });
    emit("INIT", "level=" + std::to_string(level) + " row=" +
                     std::to_string(player_.row) + " col=" +
                     std::to_string(player_.col) + " dir=" + dir_name(player_.dir) +
                     " second=" + std::to_string(static_cast<int>(second_now)));
}

void Game::build_level(int level, std::uint8_t second) {
    level_index_ = level;
    level_ = generate_level(level, second);
    birth_creatures(level, matrix_[static_cast<std::size_t>(level)], level_.rng,
                    level_.maze, ccbs_);
    attach_objects(level, ccbs_, objects_);
    queue_creatures();
}

void Game::queue_creatures() {
    for (int id : creature_tasks_) sched_.retire(id);
    creature_tasks_.clear();
    for (int slot = 0; slot < kCcbSlots; ++slot) {
        if (!ccbs_[static_cast<std::size_t>(slot)].in_use) continue;
        const std::uint8_t delay = ccbs_[static_cast<std::size_t>(slot)].move_delay;
        const int id = sched_.add({"CMOVE-" + std::to_string(slot),
                                   [this, slot] { return task_cmove(slot); },
                                   Queue::Tenth, delay, true});
        creature_tasks_.push_back(id);
    }
}

TaskResult Game::task_cmove(int slot) {
    CmoveView view;
    view.frozen = frozen_;
    view.player_row = player_.row;
    view.player_col = player_.col;
    view.level = level_index_;
    std::vector<std::string> events;
    const TaskResult r = cmove(slot, ccbs_, objects_, level_.maze, level_.rng, view, events);
    for (const std::string& e : events) {
        const auto sp = e.find(' ');
        if (sp == std::string::npos) emit(e, "");
        else emit(e.substr(0, sp), e.substr(sp + 1));
    }
    return r;
}

void Game::enter_level(int level) {
    build_level(level, sched_.counters().second);
}

TaskResult Game::task_cregen() {
    // The opening lap runs this because the task was born in Q.SCD. A later
    // NEWLVL is what turns the incremented matrix entry into a live creature.
    cregen_increment(matrix_[static_cast<std::size_t>(level_index_)], level_.rng);
    return {Queue::Minute, 5};
}

void Game::emit(const std::string& kind, const std::string& detail) {
    trace_.push_back({sched_.counters().total_jiffies,
                      sched_.counters().to_string(), kind, detail});
}

void Game::advance_jiffies(std::uint64_t n) {
    for (std::uint64_t i = 0; i < n; ++i) {
        // Collect the keystrokes timestamped for this jiffy.
        std::vector<std::uint8_t> keys;
        const std::uint64_t now = sched_.counters().total_jiffies;
        while (script_pos_ < script_.size() && script_[script_pos_].jiffy == now) {
            keys.push_back(script_[script_pos_].ch);
            ++script_pos_;
        }
        sched_.interrupt(keys);

        if (sync_pending_) {       // a command ended in SYNC: it owns this jiffy
            sync_pending_ = false;
            emit("SYNC", "display swap");
            continue;
        }
        sched_.run_ready_pass();
    }
}

// HUPDAX. The division is the original repeated-subtraction loop, which
// increments the quotient before testing the borrow, so it yields
// floor(num/den) + 1. The result is stored in one byte and read back signed.
void Game::update_heart_rate() {
    const std::uint32_t p = player_.power;
    const std::uint32_t d = player_.damage;
    const std::uint32_t num = (p * 64u) & 0xFFFFFFu;
    const std::uint32_t den = (p + 2u * d) & 0xFFFFFFu;
    std::uint8_t q = 0;
    if (den == 0) {
        q = 0;                              // degenerate: P == 0 and D == 0
    } else {
        std::int64_t acc = static_cast<std::int64_t>(num);
        while (acc >= 0) {
            acc -= static_cast<std::int64_t>(den);
            q = static_cast<std::uint8_t>(q + 1);
        }
    }
    player_.heart_rate = static_cast<std::uint8_t>(q - 19);
    const int signed_rate = static_cast<std::int8_t>(player_.heart_rate);
    if (!player_.fainted && signed_rate <= 3) {
        player_.fainted = true;
        sched_.set_faint(true);
        emit("FAINT", "heart_rate=" + std::to_string(signed_rate));
    } else if (player_.fainted && signed_rate > 4) {
        player_.fainted = false;
        sched_.set_faint(false);
        emit("REVIVE", "heart_rate=" + std::to_string(signed_rate));
    }
}

// PLAYER: drains the whole keyboard buffer on its turn, then reschedules one
// jiffy later. A carriage return inside the burst dispatches its command before
// the remaining characters are consumed.
TaskResult Game::task_player() {
    for (;;) {
        const std::uint8_t raw = sched_.keyboard().get();
        if (raw == 0) break;                            // NULL means done
        if (player_.fainted) continue;                  // just eat chars
        std::uint8_t internal;
        if (raw == kCSp) {
            internal = kISp;
        } else if (raw == kCCr) {
            internal = kICr;
        } else if (raw == kCBs) {
            internal = kIBs;
        } else if (raw >= 'A' && raw <= 'Z') {
            internal = static_cast<std::uint8_t>(raw & 0x1F);
        } else {
            internal = kISp;                            // non-alpha becomes space
        }
        feed_char(internal);
        if (sync_pending_) break;   // the command blocked on SYNC
    }
    return {Queue::Jiffy, 1};                           // SCHED$ 1,Q.JIF
}

TaskResult Game::task_hslow() {
    // HSLOW: recover 1/64th of accumulated damage, then reschedule at HEARTR.
    const std::uint16_t d = player_.damage;
    const std::int32_t recovered = static_cast<std::int32_t>(d) -
                                   static_cast<std::int32_t>(d >> 6);
    player_.damage = static_cast<std::uint16_t>(recovered > 0 ? recovered : 0);
    update_heart_rate();
    std::uint8_t delay = player_.heart_rate;
    if (delay == 0) delay = 1;    // a zero countdown would never expire
    return {Queue::Jiffy, delay};
}

void Game::feed_char(std::uint8_t ch) {
    if (ch == kICr) {
        dispatch_line();
        return;
    }
    if (ch == kIBs) {
        if (!line_.empty()) line_.pop_back();
        return;
    }
    line_.push_back(ch == kISp ? ' ' : static_cast<char>('A' + ch - 1));
    // HMAN20 falls through the buffer-full test into the carriage-return path.
    if (line_.size() == kLineBufSize) {
        emit("LINE", "buffer full, dispatching");
        dispatch_line();
    }
}

void Game::dispatch_line() {
    const std::string line = line_;
    line_.clear();
    emit("LINE", "\"" + line + "\"");

    std::size_t pos = 0;
    const ParseResult r = parse(kCmdTab, line, pos);
    if (r.status == ParseStatus::NoToken) return;        // HMAN70: nothing typed
    if (r.status == ParseStatus::NoMatch) {
        emit("OUTPUT", "???");                           // CMDERR
        return;
    }
    switch (r.type) {
        case kCmdMove: cmd_move(line, pos); break;
        case kCmdTurn: cmd_turn(line, pos); break;
        case kCmdLook: cmd_look(); break;
        default:
            emit("UNIMPLEMENTED", std::string(kCmdTab[r.type].word) +
                 " is outside the Phase 0b slice");
            break;
    }
}

// PTURN: a missing or unrecognised direction is an error; LEFT/RIGHT/AROUND
// change PDIR and end in DEC UPDATE / SYNC.
void Game::cmd_turn(const std::string& line, std::size_t& pos) {
    const ParseResult d = parse(kDirTab, line, pos);
    if (d.status != ParseStatus::Matched) {
        emit("OUTPUT", "???");
        return;
    }
    int b = static_cast<int>(player_.dir);
    if (d.type == kDirLeft) {
        --b;
    } else if (d.type == kDirRight) {
        ++b;
    } else if (d.type == kDirAround) {
        b += 2;
    } else {
        emit("OUTPUT", "???");          // UP / DOWN / BACK are not TURN arguments
        return;
    }
    player_.dir = static_cast<Dir>(b & 3);   // PREVU: ANDB #3 / STB PDIR
    emit("TURN", "dir=" + dir_name(player_.dir));
    sync_pending_ = true;
}

// PMOVE: no direction means forward. Every path, including a blocked step,
// falls through PMOV90 and pays the exertion cost.
void Game::cmd_move(const std::string& line, std::size_t& pos) {
    const ParseResult d = parse(kDirTab, line, pos);
    if (d.status == ParseStatus::NoMatch) {
        emit("OUTPUT", "???");          // BLT ERRCMD2: illegal token
        return;
    }
    int relative = 0;                   // BEQ (null token): step forward
    if (d.status == ParseStatus::Matched) {
        if (d.type == kDirBack)        relative = 2;
        else if (d.type == kDirRight)  relative = 1;
        else if (d.type == kDirLeft)   relative = 3;
        else { emit("OUTPUT", "???"); return; }   // AROUND / UP / DOWN
    }
    step_player(relative);
    movement_exertion();
    sync_pending_ = true;
}

// PLOOK: return to the forward view.
void Game::cmd_look() {
    mode_ = DisplayMode::Viewer;
    emit("LOOK", "mode=VIEWER");
}

// PSTEP
void Game::step_player(int relative_dir) {
    const Dir d = static_cast<Dir>((static_cast<int>(player_.dir) + relative_dir) & 3);
    int nr = 0, nc = 0;
    if (step_ok(level_.maze, player_.row, player_.col, d, nr, nc)) {
        player_.row = nr;
        player_.col = nc;
        emit("MOVE", "row=" + std::to_string(nr) + " col=" + std::to_string(nc) +
                         " dir=" + dir_name(player_.dir) + " ok=1");
    } else {
        emit("SOUND", "A$THUD");        // blocked: ISOUND A$THUD
        emit("MOVE", "row=" + std::to_string(player_.row) + " col=" +
                         std::to_string(player_.col) + " dir=" + dir_name(player_.dir) +
                         " ok=0");
    }
}

// PMOV90: PDAM += (POBJWT / 8) + 3, then HUPDAT.
void Game::movement_exertion() {
    const std::uint16_t add = static_cast<std::uint16_t>((player_.carried_weight >> 3) + 3);
    player_.damage = static_cast<std::uint16_t>(player_.damage + add);
    update_heart_rate();
    emit("EXERT", "damage=" + std::to_string(player_.damage) +
                      " heart_rate=" +
                      std::to_string(static_cast<int>(static_cast<std::int8_t>(player_.heart_rate))));
}

std::vector<KeyEvent> parse_script(const std::string& text, std::string& error) {
    std::vector<KeyEvent> out;
    std::istringstream in(text);
    std::string line;
    int lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        const auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream ls(line);
        std::uint64_t jiffy = 0;
        std::string key;
        if (!(ls >> jiffy >> key)) {
            if (line.find_first_not_of(" \t") == std::string::npos) continue;
            error = "line " + std::to_string(lineno) + ": expected '<jiffy> <KEY>'";
            return {};
        }
        std::uint8_t ch = 0;
        if (key == "SPACE") ch = kCSp;
        else if (key == "CR") ch = kCCr;
        else if (key == "BS") ch = kCBs;
        else if (key.size() == 1) ch = static_cast<std::uint8_t>(std::toupper(key[0]));
        else {
            error = "line " + std::to_string(lineno) + ": unknown key '" + key + "'";
            return {};
        }
        out.push_back({jiffy, ch});
    }
    return out;
}

}  // namespace dag
