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
    tasks_.push_back(std::move(t));
    if (ready_now) ready_.push_back(id);          // QUEADD onto SCDQUE
    return id;
}

void Scheduler::scan_queue(Queue q) {
    if (sleep_) return;                           // QUESCN: TST SLEEP
    // Traverse in list order; decrement each countdown; append expired tasks to
    // the ready list in the order encountered.
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        Task& t = tasks_[i];
        if (!t.alive || t.queue != q) continue;
        if (t.countdown == 0) continue;           // defensive; 0 never queued here
        --t.countdown;
        if (t.countdown == 0) {
            t.queue = Queue::Sched;
            ready_.push_back(static_cast<int>(i));
            if (trace_) trace_("QUEUE ready " + t.name);
        }
    }
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
}

void Scheduler::run_ready_pass() {
    // Snapshot the ready list: tasks made ready by this pass run on the next one.
    const std::vector<int> pass = ready_;
    for (const int id : pass) {
        Task& t = tasks_[static_cast<std::size_t>(id)];
        if (!t.alive) continue;
        if (trace_) trace_("TASK run " + t.name);
        const TaskResult r = t.run();
        if (r.queue == Queue::Sched) continue;    // stays in SCDQUE
        ready_.erase(std::remove(ready_.begin(), ready_.end(), id), ready_.end());
        requeue(id, r);
    }
}

}  // namespace dag
