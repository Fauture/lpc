// ===========================================================================
// vmcore/sched.cpp — PCB 调度器 (原版 vm_mainLoop / vm_pendCurrentProcess 模型)
//
// 全部等待/唤醒/定时/互斥挂在 PCB 上。旧核的 parkedVm/callerVm/callers_/
// runStates_/chainSeq/tlsChainTid 双轨在这里没有对应物: 跨对象阻塞调用下
// park 的就是当前 PCB (被调函数帧在它的栈上), 唤醒后弹帧回调用点。
//
// 搬运的已实证语义:
//   * parkCurrent 的两个 LOST-WAKE GUARD (注册后复查队列/事件位 — 发送方
//     在"查空-登记"窗口内到达的唤醒不得丢失)。
//   * 重新 park 时取消旧唤醒定时 (陈旧定时不得唤醒新 park)。
//   * 定时事件: 事件先到 → 丢陈旧定时; 定时先到 → 从等待表移除 + 按类型
//     恢复 (-202 mutex / -501 event)。
//   * wakeEvent 掩码匹配 + 消费位清除 (vm_f_raise_event); mask 0 匹配任何
//     raise (不得搁浅)。
//   * wake 去重 (inReady/done), 结束进程永不被陈旧定时重排。
//   * runProcess: 进程的 tid 对其整条链恒定 (get_thread_id 语义), 嵌套运行
//     结束后恢复前值 — xlibd args_stack 按 tid 键控, 嵌套泄漏 tid 就是
//     「时间标记过于古老」孤儿 bug。
// =========================================================================//
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "call.h"
#include "driver_model.h"
#include "game_service.h"   // gameServiceFlushPendingOutputs (发送聚合刷出)
#include "interp.h"
#include "object.h"
#include "vmcore.h"
#include "trace.h"

#include "builtins.h"
#include "original_vm.h"   // chain probe / liveness / flow probe (driver side)

namespace lpc {
namespace vmcore {

// [性能] CTRACE 环境开关启动时缓存: wakeEvent/park 每次都 getenv 会在
// boot 高频事件路径上线性扫描环境块+锁。
static const bool g_ctraceOn = ::getenv("LPCVM_CTRACE") != nullptr;

using lpc::vmchainBegin;
using lpc::vmchainEnd;
using lpc::vmchainRun;
using lpc::vmchainRunExit;
using lpc::vmchainPark;
using lpc::cmdActiveAttach;
using lpc::cmdActiveDetach;

int64_t msSinceStart() {
    static const std::chrono::steady_clock::time_point t0 =
        std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

// ---------------------------------------------------------------------------
struct Scheduler::Impl {
    struct TimerEntry2 {
        std::shared_ptr<VmPcb> p;
        std::shared_ptr<std::atomic<bool>> cancelled;
    };

    std::mutex mtx;
    std::condition_variable cv;
    std::deque<std::shared_ptr<VmPcb>> ready;
    std::multimap<int64_t, TimerEntry2> timers;
    std::map<std::string, std::deque<std::shared_ptr<VmPcb>>> eventWaiters;
    std::map<std::string, std::deque<std::shared_ptr<VmPcb>>> queueWaiters;
    std::map<std::string, std::deque<std::shared_ptr<VmPcb>>> mutexWaiters;
    std::map<int, std::deque<std::shared_ptr<VmPcb>>> sockWaiters;
    std::map<int, std::function<bool(unsigned int)>> serviceSockets;
    // Process-level named mutexes (PCB ownership — vm_f_take_mutex 0x80f16cc).
    struct MutexRec {
        int level = 0;
        std::shared_ptr<VmPcb> owner;
    };
    std::map<std::string, MutexRec> procMutexes;

    TidPool tidPool;
    // PCBs spawned while the tid pool is empty — they wait here until a
    // running process finishes (release) and one tid is handed over.
    std::deque<std::shared_ptr<VmPcb>> tidStarved;
    // Watchdog (CTRACE): tid -> (ms timestamp, pcb) while its runProcess runs.
    std::map<int64_t, std::pair<int64_t, std::shared_ptr<VmPcb>>> runSince;
    std::thread::id schedThreadId{};
    std::shared_ptr<VmPcb> currentProcess;   // snapshot/restore around nesting
    bool shutdown = false;
    int64_t lastSockPollMs = 0;

    int64_t nowMs() const { return msSinceStart(); }

    // -- waiter-map purge: a re-park must not leave its previous entries --
    void purgeWaitersLocked(const std::shared_ptr<VmPcb>& p) {
        auto dropFrom = [&](std::map<std::string, std::deque<std::shared_ptr<VmPcb>>>& m) {
            for (auto it = m.begin(); it != m.end();) {
                auto& dq = it->second;
                for (auto q = dq.begin(); q != dq.end();) {
                    if (*q == p) q = dq.erase(q);
                    else ++q;
                }
                if (dq.empty()) it = m.erase(it);
                else ++it;
            }
        };
        dropFrom(eventWaiters);
        dropFrom(queueWaiters);
        dropFrom(mutexWaiters);
        auto dropSock = [&](std::map<int, std::deque<std::shared_ptr<VmPcb>>>& m) {
            for (auto it = m.begin(); it != m.end();) {
                auto& dq = it->second;
                for (auto q = dq.begin(); q != dq.end();) {
                    if (*q == p) q = dq.erase(q);
                    else ++q;
                }
                if (dq.empty()) it = m.erase(it);
                else ++it;
            }
        };
        dropSock(sockWaiters);
    }

    void wakeLocked(const std::shared_ptr<VmPcb>& p, bool front) {
        if (p->inReady || p->state == VmPcb::State::Done) return;
        // LPCVM_PARKDBG(热布防): 只记录停泊 >8ms 的唤醒——一键点化/羽化
        // 每件物品之间有 ~21ms/64ms 的静默空档且 CPU 空闲,需要看清链在
        // 等什么(kind/事件/时长)。探针关时仅 1 次原子读。
        {
            static const std::atomic<bool>& pk = dynProbeFlag("LPCVM_PARKDBG");
            if (pk.load(std::memory_order_relaxed) && p->parkAtMs) {
                const int64_t dur = nowMs() - p->parkAtMs;
                if (dur > 8)
                    std::cerr << "[park-slow] t=" << nowMs() << "ms tid=" << p->tid
                              << " dur=" << dur << "ms kind=" << p->cond.kind
                              << " ev=" << p->cond.ename
                              << " entry=" << p->entry
                              << " frames=[" << p->frameChainText(4) << "]\n";
            }
        }
        p->parkAtMs = 0;
        // 原版语义（vm_removeTimer）：事件唤醒时必须作废该 pcb 挂着的
        // "超时唤醒"定时器。kind=3/5/6 的 park 双注册 eventWaiters+timers——
        // raise 唤醒后超时条目仍留在 timers 里，未来到点时把**已重新
        // park 的 pcb** 再次塞进 ready：同一 pcb 双重执行/悬挂，master
        // boot 曾因此在 statued NPC 存档阶段（高频 park→wake→park）反复
        // 无声蒸发（18577 constructs 处，无任何日志）。
        if (p->parkTimerCancelled) *p->parkTimerCancelled = true;
        p->parkTimerCancelled.reset();
        p->inReady = true;
        p->state = VmPcb::State::Ready;
        if (front) ready.push_front(p);
        else ready.push_back(p);
        // 新 pcb 入队必须唤醒空闲中的调度线程: 空闲路径是 cv.wait_for
        // (此前为不可中断的 sleep_for(20ms)),不 notify 的话每次停泊->
        // 唤醒都要白等一个完整量子 —— 一键点化/羽化每件物品停泊 3 次,
        // 实测 6 件从点击到结果 435ms(62ms/件); 原版主循环 select(timeout=0)
        // 立即调度,没有这笔开销。
        cv.notify_all();
    }
};

Scheduler::Scheduler() : impl_(new Impl) {}
Scheduler::~Scheduler() = default;
Scheduler& Scheduler::instance() {
    static Scheduler s;
    return s;
}

bool Scheduler::isSchedulerThread() const {
    return std::this_thread::get_id() == impl_->schedThreadId;
}
int64_t Scheduler::currentProcessTid() const {
    auto p = impl_->currentProcess;
    return p ? p->tid : -1;
}

// ---------------------------------------------------------------------------
// spawn / delayed
// ---------------------------------------------------------------------------
static std::shared_ptr<VmPcb> makePcb(Scheduler::Impl& im, VmObject* obj,
                                      const std::string& entry,
                                      std::vector<Value> args) {
    auto p = std::make_shared<VmPcb>();
    p->obj = obj;
    p->entry = entry;
    p->args = std::move(args);
    p->tid = im.tidPool.alloc();   // -1 = pool empty; the run loop queues it
    // [runTicks] 原版驱动对所有进程跑 runTicks 时间片 (select(timeout=0) 主循
    // 环每量子回泵 socket) —— 用户连点/停止自动羽化的命令能在量子边界被
    // 处理, 不必等当前命令整条跑完 (一键羽化连续 400ms 不让出时, 连点
    // 明显卡顿、停止自动要多点一次)。2 万条 ≈ 20-30ms 量子, 与原版体感
    // 对齐; nested 执行不抛 TimeSliceSignal (interp 内已有保护)。
    p->yieldEvery = 20000;
    p->yieldCountdown = p->yieldEvery;
    // Continuation chains (create_thread / timers / dispatched closures
    // spawned BY a running command) join that command's connection so the
    // per-connection gate counts them (vmchainTagLive semantics).
    if (p->clientNo < 0) {
        VmPcb* cur = currentPcb();
        if (cur && cur->clientNo >= 0) p->clientNo = cur->clientNo;
    }
    return p;
}

VmPcb* Scheduler::spawn(VmObject* obj, const std::string& entry,
                        std::vector<Value> args) {
    auto p = makePcb(*impl_, obj, entry, std::move(args));
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
    return p.get();
}

VmPcb* Scheduler::spawnEfunThread(VmObject* owner,
                                  const lpc::Ref<struct ClosureValue>& cl) {
    auto p = makePcb(*impl_, owner, "<efun-thread>", {});
    p->threadClosure = cl;
    if (g_ctraceOn) {
        std::cerr << "[spawn-efun-thread] tid=" << p->tid
                  << " fn=" << cl->functionName << "\n";
        std::cerr.flush();
    }
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
    return p.get();
}

VmPcb* Scheduler::spawnTelnet(VmObject* obj, const std::string& entry,
                              std::vector<Value> args, int telnetCno) {
    auto p = makePcb(*impl_, obj, entry, std::move(args));
    p->telnetCno = telnetCno;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
    return p.get();
}

VmPcb* Scheduler::spawnDelayed(VmObject* obj, const std::string& entry,
                               std::vector<Value> args, int64_t atMs,
                               std::shared_ptr<std::atomic<bool>> cancelled) {
    auto p = makePcb(*impl_, obj, entry, std::move(args));
    // 原版 max_threads = 同时**运行**的进程上限: 睡眠中的 call_out 不占 tid,
    // tid 在任务真正被调度运行时才租借 (下面的 ready 取用路径对 tid<0 就是
    // 这么做的)。此前 makePcb 创建即分配, 数百个周期性 call_out
    // (refresh_top_list/init_btimer/task_timer_execute/boss 系列, PCDUMP 实测
    // 343 个 pending) 长期占用池 tid, ~85 分钟后 1024 全部耗尽, 此后每条
    // 新命令都要等其他链退场腾位 -> 挂机数小时后间歇性 4-5 秒卡顿。
    if (p->tid >= 0) {
        impl_->tidPool.release(p->tid);
        p->tid = -1;
    }
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        p->state = VmPcb::State::Parked;
        impl_->timers.emplace(atMs, Impl::TimerEntry2{p, std::move(cancelled)});
    }
    impl_->cv.notify_one();
    return p.get();
}

VmPcb* Scheduler::spawnDelayedEfun(VmObject* obj,
                                   const lpc::Ref<struct ClosureValue>& cl,
                                   int64_t atMs,
                                   std::shared_ptr<std::atomic<bool>> cancelled) {
    if (!cl) return nullptr;
    auto p = makePcb(*impl_, obj, "<efun-timer>", {});
    // 同 spawnDelayed: 睡眠中的 efun 定时不占池 tid, 运行时再租借。
    if (p->tid >= 0) {
        impl_->tidPool.release(p->tid);
        p->tid = -1;
    }
    p->threadClosure = cl;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        p->state = VmPcb::State::Parked;
        impl_->timers.emplace(atMs, Impl::TimerEntry2{p, std::move(cancelled)});
    }
    impl_->cv.notify_one();
    return p.get();
}

VmPcb* Scheduler::spawnConstruct(std::shared_ptr<ConstructPlan> plan) {
    if (!plan || !plan->obj) return nullptr;
    auto p = makePcb(*impl_, plan->obj, "construct", {});
    // 原版构造 inline 在发起者进程上跑 (vm_f_construct_object2): 不占新 tid,
    // 借发起者的 tid — 发起者此刻 park 等构造完成, 同 tid 串行无冲突;
    // 构造里的 get_thread_id 返回发起者 tid (原版行为)。没有发起者
    // (驱动层直调) 才用 makePcb 分配的池 tid。
    VmPcb* cur = currentPcb();
    if (cur && cur->tid >= 0) {
        p->tid = cur->tid;
        p->borrowedTid = true;
    }
    p->construct = std::move(plan);
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
    return p.get();
}

// ---------------------------------------------------------------------------
// wake sources
// ---------------------------------------------------------------------------
void Scheduler::wake(VmPcb* pcb) {
    auto p = pcb->shared_from_this();   // PCBs are heap-owned via shared_ptr
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
}
void Scheduler::wakeFront(VmPcb* pcb) {
    auto p = pcb->shared_from_this();
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, true);
    }
    impl_->cv.notify_one();
}
void Scheduler::requeue(VmPcb* pcb) {
    wake(pcb);   // time-slice yield: back to the tail
}

void Scheduler::wakeQueue(const std::string& qkey) {
    std::deque<std::shared_ptr<VmPcb>> hits;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->queueWaiters.find(qkey);
        if (it != impl_->queueWaiters.end()) {
            hits = std::move(it->second);
            impl_->queueWaiters.erase(it);
        }
        for (auto& p : hits) impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_all();
}

void Scheduler::wakeQueueOne(const std::string& qkey) {
    // The original (vm_f_send_to_queue 0x80f2d94): ONE element wakes ONE
    // waiter — pQueue->pendObject.pPendingPCB is a single PCB, the element
    // is RESERVED for it (pickingElement++) and its resume hook
    // (vm_acquiredValueFromQueue) takes exactly that element.  Waking every
    // waiter made the losers resume with no message ("Bad message in
    // THREAD_D, expected function got (undefined)").  Deliver the element
    // AT SEND TIME: pop it from the driver queue and carry it on the PCB,
    // so no other receiver can steal it before the hook runs.
    if (qkey.rfind("q:", 0) != 0) {
        wakeQueue(qkey);
        return;
    }
    const int64_t qid = std::atoll(qkey.c_str() + 2);
    while (true) {
        std::shared_ptr<VmPcb> w;
        Value item{};
        bool got = false;
        {
            std::lock_guard<std::mutex> lk(impl_->mtx);
            auto it = impl_->queueWaiters.find(qkey);
            if (it == impl_->queueWaiters.end() || it->second.empty()) return;
            w = it->second.front();
            it->second.pop_front();
            if (it->second.empty()) impl_->queueWaiters.erase(it);
        }
        {
            bool ok = false;
            item = lpc::queueReceive(qid, 0, ok);
            got = ok;
        }
        if (!got) {
            // No unassigned element left — put the waiter back.
            std::lock_guard<std::mutex> lk(impl_->mtx);
            auto& dq = impl_->queueWaiters[qkey];
            dq.push_front(w);
            return;
        }
        w->queueDelivery = std::move(item);
        w->hasQueueDelivery = true;
        if (vmcore::traceOn()) {
            std::cerr << "[qwake] qkey=" << qkey
                      << " mt=" << w->queueDelivery.index()
                      << " m=" << to_string(w->queueDelivery, false).substr(0, 60)
                      << " tid=" << w->tid << "\n";
        }
        {
            std::lock_guard<std::mutex> lk(impl_->mtx);
            impl_->wakeLocked(w, false);
        }
        impl_->cv.notify_all();
    }
}

void Scheduler::wakeEvent(const std::string& ename, uint32_t raisedMask) {
    const bool ctrace = g_ctraceOn && ename.compare(0, 11, "ev_readetcd") == 0;
    if (vmcore::traceOn() || ctrace) {
        std::cerr << "[raise] ev=" << ename << " mask=0x" << std::hex
                  << raisedMask << std::dec;
    }
    lpc::flowProbeWake(ename.c_str(), "wakeEvent");
    // The raised bits live in the DRIVER's event store (builtins.cpp
    // DriverEvent — raise_event ORs them in via eventRaise BEFORE this is
    // called); the scheduler only MATCHES waiters and clears the bits a
    // waiter actually consumes (vm_f_raise_event 0x80f07b8 walks the pend
    // list and clears the FIRST intersecting waiter's consumed bits).
    std::deque<std::shared_ptr<VmPcb>> hits;
    uint32_t consumed = 0;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->eventWaiters.find(ename);
        if (it != impl_->eventWaiters.end()) {
            auto& dq = it->second;
            std::deque<std::shared_ptr<VmPcb>> keep;
            for (auto& w : dq) {
                const uint32_t wm = w->cond.mask;
                const uint32_t m = wm & raisedMask;
                if (m != 0 || wm == 0) {   // mask 0 matches any raise
                    hits.push_back(w);
                    consumed |= m ? m : raisedMask;
                } else {
                    keep.push_back(w);
                }
            }
            if (keep.empty()) impl_->eventWaiters.erase(it);
            else it->second = std::move(keep);
        }
        if (consumed && eventConsumeHook_) eventConsumeHook_(ename, consumed);
        if (ctrace) std::cerr << " waiters=" << hits.size() << "\n";
        for (auto& p : hits) {
            // A timed wait parks in BOTH maps; if the timeout timer already
            // woke it, drop this stale event wakeup.
            if (p->eventWoken) continue;
            p->eventWoken = true;
            if (ctrace) std::cerr << "[raise-hit] tid=" << p->tid << "\n";
            impl_->wakeLocked(p, false);
        }
    }
    impl_->cv.notify_all();
}

void Scheduler::wakeEventFront(const std::string& ename) {
    std::deque<std::shared_ptr<VmPcb>> hits;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->eventWaiters.find(ename);
        if (it != impl_->eventWaiters.end()) {
            hits = std::move(it->second);
            impl_->eventWaiters.erase(it);
        }
        for (auto& p : hits) {
            if (p->eventWoken) continue;
            p->eventWoken = true;
            impl_->wakeLocked(p, true);
        }
    }
    impl_->cv.notify_all();
}

void Scheduler::wakeSocket(int fd, bool readable, bool writable) {
    (void)readable;
    (void)writable;
    std::deque<std::shared_ptr<VmPcb>> hits;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->sockWaiters.find(fd);
        if (it != impl_->sockWaiters.end()) {
            hits = std::move(it->second);
            impl_->sockWaiters.erase(it);
        }
        for (auto& p : hits) impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_all();
}

// ---------------------------------------------------------------------------
// park — vm_pendCurrentProcess.  The blocking efun set pcb->cond and threw
// SuspendSignal; run()'s handler calls this with the caught process.
// ---------------------------------------------------------------------------
void Scheduler::parkCurrent(const ResumeCond& cond) {
    auto p = impl_->currentProcess;
    if (!p) {
        std::cerr << "[sched-park] SuspendSignal outside a process (dropped)\n";
        return;
    }
    if (g_ctraceOn) {
        std::cerr << "[park] tid=" << p->tid << " kind=" << cond.kind
                  << " ev=" << cond.ename << " q=" << cond.qkey
                  << " fd=" << cond.fd << " wakeAt=" << cond.wakeAtMs
                  << " now=" << impl_->nowMs()
                  << " entry=" << p->entry
                  << " frames=[" << p->frameChainText(6) << "]\n";
        std::cerr.flush();
    }
    {
        // LPCVM_PARKDBG(热布防): park 侧全量打点(含 <8ms)——kind/事件/
        // 唤醒倒计时/帧链,与 wakeLocked 的 [park-slow] 配对看停泊全貌。
        static const std::atomic<bool>& pk = dynProbeFlag("LPCVM_PARKDBG");
        if (pk.load(std::memory_order_relaxed))
            std::cerr << "[park] t=" << impl_->nowMs() << "ms tid=" << p->tid
                      << " kind=" << cond.kind << " ev=" << cond.ename
                      << " q=" << cond.qkey << " fd=" << cond.fd
                      << " wakeIn=" << (cond.wakeAtMs > 0
                                            ? cond.wakeAtMs - impl_->nowMs()
                                            : -1)
                      << "ms entry=" << p->entry
                      << " frames=[" << p->frameChainText(4) << "]\n";
    }
    std::lock_guard<std::mutex> lk(impl_->mtx);
    p->cond = cond;
    p->parkAtMs = impl_->nowMs();   // PARKDBG: 停泊时长测量
    // Fresh park: clear stale bookkeeping; purge previous wait entries.
    p->eventWoken = false;
    p->timedEvent = false;
    impl_->purgeWaitersLocked(p);
    // Cancel the PREVIOUS park's wake timer: it must not resume the new park.
    if (p->parkTimerCancelled) *p->parkTimerCancelled = true;
    p->parkTimerCancelled.reset();
    std::shared_ptr<std::atomic<bool>> token;
    if (cond.kind == 1 ||
        ((cond.kind == 3 || cond.kind == 5 || cond.kind == 6) && cond.wakeAtMs > 0)) {
        token = std::make_shared<std::atomic<bool>>(false);
        p->parkTimerCancelled = token;
    }
    p->state = VmPcb::State::Parked;
    switch (cond.kind) {
    case 1:   // timer
        impl_->timers.emplace(cond.wakeAtMs, Impl::TimerEntry2{p, token});
        break;
    case 2: { // queue + LOST-WAKE GUARD: a sender may have pushed between the
              // waiter's empty check and this registration — re-check.
        impl_->queueWaiters[cond.qkey].push_back(p);
        // (The queue store lives in the efun layer; the re-check hook is
        //  registered by it — see setQueueRecheckHook. Without a hook the
        //  wait stands and the next send wakes it.)
        if (queueRecheck_ && queueRecheck_(cond.qkey)) {
            auto it = impl_->queueWaiters.find(cond.qkey);
            if (it != impl_->queueWaiters.end()) {
                it->second.pop_back();
                if (it->second.empty()) impl_->queueWaiters.erase(it);
            }
            p->cond.kind = 0;
            p->state = VmPcb::State::Ready;
            p->inReady = true;
            impl_->ready.push_front(p);
        }
        break;
    }
    case 3: { // event (+ timeout timer) + LOST-WAKE GUARD on the raised bits
        impl_->eventWaiters[cond.ename].push_back(p);
        if (cond.wakeAtMs > 0) {
            p->timedEvent = true;
            impl_->timers.emplace(cond.wakeAtMs, Impl::TimerEntry2{p, token});
        }
        // Lost-wake guard: the bits live in the DRIVER's event store; a raise
        // that arrived between the waiter's probe and this registration left
        // them set there, and no future raise will come.
        const uint32_t raised =
            eventPeekHook_ ? eventPeekHook_(cond.ename) : 0;
        if ((raised & cond.mask) != 0) {
            auto it = impl_->eventWaiters.find(cond.ename);
            if (it != impl_->eventWaiters.end()) {
                it->second.pop_back();
                if (it->second.empty()) impl_->eventWaiters.erase(it);
            }
            const uint32_t hit = raised & cond.mask;
            if (eventConsumeHook_) eventConsumeHook_(cond.ename, hit);
            if (token) *token = true;
            p->timedEvent = false;
            p->state = VmPcb::State::Ready;
            p->inReady = true;
            impl_->ready.push_front(p);
        }
        break;
    }
    case 5:   // socket readable/writable
        impl_->sockWaiters[cond.fd].push_back(p);
        if (cond.timeoutAtMs > 0)
            impl_->timers.emplace(cond.timeoutAtMs, Impl::TimerEntry2{p, token});
        break;
    case 6:   // mutex pend
        impl_->mutexWaiters[cond.ename].push_back(p);
        if (cond.wakeAtMs > 0)
            impl_->timers.emplace(cond.wakeAtMs, Impl::TimerEntry2{p, token});
        break;
    default:  // 4 = forever: only woken externally
        break;
    }
}

// Queue re-check hook (efun layer registers the queue store's pending check).
std::function<bool(const std::string&)> Scheduler::queueRecheck_ = nullptr;
// Event raised-bit store hooks (the store is the driver's DriverEvent
// registry, wired by the efun layer at boot).
std::function<uint32_t(const std::string&)> Scheduler::eventPeekHook_ = nullptr;
std::function<void(const std::string&, uint32_t)> Scheduler::eventConsumeHook_ = nullptr;

// ---------------------------------------------------------------------------
// process-level (PCB) named mutexes — ownership per PROCESS
// ---------------------------------------------------------------------------
void Scheduler::mutexCreate(const std::string& name) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    impl_->procMutexes[name];   // fresh level 0
}
int Scheduler::mutexTake(const std::string& name, int timeoutMs, const Value&) {
    auto p = impl_->currentProcess;
    std::lock_guard<std::mutex> lk(impl_->mtx);
    auto it = impl_->procMutexes.find(name);
    if (it == impl_->procMutexes.end()) return -101;   // unknown (vm_getPendObject miss)
    auto& m = it->second;
    if (m.level == 0 || m.owner == p) {
        ++m.level;
        m.owner = p;
        return 0;
    }
    // Pend (kind 6).  The efun layer turns this into the park; the resume
    // with -202 (timeout) / 0 (ownership transferred) comes from the timer /
    // mutexGive paths.
    return 1;   // caller must park
}
void Scheduler::mutexGive(const std::string& name) {
    auto p = impl_->currentProcess;
    std::shared_ptr<VmPcb> next;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->procMutexes.find(name);
        if (it == impl_->procMutexes.end()) return;
        auto& m = it->second;
        if (m.level > 0 && m.owner == p) {
            --m.level;
            if (m.level == 0) {
                m.owner.reset();
                // Transfer to the first waiter, wake it with 0.
                auto w = impl_->mutexWaiters.find(name);
                if (w != impl_->mutexWaiters.end() && !w->second.empty()) {
                    next = w->second.front();
                    w->second.pop_front();
                    if (w->second.empty()) impl_->mutexWaiters.erase(w);
                    m.level = 1;
                    m.owner = next;
                    next->resumeValue = make_int(0);
                }
            }
        }
    }
    if (next) wake(next.get());
}

void Scheduler::mutexErase(const std::string& name) {
    std::deque<std::shared_ptr<VmPcb>> waiters;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        auto it = impl_->procMutexes.find(name);
        if (it == impl_->procMutexes.end()) return;
        auto mw = impl_->mutexWaiters.find(name);
        if (mw != impl_->mutexWaiters.end()) {
            waiters = std::move(mw->second);
            impl_->mutexWaiters.erase(mw);
        }
        impl_->procMutexes.erase(it);
    }
    for (auto& p : waiters) {
        p->resumeValue = make_int(-101);
        wake(p.get());
    }
}

// ---------------------------------------------------------------------------
// service sockets
// ---------------------------------------------------------------------------
void Scheduler::addServiceSocket(int fd, std::function<bool(unsigned int)> cb) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    impl_->serviceSockets[fd] = std::move(cb);
}
bool Scheduler::anyServiceSocket() const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    return !impl_->serviceSockets.empty();
}

// ---------------------------------------------------------------------------
// execOnMain — external threads route execution through the scheduler thread
// and block on the result; a call already ON the scheduler thread (nested,
// from a running process or efun) executes directly on the CURRENT PCB.
// ---------------------------------------------------------------------------
Value Scheduler::execOnMain(VmObject* obj, const std::string& funName,
                            const std::vector<Value>& args) {
    const OriginalFunction* fn = obj ? obj->prog()->findFn(funName) : nullptr;
    if (!fn) {
        if (vmcore::traceOn()) {
            std::cerr << "[execmain-miss] fn=" << funName
                      << " obj=" << (obj ? obj->key() : std::string("<null>"))
                      << "\n";
        }
        return make_int(0);
    }
    if (isSchedulerThread()) {
        VmPcb* pcb = currentPcb();
        if (pcb) return vmApplySync(pcb, obj, fn, args);
        // No current process (driver-level direct call on the sched thread):
        // run it as a one-shot chain on a scratch PCB.
        auto scratch = std::make_shared<VmPcb>();
        const int64_t tid = impl_->tidPool.alloc();
        scratch->tid = tid;
        Value r = vmRunFunction(scratch.get(), obj, fn, args);
        impl_->tidPool.release(tid);
        return r;
    }
    // External thread: spawn a scheduler process and block on its result.
    // The chain is attributed to the connection the dispatching thread is
    // serving (setDispatchCno) so the per-connection command gate sees the
    // command's WHOLE LPC lifetime, including parked continuations.
    auto p = makePcb(*impl_, obj, funName, args);
    p->clientNo = lpc::dispatchCno();
    p->resultPromise = std::make_shared<std::promise<Value>>();
    auto fut = p->resultPromise->get_future();
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->wakeLocked(p, false);
    }
    impl_->cv.notify_one();
    return fut.get();
}

// ---------------------------------------------------------------------------
// runProcess — one dispatch of one PCB.
// ---------------------------------------------------------------------------
// Finish the TOP construct plan: registry/wake/promise bookkeeping, pop the
// plan stack, push the object value (the efun return value the CALLER's LPC
// frame expects when its retIp is restored).  Shared by the inline path and
// the (external-thread) construct process.
static void finishTopConstructPlan(Scheduler::Impl& im,
                                   const std::shared_ptr<VmPcb>& p) {
    auto plan = p->construct;
    p->construct.reset();
    bool failed = plan ? plan->failed : true;
    if (plan) {
        if (failed)
            Registry::instance().clearConstructing(plan->obj->key());
        else {
            // Construction finished: mark the object initialized and wake the
            // parked loader(s) BEFORE anything else ("Load: ... OK" prints
            // when the loading process resumes, before fresh daemons run).
            Registry::instance().markInitialized(plan->obj->key());
        }
        if (g_ctraceOn) {
            std::cerr << "[construct-done] obj=" << plan->obj->key()
                      << " failed=" << failed << "\n";
        }
        if (plan->resultPromise) {
            plan->resultPromise->set_value(
                failed ? make_int(0)
                       : Registry::makeObjectValue(plan->obj));
            plan->resultPromise.reset();
        }
        if (!plan->doneEvent.empty())
            Scheduler::instance().wakeEventFront(plan->doneEvent);
    }
    if (!p->constructStack.empty()) p->constructStack.pop_back();
    p->construct = p->constructStack.empty()
                       ? nullptr : p->constructStack.back();
    // The caller's LPC frame resumes at its load_object/clone_object call
    // site with the object value on top of the stack (efun return semantics).
    Interp::pushValue(p.get(), failed
                  ? Value(make_int(0))
                  : Registry::makeObjectValue(plan->obj));
}

static void finishConstruct(Scheduler::Impl& im,
                            const std::shared_ptr<VmPcb>& p, bool failed) {
    // Abort path (chain died inside a construction): fail EVERY plan on the
    // stack (原版 construct_object_failed catch 覆盖整链), waking each plan's
    // waiters; the pushed values are irrelevant — the stack is being cleared.
    while (p->construct) {
        p->construct->failed = failed;
        finishTopConstructPlan(im, p);
    }
    p->state = VmPcb::State::Done;
    p->cond.kind = 0;
    if (p->resultPromise) {
        p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(make_int(0));
        p->resultPromise.reset();
    }
}

// park 窗口取证 (LPCVM_LOOPWATCH 开启时): 打印 pcb 上每个循环 ctrl 的
// (帧, 槽, ctrl 指针, 栈槽 variant index)。park 前与 resume 时各打一次,
// 两条日志用 tid 串联 —— 指针或 idx 对不上就是 park 期间有人搅了栈。
static void loopWatchDump(const char* tag, const VmPcb* p) {
    static const bool on = getenv("LPCVM_LOOPWATCH") != nullptr;
    if (!on) return;
    std::cerr << "[loopwatch-" << tag << "] tid=" << p->tid
              << " stack=" << p->stack.size()
              << " frames=" << p->frames.size();
    for (size_t fi = 0; fi < p->frames.size(); ++fi) {
        for (const auto& kv : p->frames[fi].loops) {
            const int slot = kv.first;
            const auto& ctrl = kv.second;
            unsigned char idx = 99;
            const void* ptr = nullptr;
            if (slot >= 0 && static_cast<size_t>(slot) < p->stack.size() &&
                p->stack[static_cast<size_t>(slot)].value.index() == 9) {
                const auto& sp = std::get<lpc::Ref<LoopCtrl>>(
                    p->stack[static_cast<size_t>(slot)].value);
                ptr = sp.get();
            }
            idx = slot >= 0 && static_cast<size_t>(slot) < p->stack.size()
                      ? static_cast<unsigned char>(
                            p->stack[static_cast<size_t>(slot)].value.index())
                      : 99;
            std::cerr << " f" << fi << ":s" << slot << "=p" << ptr
                      << "/i" << static_cast<int>(idx) << "(m" << ctrl.get()
                      << ")";
        }
    }
    std::cerr << "\n";
    std::cerr.flush();
}

static void runProcess(Scheduler::Impl& im, const std::shared_ptr<VmPcb>& p) {    // currentProcess snapshot/restore: a nested process run (execOnMain from
    // inside a process) must not leave ITS process behind — get_thread_id()
    // changing inside one LPC statement is the [vmown]/9112 failure class.
    auto prevProc = im.currentProcess;
    im.currentProcess = p;
    struct Restore {
        Scheduler::Impl& im;
        std::shared_ptr<VmPcb> prev;
        ~Restore() { im.currentProcess = std::move(prev); }
    } restore{im, prevProc};
    const bool mdbg = p->construct && getenv("LPCVM_MDBG");
    if (mdbg) {
        std::cerr << "[mdbg-run] tid=" << p->tid
                  << " slice=" << (p->sliceYield ? 1 : 0)
                  << " first=" << (p->firstRun ? 1 : 0)
                  << " plans=" << p->constructStack.size()
                  << " steps=" << p->construct->idx << "/"
                  << p->construct->steps.size()
                  << " chain=[" << p->frameChainText(6) << "]\n";
        std::cerr.flush();
    }
    // Tid back-lease: EVERY exit path that leaves the pcb Done (normal end,
    // chain abort, construct finished, entry missing) must return its tid to
    // the pool — the previous tail-only release leaked one tid per finished
    // construct (4782 loads > 1024 pool -> global starvation, gs froze).
    struct TidBack {
        Scheduler::Impl& im;
        const std::shared_ptr<VmPcb>& p;
        ~TidBack() {
            if (p->state != VmPcb::State::Done || p->tid < 0 || p->borrowedTid)
                return;   // borrowed tid belongs to the parked initiator
            std::lock_guard<std::mutex> lk(im.mtx);
            im.purgeWaitersLocked(p);
            im.tidPool.release(p->tid);
            p->tid = -1;
            if (!im.tidStarved.empty()) {
                auto sp = im.tidStarved.front();
                im.tidStarved.pop_front();
                sp->tid = im.tidPool.alloc();
                im.wakeLocked(sp, false);
                im.cv.notify_all();
            }
        }
    } tidBack{im, p};

    const bool wd = g_ctraceOn;
    if (wd) {
        std::lock_guard<std::mutex> lk(im.mtx);
        im.runSince[p->tid] = {im.nowMs(), p};
    }

    VmPcbSetter setPcb(p.get());   // the interpreter's current PCB    // Chain-probe / per-connection gate bookkeeping (driver side): one chain
    // is in flight while this process runs; a parked continuation keeps
    // counting until its chain really ends.
    const int64_t cno = p->clientNo;
    const int64_t chainKey = reinterpret_cast<int64_t>(p.get());
    if (cno >= 0) cmdActiveAttach(cno);
    vmchainRun(chainKey, p.get());
    struct ChainExit {
        int64_t cno;
        int64_t key;
        VmPcb* pcb;
        bool parked = false;
        ~ChainExit() {
            vmchainRunExit(key, parked);
            if (cno >= 0) cmdActiveDetach(cno);
        }
    } chainExit{cno, chainKey, p.get()};
    try {
        Value result;
        if (p->firstRun) {
            p->firstRun = false;
            if (p->threadClosure) {
                // create_thread with an EFUN closure: apply the efun on this
                // dedicated process.  A blocking efun inside (sleep in the
                // daemon loop) parks THIS process; the resume re-enters the
                // efun's continuation through the normal frame mechanism.
                auto cl = p->threadClosure;
                p->threadClosure.reset();
                vmchainBegin(cno, chainKey, p.get(), cl->functionName);
                if (cl->globalFn && cl->func) {
                    // globalFn closure (op23 bit7): run the master-program
                    // body (原版 vm_callInUserFunBody(...,vm_pGlobalProgram)),
                    // not the efun-by-name dispatcher.
                    result = vmApplySync(p.get(), p->obj, cl->func,
                                         cl->boundArgs, Interp::masterProgram());
                } else {
                    result = dispatchEfunByName(p.get(), cl->functionName,
                                                cl->boundArgs);
                }
                vmchainEnd(cno, chainKey, p.get(), cl->functionName, true);
                p->state = VmPcb::State::Done;
                p->cond.kind = 0;
                if (p->resultPromise) {
                    p->stripValues();   // [ref] notify 前清 Value, 调用方析构入参时无人并发计数
                    p->resultPromise->set_value(std::move(result));
                    p->resultPromise.reset();
                }
                return;
            }
            if (p->construct) {
                // External-thread construction process: start the hook-driven
                // inline construction on THIS pcb (原版 vm_f_construct_object2
                // 的同一帧/hook 机制), then run the frame chain.
                vmchainBegin(cno, chainKey, p.get(), "construct");
                if (p->constructStack.empty())
                    p->constructStack.push_back(p->construct);
                vmStartInlineConstruction(p.get(), p->construct);
                if (p->construct && !p->construct->finished)
                    Interp::run(p.get());   // park escapes / whole chain done
                if (p->construct && !p->construct->finished) {
                    // Returned without a SuspendSignal (no [park] printed):
                    // the chain ended while the plan is STILL unfinished —
                    // ThreadExitSignal eaten by the interp loop, a broken
                    // retHook chain, or a swallowed error.  Leaving the pcb
                    // Ready made it spin between requeue and runProcess with
                    // zero progress (gs boot froze mid load_all; the master
                    // plan never completed and 8160 never opened).
                    if (g_ctraceOn) {
                        std::cerr << "[construct-nopark] obj="
                                  << (p->construct->obj
                                          ? p->construct->obj->key()
                                          : std::string("<null>"))
                                  << " steps=" << p->construct->idx
                                  << "/" << p->construct->steps.size()
                                  << " tid=" << p->tid
                                  << " frames=[" << p->frameChainText(8)
                                  << "]\n";
                        std::cerr.flush();
                    }
                    return;                 // parked mid-construction
                }
                vmchainEnd(cno, chainKey, p.get(), "construct", true);
                p->state = VmPcb::State::Done;
                p->cond.kind = 0;
                if (p->resultPromise) {
                    p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(make_int(0));
                    p->resultPromise.reset();
                }
                return;
            }
            const OriginalFunction* fn =
                p->obj ? p->obj->prog()->findFn(p->entry) : nullptr;
            if (!fn) {
                std::cerr << "[chain-abort] entry " << p->entry
                          << " not found on "
                          << (p->obj ? p->obj->key() : std::string("<null>"))
                          << " tid=" << p->tid << "\n";
                std::cerr.flush();
                p->state = VmPcb::State::Done;
                if (p->resultPromise) {
                    p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(make_int(0));
                    p->resultPromise.reset();
                }
                return;
            }
            vmchainBegin(cno, chainKey, p.get(), p->entry);
            result = vmRunFunction(p.get(), p->obj, fn, p->args);
        } else {
            // Resume — the construction's retHook chain advances ITSELF when
            // a step frame returns (原版 vm_hookCallbackWhenReturn 语义):
            // no driver loop here.  vmResume runs to the next chain end.
            loopWatchDump("resume", p.get());
            p->cond.kind = 0;   // no longer parked; guards see a clean pcb
            if (vmcore::traceOn()) {
                std::cerr << "[resume] tid=" << p->tid
                          << " slice=" << (p->sliceYield ? 1 : 0)
                          << " hook=" << (p->resumeHook ? 1 : 0)
                          << " deliv=" << (p->hasQueueDelivery ? 1 : 0)
                          << " kind=" << p->cond.kind
                          << " entry=" << p->entry << "\n";
            }
            if (p->sliceYield) {
                p->sliceYield = false;
                loopWatchDump("sliceresume", p.get());
                result = vmResumeSlice(p.get());
            } else {
                if (p->resumeHook) {
                    auto hook = std::move(p->resumeHook);
                    p->resumeHook = nullptr;
                    hook();
                }
                result = vmResume(p.get(), p->resumeValue);
            }
        }
        // Chain finished normally.
        // BLIND-SPOT GUARD: a construct chain whose plan is STILL unfinished
        // must never be marked Done here — that silently murdered the master
        // boot twice (ready → requeue → resume "ends" via a swallowed
        // ThreadExitSignal inside interp → Done → tid released → master
        // orphaned, 8160 never opens, zero log lines).  Surface it.
        if (p->construct && !p->construct->finished) {
            std::cerr << "[chain-premature-end] tid=" << p->tid << " obj="
                      << (p->construct->obj ? p->construct->obj->key()
                                            : std::string("<null>"))
                      << " steps=" << p->construct->idx << "/"
                      << p->construct->steps.size()
                      << " chain=[" << p->frameChainText(8) << "]\n";
            std::cerr.flush();
        } else if (mdbg) {
            std::cerr << "[mdbg-done] tid=" << p->tid << " steps="
                      << p->construct->idx << "/" << p->construct->steps.size()
                      << " chain=[" << p->frameChainText(6) << "]\n";
            std::cerr.flush();
        }
        vmchainEnd(cno, chainKey, p.get(), p->entry, true);
        p->state = VmPcb::State::Done;
        p->cond.kind = 0;
        if (p->resultPromise) {
            p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(std::move(result));
            p->resultPromise.reset();
        }
    } catch (const SuspendSignal&) {
        // Blocking efun: park the CURRENT process with ITS cond (never a
        // shared slot — two processes never share a PCB).
        p->state = VmPcb::State::Parked;
        chainExit.parked = true;
        loopWatchDump("park", p.get());
        vmchainPark(cno, chainKey, p.get(), p->entry, p->cond.kind,
                    p->cond.qkey.empty() ? p->cond.ename : p->cond.qkey);
        Scheduler::instance().parkCurrent(p->cond);
    } catch (const TimeSliceSignal&) {
        // Master time-slice yield: requeue at the tail; state stays in the
        // PCB; the resume continues WITHOUT pushing a value.
        p->state = VmPcb::State::Ready;
        p->sliceYield = true;
        chainExit.parked = true;   // the run exited mid-chain (will RESUME)
        loopWatchDump("slicepark", p.get());
        if (g_ctraceOn) {
            std::cerr << "[slice-requeue] tid=" << p->tid << " inReady="
                      << p->inReady << "\n";
            std::cerr.flush();
        }
        Scheduler::instance().requeue(p.get());
    } catch (const ThreadExitSignal&) {
        // exit/exit_thread: the chain ended (interp already cleaned the PCB).
        if (p->construct && g_ctraceOn) {
            std::cerr << "[threadexit-kill] tid=" << p->tid << " obj="
                      << (p->obj ? p->obj->key() : std::string("<null>"))
                      << " chain=[" << p->frameChainText(8) << "]\n";
            std::cerr.flush();
        }
        vmchainEnd(cno, chainKey, p.get(), p->entry, true);
        if (p->construct) {
            finishConstruct(im, p, true);
            return;
        }
        p->state = VmPcb::State::Done;
        if (p->resultPromise) {
            p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(make_int(0));
            p->resultPromise.reset();
        }
    } catch (const std::exception& e) {
        // Uncaught LPC error: the chain dies — with a trace (never silent).
        std::cerr << "[chain-abort] \"" << e.what() << "\" entry=" << p->entry
                  << " tid=" << p->tid
                  << " chain=[" << p->frameChainText(10) << "]\n";
        std::cerr.flush();
        vmchainEnd(cno, chainKey, p.get(), p->entry, false);
        p->frames.clear();
        p->stack.clear();
        p->catches.clear();
        if (p->construct) {
            finishConstruct(im, p, true);
            return;
        }
        p->state = VmPcb::State::Done;
        if (p->resultPromise) {
            p->stripValues();   // [ref] notify 前清 Value
            p->resultPromise->set_value(make_int(0));
            p->resultPromise.reset();
        }
    }
    if (wd) {
        std::lock_guard<std::mutex> lk(im.mtx);
        im.runSince.erase(p->tid);
    }
    if (p->state == VmPcb::State::Done) {
        {
            std::lock_guard<std::mutex> lk(im.mtx);
            im.purgeWaitersLocked(p);   // tid release is the TidBack guard's job
        }
        // Deferred real-frees: an object destructed from inside its own call
        // chain leaves the registry only now (the issuing process finished).
        vmFlushPendingFrees(static_cast<const void*>(p.get()));
    } else if (p->state != VmPcb::State::Parked && g_ctraceOn) {
        // Process left runProcess neither Done nor Parked — its PCB must be
        // reachable somewhere (ready/tidStarved); if not, it is LOST.
        bool reachable = p->inReady;
        {
            std::lock_guard<std::mutex> lk(im.mtx);
            for (auto& q : im.tidStarved)
                if (q == p) reachable = true;
        }
        std::cerr << "[proc-exit] tid=" << p->tid << " state="
                  << static_cast<int>(p->state) << " reachable=" << reachable
                  << " entry=" << p->entry
                  << " plan=" << (p->construct && p->construct->obj
                                      ? p->construct->obj->key()
                                      : std::string("-"))
                  << " planFinished="
                  << (p->construct ? (p->construct->finished ? 1 : 0) : -1)
                  << " firstRun=" << (p->firstRun ? 1 : 0)
                  << " frames=[" << p->frameChainText(6) << "]\n";
        std::cerr.flush();
    }
}

// ---------------------------------------------------------------------------
// run — the driver's main loop.
// ---------------------------------------------------------------------------
void Scheduler::run() {
    impl_->schedThreadId = std::this_thread::get_id();
    while (true) {
        lpc::liveness().schedIters.fetch_add(1, std::memory_order_relaxed);
        if (g_ctraceOn && !impl_->runSince.empty()) {
            const int64_t now = impl_->nowMs();
            for (auto it = impl_->runSince.begin();
                 it != impl_->runSince.end();) {
                if (now - it->second.first > 60000) {
                    auto sp = it->second.second;
                    std::cerr << "[stuck] tid=" << it->first << " for "
                              << (now - it->second.first) << "ms entry="
                              << (sp ? sp->entry : std::string("?"))
                              << " frames=["
                              << (sp ? sp->frameChainText(8) : std::string("?"))
                              << "]\n";
                    std::cerr.flush();
                    it = impl_->runSince.erase(it);
                } else {
                    ++it;
                }
            }
        }
        if (impl_->shutdown) {
            std::lock_guard<std::mutex> lk(impl_->mtx);
            if (impl_->ready.empty()) return;
        }
        // LPCVM_PCDUMP (runtime-armable: type nul > _probe_on\LPCVM_PCDUMP):
        // every 10s dump EVERY live pcb with its park condition and frame
        // chain.  Boot stalls left zero traces — the master's construct
        // "finished" while loads froze and the scheduler went idle; without a
        // full pcb census there was no way to see WHERE each parked process
        // (boot master included) was actually waiting.
        {
            static const std::atomic<bool>& pcDump = lpc::dynProbeFlag("LPCVM_PCDUMP");
            static std::atomic<int64_t> lastDump{0};
            const int64_t nowD = impl_->nowMs();
            if (pcDump.load(std::memory_order_relaxed) &&
                nowD - lastDump.load() > 10000) {
                lastDump.store(nowD);
                std::lock_guard<std::mutex> lk(impl_->mtx);
                std::cerr << "[pcdump] t=" << nowD << " ready=" << impl_->ready.size()
                          << " runSince=" << impl_->runSince.size() << "\n";
                auto dumpP = [](const std::shared_ptr<VmPcb>& p,
                                const char* where, const std::string& key) {
                    std::cerr << "  " << where << key << " tid=" << p->tid
                              << " st=" << static_cast<int>(p->state)
                              << " cond=" << p->cond.kind
                              << " ev=" << p->cond.ename
                              << " q=" << p->cond.qkey
                              << " at=" << p->cond.wakeAtMs
                              << " entry=" << p->entry
                              << " frames=[" << p->frameChainText(6) << "]\n";
                };
                for (auto& p : impl_->ready)
                    dumpP(p, "ready", "");
                for (auto& [ev, dq] : impl_->eventWaiters)
                    for (auto& p : dq) dumpP(p, "ev ", ev);
                for (auto& [qk, dq] : impl_->queueWaiters)
                    for (auto& p : dq) dumpP(p, "q  ", qk);
                for (auto& [mx, dq] : impl_->mutexWaiters)
                    for (auto& p : dq) dumpP(p, "mx ", mx);
                for (auto& [fd, dq] : impl_->sockWaiters)
                    for (auto& p : dq) dumpP(p, "fd ", std::to_string(fd));
                for (auto& [at, te] : impl_->timers)
                    if (te.p) dumpP(te.p, "tm ",
                                    std::to_string(at - nowD) + "ms ");
                std::cerr.flush();
            }
        }
        // Poll parked sockets (kind-5) at most every 20ms — the original
        // vm_mainLoop scans sockets every iteration; a busy ready queue must
        // not starve connect/recv completions.
        {
            const int64_t now = impl_->nowMs();
            std::vector<int> fds;
            {
                std::lock_guard<std::mutex> lk(impl_->mtx);
                if (now - impl_->lastSockPollMs >= 20)
                    for (auto& kv : impl_->sockWaiters) fds.push_back(kv.first);
            }
            if (!fds.empty()) {
                impl_->lastSockPollMs = now;
                // The efun layer provides the poll (socket backend).
                if (sockPollHook_) {
                    std::vector<int> rd = fds, wr = fds;
                    int nrd = sockPollHook_(fds, rd, wr);
                    (void)nrd;
                    for (int fd : fds) {
                        bool r = std::find(rd.begin(), rd.end(), fd) != rd.end();
                        bool w = std::find(wr.begin(), wr.end(), fd) != wr.end();
                        if (r || w) wakeSocket(fd, r, w);
                    }
                }
            }
        }
        // Listening sockets: the original vm_mainLoop polls srvFd EVERY
        // iteration (timeout 0) and vm_gameListenSocketCallback accepts ON
        // THE MAIN LOOP THREAD, handing the ACCEPTED fd downstream
        // (vm_bindToClient + apply cmd_new_connection).  Same here: WSAPoll
        // the raw listener fds directly (they are NOT xdbc-table sockets),
        // accept, pass the accepted client fd to the registered callback.
        {
            std::vector<std::pair<int, std::function<bool(unsigned int)>>> socks;
            {
                std::lock_guard<std::mutex> lk(impl_->mtx);
                for (auto& kv : impl_->serviceSockets) socks.push_back(kv);
            }
            if (!socks.empty()) {
                std::vector<WSAPOLLFD> pfds;
                pfds.reserve(socks.size());
                for (const auto& kv : socks) {
                    WSAPOLLFD p{};
                    p.fd = static_cast<SOCKET>(kv.first);
                    p.events = POLLRDNORM;
                    pfds.push_back(p);
                }
                if (WSAPoll(pfds.data(), static_cast<ULONG>(pfds.size()), 0) > 0) {
                    for (size_t i = 0; i < socks.size(); ++i) {
                        if ((pfds[i].revents &
                             (POLLRDNORM | POLLERR | POLLHUP | POLLNVAL)) == 0)
                            continue;
                        SOCKET lfd = static_cast<SOCKET>(socks[i].first);
                        bool keep = true;
                        // Drain the backlog: the listener is non-blocking.
                        for (;;) {
                            SOCKET c = accept(lfd, nullptr, nullptr);
                            if (c == INVALID_SOCKET) {
                                int err = WSAGetLastError();
                                if (err == WSAEWOULDBLOCK) break;
                                if (err == WSAEINTR) continue;
                                std::cerr << "[accept-err] fd=" << socks[i].first
                                          << " err=" << err << "\n";
                                break;
                            }
                            keep = socks[i].second(static_cast<unsigned>(c));
                            if (!keep) {
                                closesocket(c);
                                break;
                            }
                        }
                        if (!keep) {
                            std::lock_guard<std::mutex> lk(impl_->mtx);
                            impl_->serviceSockets.erase(socks[i].first);
                        }
                    }
                }
            }
        }
        std::shared_ptr<VmPcb> p;
        {
            std::lock_guard<std::mutex> lk(impl_->mtx);
            const int64_t now = impl_->nowMs();
            // Move due timers to ready (cancel / done / eventWoken / inReady
            // rules ported; timed waits resume with -202 / -501 and are
            // dropped from their wait maps RIGHT HERE).
            while (!impl_->timers.empty() && impl_->timers.begin()->first <= now) {
                auto it = impl_->timers.begin();
                auto te = it->second;
                impl_->timers.erase(it);
                auto tp = te.p;
                if (te.cancelled && *te.cancelled) continue;
                if (!tp || tp->eventWoken || tp->state == VmPcb::State::Done ||
                    tp->inReady) {
                    // A DUE timer whose pcb gets skipped here NEVER wakes
                    // (the entry is erased right above) — this silently ate
                    // the master's post_init sleep once; log the reason.
                    if (g_ctraceOn && tp)
                        std::cerr << "[timer-skip] tid=" << tp->tid
                                  << " evW=" << tp->eventWoken
                                  << " st=" << static_cast<int>(tp->state)
                                  << " inReady=" << tp->inReady << "\n";
                    continue;
                }
                tp->eventWoken = true;
                {
                    // LPCVM_TIMERDBG: log the actual FIRE of disconnect-chain
                    // timers (time_expired = the save trigger) and the gather
                    // chain (finish_gather_item = chest disappearance).
                    static const std::atomic<bool>& tfFlag = dynProbeFlag("LPCVM_TIMERDBG");
                    if (tfFlag.load(std::memory_order_relaxed) &&
                        (tp->entry == "time_expired" || tp->entry == "logout" ||
                         tp->entry == "finish_gather_item" ||
                         tp->entry == "<efun-timer>"))
                        std::cerr << "[timer-fire] t=" << msSinceStart()
                                  << "ms tid=" << tp->tid << " obj="
                                  << (tp->obj ? tp->obj->key() : std::string("(null)"))
                                  << " fn=" << tp->entry << "\n";
                }
                if (tp->timedEvent) {
                    if (tp->cond.kind == 6) {
                        auto mw = impl_->mutexWaiters.find(tp->cond.ename);
                        if (mw != impl_->mutexWaiters.end()) {
                            auto& dq = mw->second;
                            for (auto q = dq.begin(); q != dq.end(); ++q)
                                if (*q == tp) { dq.erase(q); break; }
                            if (dq.empty()) impl_->mutexWaiters.erase(mw);
                        }
                        tp->resumeValue = make_int(-202);
                    } else if (tp->cond.kind == 5) {
                        auto sw = impl_->sockWaiters.find(tp->cond.fd);
                        if (sw != impl_->sockWaiters.end()) {
                            auto& dq = sw->second;
                            for (auto q = dq.begin(); q != dq.end(); ++q)
                                if (*q == tp) { dq.erase(q); break; }
                            if (dq.empty()) impl_->sockWaiters.erase(sw);
                        }
                        tp->resumeValue = make_int(-126);
                    } else {
                        auto ew = impl_->eventWaiters.find(tp->cond.ename);
                        if (ew != impl_->eventWaiters.end()) {
                            auto& dq = ew->second;
                            for (auto q = dq.begin(); q != dq.end(); ++q)
                                if (*q == tp) { dq.erase(q); break; }
                            if (dq.empty()) impl_->eventWaiters.erase(ew);
                        }
                        tp->resumeValue = make_int(-501);
                    }
                }
                impl_->wakeLocked(tp, false);
            }
            if (!impl_->ready.empty()) {
                // Take the first runnable pcb; one with no tid yet (pool was
                // empty at spawn) tries to lease one HERE — on failure it
                // parks in tidStarved until a running pcb returns its tid.
                while (!impl_->ready.empty()) {
                    p = std::move(impl_->ready.front());
                    impl_->ready.pop_front();
                    p->inReady = false;
                    if (p->tid < 0) {
                        p->tid = impl_->tidPool.alloc();
                        if (p->tid < 0) {
                            if (impl_->tidStarved.empty())
                                std::cerr << "[tid-wait] pool exhausted; pcbs"
                                             " queue until one finishes\n";
                            impl_->tidStarved.push_back(std::move(p));
                            p.reset();
                            continue;   // try the next ready pcb
                        }
                    }
                    p->state = VmPcb::State::Running;
                    break;
                }
            }
        }
        if (p) {
            runProcess(*impl_, p);
            continue;
        }
        // Idle: wait until the next timer or a wake source.
        // 发送聚合刷出：ready 队列空 = 本轮命令突发结束（对齐原版驱动
        // "一个命令突发跑完才一次 write"——抓包实证：一键点化/羽化原版
        // 是 2 个大包 11000+8000，逐帧 send 成十几个小包会让客户端逐包
        // 重建 UI 卡 0.x 秒）。在即将等待前把所有客户端聚合缓冲各用一次
        // write 刷出；TimeSlice 让出/恢复不会拆散突发（缓冲挂客户端）。
        lpc::gameServiceFlushPendingOutputs(true);
        {
            std::unique_lock<std::mutex> lk(impl_->mtx);
            if (!impl_->ready.empty()) continue;
            int64_t next = -1;
            if (!impl_->timers.empty()) next = impl_->timers.begin()->first;
            const bool hasSocks = !impl_->sockWaiters.empty() ||
                                  !impl_->serviceSockets.empty();
            // The original vm_mainLoop scans sockets EVERY iteration: while
            // ANY socket is live (listening or parked in select/recv), never
            // block past 20ms — an incoming connection on the listen queue
            // has no other wake source (no cv notify), so a cv.wait_for up
            // to the next timer would freeze accepts until that timer fires.
            if (hasSocks) {
                // 不可中断的 sleep_for 会让新入队的 pcb 白等一个完整量子
                // (wakeLocked 入队时 notify_all)：改为 cv.wait_for,有货立即
                // 醒来调度,无货最多 20ms 后照旧轮询 socket。原版 vm_mainLoop
                // 空闲时 select(timeout=0) 快速轮转,ready 一有货立即执行。
                // call_out/定时器停泊同样不能整量子白等:原版 timeout=0 下
                // 定时器到期即触发;这里按最近到期时刻精确等待(≤20ms 封顶,
                // 保住 socket 轮询节奏),否则一键点化/羽化每件物品的
                // call_out 链每跳都吃满 20ms(实测 6 件 435ms)。
                int64_t waitMs = 20;
                if (next >= 0) {
                    const int64_t now2 = impl_->nowMs();
                    if (next <= now2) waitMs = 0;
                    else waitMs = std::min<int64_t>(20, next - now2);
                }
                if (waitMs > 0)
                    impl_->cv.wait_for(lk, std::chrono::milliseconds(waitMs));
            } else if (next < 0) {
                impl_->cv.wait(lk);
            } else {
                const int64_t now = impl_->nowMs();
                if (next > now)
                    impl_->cv.wait_for(lk, std::chrono::milliseconds(next - now));
            }
        }
    }
}

void Scheduler::requestShutdown() {
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->shutdown = true;
    }
    impl_->cv.notify_all();
}

// Socket poll hook (efun layer provides the select backend).
std::function<int(std::vector<int>&, std::vector<int>&, std::vector<int>&)>
    Scheduler::sockPollHook_ = nullptr;

} // namespace vmcore
} // namespace lpc
