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

void Scheduler::reset_tasks() {
    tasks_.clear();
    ready_.clear();
    for (auto& list : countdown_) list.clear();
    restart_ = true;
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
    if (irq_hook_) {
        in_irq_ = true;
        irq_hook_();
        in_irq_ = false;
    }

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
    restart_ = false;                             // SCHED: CLR RSTART
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
            running_ = t.name;
            const TaskResult r = t.run();
            running_.clear();
            if (restart_) {
                // `t` no longer exists. Every new TCB is unrun this jiffy.
                restart_ = false;
                ran.assign(tasks_.size(), 0);
                break;
            }
            if (r.queue != Queue::Sched) {
                erase_id(ready_, id);
                if (r.queue == Queue::Null) t.alive = false;
                else requeue(id, r);
            }
            if (lap_hook_) {
                // The hook may replace every task; nothing below may touch them.
                const std::function<void()> hook = std::move(lap_hook_);
                lap_hook_ = nullptr;
                hook();
                return;
            }
        }
    }
}

void KeyboardBuffer::save_state(std::ostream& out) const {
    for (const std::uint8_t b : buf_) out << static_cast<int>(b) << ' ';
    out << static_cast<int>(head_) << ' ' << static_cast<int>(tail_) << ' ' << puts_ << '\n';
}

void KeyboardBuffer::load_state(std::istream& in) {
    int v = 0;
    for (std::uint8_t& b : buf_) {
        in >> v;
        b = static_cast<std::uint8_t>(v);
    }
    in >> v;
    head_ = static_cast<std::uint8_t>(v);
    in >> v;
    tail_ = static_cast<std::uint8_t>(v);
    in >> puts_;
}

void Scheduler::save_state(std::ostream& out) const {
    out << tasks_.size() << '\n';
    for (const Task& t : tasks_) {
        out << t.name << ' ' << static_cast<int>(t.queue) << ' ' << static_cast<int>(t.countdown)
            << ' ' << (t.alive ? 1 : 0) << '\n';
    }
    out << ready_.size();
    for (const int id : ready_) out << ' ' << id;
    out << '\n';
    for (const auto& list : countdown_) {
        out << list.size();
        for (const int id : list) out << ' ' << id;
        out << '\n';
    }
    const Counters& c = counters_;
    out << static_cast<int>(c.jiffy) << ' ' << static_cast<int>(c.tenth) << ' '
        << static_cast<int>(c.second) << ' ' << static_cast<int>(c.minute) << ' '
        << static_cast<int>(c.hour) << ' ' << static_cast<int>(c.day) << '\n';
    keyboard_.save_state(out);
    out << (sleep_ ? 1 : 0) << ' ' << (faint_ ? 1 : 0) << '\n';
}

void Scheduler::load_state(std::istream& in, const Resolver& resolve) {
    std::size_t n = 0;
    in >> n;
    tasks_.clear();
    for (std::size_t i = 0; i < n; ++i) {
        Task t;
        int queue = 0, countdown = 0, alive = 0;
        in >> t.name >> queue >> countdown >> alive;
        t.queue = static_cast<Queue>(queue);
        t.countdown = static_cast<std::uint8_t>(countdown);
        t.alive = alive != 0;
        t.run = resolve(t.name);
        tasks_.push_back(std::move(t));
    }
    auto read_list = [&in](std::vector<int>& list) {
        std::size_t count = 0;
        in >> count;
        list.assign(count, 0);
        for (int& id : list) in >> id;
    };
    read_list(ready_);
    for (auto& list : countdown_) read_list(list);
    int v[6] = {};
    for (int& x : v) in >> x;
    counters_.jiffy = static_cast<std::uint8_t>(v[0]);
    counters_.tenth = static_cast<std::uint8_t>(v[1]);
    counters_.second = static_cast<std::uint8_t>(v[2]);
    counters_.minute = static_cast<std::uint8_t>(v[3]);
    counters_.hour = static_cast<std::uint8_t>(v[4]);
    counters_.day = static_cast<std::uint8_t>(v[5]);
    keyboard_.load_state(in);
    int sleep = 0, faint = 0;
    in >> sleep >> faint;
    sleep_ = sleep != 0;
    faint_ = faint != 0;
}

}  // namespace dag
