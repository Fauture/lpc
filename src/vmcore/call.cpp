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
#include <cstring>

#include "call.h"
#include "interp.h"
#include "values.h"
#include "vmcore.h"
#include "trace.h"

namespace lpc {
namespace vmcore {

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
    const int n = fn->argCount;
    const int passed = static_cast<int>(args.size());
    bool placeholder = false;
    if (n == 0 && passed == 0) {
        pushItem(pcb, makeItem(Value{}));   // frame slot so bp >= 0
        placeholder = true;
    }
    const int pushCount = (passed > n) ? passed : n;
    for (int i = pushCount - 1; i >= 0; --i)
        pushValue(pcb, i < static_cast<int>(args.size())
                            ? args[static_cast<size_t>(i)] : Value{});
    (void)placeholder;
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
    pcb->returnValue = make_int(0);
    pcb->returned = false;
    return true;
}

bool vmCallPushFrame(VmPcb* pcb, VmObject* obj, const VmProgram* prog,
                     const OriginalFunction* fn, const std::vector<Value>& args,
                     int padArgs) {
    if (!pcb || !fn) return false;
    pushArgs(pcb, fn, args);
    return pushFrameInPlace(pcb, obj, prog, fn,
                            fn->argCount, true, padArgs);
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
        else pcb->returnValue = make_int(0);
    } else {
        const size_t expectedVoid = static_cast<size_t>(f.bp) + 1;
        if (pcb->stack.size() == expectedVoid)
            pcb->returnValue = Value{};               // void return (undefined)
        else if (pcb->stack.size() == expectedVoid + 1)
            pcb->returnValue = Interp::popValue(pcb);
        else if (!pcb->stack.empty())
            pcb->returnValue = Interp::popValue(pcb); // non-canonical: take top
        else
            pcb->returnValue = Value{};
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
    pcb->stack.resize(static_cast<size_t>(newSize));
    if (retCode == nullptr) {
        // Boundary frame: the chain is done; result stays in returnValue.
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
                     fn->argCount, false, 0);
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
    VmPcbSetter setPcb(pcb);
    const size_t depth = pcb->frames.size();
    pushArgs(pcb, fn, args);
    pushFrameInPlace(pcb, obj, progOverride ? progOverride
                                            : (obj ? obj->prog() : nullptr),
                     fn, fn->argCount, true, 0);
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
    pushArgs(pcb, fn, args);
    return pushFrameInPlace(pcb, obj,
                            progOverride ? progOverride
                                         : (obj ? obj->prog() : nullptr),
                            fn, fn->argCount, true, 0);
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
    if (obj->destructed())
        vmError("Object is not found (destructed).");
    const OriginalFunction* fn = obj->prog()->findFn(funName);
    if (!fn)
        vmError("Function " + funName + " is not found.");
    // previous_object() in the callee names the caller.
    VmObject* caller = pcb->frames.empty() ? nullptr : pcb->frames.back().obj;
    pcb->previousObjectName = caller ? caller->key() : std::string();
    pushArgs(pcb, fn, args);
    pushFrameInPlace(pcb, obj, obj->prog(), fn, fn->argCount, true, 0);
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
            (obj && !obj->destructed()) ? obj->prog()->findFn(st->funName) : nullptr;
        if (!fn) {
            st->results->items.push_back(make_int(0));
            continue;
        }
        pushArgs(pcb, fn, st->args);
        pcb->previousObjectName = st->caller ? st->caller->key() : std::string();
        pushFrameInPlace(pcb, obj, obj->prog(), fn, fn->argCount, true, 0);
        vmHookCallbackWhenReturn(pcb, [pcb, st](Value& rv) {
            if (st->idx < st->targets.size()) {
                st->results->items.push_back(rv);
                broadcastStartNext(pcb, st);   // frame push — consumes the value
                rv = Value{};
            } else {
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
    vmCallOtherPush(pcb, target, funName, args);
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
    if (getenv("LPCVM_CTRACE")) {
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
            failed ? make_int(0) : Registry::makeObjectValue(plan->obj));
        plan->resultPromise.reset();
    }
    if (!plan->doneEvent.empty())
        Scheduler::instance().wakeEventFront(plan->doneEvent);
    if (!pcb->constructStack.empty()) pcb->constructStack.pop_back();
    pcb->construct = pcb->constructStack.empty()
                         ? nullptr : pcb->constructStack.back();
    return failed ? Value(make_int(0)) : Registry::makeObjectValue(plan->obj);
}

static void vmConstructRunStep(VmPcb* pcb,
                               const std::shared_ptr<ConstructPlan>& plan) {
    if (plan->idx >= plan->steps.size()) return;   // finished (rv set below)
    const ConstructStep& st = plan->steps[plan->idx];
    ++plan->idx;
    pushArgs(pcb, st.fn, st.args);
    pushFrameInPlace(pcb, plan->obj, plan->obj->prog(), st.fn,
                     st.fn->argCount, true, 0);
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
        return plan->failed ? Value(make_int(0))
                            : Registry::makeObjectValue(plan->obj);
    }
    try {
        Interp::run(scratch);   // park escapes as SuspendSignal
    } catch (...) {
        plan->failed = true;
        Registry::instance().clearConstructing(plan->obj->key());
        scratch->construct = nullptr;
        return make_int(0);
    }
    scratch->construct = nullptr;
    return plan->failed ? Value(make_int(0))
                        : Registry::makeObjectValue(plan->obj);
}

} // namespace vmcore
} // namespace lpc
