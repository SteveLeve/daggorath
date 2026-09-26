// Daggorath Core — 60 Hz clock, countdown queues and the cooperative scheduler.
// Source: COMMON.ASM (CLOCK, QUESCN, SCHED, QUEADD, QUERMV, ROLTAB, KBDPUT,
//         KBDGET); CD.ASM (queue codes, TCB layout); COMDAT.ASM (TCBDAT);
//         ONCE.ASM (SYSTCB).
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace dag {

// Queue codes, exactly the original byte values (CD.ASM).
enum class Queue : std::uint8_t {
    Null = 0, Jiffy = 2, Tenth = 4, Second = 6, Minute = 8, Hour = 10, Sched = 12
};

// ROLTAB (COMMON.ASM): jiffy 6, tenth 10, second 60, minute 60, hour 24.
inline constexpr std::array<std::uint8_t, 5> kRolTab{6, 10, 60, 60, 24};

struct Counters {
    std::uint8_t jiffy = 0, tenth = 0, second = 0, minute = 0, hour = 0, day = 0;
    std::uint64_t total_jiffies = 0;    // monotonic, for traces only
    std::string to_string() const;
};

// A task's return value: the queue it wants next, and its countdown in that
// queue's time unit (SCHED$ n,Q.xxx).
struct TaskResult {
    Queue queue = Queue::Sched;
    std::uint8_t countdown = 0;
};

struct Task {
    std::string name;
    std::function<TaskResult()> run;
    Queue queue = Queue::Sched;
    std::uint8_t countdown = 0;
    bool alive = true;
};

// 32-byte circular keyboard buffer. The original performs NO overflow check
// (COMMON.ASM KBDPUT), so a burst longer than 32 characters silently overwrites
// and can make the buffer look empty. That behaviour is reproduced here.
class KeyboardBuffer {
public:
    void put(std::uint8_t ch) {
        buf_[tail_] = ch;
        tail_ = static_cast<std::uint8_t>((tail_ + 1) & 0x1F);
        ++puts_;
    }
    std::uint8_t get() {                   // 0 means "empty"
        if (head_ == tail_) return 0;
        const std::uint8_t ch = buf_[head_];
        head_ = static_cast<std::uint8_t>((head_ + 1) & 0x1F);
        return ch;
    }
    std::uint64_t puts() const { return puts_; }

private:
    std::array<std::uint8_t, 32> buf_{};
    std::uint8_t head_ = 0, tail_ = 0;
    std::uint64_t puts_ = 0;
};

// The scheduler holds the countdown queues and the ready (SCDQUE) list.
// Insertion is FIFO append (QUEADD walks to the tail), so simultaneous
// expirations enter the ready list in countdown-queue list order, and within a
// queue in the order the tasks were added to it.
class Scheduler {
public:
    using TraceFn = std::function<void(const std::string&)>;

    int add(Task t);                     // returns a task id
    Task& task(int id) { return tasks_[static_cast<std::size_t>(id)]; }

    void set_trace(TraceFn fn) { trace_ = std::move(fn); }
    void set_sleep(bool s) { sleep_ = s; }
    void set_faint(bool f) { faint_ = f; }
    bool faint() const { return faint_; }
    // DEATH's BRA *: the CPU never returns to CLOCK. Later tasks in this
    // jiffy do not run, and later interrupts are not taken.
    void halt() { halted_ = true; }
    bool halted() const { return halted_; }

    Counters& counters() { return counters_; }
    const Counters& counters() const { return counters_; }
    KeyboardBuffer& keyboard() { return keyboard_; }

    // One 1/60 s interrupt, in CLOCK's order:
    //   1. scan the jiffy queue (QUESCN on JIFQUE)
    //   2. bump JIFFY..HOUR, and at each rollover scan that queue
    //   3. poll the keyboard unless fainted
    void interrupt(const std::vector<std::uint8_t>& keys_this_jiffy);

    // Advance the counter chain `n` times without scanning queues, polling the
    // keyboard, or counting a scheduler-entry jiffy. The level-0 build runs
    // CLOCK before SCHED starts, so those interrupts move the clock only.
    void advance_clock_counters(std::uint32_t n);

    // One foreground jiffy of SCHED (ADR-0002, option 3). Walk SCDQUE in order.
    // Repeat the lap while a task queued onto SCDQUE during this jiffy has not
    // run yet. A task that returns Queue::Sched has had its one run this jiffy
    // and stays linked for the next jiffy. Queue::Null removes the task.
    void run_ready_pass();

    // Drop a task from SCDQUE and the countdown lists. NEWLVL zeros creature
    // blocks; their previous CMOVE tasks must not keep running.
    void retire(int id);

    const std::vector<int>& ready() const { return ready_; }

    // SYSTCB: clear every queue and TCB, then flag a SCHED restart (RSTART).
    // The task that caused it is not requeued: SCHED tests RSTART before the
    // disposition, and its TCB is gone. The lap then starts again at the head
    // of SCDQUE in the same jiffy.
    void reset_tasks();

private:
    void scan_queue(Queue q);
    void requeue(int id, TaskResult r);
    void bump_counters(bool scan_rollover_queues);
    void erase_id(std::vector<int>& ids, int id);

    std::vector<Task> tasks_;
    std::vector<int> ready_;             // SCDQUE, in order
    // QUEADD lists for the countdown queues, keyed by the queue's byte code.
    std::array<std::vector<int>, 13> countdown_{};
    Counters counters_;
    KeyboardBuffer keyboard_;
    TraceFn trace_;
    bool sleep_ = false;
    bool faint_ = false;
    bool halted_ = false;
    bool restart_ = false;                // RSTART
};

}  // namespace dag
