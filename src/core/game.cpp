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
constexpr std::uint8_t kCmdAttack = 0, kCmdLook = 6, kCmdMove = 7, kCmdTurn = 11;
constexpr std::uint8_t kClassRing = 1;          // CD.ASM K.RING
// EMPHND after COMDAT.ASM's INI P.OCCLS+EMPHND: class 4 (sword noises),
// magic offense 0, physical offense 5.
constexpr std::uint8_t kEmptyClass = 4, kEmptyMagic = 0, kEmptyPhysical = 5;
constexpr std::uint8_t kTypeRingEnergy = 19;
constexpr std::uint8_t kTypeRingFire = 21;
constexpr std::uint8_t kTypeRingGold = 22;
constexpr std::uint8_t kTypeTorchDead = 24;
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
    int previous = -1;
    for (const std::uint8_t type : {std::uint8_t{17}, std::uint8_t{15}}) {  // WOODEN, PINE
        Ocb bag = birth_player_object(type, 0);
        bag.owner = 1;                       // INC of the zeroed ownership byte
        bag.reveal = 0;                      // GAME30 clears the reveal requirement
        objects_.push_back(bag);
        const int index = static_cast<int>(objects_.size()) - 1;
        if (previous < 0) player_.bag_head = index;
        else objects_[static_cast<std::size_t>(previous)].next = index;
        previous = index;
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
    sched_.add({"BURNER", [this] { return task_burner(); },
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

void Game::hold(bool right, int object_index) {
    (right ? player_.right_hand : player_.left_hand) = object_index;
}

void Game::wield_torch(int object_index) { player_.torch = object_index; }

Fighter Game::player_fighter() const {
    Fighter f;
    f.power = player_.power;
    f.damage = player_.damage;
    return f;
}

void Game::store_player_fighter(const Fighter& fighter) {
    player_.power = fighter.power;
    player_.damage = fighter.damage;
}

namespace {

HeldShield shield_from(const std::vector<Ocb>& objects, int index) {
    HeldShield hand;
    if (index < 0 || static_cast<std::size_t>(index) >= objects.size()) return hand;
    const Ocb& o = objects[static_cast<std::size_t>(index)];
    hand.present = true;
    hand.cls = o.cls;
    hand.magic_defense = o.spec[0];
    hand.physical_defense = o.spec[1];
    return hand;
}

}  // namespace

TaskResult Game::task_cmove(int slot) {
    CmoveView view;
    view.frozen = frozen_;
    view.player_row = player_.row;
    view.player_col = player_.col;
    view.level = level_index_;
    Fighter fighter = player_fighter();
    view.player = &fighter;
    view.left = shield_from(objects_, player_.left_hand);
    view.right = shield_from(objects_, player_.right_hand);
    bool heart = false;
    view.heart_update = &heart;
    std::vector<std::string> events;
    const TaskResult r = cmove(slot, ccbs_, objects_, level_.maze, level_.rng, view, events);
    store_player_fighter(fighter);
    if (heart) update_heart_rate();
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
    } else     if (player_.fainted && signed_rate > 4) {
        player_.fainted = false;
        sched_.set_faint(false);
        emit("REVIVE", "heart_rate=" + std::to_string(signed_rate));
    }
    // HUPD90: BLO, so equal power and damage is not death.
    if (!player_.dead && player_.power < player_.damage) {
        player_.dead = true;
        sched_.halt();
        emit("DEATH", "power=" + std::to_string(player_.power) +
                          " damage=" + std::to_string(player_.damage));
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
        if (sched_.halted()) break; // DEATH ends in BRA * (HUPDAT.ASM)
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
        case kCmdAttack: cmd_attack(line, pos); break;
        case 1: cmd_climb(line, pos); break;
        case 2: cmd_drop(line, pos); break;
        case 3: cmd_examine(); break;
        case 4: cmd_get(line, pos); break;
        case 5: cmd_incant(line, pos); break;
        case 8: cmd_pull(line, pos); break;
        case 9: cmd_reveal(line, pos); break;
        case 10: cmd_stow(line, pos); break;
        case 12: cmd_use(line, pos); break;
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

constexpr std::uint8_t kClassWeight[] = {5, 1, 10, 25, 25, 10};
constexpr std::uint8_t kTypeFlaskThews = 5;
constexpr std::uint8_t kTypeScrollSeer = 4;
constexpr std::uint8_t kTypeScrollVision = 7;
constexpr std::uint8_t kTypeFlaskAbye = 8;
constexpr std::uint8_t kTypeFlaskHale = 9;
constexpr std::uint8_t kTypeFlaskEmpty = 23;
constexpr std::uint8_t kTypeRingFinal = 18;
constexpr std::uint8_t kClassTorch = 5;

bool Game::parse_hand(const std::string& line, std::size_t& pos, bool& right, int& held) {
    const ParseResult hand = parse(kDirTab, line, pos);
    if (hand.status != ParseStatus::Matched ||
        (hand.type != kDirLeft && hand.type != kDirRight)) {
        emit("OUTPUT", "???");
        return false;
    }
    right = hand.type == kDirRight;
    held = right ? player_.right_hand : player_.left_hand;
    return true;
}

bool Game::parse_object(const std::string& line, std::size_t& pos, bool& specific,
                        std::uint8_t& kind) {
    const std::size_t token_start = pos;
    const ParseResult generic = parse(kGenTab, line, pos);
    if (generic.status == ParseStatus::Matched) {
        specific = false;
        kind = generic.type;
        return true;
    }
    if (generic.status == ParseStatus::NoToken) {
        emit("OUTPUT", "???");
        return false;
    }
    // POBJ10: PARSE0 classifies the token GENTAB just rejected; it reads no new one.
    pos = token_start;
    const ParseResult adjective = parse(kAdjTab, line, pos);
    const ParseResult genus = parse(kGenTab, line, pos);
    if (adjective.status != ParseStatus::Matched || genus.status != ParseStatus::Matched ||
        genus.token_class != adjective.token_class) {
        emit("OUTPUT", "???");
        return false;
    }
    specific = true;
    kind = adjective.type;
    return true;
}

void Game::add_weight(int delta) {
    player_.carried_weight = static_cast<std::uint16_t>(player_.carried_weight + delta);
    update_heart_rate();
    emit("BURDEN", "weight=" + std::to_string(player_.carried_weight));
}

void Game::stow_index(bool right, int index) {
    Ocb& object = objects_[static_cast<std::size_t>(index)];
    object.next = player_.bag_head;
    player_.bag_head = index;
    if (right) player_.right_hand = -1;
    else player_.left_hand = -1;
    emit("STOW", "object=" + std::to_string(index));
}

void Game::refresh_light() {
    if (player_.torch < 0) {
        player_.regular_light = 0;
        player_.magic_light = 0;
        return;
    }
    const Ocb& torch = objects_[static_cast<std::size_t>(player_.torch)];
    player_.regular_light = torch.spec[1];
    player_.magic_light = torch.spec[2];
}

TaskResult Game::task_burner() {
    if (player_.torch >= 0) {
        Ocb& torch = objects_[static_cast<std::size_t>(player_.torch)];
        if (torch.spec[0] != 0) {
            torch.spec[0] = static_cast<std::uint8_t>(torch.spec[0] - 1);
            if (torch.spec[0] <= 5) {
                torch.type = kTypeTorchDead;
                torch.reveal = 0;
                emit("TORCH", "dead timer=" + std::to_string(torch.spec[0]));
            }
            if (torch.spec[0] < torch.spec[1]) torch.spec[1] = torch.spec[0];
            if (torch.spec[0] < torch.spec[2]) torch.spec[2] = torch.spec[0];
            refresh_light();
            emit("TORCH", "timer=" + std::to_string(torch.spec[0]) +
                              " light=" + std::to_string(player_.regular_light));
        }
    }
    return {Queue::Minute, 1};
}

void Game::cmd_get(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held >= 0) {
        emit("OUTPUT", "???");
        return;
    }
    bool specific = false;
    std::uint8_t kind = 0;
    if (!parse_object(line, pos, specific, kind)) return;
    int found = -1;
    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        const Ocb& o = objects_[static_cast<std::size_t>(i)];
        if (o.owner != 0 || o.level != level_index_) continue;
        if (o.row != player_.row || o.col != player_.col) continue;
        const bool match = specific ? o.type == kind : o.cls == kind;
        if (match) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        emit("OUTPUT", "???");
        return;
    }
    Ocb& object = objects_[static_cast<std::size_t>(found)];
    object.owner = 1;
    if (right) player_.right_hand = found;
    else player_.left_hand = found;
    add_weight(kClassWeight[object.cls]);
    emit("GET", "object=" + std::to_string(found));
}

void Game::cmd_drop(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held < 0) {
        emit("OUTPUT", "???");
        return;
    }
    Ocb& object = objects_[static_cast<std::size_t>(held)];
    if (right) player_.right_hand = -1;
    else player_.left_hand = -1;
    object.owner = 0;
    object.row = static_cast<std::uint8_t>(player_.row);
    object.col = static_cast<std::uint8_t>(player_.col);
    object.level = static_cast<std::uint8_t>(level_index_);
    const int weight = kClassWeight[object.cls];
    add_weight(-weight);
    emit("DROP", "object=" + std::to_string(held));
}

void Game::cmd_stow(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held < 0) {
        emit("OUTPUT", "???");
        return;
    }
    stow_index(right, held);
}

void Game::cmd_pull(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held >= 0) {
        emit("OUTPUT", "???");
        return;
    }
    bool specific = false;
    std::uint8_t kind = 0;
    if (!parse_object(line, pos, specific, kind)) return;
    int previous = -1;
    int current = player_.bag_head;
    while (current >= 0) {
        Ocb& object = objects_[static_cast<std::size_t>(current)];
        const bool match = specific ? object.type == kind : object.cls == kind;
        if (match) {
            if (previous < 0) player_.bag_head = object.next;
            else objects_[static_cast<std::size_t>(previous)].next = object.next;
            object.next = -1;
            if (right) player_.right_hand = current;
            else player_.left_hand = current;
            if (current == player_.torch) {
                player_.torch = -1;
                refresh_light();
            }
            emit("PULL", "object=" + std::to_string(current));
            return;
        }
        previous = current;
        current = object.next;
    }
    emit("OUTPUT", "???");
}

void Game::cmd_use(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held < 0) return;
    Ocb& object = objects_[static_cast<std::size_t>(held)];
    if (object.cls == kClassTorch) {
        player_.torch = held;
        refresh_light();
        stow_index(right, held);
        emit("SOUND", "A$TORC");
        return;
    }
    if (object.type == kTypeFlaskThews) {
        player_.power = static_cast<std::uint16_t>(player_.power + 1000);
    } else if (object.type == kTypeFlaskHale) {
        player_.damage = 0;
    } else if (object.type == kTypeFlaskAbye) {
        player_.damage = static_cast<std::uint16_t>(
            player_.damage + scal16(player_.power, 102));
    } else if (object.type == kTypeScrollVision || object.type == kTypeScrollSeer) {
        if (object.reveal != 0) return;
        player_.map_features = object.type == kTypeScrollSeer;
        mode_ = DisplayMode::Mapper;
        emit("MAP", player_.map_features ? "features=1" : "features=0");
        return;
    } else {
        return;
    }
    object.type = kTypeFlaskEmpty;
    object.reveal = 0;
    emit("SOUND", "A$FLAS");
    update_heart_rate();
    emit("USE", "flask=" + std::to_string(held));
}

void Game::cmd_reveal(const std::string& line, std::size_t& pos) {
    bool right = false;
    int held = -1;
    if (!parse_hand(line, pos, right, held)) return;
    if (held < 0) return;
    Ocb& object = objects_[static_cast<std::size_t>(held)];
    if (object.reveal == 0) return;
    const unsigned need = static_cast<unsigned>(object.reveal) * 25u;
    if (need > player_.power) return;
    fill_ocb_specific(object);
    object.reveal = 0;
    emit("REVEAL", "object=" + std::to_string(held) + " type=" + std::to_string(object.type));
}

bool Game::incant_hand(int index, std::uint8_t word) {
    if (index < 0) return false;
    Ocb& object = objects_[static_cast<std::size_t>(index)];
    if (object.cls != kClassRing) return false;
    if (object.spec[1] == 0 || object.spec[1] != word) return false;
    object.type = object.spec[1];
    fill_ocb_specific(object);
    object.spec[1] = 0;
    emit("SOUND", "A$RING");
    emit("INCANT", "object=" + std::to_string(index) + " type=" + std::to_string(object.type));
    if (object.type == kTypeRingFinal) emit("DEFER", "winner");
    return object.type == kTypeRingFinal;
}

void Game::cmd_incant(const std::string& line, std::size_t& pos) {
    const ParseResult word = parse(kAdjTab, line, pos);
    if (word.status != ParseStatus::Matched || !word.full_word) return;
    if (incant_hand(player_.left_hand, word.type)) return;
    incant_hand(player_.right_hand, word.type);
}

void Game::cmd_examine() {
    mode_ = DisplayMode::Examine;
    const int creature = find_creature(player_.row, player_.col);
    emit("EXAMINE", "creature=" + std::to_string(creature));
}

void Game::cmd_climb(const std::string& line, std::size_t& pos) {
    const int feature = vfind(level_index_, player_.row, player_.col);
    const ParseResult dir = parse(kDirTab, line, pos);
    if (feature < 0 || dir.status != ParseStatus::Matched) {
        emit("OUTPUT", "???");
        return;
    }
    int delta = 0;
    if (dir.type == 4) {  // UP
        if (feature != 1) {
            emit("OUTPUT", "???");
            return;
        }
        delta = -1;
    } else if (dir.type == 5) {  // DOWN
        if ((feature & 2) == 0) {
            emit("OUTPUT", "???");
            return;
        }
        delta = 1;
    } else {
        emit("OUTPUT", "???");
        return;
    }
    const int next = level_index_ + delta;
    if (next < 0 || next > 4) {
        emit("OUTPUT", "???");
        return;
    }
    emit("CLIMB", "level=" + std::to_string(next));
    enter_level(next);
}

int Game::find_creature(int row, int col) const {
    for (int i = 0; i < kCcbSlots; ++i) {
        const Ccb& c = ccbs_[static_cast<std::size_t>(i)];
        if (c.in_use && c.row == row && c.col == col) return i;
    }
    return -1;
}

void Game::kill_creature(int slot) {
    Ccb& creature = ccbs_[static_cast<std::size_t>(slot)];
    int obj = creature.object_head;
    while (obj >= 0) {
        Ocb& o = objects_[static_cast<std::size_t>(obj)];
        const int next = o.next;
        o.owner = 0;
        o.carrier = -1;
        o.row = creature.row;
        o.col = creature.col;
        emit("LOOT", "object=" + std::to_string(obj) + " row=" + std::to_string(creature.row) +
                         " col=" + std::to_string(creature.col));
        obj = next;
    }
    auto& row = matrix_[static_cast<std::size_t>(level_index_)];
    const std::uint8_t type = creature.type;
    row[type] = static_cast<std::uint8_t>(row[type] - 1);
    creature.in_use = 0;
    emit("KILL", "slot=" + std::to_string(slot) + " type=" + std::to_string(type) +
                     " matrix=" + std::to_string(row[type]));
    const std::int16_t eighth = static_cast<std::int16_t>(creature.power) >> 3;
    const std::int16_t sum =
        static_cast<std::int16_t>(static_cast<std::int16_t>(player_.power) + eighth);
    if (sum < 0) {
        player_.power = static_cast<std::uint16_t>(0x7F00u | static_cast<std::uint16_t>(sum & 0xFF));
    } else {
        player_.power = static_cast<std::uint16_t>(sum);
    }
    emit("ABSORB", "power=" + std::to_string(player_.power));
    if (type == 10 || type == 11) {
        if (type == 11) set_frozen(true);
        emit("DEFER", "endgame " + std::to_string(type));
    }
}

void Game::cmd_attack(const std::string& line, std::size_t& pos) {
    const ParseResult hand = parse(kDirTab, line, pos);
    if (hand.status != ParseStatus::Matched ||
        (hand.type != kDirLeft && hand.type != kDirRight)) {
        emit("OUTPUT", "???");
        return;
    }
    const int index = hand.type == kDirRight ? player_.right_hand : player_.left_hand;
    std::uint8_t magic = kEmptyMagic;
    std::uint8_t physical = kEmptyPhysical;
    std::uint8_t cls = kEmptyClass;
    std::uint8_t type = 0;
    Ocb* held = nullptr;
    if (index >= 0 && static_cast<std::size_t>(index) < objects_.size()) {
        held = &objects_[static_cast<std::size_t>(index)];
        magic = held->magic_offense;
        physical = held->physical_offense;
        cls = held->cls;
        type = held->type;
    }
    const std::uint16_t effort = scal16(
        player_.power, static_cast<std::uint8_t>((static_cast<unsigned>(physical) + magic) >> 3));
    player_.damage = static_cast<std::uint16_t>(player_.damage + effort);
    emit("EXERT", "damage=" + std::to_string(player_.damage));
    emit("SOUND", "class=" + std::to_string(cls));
    if (held != nullptr && type >= kTypeRingEnergy && type <= kTypeRingFire) {
        held->spec[0] = static_cast<std::uint8_t>(held->spec[0] - 1);
        if (held->spec[0] == 0) {
            held->type = kTypeRingGold;
            emit("RING", "spent");
        }
    }
    const int slot = find_creature(player_.row, player_.col);
    if (slot < 0) {
        update_heart_rate();
        return;
    }
    Ccb& creature = ccbs_[static_cast<std::size_t>(slot)];
    const bool ring = cls == kClassRing;
    if (!ring) {
        Fighter defender;
        defender.power = creature.power;
        defender.damage = creature.damage;
        if (!attack_hits(player_.power, defender.power, defender.damage, level_.rng.next())) {
            emit("MISS", "slot=" + std::to_string(slot));
            update_heart_rate();
            return;
        }
        const bool dark = player_.torch < 0 ||
                          objects_[static_cast<std::size_t>(player_.torch)].type == kTypeTorchDead;
        if (dark) {
            const std::uint8_t gate = level_.rng.next();
            if ((gate & 3) != 0) {
                emit("DARK", "roll=" + std::to_string(gate));
                update_heart_rate();
                return;
            }
        }
    }
    emit("HIT", "slot=" + std::to_string(slot));
    Fighter attacker;
    attacker.power = player_.power;
    attacker.magic_offense = magic;
    attacker.physical_offense = physical;
    Fighter defender;
    defender.power = creature.power;
    defender.damage = creature.damage;
    defender.magic_defense = creature.magic_defense;
    defender.physical_defense = creature.physical_defense;
    apply_damage(attacker, defender);
    creature.damage = defender.damage;
    emit("DAMAGE", "slot=" + std::to_string(slot) + " damage=" + std::to_string(creature.damage));
    if (!damage_survived(defender)) kill_creature(slot);
    update_heart_rate();
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
