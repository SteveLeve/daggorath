#include "daggorath/scheduler.hpp"

#include <algorithm>
#include <sstream>

namespace dag {

std::string Counters::to_string() const {
    std::ostringstream os;
    os << static_cast<int>(hour) << ':' << static_cast<int>(minute) << ':'
       << static_cast<int>(second) << '.' << static_cast<int>(tenth)
       << '.' << static_cast<int>(jiffy);
    return os.str();
}

int Scheduler::add(Task t) {
    const int id = static_cast<int>(tasks_.size());
    const bool ready_now = (t.queue == Queue::Sched);
    const Queue q = t.queue;
    tasks_.push_back(std::move(t));
    if (ready_now) ready_.push_back(id);          // QUEADD onto SCDQUE
    else countdown_[static_cast<std::size_t>(q)].push_back(id);
    return id;
}

void Scheduler::erase_id(std::vector<int>& ids, int id) {
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
}

void Scheduler::retire(int id) {
    if (id < 0 || static_cast<std::size_t>(id) >= tasks_.size()) return;
    tasks_[static_cast<std::size_t>(id)].alive = false;
    erase_id(ready_, id);
    for (auto& list : countdown_) erase_id(list, id);
}

void Scheduler::scan_queue(Queue q) {
    if (sleep_) return;                           // QUESCN: TST SLEEP
    // QUEADD order, not allocation order. A task readied here is appended to
    // SCDQUE and unlinked from this countdown list (QUERMV + QUEADD).
    std::vector<int>& list = countdown_[static_cast<std::size_t>(q)];
    std::vector<int> stay;
    stay.reserve(list.size());
    for (const int id : list) {
        Task& t = tasks_[static_cast<std::size_t>(id)];
        if (!t.alive || t.queue != q) continue;
        if (t.countdown == 0) continue;
        --t.countdown;
        if (t.countdown == 0) {
            t.queue = Queue::Sched;
            ready_.push_back(id);
            if (trace_) trace_("QUEUE ready " + t.name);
        } else {
            stay.push_back(id);
        }
    }
    list.swap(stay);
}

void Scheduler::bump_counters(bool scan_rollover_queues) {
    // CLK42: bump JIFFY, then each higher counter on rollover.
    std::uint8_t* const c[5] = {&counters_.jiffy, &counters_.tenth, &counters_.second,
                                &counters_.minute, &counters_.hour};
    const Queue qs[5] = {Queue::Tenth, Queue::Second, Queue::Minute, Queue::Hour,
                         Queue::Hour};
    for (int level = 0; level < 5; ++level) {
        ++(*c[level]);
        if (*c[level] < kRolTab[static_cast<std::size_t>(level)]) break;
        *c[level] = 0;
        if (level == 4) { ++counters_.day; break; }
        if (scan_rollover_queues) scan_queue(qs[level]);
    }
}

void Scheduler::advance_clock_counters(std::uint32_t n) {
    for (std::uint32_t i = 0; i < n; ++i) bump_counters(false);
}

void Scheduler::interrupt(const std::vector<std::uint8_t>& keys_this_jiffy) {
    if (halted_) return;
    ++counters_.total_jiffies;

    scan_queue(Queue::Jiffy);                     // CLK40: always the jiffy queue
    bump_counters(true);

    // CLK50: keyboard polling is skipped entirely while fainted.
    if (!faint_) {
        for (const std::uint8_t k : keys_this_jiffy) keyboard_.put(k);
    }
}

void Scheduler::requeue(int id, TaskResult r) {
    Task& t = tasks_[static_cast<std::size_t>(id)];
    t.queue = r.queue;
    t.countdown = r.countdown;
    if (r.queue == Queue::Null || r.queue == Queue::Sched) return;
    countdown_[static_cast<std::size_t>(r.queue)].push_back(id);
}

void Scheduler::run_ready_pass() {
    // ADR-0002 option 3. SCHED restarts at the head when the tail is reached
    // (COMMON.ASM SCHED). A task that returns Q.SCD stays linked and would run
    // again on the next source lap; this jiffy gives it one run. A task another
    // task queues onto SCDQUE during the jiffy runs before the jiffy ends.
    std::vector<char> ran(tasks_.size(), 0);
    for (;;) {
        std::vector<int> pass;
        for (const int id : ready_) {
            if (static_cast<std::size_t>(id) >= ran.size()) ran.resize(tasks_.size(), 0);
            if (!ran[static_cast<std::size_t>(id)] && tasks_[static_cast<std::size_t>(id)].alive)
                pass.push_back(id);
        }
        if (pass.empty()) break;
        for (const int id : pass) {
            if (static_cast<std::size_t>(id) >= ran.size()) ran.resize(tasks_.size(), 0);
            ran[static_cast<std::size_t>(id)] = 1;
            Task& t = tasks_[static_cast<std::size_t>(id)];
            if (!t.alive || halted_) continue;
            if (trace_) trace_("TASK run " + t.name);
            const TaskResult r = t.run();
            if (r.queue == Queue::Sched) continue;
            erase_id(ready_, id);
            if (r.queue == Queue::Null) {
                t.alive = false;
                continue;
            }
            requeue(id, r);
        }
    }
}

}  // namespace dag
