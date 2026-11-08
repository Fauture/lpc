// ===========================================================================
// vmcore/vmcore.h — 新执行核心的公共面 (阶段0: API 契约)
//
// 契约声明:
//   * 驱动侧 (game_service / main / builtins / pkt_protocol / value) 只允许
//     经由本头文件与 object.h/pcb.h 的公共类型使用新核; 不引用
//     original_vm.cpp 的任何内部结构 (OriginalVm/Scheduler::Process/
//     t_efunCallerVm 等)。诊断上下文 (原 stubCallCtx) 由 currentPcb 派生。
//   * 原 original_vm.h 的 public 函数签名保持不变 (驱动兼容面), 其实现
//     全部转调本文件入口 — 驱动侧代码不感知内核替换。
//   * 语义依据: 原版 magic_Linux32x (IDA 地址标注在各实现处)。
// ===========================================================================
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "pcb.h"

namespace lpc {
namespace vmcore {

// ---- current process (vm_getCurrentPCB 0x80a9310) -------------------------
// 当前 PCB。驱动在调度线程上执行 LPC 时必有当前 PCB; 纯 C++ 上下文
// (无 LPC 在跑) 返回 null。thread_local: 各服务线程各持各的当前进程。
VmPcb* currentPcb();

// [mips] 全局指令计数 (诊断): Interp::loop 每指令 +1, [hb] 心跳换算
// 指令吞吐。VM 执行速度是否达到原版水平的直接标尺。
extern std::atomic<uint64_t> g_vmInsnCount;
class VmPcbSetter {
public:
    explicit VmPcbSetter(VmPcb* pcb);
    ~VmPcbSetter();
private:
    VmPcb* prev_;
};

// get_thread_id (0x80ed79c): the CURRENT PCB's tid (-1 outside LPC).
int64_t currentTid();
// previous_object(): name of the object that issued the current call.
std::string currentPreviousObject();
// The object whose function is currently executing (current frame's obj).
VmObject* currentObject();

// ---- error channel (vm_error 0x8097b22 → vm_errorQuit → vm_traceError) ----
// 统一错误出口: 打印留痕 (被 catch 的错误也打印), 然后抛 VmError;
// 解释器 catch 链恢复, 无人捕获则终止当前链。
[[noreturn]] void vmError(const std::string& what);

// ---- call primitives (vm_callInUserFunBody 0x8093eb0 同构) ----------------
// 压帧: 在当前 PCB 的栈上压入绑定 (obj, prog, fn) 的帧。args 按 param0
// 在栈顶的布局压入 (缺省声明参数补 undefined — 原版语义, 不是 0)。
// 返回 false 仅当栈空间检查失败。
bool vmCallPushFrame(VmPcb* pcb, VmObject* obj, const VmProgram* prog,
                     const OriginalFunction* fn, const std::vector<Value>& args,
                     int padArgs = 0);
// vm_hookCallbackWhenReturn (0x8094e70): hook runs when the current
// (innermost) frame returns, with the return value.
void vmHookCallbackWhenReturn(VmPcb* pcb, std::function<void(Value&)> hook);

// ---- top-level execution ---------------------------------------------------
// Run `fn` on `obj` as a fresh chain on `pcb` (external entry: driver API,
// scheduler first-run).  Pushes a boundary frame and runs the interpreter
// until it returns.  Blocking efuns inside throw SuspendSignal out (the
// scheduler parks the PCB and resumes by re-entering this).
Value vmRunFunction(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                    const std::vector<Value>& args);
// Resume a parked chain: push resumeValue and continue at pcb->ip.
Value vmResume(VmPcb* pcb, const Value& resumeValue);
// Resume a SLICE-yielded chain (runTicks/relinquish): continue WITHOUT
// pushing — relinquish() pushed its 1 before yielding (the old rule).
Value vmResumeSlice(VmPcb* pcb);

// ---- call_other (vm_callInOtherFun 0x8094cd8) ------------------------------
// Target resolution, in the original's order: an object VALUE is the pointer
// (zero lookup); a string name goes through the registry (canonical key, no
// suffix scans).  Callee frame is pushed on the CURRENT PCB.
Value vmCallOther(const Value& target, const std::string& funName,
                  const std::vector<Value>& args);

// Broadcast (vm_callInOtherArray2/3 0x80bcf14/0x80bcdec): call `funName` on
// every object of `targets` on the current PCB, collecting results into an
// array via return hooks.
Value vmCallOtherArray(const std::vector<Value>& targets, const std::string& funName,
                       const std::vector<Value>& args);

// ---- scheduler -------------------------------------------------------------
class Scheduler {
public:
    static Scheduler& instance();

    struct Impl;                          // pimpl (sched.cpp)
    ~Scheduler();

    // Efun-layer hooks (registered at boot by the adapter):
    // queueRecheck — does queue `qkey` already hold pending items? (the
    //   park lost-wake guard needs the queue store, which lives in the
    //   efun layer).
    static std::function<bool(const std::string&)> queueRecheck_;
    // eventPeek / eventConsume — the raised-bit store is the DRIVER's event
    //   registry (builtins.cpp DriverEvent; raise_event from any thread ORs
    //   into it).  The scheduler consults it so parked waiters and cond-var
    //   waiters see ONE store, never two.
    static std::function<uint32_t(const std::string&)> eventPeekHook_;
    static std::function<void(const std::string&, uint32_t)> eventConsumeHook_;
    // sockPoll — select/poll over fds; returns ready counts in rd/wr.
    static std::function<int(std::vector<int>&, std::vector<int>&,
                             std::vector<int>&)> sockPollHook_;

    // Spawn a process (create_thread / set_timer / post_init / network
    // callback / driver entry).  Runs on the scheduler thread.
    VmPcb* spawn(VmObject* obj, const std::string& entry, std::vector<Value> args);
    // create_thread with an efun closure: a process whose first run applies
    // the closure's efun (see VmPcb::threadClosure).
    VmPcb* spawnEfunThread(VmObject* owner,
                           const lpc::Ref<struct ClosureValue>& cl);
    VmPcb* spawnTelnet(VmObject* obj, const std::string& entry,
                       std::vector<Value> args, int telnetCno);
    // Delayed one-shot process (set_timer's fire).  `cancelled` is the
    // timer registry's flag: delete_timer sets it and the due-timer scan
    // drops the entry without running it.
    VmPcb* spawnDelayed(VmObject* obj, const std::string& entry,
                        std::vector<Value> args, int64_t atMs,
                        std::shared_ptr<std::atomic<bool>> cancelled = nullptr);
    // Delayed EFUN-closure process (set_timer with (: call_other, ob, "fn" :)
    // etc.): the fire applies the closure's efun on its own process — the
    // same dispatch as spawnEfunThread.  Scheduling it as a plain named entry
    // made the fire look "call_other" up in the owner's program table,
    // findFn failed and the scheduler chain-aborted the callback (5493
    // aborted NPC timer fires; every RPC reply wait never woke).
    VmPcb* spawnDelayedEfun(VmObject* obj,
                            const lpc::Ref<struct ClosureValue>& cl,
                            int64_t atMs,
                            std::shared_ptr<std::atomic<bool>> cancelled = nullptr);
    // Construction process (load_object / clone_object / master boot): the
    // plan is prebuilt (buildConstructPlan); runProcess drives its steps,
    // parks mid-step like any chain, and on completion marks the object
    // initialized + raises the "obj-init:<key>" event for the parked loader.
    VmPcb* spawnConstruct(std::shared_ptr<struct ConstructPlan> plan);

    // Park the current process (called when a blocking efun threw
    // SuspendSignal): registers the wake condition and marks Parked.
    void parkCurrent(const ResumeCond& cond);
    // Wake sources.
    void wake(VmPcb* pcb);                       // requeue (tail)
    void wakeFront(VmPcb* pcb);                  // requeue (front)
    void requeue(VmPcb* pcb);                    // time-slice yield (tail)
    void wakeQueue(const std::string& qkey);
    // One waiter per element, delivered at send time (the original's
    // reservation model: vm_f_send_to_queue resumes ONE pPendingPCB and
    // reserves the element for it — vm_acquiredValueFromQueue takes it).
    void wakeQueueOne(const std::string& qkey);
    void wakeEvent(const std::string& ename, uint32_t raisedMask);
    void wakeEventFront(const std::string& ename);
    void wakeSocket(int fd, bool readable, bool writable);

    // External-thread execution: run a call on the scheduler thread and
    // block the caller on the result (the driver stays effectively
    // single-threaded on the VM).
    Value execOnMain(VmObject* obj, const std::string& funName,
                     const std::vector<Value>& args);

    // Listening sockets owned by the main loop (accept on scheduler thread).
    void addServiceSocket(int fd, std::function<bool(unsigned int)> cb);
    bool anyServiceSocket() const;

    // Main loop (schedulerRun): ready queue / due timers / queue-event-socket
    // wakeups / service sockets.  Does not return until shutdown.
    void run();
    void requestShutdown();

    bool isSchedulerThread() const;
    int64_t currentProcessTid() const;

    // ---- process-level (PCB) named mutexes (vm_f_take_mutex 0x80f16cc) ----
    void mutexCreate(const std::string& name);
    int mutexTake(const std::string& name, int timeoutMs, const Value& onTimeout);
    void mutexGive(const std::string& name);
    void mutexErase(const std::string& name);

private:
    Scheduler();
    std::unique_ptr<Impl> impl_;
};

// Milliseconds since driver start (one shared clock for every subsystem).
int64_t msSinceStart();

// Boot wiring for the efun adapter layer (event peek/consume hooks, socket
// poll backend).  Call ONCE before the scheduler runs.
void wireEfunLayer();

} // namespace vmcore
} // namespace lpc
