#include "daggorath/game.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <istream>
#include <ostream>
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

    sched_.set_trace([this](const std::string& msg) {
        const auto sp = msg.find(' ');
        emit(msg.substr(0, sp), msg.substr(sp + 1));
    });
    sched_.set_irq_hook([this] { heartbeat_interrupt(); });
    inivu();   // ONCE.ASM GAME50: SWI INIVU
    emit("INIT", "level=" + std::to_string(level) + " row=" +
                     std::to_string(player_.row) + " col=" +
                     std::to_string(player_.col) + " dir=" + dir_name(player_.dir) +
                     " second=" + std::to_string(static_cast<int>(second_now)));
}

// ONCE.ASM SYSTCB: every queue and TCB is cleared, then the TCBDAT tasks go
// onto SCDQUE in this order. LUKNEW stays inert. CREGEN performs the matrix
// increment. CBIRTH later queues CMOVE on Q.TEN, ahead of LUKNEW's QUEADD.
void Game::systcb() {
    sched_.reset_tasks();
    creature_tasks_.clear();
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
}

// NEWLVL: zero the CCBs, SYSTCB, DGNGEN, CBIRTH per CMXLND, attach objects.
void Game::build_level(int level, std::uint8_t second) {
    level_index_ = level;
    systcb();
    level_ = generate_level(level, second);
    birth_creatures(level, matrix_[static_cast<std::size_t>(level)], level_.rng,
                    level_.maze, ccbs_);
    attach_objects(level, ccbs_, objects_);
    queue_creatures();
}

void Game::queue_creatures() {
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
    std::vector<CmoveView::Sound> sounds;
    view.sounds = &sounds;
    view.incoming_damage_percent = incoming_damage_percent_;
    std::vector<std::string> events;
    const TaskResult r = cmove(slot, ccbs_, objects_, level_.maze, level_.rng, view, events);
    store_player_fighter(fighter);
    if (heart) update_heart_rate();
    for (const auto& s : sounds) {
        sound(s.cue, s.volume, s.range, slot);
        events_.back().entry = s.entry;
    }
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
    if (kind == "OUTPUT" || kind == "DIALOGUE") text(detail);
}

CoreEvent& Game::push_event(CoreEventKind kind) {
    CoreEvent e;
    e.jiffy = sched_.counters().total_jiffies;
    e.sequence = static_cast<std::uint32_t>(events_.size());
    if (sched_.in_irq()) e.position = "IRQ";
    else if (!sched_.running().empty()) e.position = sched_.running();
    else e.position = "FG";
    e.kind = kind;
    events_.push_back(std::move(e));
    return events_.back();
}

void Game::sound(SoundCue cue) {
    auto& e = push_event(CoreEventKind::Sound);
    e.cue = static_cast<std::uint8_t>(cue);
    e.volume = 0xFF;
    e.entry = SoundEntry::Isound;
    e.range = -1;
    e.source = -1;
    e.duration_known = false;
}

void Game::sound(std::uint8_t cue, std::uint8_t volume, int range, int source) {
    auto& e = push_event(CoreEventKind::Sound);
    e.cue = cue;
    e.volume = volume;
    e.entry = SoundEntry::Sounds;
    e.range = range;
    e.source = source;
    e.duration_known = false;
}

void Game::text(const std::string& s) {
    auto& e = push_event(CoreEventKind::Text);
    e.text = s;
}

void Game::set_mode(DisplayMode mode) {
    mode_ = mode;
    auto& e = push_event(CoreEventKind::DisplayMode);
    e.mode = mode;
    if (mode == DisplayMode::Mapper) heart_.heartf = 0;   // PUSE.ASM:123 CLR HEARTF
}

void Game::block(BlockKind kind, std::uint32_t loops, std::uint32_t jiffies, bool known) {
    auto& e = push_event(CoreEventKind::Block);
    e.block = kind;
    e.loop_count = loops;
    e.duration_jiffies = jiffies;
    e.duration_known = known;
}

void Game::inivu() {
    // PLOOK.ASM INIVUX: HUPDAT, INC HEARTC, DEC HEARTF, DEC HBEATF.
    update_heart_rate();
    heart_.heartc = static_cast<std::uint8_t>(heart_.heartc + 1);
    heart_.heartf = static_cast<std::uint8_t>(heart_.heartf - 1);
    heart_.hbeatf = static_cast<std::uint8_t>(heart_.hbeatf - 1);
}

void Game::wizard_fade_in() {
    heart_.hbeatf = 0;   // MISC.ASM WIZIX CLR HBEATF
}

void Game::heartbeat_interrupt() {
    // COMMON.ASM CLK30. The glyph is presentation; the toggle is CLOCK.
    if (heart_.hbeatf == 0) return;
    heart_.heartc = static_cast<std::uint8_t>(heart_.heartc - 1);
    if (heart_.heartc != 0) return;
    heart_.heartc = player_.heart_rate;
    heart_.audio_level = !heart_.audio_level;
    const bool visual = heart_.heartf != 0;
    bool large = false;
    if (visual) {
        heart_.hearts = static_cast<std::uint8_t>(~heart_.hearts);
        large = heart_.hearts != 0;
    }
    auto& e = push_event(CoreEventKind::Heartbeat);
    e.audio_level = heart_.audio_level;
    e.visual = visual;
    e.large = large;
}

void Game::set_incoming_damage_percent(int percent) {
    if (percent < 0) percent = 0;
    incoming_damage_percent_ = percent;
}

void Game::apply_due_harness(std::uint64_t now) {
    while (harness_pos_ < harness_.size() && harness_[harness_pos_].jiffy <= now) {
        const HarnessEvent& e = harness_[harness_pos_++];
        if (e.kind == HarnessFudge::Incoming) {
            set_incoming_damage_percent(e.percent);
            emit("FUDGE", "incoming=" + std::to_string(incoming_damage_percent_));
        } else if (e.kind == HarnessFudge::Rest) {
            player_.damage = 63;
            update_heart_rate();
            emit("FUDGE", "rest");
        }
    }
}

void Game::advance_jiffies(std::uint64_t n) {
    for (std::uint64_t i = 0; i < n; ++i) {
        // Collect the keystrokes timestamped for this jiffy.
        std::vector<std::uint8_t> keys;
        const std::uint64_t now = sched_.counters().total_jiffies;
        apply_due_harness(now);
        while (script_pos_ < script_.size() && script_[script_pos_].jiffy == now) {
            keys.push_back(script_[script_pos_].ch);
            ++script_pos_;
        }
        sched_.interrupt(keys);

        if (sync_pending_) {       // a command ended in SYNC: it owns this jiffy
            sync_pending_ = false;
            block(BlockKind::Sync, 1, 1, true);
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
        wizard_fade_in();
        emit("DIALOGUE", "^ YET ANOTHER DOES NOT RETURN...");   // HUPDAT.ASM:143 OUTSTI
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
    if (zflag_ != 0) sched_.end_lap([this] { tape_operation(); });
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
    if (heart_.heartf == 0) inivu();   // HUMAN.ASM HMAN10
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
        case 13: cmd_zload(line, pos); break;
        case 14: cmd_zsave(line, pos); break;
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
    block(BlockKind::TurnAnimation, 8, 0, false);  // PTURN.ASM LRTURN, D-4a
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
    block(BlockKind::MoveAnimation, 8, 0, false);  // PTURN.ASM PMOVE, D-4a
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
        sound(SoundCue::TORC);
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
        sound(SoundCue::SCRO);   // PUSE.ASM USC210 ISOUND A$SCRO; no extra trace line
        set_mode(DisplayMode::Mapper);
        emit("MAP", player_.map_features ? "features=1" : "features=0");
        return;
    } else {
        return;
    }
    object.type = kTypeFlaskEmpty;
    object.reveal = 0;
    emit("SOUND", "A$FLAS");
    sound(SoundCue::FLAS);
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
    sound(SoundCue::RING);
    emit("INCANT", "object=" + std::to_string(index) + " type=" + std::to_string(object.type));
    if (object.type == kTypeRingFinal) {
        player_.won = true;
        sched_.halt();
        emit("WINNER", "final ring");
        // PINCAN.ASM:64 and :88 OUTSTI, then BRA *.
        emit("DIALOGUE", "^BEHOLD! DESTINY AWAITS THE HAND");
        emit("DIALOGUE", "        OF A NEW WIZARD...");
    }
    return object.type == kTypeRingFinal;
}

void Game::cmd_incant(const std::string& line, std::size_t& pos) {
    const ParseResult word = parse(kAdjTab, line, pos);
    if (word.status != ParseStatus::Matched || !word.full_word) return;
    if (incant_hand(player_.left_hand, word.type)) return;
    incant_hand(player_.right_hand, word.type);
}

void Game::cmd_examine() {
    set_mode(DisplayMode::Examine);
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
    if (type == 10) endgame_image();
    if (type == 11) endgame_wizard();
}

void Game::endgame_image() {
    // ENDGAM (PATTK.ASM): the two messages, then BAGPTR = PTORCH with the
    // torch's link cleared. PLHAND, PRHAND, and PTORCH are kept. Weight becomes
    // 200, level 3 is rebuilt, and FNDCEL relocates.
    emit("ENDGAM", "image");
    emit("DIALOGUE", "^ ENOUGH! I TIRE OF THIS PLAY...");   // PATTK.ASM:198
    emit("DIALOGUE", "   PREPARE TO MEET THY DOOM!!!");     // PATTK.ASM:222
    player_.bag_head = -1;
    if (player_.torch >= 0) {
        objects_[static_cast<std::size_t>(player_.torch)].next = -1;
        player_.bag_head = player_.torch;
    }
    player_.carried_weight = 200;
    enter_level(3);
    for (;;) {
        const int col = level_.rng.next() & 31;
        const int row = level_.rng.next() & 31;
        if (level_.maze.at(row, col) == 0xFF) continue;
        player_.row = row;
        player_.col = col;
        break;
    }
    emit("RELOCATE", "row=" + std::to_string(player_.row) + " col=" + std::to_string(player_.col));
}

void Game::endgame_wizard() {
    set_frozen(true);
    player_.regular_light = 0x07;
    player_.magic_light = 0x13;
    player_.bag_head = -1;
    player_.torch = -1;
    player_.left_hand = -1;
    player_.right_hand = -1;
    emit("ENDGAM", "wizard");
    wizard_fade_in();
}

std::string Game::filename_token(const std::string& line, std::size_t& pos) const {
    std::string name;
    while (pos < line.size() && line[pos] == ' ') ++pos;
    while (pos < line.size() && line[pos] != ' ') name.push_back(line[pos++]);
    if (name.size() > 8) name.resize(8);
    return name;
}

// PZSAVE / PZLOAD only record the filename and set ZFLAG. SCHED1 performs the
// tape operation once PLAYER has been requeued.
void Game::cmd_zsave(const std::string& line, std::size_t& pos) {
    tape_name_ = filename_token(line, pos);
    zflag_ = 1;
}

void Game::cmd_zload(const std::string& line, std::size_t& pos) {
    tape_name_ = filename_token(line, pos);
    zflag_ = -1;
}

void Game::tape_operation() {
    const int flag = zflag_;
    const std::string name = tape_name_;
    if (flag > 0) {
        const std::string image = ram_image();
        tapes_.push_back({name, image});
        emit("ZSAVE", name + " bytes=" + std::to_string(image.size()));
    } else {
        // LOAD reads blocks until a file header's name matches. With no match
        // the original keeps reading tape; this core reports ??? and resumes.
        const std::string* image = nullptr;
        for (auto it = tapes_.rbegin(); it != tapes_.rend(); ++it)
            if (it->first == name) { image = &it->second; break; }
        if (image == nullptr) {
            zflag_ = 0;
            emit("OUTPUT", "???");
            return;
        }
        restore_ram_image(*image);
        emit("ZLOAD", name);
    }
    // LOAD90: CLR ZFLAG, INIVU, PROMPT.
    zflag_ = 0;
    inivu();
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
    sound(static_cast<std::uint8_t>(kSndObj + cls), 0xFF, -1, -1);
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
    sound(SoundCue::KLK2);   // PATTK.ASM ISOUND A$KLK2
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
    set_mode(DisplayMode::Viewer);
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
        sound(SoundCue::THUD);
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

std::function<TaskResult()> Game::task_body(const std::string& name) {
    if (name == "PLAYER") return [this] { return task_player(); };
    if (name == "LUKNEW") return [] { return TaskResult{Queue::Tenth, 3}; };
    if (name == "HSLOW") return [this] { return task_hslow(); };
    if (name == "BURNER") return [this] { return task_burner(); };
    if (name == "CREGEN") return [this] { return task_cregen(); };
    if (name.rfind("CMOVE-", 0) == 0) {
        const int slot = std::stoi(name.substr(6));
        return [this, slot] { return task_cmove(slot); };
    }
    std::abort();   // every TCB the core creates has one of these names
}

void Game::save_ram(std::ostream& out) const {
    const PlayerState& p = player_;
    out << p.row << ' ' << p.col << ' ' << static_cast<int>(p.dir) << ' ' << p.power << ' '
        << p.damage << ' ' << p.carried_weight << ' ' << static_cast<int>(p.heart_rate) << ' '
        << p.fainted << ' ' << p.dead << ' ' << p.won << ' ' << p.left_hand << ' '
        << p.right_hand << ' ' << p.torch << ' ' << p.bag_head << ' ' << p.map_features << ' '
        << static_cast<int>(p.regular_light) << ' ' << static_cast<int>(p.magic_light) << '\n';
    out << static_cast<int>(mode_) << ' ' << frozen_ << ' ' << sync_pending_ << ' '
        << level_index_ << ' ' << line_.size() << ' ' << line_ << "|\n";
    out << static_cast<int>(heart_.heartf) << ' ' << static_cast<int>(heart_.heartc) << ' '
        << static_cast<int>(heart_.hearts) << ' ' << static_cast<int>(heart_.hbeatf) << ' '
        << (heart_.audio_level ? 1 : 0) << '\n';
    for (const auto& row : matrix_) {
        for (const std::uint8_t v : row) out << static_cast<int>(v) << ' ';
        out << '\n';
    }
    for (const Ccb& c : ccbs_) {
        out << c.power << ' ' << static_cast<int>(c.magic_offense) << ' '
            << static_cast<int>(c.magic_defense) << ' ' << static_cast<int>(c.physical_offense)
            << ' ' << static_cast<int>(c.physical_defense) << ' ' << static_cast<int>(c.move_delay)
            << ' ' << static_cast<int>(c.attack_delay) << ' ' << c.object_head << ' ' << c.damage
            << ' ' << static_cast<int>(c.in_use) << ' ' << static_cast<int>(c.type) << ' '
            << static_cast<int>(c.dir) << ' ' << static_cast<int>(c.row) << ' '
            << static_cast<int>(c.col) << '\n';
    }
    out << objects_.size() << '\n';
    for (const Ocb& o : objects_) {
        out << o.next << ' ' << static_cast<int>(o.row) << ' ' << static_cast<int>(o.col) << ' '
            << static_cast<int>(o.level) << ' ' << static_cast<int>(o.owner) << ' '
            << static_cast<int>(o.spec[0]) << ' ' << static_cast<int>(o.spec[1]) << ' '
            << static_cast<int>(o.spec[2]) << ' ' << static_cast<int>(o.type) << ' '
            << static_cast<int>(o.cls) << ' ' << static_cast<int>(o.reveal) << ' '
            << static_cast<int>(o.magic_offense) << ' ' << static_cast<int>(o.physical_offense)
            << ' ' << o.carrier << '\n';
    }
    for (const std::uint8_t b : level_.maze.bytes()) out << static_cast<int>(b) << ' ';
    out << '\n';
    for (const auto* seed : {&level_.rng_before_spin, &level_.rng_after_spin, &level_.rng.seed()})
        out << static_cast<int>((*seed)[0]) << ' ' << static_cast<int>((*seed)[1]) << ' '
            << static_cast<int>((*seed)[2]) << ' ';
    out << level_.spin_count << '\n';
    sched_.save_state(out);
    out << player_task_ << ' ' << hslow_task_ << ' ' << creature_tasks_.size();
    for (const int id : creature_tasks_) out << ' ' << id;
    out << '\n';
}

void Game::load_ram(std::istream& in) {
    PlayerState& p = player_;
    int dir = 0, heart = 0, fainted = 0, dead = 0, won = 0, features = 0, rl = 0, ml = 0;
    in >> p.row >> p.col >> dir >> p.power >> p.damage >> p.carried_weight >> heart >> fainted >>
        dead >> won >> p.left_hand >> p.right_hand >> p.torch >> p.bag_head >> features >> rl >> ml;
    p.dir = static_cast<Dir>(dir & 3);
    p.heart_rate = static_cast<std::uint8_t>(heart);
    p.fainted = fainted != 0;
    p.dead = dead != 0;
    p.won = won != 0;
    p.map_features = features != 0;
    p.regular_light = static_cast<std::uint8_t>(rl);
    p.magic_light = static_cast<std::uint8_t>(ml);
    int mode = 0, frozen = 0, sync = 0;
    std::size_t line_size = 0;
    in >> mode >> frozen >> sync >> level_index_ >> line_size;
    mode_ = static_cast<DisplayMode>(mode);
    frozen_ = frozen != 0;
    sync_pending_ = sync != 0;
    in.get();
    line_.assign(line_size, ' ');
    in.read(line_.data(), static_cast<std::streamsize>(line_size));
    in.get();   // '|'
    int hf = 0, hc = 0, hs = 0, hb = 0, al = 0;
    in >> hf >> hc >> hs >> hb >> al;
    heart_.heartf = static_cast<std::uint8_t>(hf);
    heart_.heartc = static_cast<std::uint8_t>(hc);
    heart_.hearts = static_cast<std::uint8_t>(hs);
    heart_.hbeatf = static_cast<std::uint8_t>(hb);
    heart_.audio_level = al != 0;
    int v = 0;
    for (auto& row : matrix_)
        for (std::uint8_t& cell : row) {
            in >> v;
            cell = static_cast<std::uint8_t>(v);
        }
    auto byte = [&in]() {
        int x = 0;
        in >> x;
        return static_cast<std::uint8_t>(x);
    };
    for (Ccb& c : ccbs_) {
        in >> c.power;
        c.magic_offense = byte();
        c.magic_defense = byte();
        c.physical_offense = byte();
        c.physical_defense = byte();
        c.move_delay = byte();
        c.attack_delay = byte();
        in >> c.object_head >> c.damage;
        c.in_use = byte();
        c.type = byte();
        c.dir = byte();
        c.row = byte();
        c.col = byte();
    }
    std::size_t count = 0;
    in >> count;
    objects_.assign(count, Ocb{});
    for (Ocb& o : objects_) {
        in >> o.next;
        o.row = byte();
        o.col = byte();
        o.level = byte();
        o.owner = byte();
        o.spec[0] = byte();
        o.spec[1] = byte();
        o.spec[2] = byte();
        o.type = byte();
        o.cls = byte();
        o.reveal = byte();
        o.magic_offense = byte();
        o.physical_offense = byte();
        in >> o.carrier;
    }
    for (int i = 0; i < Maze::kBytes; ++i) level_.maze.put(i / Maze::kSize, i % Maze::kSize, byte());
    Rng::Seed seeds[3] = {};
    for (auto& s : seeds) s = {byte(), byte(), byte()};
    level_.rng_before_spin = seeds[0];
    level_.rng_after_spin = seeds[1];
    level_.rng.set_seed(seeds[2]);
    in >> level_.spin_count;
    sched_.load_state(in, [this](const std::string& name) { return task_body(name); });
    std::size_t creatures = 0;
    in >> player_task_ >> hslow_task_ >> creatures;
    creature_tasks_.assign(creatures, 0);
    for (int& id : creature_tasks_) in >> id;
}

std::string Game::ram_image() const {
    std::ostringstream os;
    os << "DAGRAM 1\n";
    save_ram(os);
    return os.str();
}

void Game::restore_ram_image(const std::string& image) {
    std::istringstream in(image);
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "DAGRAM" || version != 1) std::abort();
    load_ram(in);
}

std::string Game::snapshot() const {
    std::ostringstream os;
    os << "DAGSNAP 1\n";
    save_ram(os);
    os << sched_.counters().total_jiffies << ' ' << sched_.halted() << ' ' << zflag_ << ' '
       << tape_name_.size() << ' ' << tape_name_ << "|\n";
    os << tapes_.size() << '\n';
    for (const auto& [name, image] : tapes_)
        os << name.size() << ' ' << name << '|' << image.size() << ' ' << image << '\n';
    os << incoming_damage_percent_ << '\n';
    return os.str();
}

void Game::restore_snapshot(const std::string& bytes) {
    std::istringstream in(bytes);
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "DAGSNAP" || version != 1) std::abort();
    load_ram(in);
    std::uint64_t total = 0;
    int halted = 0;
    std::size_t name_size = 0;
    in >> total >> halted >> zflag_ >> name_size;
    sched_.counters().total_jiffies = total;
    sched_.set_halted(halted != 0);
    in.get();
    tape_name_.assign(name_size, ' ');
    in.read(tape_name_.data(), static_cast<std::streamsize>(name_size));
    in.get();
    std::size_t tapes = 0;
    in >> tapes;
    tapes_.clear();
    for (std::size_t i = 0; i < tapes; ++i) {
        std::size_t n = 0, m = 0;
        in >> n;
        in.get();
        std::string name(n, ' ');
        in.read(name.data(), static_cast<std::streamsize>(n));
        in.get();
        in >> m;
        in.get();
        std::string image(m, ' ');
        in.read(image.data(), static_cast<std::streamsize>(m));
        tapes_.push_back({name, image});
    }
    int percent = 100;
    if (in >> percent) incoming_damage_percent_ = percent;
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
        {
            std::string first;
            if (!(ls >> first)) continue;
            if (first == "FUDGE") continue;  // harness line, not a keystroke
            std::istringstream back(first);
            if (!(back >> jiffy)) {
                error = "line " + std::to_string(lineno) + ": expected '<jiffy> <KEY>'";
                return {};
            }
            if (!(ls >> key)) {
                if (line.find_first_not_of(" \t") == std::string::npos) continue;
                error = "line " + std::to_string(lineno) + ": expected '<jiffy> <KEY>'";
                return {};
            }
            if (key == "FUDGE") continue;  // "<jiffy> FUDGE ..."
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

std::vector<Game::HarnessEvent> parse_harness(const std::string& text, std::string& error) {
    std::vector<Game::HarnessEvent> out;
    std::istringstream in(text);
    std::string line;
    int lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        const auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream ls(line);
        std::string a, b, c;
        if (!(ls >> a)) continue;
        std::uint64_t jiffy = 0;
        if (a != "FUDGE") {
            std::istringstream num(a);
            if (!(num >> jiffy) || !(ls >> a) || a != "FUDGE") continue;
        }
        if (!(ls >> b)) {
            error = "line " + std::to_string(lineno) + ": FUDGE needs a verb";
            return {};
        }
        Game::HarnessEvent ev;
        ev.jiffy = jiffy;
        if (b == "incoming") {
            if (!(ls >> ev.percent)) {
                error = "line " + std::to_string(lineno) + ": FUDGE incoming needs a percent";
                return {};
            }
            ev.kind = Game::HarnessFudge::Incoming;
        } else if (b == "rest") {
            ev.kind = Game::HarnessFudge::Rest;
        } else {
            error = "line " + std::to_string(lineno) + ": unknown FUDGE '" + b + "'";
            return {};
        }
        (void)c;
        out.push_back(ev);
    }
    std::sort(out.begin(), out.end(),
              [](const Game::HarnessEvent& a, const Game::HarnessEvent& b) {
                  return a.jiffy < b.jiffy;
              });
    return out;
}

}  // namespace dag
