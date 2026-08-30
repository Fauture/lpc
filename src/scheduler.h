#pragma once
// ---------------------------------------------------------------------------
// lpcvm cooperative scheduler (single-thread event loop, faithful to the
// original magic_Linux32x vm_scheduler).
//
// The original driver runs every LPC process (PCB) on ONE thread in a
// round-robin event loop (vm_mainLoop -> vm_scheduleProcessInternal):
//   - a ready list (vm_pActiveProcess, doubly-linked circular list)
//   - a timer list (vm_pTimerList, sorted by timeoutTicks)
//   - an invoke list (vm_pInvokeList, immediate calls)
//   - pending tables (processes blocked on objects / queues / events /
//     sockets), resumed by vm_resumeFromObject / vm_resumePCBFromService /
//     vm_resumePCBFromSelect.
//
// This port models each LPC execution context (object VM run, create_thread,
// timer callback, closure invocation) as a cooperative Process backed by a
// Windows Fiber.  A blocking efun (sleep / wait_event / receive_from_queue /
// network recv / mysql query) calls Scheduler::yield() with a resume
// condition; the scheduler switches to the next ready process and resumes the
// blocked one when its condition is satisfied.  All execution stays on one OS
// thread, so no mutexes / atomics / cross-thread queues are needed.
// ---------------------------------------------------------------------------
#include <windows.h>

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lpc {

// Resume conditions for a pended process.  Mirrors the original's pend-on
// object / queue / event / socket / timer semantics.
enum class PendKind {
    None = 0,
    Timeout,    // resume when the deadline passes (sleep / timer)
    Queue,      // resume when a message arrives on queue `key`
    Event,      // resume when event `key` gets raised bit(s) from `mask`
    Socket,     // resume when fd `key` becomes readable/writable
    Object,     // resume when `key` (an object or service) completes
    Service,    // resume when a dispatched service closure finishes
};

struct ResumeCond {
    PendKind kind = PendKind::None;
    int64_t key = 0;          // queue id / event id / socket fd / object key
    uint32_t mask = 0;        // event bits (Event)
    int64_t deadlineMs = -1;  // absolute ms (Timeout) or -1 = forever
    int64_t result = 0;       // result written back on resume (e.g. recv len)
    std::vector<unsigned char> data;   // data written back on resume (recv)
};

// A cooperative process: one LPC execution context on its own fiber.
struct Process {
    int64_t id = 0;
    std::string name;                 // "tthreadsrv1", "保存彩票信息", ...
    void* fiber = nullptr;            // Windows Fiber handle (null = main)
    enum class Status { Dead, Ready, Running, Pended } status = Status::Dead;
    ResumeCond pend;                  // why we are pended
    int64_t tickBudget = 0;           // runTicks remaining this round
    bool forceRelinquish = false;     // must yield after current efun
    std::function<void()> entry;      // fiber body (runs execute / callback)
    void* ctx = nullptr;              // user context (e.g. OriginalVm*)
    // --- timers (sleep) ---
    int64_t timerId = 0;              // g_timerSeq value; 0 = no timer
    int64_t wakeAtMs = 0;             // absolute ms when the sleep timer fires
    // --- queue/event pending linkage (simple lists per key) ---
    std::vector<std::shared_ptr<Process>>::iterator listIt;  // valid while pended
    bool inList = false;
};

class Scheduler {
public:
    static Scheduler& instance();

    // ---- lifecycle ----
    void init();                       // ConvertThreadToFiber (must run on the scheduler thread)
    void run();                        // event loop until stop() (blocks)
    void stop();
    bool running() const { return running_; }

    // ---- process management ----
    // Create a cooperative process and put it on the ready list.  `entry` runs
    // on the new fiber; it should call yield() when it blocks and return when
    // the process is done.
    std::shared_ptr<Process> spawn(std::string name, std::function<void()> entry, void* ctx = nullptr);
    void kill(const std::shared_ptr<Process>& p);

    // ---- cooperative yield (called from inside a process fiber) ----
    // Suspend the CURRENT process with `cond`.  Returns when the condition is
    // satisfied (control returns to the fiber).  Never returns if the process
    // is killed.
    void yield(const ResumeCond& cond);
    void relinquish();                 // yield without a condition (round-robin)

    // ---- resume from the scheduler side ----
    void resumeQueue(int64_t qid, int64_t result = 0);
    void resumeEvent(int64_t key, uint32_t raisedBits);
    void resumeSocket(int64_t fd, int what = 0);
    void resumeObject(int64_t key, int64_t result = 0);
    void resumeService(int64_t result = 0);
    void resumeProcess(const std::shared_ptr<Process>& p);

    // ---- timers (sleep) ----
    void scheduleTimer(const std::shared_ptr<Process>& p, int64_t afterMs);

    // ---- scheduler internals ----
    // Current process on this thread (null on the main loop).
    std::shared_ptr<Process> current() const { return current_; }
    void setCurrent(const std::shared_ptr<Process>& p) { current_ = p; }
    void* mainFiber() const { return mainFiber_; }
    bool hasWork() const;

    // Run one scheduling round (used by run() and by tests).  Returns false
    // when no process is runnable and no timer is pending.
    bool runOnce();

private:
    Scheduler() = default;
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void switchTo(std::shared_ptr<Process> p);   // switch main->p (or back)
    void processTimers();                         // fire due sleep timers
    void processReadyQueue();                     // run one ready process
    int64_t nextDeadlineMs() const;               // min wakeAtMs in ready/timer

    // ---- state ----
    bool inited_ = false;
    bool running_ = false;
    void* mainFiber_ = nullptr;
    std::shared_ptr<Process> current_;

    std::deque<std::shared_ptr<Process>> ready_;        // ready queue (round-robin)
    std::vector<std::shared_ptr<Process>> all_;         // all live processes (id lookup)
    std::unordered_map<int64_t, std::shared_ptr<Process>> byId_;
    int64_t nextId_ = 1;

    // sleep timers: processes waiting for a deadline (sorted by wakeAtMs)
    std::vector<std::shared_ptr<Process>> timers_;
    int64_t nextTimerId_ = 1;

    // pending tables: key -> list of processes blocked on that key
    std::unordered_map<int64_t, std::vector<std::shared_ptr<Process>>> pendQueue_;
    std::unordered_map<int64_t, std::vector<std::shared_ptr<Process>>> pendEvent_;
    std::unordered_map<int64_t, std::vector<std::shared_ptr<Process>>> pendSocket_;
    std::unordered_map<int64_t, std::vector<std::shared_ptr<Process>>> pendObject_;
    std::vector<std::shared_ptr<Process>> pendService_;
};

// Convenience: current process id (0 on the main loop).
int64_t schedulerCurrentProcessId();

} // namespace lpc
