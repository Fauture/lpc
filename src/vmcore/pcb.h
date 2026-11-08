// ===========================================================================
// vmcore/pcb.h — VmPcb: the process control block (原版 PCB 模型)
//
// 原版实证（IDA, magic_Linux32x）:
//   * 解释器全局只有一套执行栈 vm_stack(0x8661964)/vm_sp(0x86619cc)，
//     栈值压取全部经当前 PCB: vm_push*OfPCB / vm_getPCB*Argument 系列。
//   * vm_getCurrentPCB (0x80a9310) / vm_getPCB (0x80a93d4) — PCB 是进程的
//     唯一状态载体，携带 tid(vm_f_get_thread_id 0x80ed79c)、euid、
//     错误状态(vm_setPCBLastError)。
//   * 阻塞 = PCB 挂起并登记唤醒条件 (vm_pendCurrentProcess)；唤醒 = 同一栈
//     同一帧链从保存的 ip 继续。没有任何"第二执行状态"。
//
// 本文件是该模型在新核中的全部定义。旧核的 per-object ExecState、
// runStates_ 链表、switchToChain、chainSeq、tlsChainTid/tlsChainKey 双轨、
// runDetached detachedStack 在这里没有对应物 — 不是被替换，是结构性不存在:
// 执行状态只有一份，住在 PCB 里。
// ===========================================================================
#pragma once
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "object.h"
#include "o_loader.h"
#include "value.h"

namespace lpc {
namespace vmcore {

// ---------------------------------------------------------------------------
// Stack item — 对齐原版值栈布局：原版 vm_stack 元素是 24B POD
// (type/attrib/union)，引用(vptr)只是其中一种值类型。纯值槽(>95% 的压栈)
// 只携带 Value；引用信息仅引用槽分配(堆上 StackRef)。
// (Same layout the verified implementation used; the stack machine's
// lvalue/reference semantics depend on it: push_obj_vptr/push_global_vptr/
// push_stack_vptr create refs, assignments write through them.)
// ---------------------------------------------------------------------------
struct StackRef {
    int slotIndex = -1;                       // ref to a stack slot
    int globalIndex = -1;                     // ref to a shared global
    int objIndex = -1;                        // ref to an object instance variable
    VmObject* objRef = nullptr;               // object owning objIndex (原版:
                                              // vptr 是对象值数组的地址 — 跨帧
                                              // 传 by-ref 时必须锚定所属对象,
                                              // 而不是读"当前帧"的变量表)
    lpc::Ref<ArrayValue> arr;          // member ref container
    lpc::Ref<MappingValue> map;
    Value key;

    // [perf] 引用槽在热路径上高频生灭 (push_vptr 族指令 / m[k]=v / 传引用
    // 参数) —— 全局 malloc/free 曾占调度器执行样本的一半 (stackdump 实锤
    // Interp::makeSlotRef -> operator new)。走线程本地自由链池。
    static void* operator new(std::size_t) { return VmPool<StackRef>::pop(); }
    static void operator delete(void* p) noexcept { VmPool<StackRef>::push(p); }
};

struct StackItem {
    Value value;                              // 纯值槽的唯一有效成员
    std::unique_ptr<StackRef> ref;            // 引用槽才分配 (vptr 槽)
    bool isRef() const { return ref != nullptr; }
};

// ---------------------------------------------------------------------------
// Catch record: an in-flight LPC catch's unwind target.  On vmError the
// innermost record wins: unwind stack/frames to the recorded sizes and resume
// at ipTarget (原版 vm_traceError → catch 恢复语义; 被捕获的错误也必须留痕).
// ---------------------------------------------------------------------------
struct CatchRecord {
    size_t ipTarget = 0;     // resume ip when an error is caught
    size_t stackSize = 0;    // stack size to unwind to
    size_t frameCount = 0;   // frames to keep
};

// ---------------------------------------------------------------------------
// ResumeCond — what a parked PCB waits for, and the value pushed on wake.
// (Verified kinds from the scheduler implementation; kind numbers are ours,
// semantics are the original's: timer / queue / event-mask / forever / socket /
// mutex.  raise_event wakes ONLY waiters whose mask intersects the raised
// bits — vm_f_raise_event semantics.)
// ---------------------------------------------------------------------------
struct ResumeCond {
    // kind: 0 = not parked, 1 = timer (wakeAtMs), 2 = queue (qkey),
    //       3 = event (ename + mask), 4 = forever, 5 = socket (fd),
    //       6 = mutex (ename = mutex name)
    int kind = 0;
    int64_t wakeAtMs = 0;       // kind 1
    std::string qkey;           // kind 2
    std::string ename;          // kind 3 / kind 6
    uint32_t mask = 0;          // kind 3: wait bits this park consumes
    int fd = -1;                // kind 5
    bool wantRead = false;      // kind 5
    bool wantWrite = false;     // kind 5
    int64_t timeoutAtMs = 0;    // kind 5: wake anyway at this time
    Value resumeValue;          // pushed onto the stack when resumed
};

// ---------------------------------------------------------------------------
// Signals — NOT derived from std::exception (so the interpreter's
// catch(VmError)/catch(std::exception) handlers never swallow them); every
// C++ catch(...) boundary must re-throw them unchanged.
//
// 与旧核的关键差别: 信号不携带 VM 指针。执行状态只有一份, 住在当前 PCB 里,
// 跨对象调用本身就是当前 PCB 上的帧, 所以阻塞永远阻塞"当前 PCB" —
// 调度器挂起 currentPcb() 即可, 不存在"信号发生在另一个 VM 上"的歧态。
// ---------------------------------------------------------------------------
struct SuspendSignal {};        // blocking efun: park currentPcb with its cond
struct ThreadExitSignal {};     // exit/exit_thread/clear_env: end this chain
struct TimeSliceSignal {};      // time-slice yield: requeue currentPcb at tail

// ---------------------------------------------------------------------------
// VmFrame — the funContext (原版 vm_callInUserFunBody 压的帧).
//
// 上下文住进帧: 帧绑定 (对象实例 obj, 程序 prog, 函数 fn)。解释器取
// this_object/实例变量表/函数索引表/字符串表永远从当前帧取; 帧空则用
// 驱动兜底上下文。压帧即切换, 弹帧即恢复 — 天然配对。
//
// 旧核 Frame 的 savedObjVars / savedFuncLookup / funcLookupSwitched /
// thisNameOverride / thisObjOverride 在这里没有对应物: 那些字段是
// "上下文存 VM 成员、帧负责恢复"模型的补丁, 该模型已消亡。
// ---------------------------------------------------------------------------
struct VmFrame {
    const OriginalFunction* fn = nullptr;
    // The object whose function this frame runs: this_object() identity AND
    // the instance-variable table its obj-var accesses land in
    // (vars[varOffset + index]).  Null only for a driver-level boundary
    // frame that runs no bytecode of its own.
    VmObject* obj = nullptr;
    // The program whose merged function table the frame's local-index calls
    // resolve against (obj->prog for object functions; the master program
    // for global functions — vm_callInUserFun reads
    // vm_pGlobalProgram->pFunctions[index] regardless of the calling object).
    const VmProgram* prog = nullptr;
    int bp = 0;                       // stack index of arg0 (param0 = top)
    int argNum = 0;
    int localCount = 0;
    // String table of the PROGRAM the function belongs to (inherited
    // functions carry their own program's table; fn->strings overrides).
    const std::vector<std::string>* strings = nullptr;
    // MAX argument count (declared + defaulted) as recorded by the last
    // check_args_num (op 15) of this frame — resort_stack needs it.
    int maxArgNo = 0;
    // Accumulated baseVarOffset / baseFunOffset of the function's
    // constitution (vm_pushObjVar 0x8091224 / vm_callInUserFun 0x8094080).
    int varOffset = 0;
    int funOffset = 0;
    // Explicit-stack-machine return context: the caller's code/meta/ip that
    // execution resumes at when this frame returns (vm_returnFromFun pops the
    // caller's funContext and jumps back).  retCode == nullptr marks a
    // top-level boundary frame: returning from it ends the chain.
    const std::vector<OriginalInsn>* retCode = nullptr;
    const std::vector<uint8_t>* retMeta = nullptr;
    size_t retIp = 0;
    // Entered with the zero-arg placeholder slot (bp >= 0 needs one slot).
    bool zeroArgPlaceholder = false;
    // Zero-pad slots inserted BELOW the passed args for omitted DECLARED
    // parameters (the original zero-inits the whole declared param area).
    int padArgs = 0;
    int passedArgs = -1;
    // Loop control blocks (init_forLoop value type 20): kept per-frame so
    // pops below the local region cannot corrupt an enclosing loop's state.
    std::map<int, lpc::Ref<LoopCtrl>> loops;
    // vm_hookCallbackWhenReturn (0x8094e70): hooks run when this frame
    // returns, in registration order, with the return value (call_other
    // broadcast collects its results here; every cross-call post-processing
    // that runDetached used to do belongs in a hook).
    std::vector<std::function<void(Value&)>> retHooks;
};

// ---------------------------------------------------------------------------
// VmPcb — the process.  ALL execution state lives here (the original's PCB
// plus the funContext chain it resumes through).  Nothing is switched, saved
// or restored anywhere: parking simply leaves this struct as-is and waking
// re-enters the interpreter with it.
// ---------------------------------------------------------------------------
struct VmPcb : std::enable_shared_from_this<VmPcb> {
    // ---- execution state (唯一执行状态) ----
    // [perf] 预留容量: 4096 槽 × 48B ≈ 192KB。羽化链栈深 ~150 槽, 但
    // push_back 倍增扩容会周期性搬移整个数组 (几百 µs 抖动), 预留后
    // 稳态零 realloc。
    std::vector<StackItem> stack;
    std::vector<VmFrame> frames;
    std::vector<CatchRecord> catches;
    VmPcb() {
        stack.reserve(4096);
        frames.reserve(256);
    }
    int catchDepth = 0;
    const std::vector<OriginalInsn>* code = nullptr;
    const std::vector<uint8_t>* meta = nullptr;
    size_t ip = 0;
    Value returnValue = make_int(0);
    bool returned = false;
    int lastOp = -1;
    size_t lastIp = 0;

    // ---- park state (vm_pendCurrentProcess) ----
    ResumeCond cond;                 // kind != 0 while parked
    Value resumeValue = make_int(0);
    // Queue delivery (the original's reservation model): a send hands its
    // element to ONE waiter directly at send time — the item leaves the
    // queue immediately, so no other receiver can steal it before the
    // waiter's resume hook runs.
    Value queueDelivery{};
    bool hasQueueDelivery = false;
    // Resume hook (原版 parkAndSuspend 的 hook 参数): 唤醒条件命中后、
    // 回到解释器之前运行 — recv/select/receive_from_queue 在这里把数据
    // 写穿引用并设定 resumeValue。park 时由阻塞 efun 设置, 运行后清除。
    std::function<void()> resumeHook;
    // 停泊期间的引用槽暂存 (move-only lambda 不能进 std::function):
    // efun 把 out-ref 槽 move 进来, resumeHook 按索引写回; 新停泊时清空。
    std::vector<StackItem> parkedItems;

    // ---- process identity ----
    int64_t tid = -1;                // tid pool (0..127, 单调兜底 ≥128)
    int euid = 0;
    std::string previousObjectName;  // previous_object() in the callee

    // ---- lifecycle ----
    enum class State { Ready, Running, Parked, Done };
    State state = State::Ready;
    std::string entry;               // function to call on first run
    std::vector<Value> args;         // entry arguments
    bool firstRun = true;            // false => resume an existing chain
    VmObject* obj = nullptr;         // spawn target object (entry's object)
    // ---- construction plan (load_object / clone_object / master boot) ----
    // 原版 vm_f_construct_object2: 构造 INLINE 在当前进程执行流上
    // (vm_callInUserFunBody 当前栈直调 create, vm_hookCallbackWhenReturn
    // 推进下一步), 嵌套构造靠 funContext 栈。对应物: construct = 当前
    // (栈顶)计划, constructStack = 嵌套构造计划栈 (后进先出)。
    std::shared_ptr<struct ConstructPlan> construct;
    std::vector<std::shared_ptr<struct ConstructPlan>> constructStack;

    // create_thread with an EFUN closure ((:call_other, ob, "loop_fn":)):
    // the thread's body is an efun call (whose target may loop forever —
    // stalld's while(1)+sleep(180) daemon loop).  It must run on ITS OWN
    // process, never inline on the spawner (the boot master chained into
    // stalld's infinite loop and never reached 8160).
    lpc::Ref<struct ClosureValue> threadClosure;

    // ---- wake bookkeeping ----
    bool inReady = false;            // sits in the ready queue (no double-enqueue)
    bool eventWoken = false;         // event fired before its timeout timer
    bool timedEvent = false;         // kind-3 wait with a timeout
    int64_t parkAtMs = 0;            // park 时刻(msSinceStart)，PARKDBG 测停泊时长
    int64_t lastWakeMs = 0;          // 上次唤醒时刻 —— park 时 ran=parkAtMs-lastWakeMs 即本轮执行时长
    std::shared_ptr<std::atomic<bool>> parkTimerCancelled;
    std::shared_ptr<void> timerEntry;   // set_timer cancellation token

    // ---- external-thread call (execOnMain) ----
    std::shared_ptr<std::promise<Value>> resultPromise;

    // [ref] 唤醒外部等待者前清空 pcb 携带的全部 lpc Value: Ref/SharedString
    // 计数非原子, 纪律 = 调度线程在 notify 之前把本 pcb 对所有 Value 块的
    // 引用全部释放, 此后调用方线程析构自己的入参拷贝时无人并发计数。
    void stripValues() {
        stack.clear();
        frames.clear();      // 帧内的 loops map 随帧析构一并释放
        catches.clear();
        threadClosure.reset();
        constructStack.clear();
    }

    // ---- connection accounting (driver bookkeeping, not VM semantics) ----
    int64_t clientNo = -1;
    int telnetCno = -1;

    // ---- time slice (master process: 原版 runTicks) ----
    int yieldEvery = 0;
    int yieldCountdown = 0;
    uint64_t instrCount = 0;   // [loopsplit] 本次 run() 已执行指令数
    // True when the last exit was a runTicks/relinquish SLICE yield (not a
    // park): the resume continues WITHOUT pushing a value — relinquish()
    // pushed its 1 before yielding (the old core's exact rule).
    bool sliceYield = false;
    // Construct process running on the INITIATOR's tid (原版 inline 构造语义):
    // the initiator is parked while this process runs, so the same tid is
    // safe — and TidBack must NOT release it back to the pool.
    bool borrowedTid = false;
    // Inline construction took over the execution flow (frame pushed, the
    // object value is pushed by the retHook chain on completion): dispatchEfun
    // must NOT push its placeholder — the frames.size() heuristic fails for
    // synchronously-finished constructions (frames pop back to the same depth).
    bool efunTakeover = false;

    // ---- diagnostics ----
    std::string frameChainText(size_t maxFrames = 6) const;
};

// ---------------------------------------------------------------------------
// TidPool — 原版 PCB->tid: 0..N-1 唯一分配, 进程生命期不变, 结束归还。
// 池耗尽时 alloc() 返回 -1 — 调用方 (调度器) 把 PCB 挂入待分配队列,
// 等 release 归还后再运行 (原版 max_threads 上限 = 同时运行的进程数上限,
// 超出的任务在任务表里排队, 绝不给 ≥N 的越组 tid — 那会让
// ev_filed<tid/32> 等到不存在的事件组, 进程死等, tid 永不归还)。
// ---------------------------------------------------------------------------
class TidPool {
public:
    TidPool();
    int64_t alloc();                    // -1 = 池空 (调用方排队)
    void release(int64_t tid);
private:
    std::mutex mtx_;
    std::vector<int64_t> free_;        // 0..1023
};

} // namespace vmcore
} // namespace lpc
