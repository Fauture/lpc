// ===========================================================================
// vmcore/call.cpp — 调用原语 (与 vm_callInUserFunBody 0x8093eb0 同构)
//
// 唯一调用路径: 本地调用、继承调用、call_other、闭包、广播——全部 =
// "在当前 PCB 的栈上压一个绑定 (obj, prog, fn) 的帧 + 返回弹帧 (+return hook)"。
// 没有跳转、没有第二执行状态、没有桥接。
//
// 返回语义 (vm_returnFromFun 0x8094978, 已实证搬运):
//   * op 72: 返回值 = 栈顶。
//   * op 73: 栈形判 void — size==bp+1 → 压 undefined; size==bp+2 → 弹返回值;
//     否则取栈顶兜底。
//   * 弹帧后栈清到 newSize = bp - argNum + 1 - zeroArgPlaceholder - padArgs。
//   * 上下文帧 (fn==nullptr, 全局 init 段) 不弹 — 由压它的驱动代码弹。
//   * 帧内开的 catch 区随帧关闭 (vm_endThreadCatch)。
//   * 边界帧 (retCode==nullptr) 弹出 = 整条链结束。
// ===========================================================================
#include <atomic>
#include <cstring>

#include "call.h"
#include "interp.h"
#include "original_vm.h"
#include "driver_model.h"
#include "values.h"
#include "vmcore.h"
#include "trace.h"

namespace lpc {
namespace vmcore {

// [性能] CTRACE 环境开关启动时缓存 (construct-done 热路径)。
static const bool g_ctraceOn = std::getenv("LPCVM_CTRACE") != nullptr;

using Interp::pushItem;
using Interp::pushValue;
using Interp::popValue;
using Interp::popItem;
using Interp::makeItem;

// popValue for C++-driven nested calls (vmCallOther / vmApplySync /
// vmCallOtherArray).  When the nested chain ended through the
// ThreadExitSignal path (LPC exit()/exit_thread()/clear_env: the interp loop
// CLEARS frames+stack and returns instead of rethrowing), the stack is EMPTY
// and popValue's stack.back() reads uninitialized memory — the popped
// StackItem's shared_ptr then destructs a garbage control block, which CFG
// traps as __castguard_check_failure_os_handled_fptr (0xc0000005 write,
// gs boot crashed twice at the same spot).  Fall back to the chain's return
// value and leave a trace instead.
static Value popValueNested(VmPcb* pcb) {
    if (!pcb->stack.empty()) return popValue(pcb);
    std::cerr << "[pop-guard] nested pop on EMPTY stack tid=" << pcb->tid
              << " frames=" << pcb->frames.size()
              << " returned=" << pcb->returned << "\n";
    std::cerr.flush();
    return pcb->returned ? pcb->returnValue : Value{};
}

// ---------------------------------------------------------------------------
// push helpers — args go on the stack param0-on-top (原版压参布局)。
// 外部 API 入口: 缺省的声明参数补 undefined (type 0), 不是 0 —
// gs::prompt_by 两参调用时 portrait 必须 undefined 才走 is_undefined 分支;
// 声明了默认值的参数由被调方 check_args_num 填。
// ---------------------------------------------------------------------------
static void pushArgs(VmPcb* pcb, const OriginalFunction* fn,
                     const std::vector<Value>& args) {
    // Push ONLY the caller-supplied args (param0 ends on top).  Do NOT pad
    // missing params with placeholders: the callee's check_args_num +
    // resort_stack prologue executes the DECLARED DEFAULTS for them
    // (original vm semantics).  Padding here made checkargs see
    // passed==argCount and skip the default block — build_msg_message's
    // server/show_extra reached the wire as ""/0 and chat packets lost the
    // 线路 (server name) field.
    const int passed = static_cast<int>(args.size());
    const int n = fn ? fn->argCount : 0;
    if (passed == 0 && n == 0) {
        pushItem(pcb, makeItem(Value{}));   // frame slot so bp >= 0
        return;
    }
    for (int i = passed - 1; i >= 0; --i) {
        const Value& v = args[static_cast<size_t>(i)];
        // CrossVmRef 载体 (dispatchEfunClosure 的转发型引用): 还原为栈引用
        // 槽 —— 原版 vm_callFunPtr 把 pArgs 原样压栈 (vptr 保留), 被调方
        // 参数槽就是同一 vptr, 读 (slotValue) / 写 (op77 storeItem) 都落在
        // 调用方的槽上。
        if (v.index() == 11) {
            const auto& r =
                std::get<std::shared_ptr<CrossVmRef>>(v);
            if (r && r->slotIndex >= 0) {
                StackItem ref;
                ref.isRef = true;
                ref.slotIndex = r->slotIndex;
                pushItem(pcb, ref);
                continue;
            }
        }
        pushValue(pcb, v);
    }
}

// The frame's argNum: how many args the caller REALLY supplied (not the
// declared count) — check_args_num compares against this.
static int passedArgCount(const std::vector<Value>& args) {
    return static_cast<int>(args.size());
}

// Resolve the frame's string table: the function's own table when the .o
// carries one, else the callee PROGRAM's table — never the caller's.
static const std::vector<std::string>* frameStrings(
    const OriginalFunction* fn, const VmProgram* prog) {
    const std::vector<std::string>* tbl =
        (fn && fn->strings && !fn->strings->empty()) ? fn->strings.get() : nullptr;
    if (!tbl && prog) tbl = &prog->src().strings;
    return tbl;
}

// ---------------------------------------------------------------------------
// pushFrameInPlace — the ONE frame push.  Args are ALREADY on the stack
// (compiler layout, param0 on top) for bytecode calls; the API flavor
// (vmCallPushFrame) pushes them first via pushArgs.
// ---------------------------------------------------------------------------
bool pushFrameInPlace(VmPcb* pcb, VmObject* obj, const VmProgram* prog,
                      const OriginalFunction* fn, int passedArgs,
                      bool resumeCurrentStream, int padArgs) {
    if (!pcb || !fn) return false;
    if (pcb->stack.empty()) {
        // Caller-less stream (construct chain / global init segment): the
        // original's stack ALWAYS has a bottom slot under the running frame —
        // seed one here so the frame's bp has something to anchor to.
        Interp::pushValue(pcb, Value{});
    }
    VmFrame f;
    f.fn = fn;
    f.obj = obj;
    f.prog = prog;
    f.bp = static_cast<int>(pcb->stack.size()) - 1;
    f.argNum = passedArgs >= 0 ? passedArgs : fn->argCount;
    f.localCount = fn->localCount;
    f.strings = frameStrings(fn, prog);
    f.varOffset = fn->varOffset;
    f.funOffset = fn->funOffset;
    f.padArgs = padArgs;
    f.passedArgs = passedArgs;
    if (resumeCurrentStream) {
        f.retCode = pcb->code;
        f.retMeta = pcb->meta;
        f.retIp = pcb->ip;      // ip already points past the calling opcode
    }
    pcb->frames.push_back(std::move(f));
    pcb->code = &fn->code;
    pcb->meta = &fn->metaData;
    pcb->ip = 0;
    pcb->returnValue = Value{};   // 占位: 原版 pushVoid 语义
    pcb->returned = false;
    return true;
}

bool vmCallPushFrame(VmPcb* pcb, VmObject* obj, const VmProgram* prog,
                     const OriginalFunction* fn, const std::vector<Value>& args,
                     int padArgs) {
    if (!pcb || !fn) return false;
    pushArgs(pcb, fn, args);
    const bool ok = pushFrameInPlace(pcb, obj, prog, fn,
                                     passedArgCount(args), true, padArgs);
    // [fix] 同 vmCallOtherPush: 0 参调用的占位槽标记。
    if (ok && args.empty() && fn->argCount == 0 && !pcb->frames.empty())
        pcb->frames.back().zeroArgPlaceholder = true;
    return ok;
}

// ---------------------------------------------------------------------------
// vm_hookCallbackWhenReturn (0x8094e70)
// ---------------------------------------------------------------------------
void vmHookCallbackWhenReturn(VmPcb* pcb, std::function<void(Value&)> hook) {
    if (!pcb || !hook || pcb->frames.empty()) return;
    pcb->frames.back().retHooks.push_back(std::move(hook));
}

// ---------------------------------------------------------------------------
// vmReturnFromFun — pop semantics (see file header).
// ---------------------------------------------------------------------------
int vmReturnFromFun(VmPcb* pcb, int op) {
    if (pcb->frames.empty()) {
        pcb->returned = true;
        pcb->catches.clear();
        return Interp::RET_CHAIN_END;
    }
    const VmFrame f = pcb->frames.back();   // copy: popped below
    if (op == 72) {
        if (!pcb->stack.empty()) pcb->returnValue = Interp::popValue(pcb);
        else pcb->returnValue = Value{};   // 栈空: void
    } else {
        // 原版 vm_returnFromFun (0x8094978): 残留槽数 v8 相对
        // "参数+局部全在栈上" 的期望位置计 —— v8 = bp - localCount - sp
        // (原版向下栈)。本 VM 向上栈等价: 期望 void 位置 =
        // bp + 1 + localCount (参数 [bp-argNum+1..bp], 局部
        // [bp+1..bp+localCount] 由 op27 enter 压入)。
        // 此前漏加 localCount: localCount==1 的函数正常跑完 ret 时
        // stack == expectedVoid+1 被误判为"返回了 1 个值", 局部槽残留
        // 内容成了返回值 —— test_popup_menu 局部槽恰有玩家对象,
        // create_menu 的 is_object 判真 → 剧情战斗走错菜单分支
        // (lw108 fn-trace 逐指令栈深实锤: bp=87 stack=89 localCount=1)。
        const size_t expectedVoid = static_cast<size_t>(f.bp) + 1 +
                                    static_cast<size_t>(f.localCount);
        const size_t residual =
            pcb->stack.size() > expectedVoid
                ? pcb->stack.size() - expectedVoid : 0;
        if (residual == 0)
            pcb->returnValue = Value{};               // void return (undefined)
        else if (residual == 1)
            pcb->returnValue = Interp::popValue(pcb);
        else {
            // 原版: 无原型函数残留>1 = vm_endProcess; 有原型 = 报错。
            // 本 VM 保留 take top 兜底 (栈顶恰为返回值, 行为与修复前一致);
            // 泄漏取证用 LPCVM_RETMIS 热布防 (boot 期上万条 stderr 拖启动)。
            static const std::atomic<bool>& rmFlag =
                lpc::dynProbeFlag("LPCVM_RETMIS");
            // [target] _do_combat_team 栈残留直接导致剧本刷怪断链
            // (join_combat_team 后 add_member 不执行 → 战斗误判结束 →
            //  scenario26/五色灵石永不触发), 对它单独取证, 不占通用配额。
            const bool targetFn = f.fn && f.fn->name == "_do_combat_team";
            if (rmFlag.load(std::memory_order_relaxed) || targetFn) {
                std::cerr << "[ret-mismatch] fn="
                          << (f.fn ? f.fn->name : std::string("<ctx>"))
                          << " residual=" << residual
                          << " bp=" << f.bp << " locals=" << f.localCount
                          << " depth=" << pcb->stack.size() << "\n";
                static std::atomic<int> mmShown{0};
                if (f.fn && (targetFn || mmShown.fetch_add(1) < 400)) {
                    size_t hist[16] = {};
                    for (size_t i = expectedVoid;
                         i < pcb->stack.size() && i < expectedVoid + 40; ++i) {
                        unsigned k = pcb->stack[i].value.index();
                        if (k < 16) hist[k]++;
                    }
                    std::cerr << "[ret-mismatch]   slots idx:";
                    for (int k = 0; k < 16; ++k)
                        if (hist[k])
                            std::cerr << " " << k << "x" << hist[k];
                    std::cerr << " caller="
                              << (pcb->frames.empty() ||
                                          !pcb->frames.back().fn
                                      ? "<ctx>"
                                      : pcb->frames.back().fn->name)
                              << "\n";
                    // [target] 目标函数额外打每个残留槽的短值, 定位压栈指令
                    if (targetFn) {
                        size_t n = 0;
                        for (size_t i = expectedVoid;
                             i < pcb->stack.size() && n < 20; ++i, ++n)
                            std::cerr << "[ret-mismatch]     slot[" << n
                                      << "] = "
                                      << to_string(pcb->stack[i].value, false)
                                             .substr(0, 80)
                                      << "\n";
                    }
                }
                std::cerr.flush();
            }
            if (!pcb->stack.empty())
                pcb->returnValue = Interp::popValue(pcb);
            else
                pcb->returnValue = Value{};
        }
    }
    pcb->returned = true;
    if (vmcore::traceOn()) {
        const std::string fnName = f.fn ? f.fn->name : std::string("<ctx>");
        if (vmcore::traceWant(fnName))
            std::cerr << "[ret] fn=" << fnName
                      << " val=" << to_string(pcb->returnValue, false).substr(0, 60)
                      << " obj=" << (f.obj ? f.obj->key() : std::string("<null>"))
                      << " depth=" << pcb->stack.size() << "\n";
    }
    // Context frame (fn == nullptr, a global-init segment): do NOT pop —
    // the code that pushed it pops it; just end this segment.
    if (f.fn == nullptr) {
        while (!pcb->catches.empty() &&
               pcb->catches.back().frameCount >= pcb->frames.size())
            pcb->catches.pop_back();
        return Interp::RET_CHAIN_END;
    }
    // Catch regions opened inside the returning frame close with it.
    while (!pcb->catches.empty() &&
           pcb->catches.back().frameCount >= pcb->frames.size())
        pcb->catches.pop_back();
    // Stack cleanup: bp - argNum + 1 - placeholder - pads (live frame values —
    // resort_stack may have moved the base and updated argNum).
    int newSize = f.bp - f.argNum + 1 - (f.zeroArgPlaceholder ? 1 : 0) - f.padArgs;
    if (newSize < 0) newSize = 0;
    const std::vector<OriginalInsn>* retCode = f.retCode;
    const std::vector<uint8_t>* retMeta = f.retMeta;
    const size_t retIp = f.retIp;
    std::vector<std::function<void(Value&)>> hooks =
        std::move(pcb->frames.back().retHooks);
    pcb->frames.pop_back();
    // 原版 0x4000 语义 (vm_destructObject 0x80b1894): 帧弹出使 execRef 回落,
    // 该对象若挂有待真释放 (pendingFree) 且帧栈已无它的帧 → 立即真释放。
    // 同一条命令链内 dismiss 后紧接的 is_object (all_team 复用点) 由此才能
    // 判假 (此前拖到整链 Done 才 flush, 中途判真复用了僵尸 team, 星君变身
    // 被跳过)。pendingFrees 为空时仅一次空检查即返回。
    vmFlushPendingFrees(static_cast<const void*>(pcb));
    // Tripwire: 截断范围内若有活跃 LoopCtrl 槽 (index 9) —— 帧尺寸算错，
    // post_init "Bad loop ctrl" 元凶现场。
    // [性能] dynProbeFlag 门控 (默认关): 构造期每次帧返回都扫, 命中十余万
    // 次, 无缓冲 stderr 把 boot 拖慢 2 倍+ (lw47 实测 21.3 万条噪声)。
    static const std::atomic<bool>& slotSnagR =
        dynProbeFlag("LPCVM_SLOTSNAG");
    if (slotSnagR.load(std::memory_order_relaxed) &&
        pcb->stack.size() > static_cast<size_t>(newSize))
        for (size_t i = static_cast<size_t>(newSize); i < pcb->stack.size(); ++i)
            if (pcb->stack[i].value.index() == 9)
                std::cerr << "[loop-slot-kill] tid=" << pcb->tid
                          << " frames=" << pcb->frames.size()
                          << " resize-drop slot=" << i
                          << " newSize=" << newSize
                          << " retfn=" << (f.fn ? f.fn->name : std::string("<ctx>"))
                          << " bp=" << f.bp << " argNum=" << f.argNum << "\n";
    const int consDbgDepthIn = static_cast<int>(pcb->stack.size());
    pcb->stack.resize(static_cast<size_t>(
        newSize > static_cast<int>(pcb->stack.size())
            ? pcb->stack.size()          // 帧清栈只缩不扩: 扩=核算错误,
                                         // 扩出来的空槽就是 residual 泄漏
            : newSize));
    {
        // [consdbg-ret] 构造帧 ret 边界栈账 (lw122: 递归构造每层 +1,
        // add_goods residual=20 主力 —— 需要 ret 侧 newSize/depth 实锤)。
        static const std::atomic<bool>& cdr =
            lpc::dynProbeFlag("LPCVM_CONSDBG");
        if (cdr.load(std::memory_order_relaxed) && f.fn &&
            (f.fn->name == "<init-seg>" || f.fn->name == "create"))
            std::cerr << "[consdbg-ret] fn=" << f.fn->name
                      << " bp=" << f.bp << " argNum=" << f.argNum
                      << " ph=" << (f.zeroArgPlaceholder ? 1 : 0)
                      << " pads=" << f.padArgs << " depthIn="
                      << consDbgDepthIn
                      << " newSize=" << newSize << "\n";
    }
    {
        // [retw] 栈深平衡审计: op7 进入侧 [op7w] 打了 preStack, 正常返回
        // newSize == preStack(收缩回调用点) —— 差非 0 即栈漂移现场。
        static const bool wret = getenv("LPCVM_LOOPWATCH") != nullptr;
        if (wret && f.fn) {
            const char* rn = f.fn->name.c_str();
            if (!std::strcmp(rn, "create_search_terms") ||
                !std::strcmp(rn, "query_objects") ||
                !std::strcmp(rn, "sql_select") ||
                !std::strcmp(rn, "create_bit_search") ||
                !std::strcmp(rn, "do_equipment_filter") ||
                !std::strcmp(rn, "mdb_select") ||
                !std::strcmp(rn, "revise_scan_items") ||
                !std::strcmp(rn, "covert_string") ||
                !std::strcmp(rn, "build_xiaobssb_equip_buf"))
                std::cerr << "[retw] tid=" << pcb->tid << " " << rn
                          << " newSize=" << newSize
                          << " bp=" << f.bp << " argNum=" << f.argNum
                          << " ph=" << (f.zeroArgPlaceholder ? 1 : 0)
                          << " depth=" << pcb->stack.size() << "\n";
        }
    }
    if (retCode == nullptr) {
        // Boundary frame: the chain is done; result stays in returnValue.
        // retHooks MUST still run: a single-step construct plan's Init frame
        // IS a boundary frame (resumeCurrentStream captured a null stream),
        // and its hook is the only thing that calls vmConstructFinish —
        // dropping hooks here left the master <main> plan unfinished forever
        // ([construct-nopark] steps=1/1, boot dead after "Driver startup",
        // 8160 never opened).
        for (auto& hook : hooks) {
            Value hv = pcb->returnValue;
            const size_t framesBefore = pcb->frames.size();
            hook(hv);
            pcb->returnValue = hv;
            if (pcb->frames.size() > framesBefore)
                return Interp::RET_CONTINUE;   // hook pushed a frame: keep running
        }
        return Interp::RET_CHAIN_END;
    }
    pcb->code = retCode;
    pcb->meta = retMeta;
    pcb->ip = retIp;
    // Return hooks: broadcast collection, efun per-element loops, cross-call
    // post-processing.  A hook may push the next frame of its own (broadcast
    // continuation) — then it consumed the return value and the loop
    // continues into the new callee.
    for (auto& hook : hooks) {
        Value hv = pcb->returnValue;
        const size_t framesBefore = pcb->frames.size();
        hook(hv);
        if (pcb->frames.size() > framesBefore) {
            pcb->returnValue = hv;   // stack takeover
            return Interp::RET_CONTINUE;
        }
        pcb->returnValue = hv;
    }
    // [op8ret] post_init 重注册者取证: 所有 query_objects 的返回值 + 调用
    // 者函数名 —— statued/change_lookd/artifactd/vendued 的门槛查询返回
    // 一目了然, 找出每轮 register_post_init 的守护。
    {
        static const std::atomic<bool>& r8 = dynProbeFlag("LPCVM_OP8DBG");
        if (r8.load(std::memory_order_relaxed) && f.fn &&
            (f.fn->name == "query_objects" || f.fn->name == "register_trigger" ||
             f.fn->name == "query_combat_trigger" ||
             (f.fn->name == "get_name" && !pcb->frames.empty() &&
              pcb->frames.back().fn &&
              pcb->frames.back().fn->name == "register_trigger"))) {
            static std::atomic<int> r8Log{0};
            if (r8Log.fetch_add(1) < 300) {
                size_t asz = 0;
                if (pcb->returnValue.index() == 5 &&
                    std::get<std::shared_ptr<ArrayValue>>(pcb->returnValue))
                    asz = std::get<std::shared_ptr<ArrayValue>>(
                              pcb->returnValue)
                              ->items.size();
                std::string extra;
                if (f.fn->name == "get_name" &&
                    pcb->returnValue.index() == 4) {
                    const std::string s = std::get<SharedString>(
                                              pcb->returnValue)
                                              .str();
                    std::string hex;
                    for (unsigned char ch : s) {
                        char b[4];
                        snprintf(b, sizeof(b), "%02x", ch);
                        hex += b;
                    }
                    extra = " hex=" + hex;
                }
                std::cerr << "[op8ret] " << f.fn->name << " ret type="
                          << pcb->returnValue.index()
                          << " arrSize=" << asz << extra
                          << " depth=" << pcb->stack.size()
                          << " caller="
                          << (pcb->frames.empty() || !pcb->frames.back().fn
                                  ? "<ctx>"
                                  : pcb->frames.back().fn->name)
                          << "@"
                          << (pcb->frames.back().obj
                                  ? pcb->frames.back().obj->key()
                                  : "<null>")
                          << "\n";
            }
        }
    }
    Interp::pushValue(pcb, pcb->returnValue);
    return Interp::RET_CONTINUE;
}

// ---------------------------------------------------------------------------
// Top-level entry / resume
// ---------------------------------------------------------------------------
Value vmRunFunction(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                    const std::vector<Value>& args) {
    VmPcbSetter setPcb(pcb);
    pushArgs(pcb, fn, args);
    pushFrameInPlace(pcb, obj, obj ? obj->prog() : nullptr, fn,
                     passedArgCount(args), false, 0);
    Interp::run(pcb);
    return pcb->returnValue;
}

Value vmResume(VmPcb* pcb, const Value& resumeValue) {
    VmPcbSetter setPcb(pcb);
    Interp::pushValue(pcb, resumeValue);
    Interp::run(pcb);
    return pcb->returnValue;
}

Value vmResumeSlice(VmPcb* pcb) {
    VmPcbSetter setPcb(pcb);
    Interp::run(pcb);
    return pcb->returnValue;
}

// ---------------------------------------------------------------------------
// vmApplySync — a C++ caller (efun callback, driver apply) runs a callee to
// completion on the CURRENT PCB: push the frame, run a NESTED loop until that
// frame pops, pop the pushed return value.  vm_apply from inside an efun —
// same stack, same PCB, one extra funContext; balanced for the C++ caller.
// ---------------------------------------------------------------------------
Value vmApplySync(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                  const std::vector<Value>& args,
                  const VmProgram* progOverride) {
    // park 窗口重入取证: cond.kind != 0 说明该 pcb 正 park 在表达式中间
    // (SuspendSignal 已抛、还没 resume) —— 此时往它栈上压帧就是毁链元凶。
    if (pcb && pcb->cond.kind != 0) {
        std::cerr << "[apply-on-parked] kind=" << pcb->cond.kind
                  << " ev=" << pcb->cond.ename << " q=" << pcb->cond.qkey
                  << " fn=" << (fn ? fn->name : "?")
                  << " obj=" << (obj ? obj->key() : std::string("<null>"))
                  << " stack=" << pcb->stack.size()
                  << " frames=" << pcb->frames.size() << "\n";
        std::cerr.flush();
    }
    VmPcbSetter setPcb(pcb);
    const size_t depth = pcb->frames.size();
    pushArgs(pcb, fn, args);
    const bool okA = pushFrameInPlace(pcb, obj, progOverride ? progOverride
                                            : (obj ? obj->prog() : nullptr),
                     fn, passedArgCount(args), true, 0);
    // [fix] 同 vmPushFunCall: 0 参占位槽必须标记, 否则同步 apply 泄漏 1 槽。
    if (okA && args.empty() && fn->argCount == 0 && !pcb->frames.empty())
        pcb->frames.back().zeroArgPlaceholder = true;
    Interp::runNested(pcb, depth);
    return popValueNested(pcb);
}

// ---------------------------------------------------------------------------
// vmPushFunCall — 原版 vm_callInUserFun/vm_callFunPtr 语义的压帧原语：
// 压参 + 压帧（返回当前流），调用方立即返回（efunTakeover）——控制流由
// 指令循环继续进被调帧。供 efun handler 的"压帧即返回"路径使用（closure
// 调用 / destruct 链 / update_config），替代 C++ 同步递归 vmApplySync。
// ---------------------------------------------------------------------------
bool vmPushFunCall(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                   const std::vector<Value>& args,
                   const VmProgram* progOverride) {
    if (!pcb || !fn) return false;
    if (pcb->cond.kind != 0) {
        std::cerr << "[push-on-parked] kind=" << pcb->cond.kind
                  << " ev=" << pcb->cond.ename << " q=" << pcb->cond.qkey
                  << " fn=" << fn->name
                  << " obj=" << (obj ? obj->key() : std::string("<null>"))
                  << " stack=" << pcb->stack.size()
                  << " frames=" << pcb->frames.size() << "\n";
        std::cerr.flush();
    }
    pushArgs(pcb, fn, args);
    const bool ok = pushFrameInPlace(pcb, obj,
                                     progOverride ? progOverride
                                                  : (obj ? obj->prog() : nullptr),
                                     fn, passedArgCount(args), true, 0);
    // [fix] 0 参 0 声明压了 bp 锚占位槽 (pushArgs 75-77), 帧弹出清栈公式
    // 依赖 zeroArgPlaceholder 把它清掉 —— 漏标则每次调用漏 1 个 undefined
    // 槽。destruct 链每段都是 0 参 vmPushFunCall, monster 析构链 8 段 =
    // 每次 destruct_object 泄漏 8 个 undefined 到调用者栈 (fn-trace 实锤:
    // _do_combat_team residual=9/17, do_destruct residual=8; 残留值全是
    // undefined + 栈顶真返回值, 数量=析构段数×被析构对象数) —— 暴怒真君
    // 因此没进防守方 (join_combat_team 后 add_member 不执行), 战斗误判
    // 结束, scenario26/五色灵石永不触发。与 vmCallPushFrame 的标记对齐。
    if (ok && args.empty() && fn->argCount == 0 && !pcb->frames.empty())
        pcb->frames.back().zeroArgPlaceholder = true;
    return ok;
}

// ---------------------------------------------------------------------------
// vmCallOtherPush — 原版 vm_callInOtherFun (0x8094cd8):
//   * 对象值: 零查找 — vmObj 指针即对象; 析构 → vm_error。
//   * 字符串名: 注册表别名点查; 未加载 → vm_error (原版语义)。
//   Args 已在栈上 (op-8 布局) — 压帧于当前 PCB, by-ref 参数 (slot 引用)
//   指向调用者栈槽, 同栈天然有效。
// ---------------------------------------------------------------------------
void vmCallOtherPush(VmPcb* pcb, const Value& target, const std::string& funName,
                     const std::vector<Value>& args) {
    VmObject* obj = nullptr;
    if (target.index() == 7) {
        auto ov = std::get<std::shared_ptr<ObjectValue>>(target);
        if (ov) {
            obj = ov->vmObj;
            if (!obj && !ov->programName.empty())
                obj = Registry::instance().findByName(ov->programName);
        }
    } else if (target.index() == 4) {
        obj = Registry::instance().findByName(std::get<SharedString>(target).str());
    } else {
        vmError("call_other: bad target type.");
    }
    if (!obj)
        vmError("Object is not found (when call in function).");
    if (obj->reallyGone())
        vmError("Object is not found (destructed).");
    const OriginalFunction* fn = obj->prog()->findFn(funName);
    if (!fn) {
        // [fnmiss] call_other 函数解析失败现场: 目标对象/程序/函数名。
        // [原版语义] gs.log(原版) 全程无 "Function ... is not found":
        // inspect_cityd 只定义 init_menu_content, dynamic_menu.c L167
        // (*fa)(this_object()) 注册即调 action 闭包, 原版静默通过 ——
        // 缺失函数 = 压 int 0, 不 vmError。
        static std::atomic<int> fnMissLog{0};
        if (fnMissLog.fetch_add(1) < 20) {
            const auto& progFns = obj->prog()->src().functions;
            std::string near;
            for (const auto& f : progFns)
                if (f.name.find(funName.substr(0, 8)) != std::string::npos) {
                    near += f.name + " ";
                    if (near.size() > 120) break;
                }
            std::cerr << "[fnmiss] obj=" << obj->key()
                      << " progFnCount=" << progFns.size()
                      << " funName=" << funName
                      << " near=[" << near << "]"
                      << " caller="
                      << (pcb->frames.empty() || !pcb->frames.back().obj
                              ? "<null>"
                              : pcb->frames.back().obj->key())
                      << "\n";
            std::cerr.flush();
        }
        Interp::pushValue(pcb, Value{});   // 无效目标: void 结果元素
        return;
    }
    // previous_object() in the callee names the caller.
    VmObject* caller = pcb->frames.empty() ? nullptr : pcb->frames.back().obj;
    pcb->previousObjectName = caller ? caller->key() : std::string();
    {
        // LPCVM_SAVEDBG: trace the logout save chain's call_other hops
        // (user.o::destruct -> userd::destroy_user -> me->save -> userd::
        // save_to_text -> filed write_content_to).  A missing hop pinpoints
        // where the chain silently dies (all mudlib-side, invisible otherwise).
        static const std::atomic<bool>& svFlag = dynProbeFlag("LPCVM_SAVEDBG");
        if (svFlag.load(std::memory_order_relaxed) &&
            (funName == "destroy_user" || funName == "save_to_text" ||
             funName == "write_content_to" || funName == "create_content_to" ||
             funName == "save" || funName == "post_enter_world_failed" ||
             funName == "enter_world" || funName == "start_game" ||
             funName == "select_char" || funName == "check_name_used" ||
             funName == "restore_from_text" || funName == "restore_raw" ||
             funName == "create_gid" || funName == "disconnected" ||
             funName == "create_char" || funName == "apply" ||
             funName == "login" || funName == "post_enter_world")) {
            std::cerr << "[save] " << (caller ? caller->key() : "<null>")
                      << " -> " << obj->key() << "::" << funName << "\n";
            std::cerr.flush();
        }
    }
    pushArgs(pcb, fn, args);
    pushFrameInPlace(pcb, obj, obj->prog(), fn, passedArgCount(args), true, 0);
    // [fix] 0 参调用 pushArgs 压了占位槽 (bp 锚), ret 清栈必须扣除 ——
    // 不标记则每次 0 参 call_other 泄漏 1 槽。
    if (args.empty() && fn->argCount == 0 && !pcb->frames.empty())
        pcb->frames.back().zeroArgPlaceholder = true;
}

// ---------------------------------------------------------------------------
// Broadcast (vm_callInOtherArray2/3): per-element chain driven by RETURN
// HOOKS; the LAST element's hook substitutes the results array as the return
// value — vmReturnFromFun pushes it where the caller expects the result.
// ---------------------------------------------------------------------------
struct BroadcastState {
    std::vector<VmObject*> targets;
    std::string funName;
    std::vector<Value> args;
    std::shared_ptr<ArrayValue> results;
    size_t idx = 0;
    VmObject* caller = nullptr;
};

static void broadcastStartNext(VmPcb* pcb, const std::shared_ptr<BroadcastState>& st) {
    while (st->idx < st->targets.size()) {
        VmObject* obj = st->targets[st->idx++];
        const OriginalFunction* fn =
            (obj && !obj->reallyGone()) ? obj->prog()->findFn(st->funName) : nullptr;
        if (!fn) {
            st->results->items.push_back(Value{});   // 缺函数: void 结果元素
            continue;
        }
        pushArgs(pcb, fn, st->args);
        pcb->previousObjectName = st->caller ? st->caller->key() : std::string();
        pushFrameInPlace(pcb, obj, obj->prog(), fn, passedArgCount(st->args), true, 0);
        // [fix] 同 vmCallOtherPush: 0 参广播元素的占位槽要在 ret 清栈时扣除。
        if (st->args.empty() && fn->argCount == 0 && !pcb->frames.empty())
            pcb->frames.back().zeroArgPlaceholder = true;
        vmHookCallbackWhenReturn(pcb, [pcb, st](Value& rv) {
            if (st->idx < st->targets.size()) {
                st->results->items.push_back(rv);
                broadcastStartNext(pcb, st);   // frame push — consumes the value
                rv = Value{};
            } else {
                // [fix] 末元素的结果也必须 collect!  原实现直接 rv=results
                // 返回, 单元素广播 (最常见: 队伍里只有玩家自己) 的 results
                // 永远是空数组 —— count_total(obs->is_player())=0 → 刷怪数
                // 0 → 空队伍 → init_thief "It is not a object!" → 暗雷/GM
                // 战斗全挂。全 mudlib 的 arr->fn() 单元素调用都中招。
                st->results->items.push_back(rv);
                rv = Value(st->results);        // last element: result = the array
            }
        });
        return;
    }
}

Value vmCallOtherArray(const std::vector<Value>& targets, const std::string& funName,
                       const std::vector<Value>& args) {
    VmPcb* pcb = currentPcb();
    if (!pcb) vmError("call_other broadcast outside a process.");
    const size_t depth = pcb->frames.size();
    vmCallOtherArrayPush(pcb, targets, funName, args);
    if (pcb->frames.size() == depth)
        return Interp::popValue(pcb);   // all misses: the array is on the stack
    // The final hook leaves the results array on the stack top — pop it for
    // the C++ caller.  (A blocking efun inside an element throws SuspendSignal
    // out; the bytecode-level broadcast continues on resume via the hooks.)
    Interp::runNested(pcb, depth);
    return popValueNested(pcb);
}

void vmCallOtherArrayPush(VmPcb* pcb, const std::vector<Value>& targets,
                          const std::string& funName,
                          const std::vector<Value>& args) {
    auto st = std::make_shared<BroadcastState>();
    st->funName = funName;
    st->args = args;
    st->results = std::make_shared<ArrayValue>();
    st->caller = pcb->frames.empty() ? nullptr : pcb->frames.back().obj;
    for (const auto& t : targets) {
        VmObject* obj = nullptr;
        if (t.index() == 7) {
            auto ov = std::get<std::shared_ptr<ObjectValue>>(t);
            if (ov) {
                obj = ov->vmObj;
                if (!obj && !ov->programName.empty())
                    obj = Registry::instance().findByName(ov->programName);
            }
        } else if (t.index() == 4) {
            obj = Registry::instance().findByName(std::get<SharedString>(t).str());
        }
        st->targets.push_back(obj);   // null/destructed elements yield int 0
    }
    const size_t framesBefore = pcb->frames.size();
    broadcastStartNext(pcb, st);
    if (pcb->frames.size() == framesBefore)
        Interp::pushValue(pcb, Value(st->results));   // no element started
}

Value vmCallOther(const Value& target, const std::string& funName,
                  const std::vector<Value>& args) {
    VmPcb* pcb = currentPcb();
    if (!pcb) vmError("call_other outside a process.");
    const size_t framesBefore = pcb->frames.size();
    vmCallOtherPush(pcb, target, funName, args);
    if (pcb->frames.size() == framesBefore) {
        // [fnmiss] 函数缺失: vmCallOtherPush 已压 int 0 且未压帧 ——
        // 直接取走返回值, 不进 nested loop。
        return Interp::popValue(pcb);
    }
    const size_t depth = pcb->frames.size() - 1;
    Interp::runNested(pcb, depth);
    return popValueNested(pcb);
}

// ---------------------------------------------------------------------------
// Construction (vm_f_construct_object2 / runInit).  The plan is prebuilt
// (buildConstructPlan); the construct PROCESS drives it step by step:
//   * an Init step runs a global-init segment under a CONTEXT frame
//     (fn == nullptr) — Interp::run returns at the segment's end;
//   * a Create step pushes the create function as a BOUNDARY frame (the
//     construct process has no caller stream) and runs to its return.
// A blocking efun inside any step throws SuspendSignal out with idx
// unchanged; the resume re-enters Interp::run, finishes the step, and
// runProcess's next vmDriveConstruction call continues the remaining steps.
// ---------------------------------------------------------------------------
std::shared_ptr<ConstructPlan> buildConstructPlan(VmObject* obj,
                                                  std::vector<Value> createArgs) {
    auto plan = std::make_shared<ConstructPlan>();
    if (!obj || !obj->prog()) return plan;
    const VmProgram* prog = obj->prog();
    plan->obj = obj;
    plan->doneEvent = "obj-init:" + obj->key();
    // 1. Own global-init segment (phase 0).
    if (!prog->src().globalCode.empty()) {
        ConstructStep st;
        st.kind = ConstructStep::Kind::Init;
        st.code = &prog->src().globalCode;
        st.meta = &prog->src().globalMeta;
        st.varOffset = 0;
        st.strings = &prog->src().strings;
        plan->steps.push_back(std::move(st));
    }
    // 2. Inherited init segments (parents-first, own constitution each).
    for (const auto& seg : prog->initSegments()) {
        if (!seg.prog->globalCode.empty()) {
            ConstructStep st;
            st.kind = ConstructStep::Kind::Init;
            st.code = &seg.prog->globalCode;
            st.meta = &seg.prog->globalMeta;
            st.varOffset = seg.varOffset;
            st.funOffset = seg.funOffset;
            st.strings = &seg.prog->strings;
            plan->steps.push_back(std::move(st));
        }
    }
    // 3. The create chain (base-first; createArgs only to creates that
    //    declare parameters; missing declared args padded with undefined).
    for (const OriginalFunction* fn : prog->createChain()) {
        if (!fn) continue;
        ConstructStep st;
        st.kind = ConstructStep::Kind::Create;
        st.fn = fn;
        if (fn->argCount >= 1) {
            for (size_t i = 0; i < createArgs.size(); ++i)
                st.args.push_back(createArgs[i]);
            for (int i = static_cast<int>(st.args.size()); i < fn->argCount; ++i)
                st.args.push_back(Value{});   // missing create arg -> undefined
        }
        plan->steps.push_back(std::move(st));
    }
    // Wrap every Init segment in a real function frame (原版没有伪函数——
    // 原版 init 段就在调用者执行流上, 步进靠 hookCallbackWhenReturn; 这里
    // 同一语义: 段=普通帧, 与 create 同一 park/resume/hook 机制)。
    for (auto& st : plan->steps) {
        if (st.kind != ConstructStep::Kind::Init || st.fakeFn) continue;
        auto fk = std::make_shared<OriginalFunction>();
        fk->name = "<init-seg>";
        fk->argCount = 0;
        fk->localCount = 0;
        fk->code = *st.code;
        fk->metaData = *st.meta;
        fk->strings = std::make_shared<const std::vector<std::string>>(
            st.strings ? *st.strings : std::vector<std::string>{});
        fk->varOffset = st.varOffset;
        fk->funOffset = st.funOffset;
        st.fakeFn = std::move(fk);
        st.fn = st.fakeFn.get();
    }
    return plan;
}

// ---------------------------------------------------------------------------
// Hook-driven inline construction (原版 vm_f_construct_object2 0x80f90b4):
// 每步压普通帧 (保存调用者流), retHook 推进; 阻塞 efun park 当前进程后
// resume 由通用帧机制自动续 — 无 spawn、无新 tid、无构造进程物种。
// ---------------------------------------------------------------------------
static Value vmConstructFinish(VmPcb* pcb,
                               const std::shared_ptr<ConstructPlan>& plan) {
    plan->finished = true;
    const bool failed = plan->failed;
    // CTRACE: WHICH construction completed.  A boot stall showed the master
    // construct "finishing" silently mid load_all (gs froze at loads=3697,
    // scheduler idle) — without naming the plan there was no way to tell a
    // legit completion from a wrong one.
    if (g_ctraceOn) {
        std::cerr << "[construct-done] obj="
                  << (plan->obj ? plan->obj->key() : std::string("<null>"))
                  << " failed=" << failed
                  << " steps=" << plan->idx << "/" << plan->steps.size()
                  << " stackDepth=" << pcb->constructStack.size() << "\n";
        std::cerr.flush();
    }
    if (failed)
        Registry::instance().clearConstructing(plan->obj->key());
    else
        Registry::instance().markInitialized(plan->obj->key());
    if (plan->resultPromise) {
        plan->resultPromise->set_value(
            failed ? Value{} : Registry::makeObjectValue(plan->obj));
        plan->resultPromise.reset();
    }
    if (!plan->doneEvent.empty())
        Scheduler::instance().wakeEventFront(plan->doneEvent);
    if (!pcb->constructStack.empty()) pcb->constructStack.pop_back();
    pcb->construct = pcb->constructStack.empty()
                         ? nullptr : pcb->constructStack.back();
    return failed ? Value{} : Registry::makeObjectValue(plan->obj);
}

static void vmConstructRunStep(VmPcb* pcb,
                               const std::shared_ptr<ConstructPlan>& plan) {
    if (plan->idx >= plan->steps.size()) return;   // finished (rv set below)
    const ConstructStep& st = plan->steps[plan->idx];
    ++plan->idx;
    pushArgs(pcb, st.fn, st.args);
    // [consdbg] 栈账实锤: 每步骤帧的进入栈深/锚位 (residual=2 主力取证,
    // lw118: create 带参路径比 init-seg 路径每帧多漏 1)。
    {
        static const std::atomic<bool>& cdFlag =
            lpc::dynProbeFlag("LPCVM_CONSDBG");
        if (cdFlag.load(std::memory_order_relaxed))
            std::cerr << "[consdbg] step idx=" << (plan->idx - 1) << "/"
                      << plan->steps.size() << " kind="
                      << (st.kind == ConstructStep::Kind::Create ? "create"
                                                                 : "init")
                      << " fn=" << (st.fn ? st.fn->name : "<null>")
                      << " argNum=" << passedArgCount(st.args)
                      << " locals=" << (st.fn ? st.fn->localCount : -1)
                      << " depth=" << pcb->stack.size() << " obj="
                      << (plan->obj ? plan->obj->key() : "<null>") << "\n";
    }
    // 0-实参步骤 (init-seg 常见): caller 栈顶恰好是 load_object 等 efun 的
    // 返回值槽 —— pushFrameInPlace 的 bp=栈顶 会把它当成"参数区顶"(argNum=0
    // 的帧 bp 无锚), op72 收尾时 pop 1 弹掉的就是那个 rv, 且清栈 newSize =
    // bp-argNum+1 (ph 未设) 会 resize 扩栈补 1 个空槽 —— 每个 0 参步骤帧
    // 净漏 2 槽 (lw110 fn-trace: safe_load_object residual=2/17/34 全
    // undefined, 与 depend_files 递归深度吻合)。
    // 修复: 无实参时压 1 个种子槽并标记 zeroArgPlaceholder —— 与 op5
    // call_fun_ptr 的种子机制同构, op72 pop 种子、清栈裁种子, 栈复原。
    // 0-实参步骤帧的占位: pushArgs 对 0 实参帧已压 1 个 frame slot
    // (bp >= 0 锚, 见 pushArgs) —— 它就是 zeroArgPlaceholder 占位槽。
    // 7b45fd4 又在此压一个种子 → 双压, 清栈只裁 1 个 → 每步净 +1:
    // 34 步 plan +34 (lw124 实锤: gather_item start depth=422, 34 步后
    // 下一对象 start depth=456; add_goods residual=20 = 20 步 plan)。
    // 修复: 不再重复压种子, pushArgs 的 frame slot 即占位。
    const bool zeroArg = passedArgCount(st.args) == 0;
    if (pushFrameInPlace(pcb, plan->obj, plan->obj->prog(), st.fn,
                         passedArgCount(st.args), true, 0)) {
        if (zeroArg) pcb->frames.back().zeroArgPlaceholder = true;
    } else if (zeroArg) {
        Interp::popValue(pcb);   // frame refused: undo the frame slot
        return;
    }
    vmHookCallbackWhenReturn(pcb, [pcb, plan](Value& rv) {
        if (plan->idx >= plan->steps.size()) {
            rv = vmConstructFinish(pcb, plan);   // rv = the efun return value
            return;
        }
        vmConstructRunStep(pcb, plan);   // push the next step's frame
    });
}

void vmStartInlineConstruction(VmPcb* pcb,
                               const std::shared_ptr<ConstructPlan>& plan) {
    {
        static const std::atomic<bool>& cdFlag =
            lpc::dynProbeFlag("LPCVM_CONSDBG");
        if (cdFlag.load(std::memory_order_relaxed))
            std::cerr << "[consdbg] start steps=" << plan->steps.size()
                      << " depth=" << pcb->stack.size() << " obj="
                      << (plan->obj ? plan->obj->key() : "<null>") << "\n";
    }
    if (plan->idx >= plan->steps.size()) {
        vmConstructFinish(pcb, plan);   // empty plan: finish immediately
        return;
    }
    vmConstructRunStep(pcb, plan);
}

Value vmRunConstructionSync(VmPcb* scratch,
                            const std::shared_ptr<ConstructPlan>& plan) {
    scratch->constructStack.push_back(plan);
    scratch->construct = plan;
    vmStartInlineConstruction(scratch, plan);
    if (plan->finished) {
        scratch->construct = nullptr;
        return plan->failed ? Value{}
                            : Registry::makeObjectValue(plan->obj);
    }
    try {
        Interp::run(scratch);   // park escapes as SuspendSignal
    } catch (...) {
        plan->failed = true;
        Registry::instance().clearConstructing(plan->obj->key());
        scratch->construct = nullptr;
        return Value{};   // 构造失败: void
    }
    scratch->construct = nullptr;
    return plan->failed ? Value(make_int(0))
                        : Registry::makeObjectValue(plan->obj);
}

} // namespace vmcore
} // namespace lpc
