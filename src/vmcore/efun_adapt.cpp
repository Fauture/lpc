// ===========================================================================
// vmcore/efun_adapt.cpp — efun 适配层 (阶段5)
//
// dispatchEfun: 解释器 op-6 的唯一出口。分发顺序:
//   1. RAW-REF 拦截 (操作数必须以引用形态存活的 efun): sscanf / recv /
//      select / receive_from_queue / put_sub_data / set_timer 的 &id 出参。
//   2. 哈希分发链 (旧核 callEfunWithArgs 的完整移植, 剥掉探测块; 状态来源
//      全部换成 当前 PCB + 当前帧 + Registry):
//      - 阻塞 efun (sleep/wait_event/raise_event/accept/connect): 置
//        pcb->cond (+resumeHook) 后 throw SuspendSignal — park 的永远是
//        当前 PCB。
//      - 队列/事件/套接字的存储一律是驱动侧真实子系统 (builtins.cpp 的
//        queue*/event*/xdbc) — 新核不再有第二套存储。
//   3. 兜底: call_builtin(nullptr, name, args) — builtins.cpp 是驱动侧资产。
//
// 阻塞 efun 约定: 置 pcb->cond + pcb->resumeValue(默认值) + (可选)
// pcb->resumeHook, 然后 throw SuspendSignal{} (调度器 parkCurrent 用
// pcb->cond — 每进程一份, 不存在旧核的共享槽覆写)。
// ===========================================================================
#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>

#include "call.h"
#include "driver_model.h"
#include "efun_table.h"
#include "interp.h"
#include "object.h"
#include "values.h"
#include "vmcore.h"
#include "trace.h"

#include "builtins.h"
#include "map_module.h"
#include "o_loader.h"
#include "original_vm.h"   // flow probe (driver contract)
#include "xdbc.h"

namespace lpc {
namespace vmcore {

using Interp::pushValue;
using Interp::popItem;
using Interp::popValue;
using Interp::makeItem;

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------
static int64_t steadyMs() {
    // The SCHEDULER's shared clock (msSinceStart: steady_clock since process
    // start).  Timer wake times computed against the clock's EPOCH would be
    // astronomically larger than the scheduler's now() and never fire.
    return msSinceStart();
}

// The CURRENT frame's object (this_object()); null in a driver context.
static VmObject* frameObj(VmPcb* pcb) {
    return pcb && !pcb->frames.empty() ? pcb->frames.back().obj : nullptr;
}
static std::string frameObjKey(VmPcb* pcb) {
    VmObject* o = frameObj(pcb);
    return o ? o->key() : std::string();
}
static std::string frameFnName(VmPcb* pcb) {
    if (!pcb || pcb->frames.empty() || !pcb->frames.back().fn)
        return "<global>";
    return pcb->frames.back().fn->name;
}
// object | string argument → canonical registry key ("" when unresolvable).
static VmObject* objOfValue(const Value& v) {
    if (v.index() == 7) {
        auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
        if (ov) {
            if (ov->vmObj) return ov->vmObj;
            if (!ov->programName.empty())
                return Registry::instance().findByName(ov->programName);
        }
        return nullptr;
    }
    if (v.index() == 4)
        return Registry::instance().findByName(std::get<SharedString>(v).str());
    return nullptr;
}
static std::string keyOfArg(const Value& v) {
    if (v.index() == 7) {
        auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
        if (!ov) return std::string();
        if (ov->vmObj) return ov->vmObj->key();
        if (!ov->programName.empty() && ov->programName != "<this>" &&
            ov->programName != ".")
            return ov->programName;
        return std::string();
    }
    if (v.index() == 4) return std::get<SharedString>(v).str();
    return std::string();
}

// ---------------------------------------------------------------------------
// park helpers — set cond (+ default resume value, + hook), throw.
// ---------------------------------------------------------------------------
static void parkAndSuspend(VmPcb* pcb, const ResumeCond& cond,
                           std::function<void()> hook, Value defResume) {
    pcb->cond = cond;
    pcb->resumeValue = std::move(defResume);
    // Keep a PRE-INSTALLED hook (several efun sites install theirs before
    // parking); only an explicitly passed hook replaces it.  Passing nullptr
    // used to wipe the installed hook — the parked process then resumed
    // with nobody to write through its by-ref out slots.
    if (hook) pcb->resumeHook = std::move(hook);
    throw SuspendSignal{};
}

// ---------------------------------------------------------------------------
// closure invocation (vm_callFunPtr / liveClosureFunc):
// the function is re-addressed through the OWNER's CURRENT merged table on
// every call (never the cached pointer); an efun closure goes to the efun.
// ---------------------------------------------------------------------------
Value vmInvokeClosure(VmPcb* pcb, const std::shared_ptr<ClosureValue>& cl,
                      const std::vector<Value>& extra) {
    if (!cl) return make_int(0);
    std::vector<Value> all = cl->boundArgs;
    all.insert(all.end(), extra.begin(), extra.end());
    // globalFn closure (op23 bit7): run the master-program body with the
    // CURRENT object as context (原版 vm_callInUserFunBody 语义).
    if (cl->globalFn && cl->func) {
        VmObject* ctx = pcb ? frameObj(pcb) : nullptr;
        if (!ctx) ctx = Interp::masterObject();
        if (!pcb) {
            VmPcb scratch;
            VmPcbSetter set(&scratch);
            return vmApplySync(&scratch, ctx, cl->func, all,
                               Interp::masterProgram());
        }
        return vmApplySync(pcb, ctx, cl->func, all, Interp::masterProgram());
    }
    VmObject* owner = nullptr;
    if (!cl->ownerKey.empty()) owner = Registry::instance().findByName(cl->ownerKey);
    if (!owner && cl->owner) {
        // Fall back to the master when only the raw program is known.
        owner = Interp::masterObject();
    }
    const OriginalFunction* fn = nullptr;
    if (owner && !cl->functionName.empty())
        fn = owner->prog()->findFn(cl->functionName);
    if (!fn && cl->efunClosure) {
        return dispatchEfunByName(pcb, cl->functionName, all);
    }
    if (!fn) {
        if (!cl->functionName.empty())
            return dispatchEfunByName(pcb, cl->functionName, all);
        return make_int(0);
    }
    if (!pcb) {
        // Outside a process (should not happen on the scheduler): run on a
        // scratch PCB synchronously.
        VmPcb scratch;
        VmPcbSetter set(&scratch);
        return vmApplySync(&scratch, owner, fn, all);
    }
    return vmApplySync(pcb, owner, fn, all);
}

// ---------------------------------------------------------------------------
// vmInvokeClosurePush — 原版 vm_callFunPtr 语义（范本 vm_f_mapping_array1:
// vm_pushValue + vm_callFunPtr + vm_hookCallbackWhenReturnAt）：压帧调用
// closure 后立即返回，控制流交还指令循环去跑被调帧；结果经该帧的返回值
// 交付，后置 C++ 逻辑通过 retHook 挂在被调帧上（钩子随帧活在 PCB 上，
// 阻塞 efun 在被调帧里 park 后 resume 照常触发——C++ 续延不再存在于
// 调用栈上，这是与 vmApplySync 的本质区别，见 2026-10-31 三连死复盘）。
// 返回 false = 无法压帧（efun closure / 函数未找到），调用方走同步兜底。
// ---------------------------------------------------------------------------
bool vmInvokeClosurePush(VmPcb* pcb, const std::shared_ptr<ClosureValue>& cl,
                         const std::vector<Value>& extra,
                         std::function<void(Value&)> retHook) {
    if (!cl || !pcb) return false;
    std::vector<Value> all = cl->boundArgs;
    all.insert(all.end(), extra.begin(), extra.end());
    VmObject* owner = nullptr;
    if (!cl->ownerKey.empty())
        owner = Registry::instance().findByName(cl->ownerKey);
    if (!owner && cl->owner) owner = Interp::masterObject();
    const OriginalFunction* fn = nullptr;
    if (owner && !cl->functionName.empty())
        fn = owner->prog()->findFn(cl->functionName);
    // globalFn closure (op23 bit7): the body lives in the GLOBAL program —
    // 原版 vm_callInUserFunBody(bit7,...,vm_pGlobalProgram) runs the master
    // bytecode with the CURRENT object as context.  The owner's own program
    // has neither the index nor the name (scand.o has no
    // find_basic_object_by_import_name), so the name lookup above misses and
    // the closure used to fall into the efun stub returning 0 forever.
    if (cl->globalFn && cl->func) {
        VmObject* ctx = frameObj(pcb);
        if (!ctx) ctx = owner ? owner : Interp::masterObject();
        if (!vmPushFunCall(pcb, ctx, cl->func, all, Interp::masterProgram()))
            return false;
        if (retHook) vmHookCallbackWhenReturn(pcb, std::move(retHook));
        // 注意: 这里【不置位】efunTakeover —— 本函数是"压帧+hook 链"语义,
        // hook 链推进发生在 vmReturnFromFun 的 retHook 里, 不经过任何
        // dispatchEfun 尾部, 置了无人消费 → 残留 → 下一个无关 efun 的
        // 返回值被吞 (create_search_terms 循环 ctrl 槽被吃 → Bad loop
        // ctrl → scand.do_init 中断 → oper_scan_table_flag 恒 0 →
        // statued 死循环 → post_init 永不排空 → 8160 不开)。外层 efun
        // (sort_array/mapping_array/fireMapHooks 宿主) 的分支自己置位,
        // 由各自的 dispatchEfun 尾部消费一次 —— 那才是成对的。
        return true;
    }
    if (!fn) return false;   // efun closure / unresolved: caller falls back
    if (!vmPushFunCall(pcb, owner, fn, all)) return false;
    if (retHook) vmHookCallbackWhenReturn(pcb, std::move(retHook));
    return true;
}

// ---------------------------------------------------------------------------
// special efuns (语义必须贴 VM 内部)
// ---------------------------------------------------------------------------
// put_sub_data(&var, pos, val) — vm_f_put_sub_data (0x80d9b64): writes val
// into the caller's string/buffer REFERENCE; val is one byte (-255..255) or a
// whole string/buffer; memcpy at pos; returns NOTHING.
static void efunPutSubData(VmPcb* pcb, std::vector<StackItem>& raw) {
    // raw[0] = &var (ref), raw[1] = pos, raw[2] = val  (param0 first)
    if (raw.size() < 3) return;
    StackItem& ref = raw[0];
    const int64_t pos = toInt(raw[1].value);
    const Value& val = raw[2].value;
    Value cur = Interp::derefItem(pcb, ref);
    std::string data;
    if (cur.index() == 4) data = std::get<SharedString>(cur).str();
    else if (cur.index() == 10) data = std::get<std::shared_ptr<BufferValue>>(cur)->data;
    else return;
    std::string put;
    if (val.index() == 2) {
        const int64_t b = std::get<int64_t>(val);
        put += static_cast<char>(static_cast<unsigned char>(b & 0xFF));
    } else if (val.index() == 4) {
        put = std::get<SharedString>(val).str();
    } else if (val.index() == 10) {
        put = std::get<std::shared_ptr<BufferValue>>(val)->data;
    } else {
        return;
    }
    if (pos < 0 || static_cast<size_t>(pos) >= data.size()) return;
    const size_t n = put.size();
    if (static_cast<size_t>(pos) + n > data.size()) return;
    data.replace(static_cast<size_t>(pos), n, put);
    Interp::storeItem(pcb, ref, cur.index() == 4 ? Value(make_string(data))
                                                 : Value(make_buffer(data)));
}

// sort(array[, cmp[, order]]) — vm_f_sort (0x80dbf4c).  The comparator form
// is KEY EXTRACTION (one call per element — the original builds the key array
// first, then sorts), the order applies to BOTH forms, and the array /
// comparator / direction are located BY TYPE (sort_obs_by_speed delivers the
// direction/closure in front of the array).
static Value efunSort(VmPcb* pcb, std::vector<Value>& args) {
    if (args.size() > 3) vmError("Too many arguments to sort.\n");
    int sArr = -1, sCmp = -1, sDir = -1;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].index() == 5 && sArr < 0) sArr = static_cast<int>(i);
        else if (args[i].index() == 8 && sCmp < 0) sCmp = static_cast<int>(i);
        else if (args[i].index() == 2 && sDir < 0) sDir = static_cast<int>(i);
    }
    if (sArr < 0) return make_array();   // no array: keep the lenient behaviour
    int64_t order = 1;
    if (sDir >= 0) {
        order = std::get<int64_t>(args[static_cast<size_t>(sDir)]);
        if (order == 0) vmError("You must specific order for sort.\n");
        if (order > 2) order = 1;
        else if (order < -2) order = -1;
    }
    const bool desc = order < 0;
    auto src = std::get<std::shared_ptr<ArrayValue>>(args[static_cast<size_t>(sArr)]);
    std::vector<Value> items = src->items;
    if (sCmp >= 0) {
        auto cl = std::get<std::shared_ptr<ClosureValue>>(
            args[static_cast<size_t>(sCmp)]);
        // 原版 vm_f_sort_mapping 范本：先逐元素压帧调 closure 收 key（钩子
        // 链推进），全部收齐后 vm_sortArray 排序收尾。park 安全同 mapping_array。
        struct SortCtx {
            VmPcb* pcb;
            std::shared_ptr<ClosureValue> cl;
            std::vector<Value> items;
            std::vector<Value> keys;
            bool desc;
            Value result;          // 排序收尾数组 (hook 链经 rv 交付)
            bool completed = false;
        };
        static const std::function<void(const std::shared_ptr<SortCtx>&,
                                        size_t, bool)> sortStep =
            [](const std::shared_ptr<SortCtx>& st, size_t i, bool viaHook) {
                if (i >= st->items.size()) {
                    std::vector<std::pair<Value, Value>> keyed(st->items.size());
                    for (size_t k = 0; k < st->items.size(); ++k)
                        keyed[k] = {std::move(st->keys[k]),
                                    std::move(st->items[k])};
                    if (st->desc) {
                        std::stable_sort(
                            keyed.begin(), keyed.end(),
                            [](const std::pair<Value, Value>& a,
                               const std::pair<Value, Value>& b) {
                                return compareValuesOriginal(b.first, a.first) < 0;
                            });
                    } else {
                        std::stable_sort(
                            keyed.begin(), keyed.end(),
                            [](const std::pair<Value, Value>& a,
                               const std::pair<Value, Value>& b) {
                                return compareValuesOriginal(a.first, b.first) < 0;
                            });
                    }
                    auto out = std::make_shared<ArrayValue>();
                    out->items.reserve(keyed.size());
                    for (auto& kv : keyed) out->items.push_back(std::move(kv.second));
                    st->result = Value{out};
                    st->completed = true;
                    // 仅整条链在 efun 体内同步走完时由这里压栈; hook 链路径
                    // 由末元素 hook 的 rv 出参交付 —— 在 hook 里 pushValue 会
                    // 与 vmReturnFromFun 的 pushValue(returnValue) 双重压栈,
                    // 调用方拿到的是闭包(比较键)返回值而非排序数组(同
                    // mapping_array 的 [op8ret] 实锤模式)。
                    if (!viaHook)
                        Interp::pushValue(st->pcb, st->result);   // efun 返回值
                    return;
                }
                if (!vmInvokeClosurePush(
                        st->pcb, st->cl, {st->items[i]},
                        [st, i](Value& rv) {
                            st->keys[i] = std::move(rv);
                            sortStep(st, i + 1, true);
                            if (st->completed) rv = st->result;   // 末元素/链同步走完: 经 rv 交付
                        })) {
                    st->keys[i] =
                        vmInvokeClosure(st->pcb, st->cl, {st->items[i]});
                    sortStep(st, i + 1, viaHook);
                }
            };
        auto st = std::make_shared<SortCtx>();
        st->pcb = pcb; st->cl = cl; st->items = std::move(items);
        st->keys.assign(st->items.size(), Value{}); st->desc = desc;
        sortStep(st, 0, false);
        pcb->efunTakeover = true;
        return Value{};
    } else {
        if (desc)
            std::stable_sort(items.begin(), items.end(),
                             [](const Value& a, const Value& b) {
                                 return compareValuesOriginal(b, a) < 0;
                             });
        else
            std::stable_sort(items.begin(), items.end(),
                             [](const Value& a, const Value& b) {
                                 return compareValuesOriginal(a, b) < 0;
                             });
    }
    auto out = std::make_shared<ArrayValue>();
    out->items = std::move(items);
    return out;
}

// ---------------------------------------------------------------------------
// object loading / cloning (vm_f_load_object 0x80f8cbc / vm_f_clone_object
// 0x80f8eac).  Construction runs as a SCHEDULER PROCESS (the construct plan);
// the caller parks on "obj-init:<key>" until it finishes — the original runs
// the construction inline on the loading process, so a blocking create()
// parks the whole process and resumes inside it; the spawn+park pair
// reproduces exactly that on the cooperative scheduler.
// ---------------------------------------------------------------------------
static Value parkForConstruction(VmPcb* pcb, VmObject* obj) {
    const std::string key = obj->key();
    if (vmcore::traceOn()) {
        std::cerr << "[construct-park] obj=" << key
                  << " by=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                   ? std::string("<ctx>")
                                   : pcb->frames.back().fn->name)
                  << " tid=" << pcb->tid << "\n";
    }
    ResumeCond cond;
    cond.kind = 3;
    cond.ename = "obj-init:" + key;
    cond.mask = 0;                       // any completion raise matches
    pcb->resumeHook = [pcb, obj]() {
        // Re-check the registry: a FAILED construction must resume with 0,
        // not a phantom object.
        if (Registry::instance().isInitialized(obj->key()))
            pcb->resumeValue = Registry::makeObjectValue(obj);
        else
            pcb->resumeValue = make_int(0);
    };
    parkAndSuspend(pcb, cond, nullptr, make_int(0));
    return make_int(0);   // unreachable (SuspendSignal thrown)
}

// Load (if needed) and CONSTRUCT the object; returns its object value.
// progImage non-empty = the third load_object argument (string = LPC source
// for the anti-cheat challenge, buffer = a compiled .o image).
Value vmLoadObjectValue(const std::string& vpath,
                        const std::vector<Value>& createArgs,
                        const std::string& progImage) {
    VmPcb* pcb = currentPcb();
    Registry& reg = Registry::instance();
    if (vpath.empty()) return make_int(0);

    // ---- dynamic image / source branch (check_server's challenge) ----
    if (!progImage.empty()) {
        if (minilpc::looksLikeSource(progImage)) {
            std::shared_ptr<minilpc::Prog> pg;
            try {
                pg = minilpc::parseSource(progImage);
            } catch (const std::exception& e) {
                std::cerr << "[dyn-load] " << vpath
                          << " source compile failed: " << e.what() << "\n";
                return make_int(0);
            }
            const std::string key = "dyn:" + vpath;
            // Register/REPLACE the program: check_server reloads fresh content
            // every check; a stale entry would answer the OLD challenge.
            minilpc::registerMini(key, pg);
            return make_object(key);
        }
        // A buffer is a compiled .o image: parse and register it.
        const std::string key = "dyn:" + vpath;
        if (!reg.find(key)) {
            std::vector<uint8_t> bytes(progImage.begin(), progImage.end());
            std::shared_ptr<OriginalObject> src;
            try {
                src = std::make_shared<OriginalObject>(
                    loadOriginalObjectFromBytes(bytes));
            } catch (const std::exception& e) {
                std::cerr << "[dyn-load] " << vpath << " parse failed: "
                          << e.what() << "\n";
                return make_int(0);
            }
            auto progRaw = std::make_shared<VmProgram>(
                src, [](const std::string& vp) {
                    return Registry::instance().loadInheritProgramPublic(vp);
                });
            auto prog = std::shared_ptr<const VmProgram>(progRaw);
            progRaw->setSelf(prog);
            VmObject* obj = new VmObject(prog, key);
            parkVmObject(obj);
            reg.registerObject(key, obj);
            std::cerr << "[dyn-load] " << vpath << " key=" << key
                      << " bytes=" << bytes.size()
                      << " funcs=" << src->functions.size() << "\n";
        }
        VmObject* obj = reg.find(key);
        if (!obj) return make_int(0);
        if (reg.isInitialized(key)) return Registry::makeObjectValue(obj);
        // Recursive load_object of the object THIS process is constructing.
        if (pcb && pcb->construct && pcb->construct->obj == obj)
            return Registry::makeObjectValue(obj);
        if (!pcb) {
            if (Scheduler::instance().isSchedulerThread()) {
                reg.markConstructing(key);
                auto plan = buildConstructPlan(obj, createArgs);
                VmPcb scratch;
                Value r = vmRunConstructionSync(&scratch, plan);
                if (r.index() == 7) return r;
                return make_int(0);
            }
            reg.markConstructing(key);
            auto plan = buildConstructPlan(obj, createArgs);
            plan->resultPromise = std::make_shared<std::promise<Value>>();
            auto fut = plan->resultPromise->get_future();
            Scheduler::instance().spawnConstruct(std::move(plan));
            return fut.get();
        }
        if (reg.isConstructing(key))
            return Registry::makeObjectValue(obj);   // 原版 find-object 语义:
                                                     // 已在表即返回 (构造中
                                                     // 的半成品也返回), 不等。
        reg.markConstructing(key);
        auto plan = buildConstructPlan(obj, createArgs);
        pcb->constructStack.push_back(plan);
        pcb->construct = plan;
        vmStartInlineConstruction(pcb, plan);   // frame takeover (see file load)
        if (!plan->finished) pcb->efunTakeover = true;
        return make_int(0);
    }

    // ---- file/pak load ----
    const std::string ckey = Registry::canonicalKey(
        vpath, reg.diskRootOf());
    VmObject* obj = reg.find(ckey);
    if (!obj) {
        auto prog = reg.loadProgram(vpath, reg.diskRootOf());
        if (!prog) return make_int(0);
        obj = new VmObject(prog, ckey);
        parkVmObject(obj);
        reg.registerObject(ckey, obj);
    }
    if (reg.isInitialized(ckey)) return Registry::makeObjectValue(obj);
    // Recursive load_object of the object THIS process is constructing: the
    // original finds it in the object table and returns it.
    if (pcb && pcb->construct && pcb->construct->obj == obj)
        return Registry::makeObjectValue(obj);
    if (!pcb) {
        if (Scheduler::instance().isSchedulerThread()) {
            // Driver-level call on the scheduler thread OUTSIDE any process:
            // run the construction synchronously on a scratch PCB (a parking
            // construct cannot complete here — report failure like the old
            // synchronous loader did).
            reg.markConstructing(ckey);
            auto plan = buildConstructPlan(obj, createArgs);
            VmPcb scratch;
            Value r = vmRunConstructionSync(&scratch, plan);
            return r;
        }
        // External caller: spawn the construction and block on it.
        reg.markConstructing(ckey);
        auto plan = buildConstructPlan(obj, createArgs);
        plan->resultPromise = std::make_shared<std::promise<Value>>();
        auto fut = plan->resultPromise->get_future();
        Scheduler::instance().spawnConstruct(std::move(plan));
        return fut.get();
    }
    if (reg.isConstructing(ckey))
        return Registry::makeObjectValue(obj);   // 原版 find-object 语义 (同上)
    // 原版 vm_f_construct_object2: 构造 INLINE 在当前进程上 — 每步压普通帧
    // (保存调用者流), retHook 链推进; 阻塞 efun park 当前进程 (同 tid),
    // resume 后 hook 链自动续。无 spawn、无新 tid、无构造进程物种。
    reg.markConstructing(ckey);
    auto plan = buildConstructPlan(obj, createArgs);
    pcb->constructStack.push_back(plan);
    pcb->construct = plan;
    vmStartInlineConstruction(pcb, plan);
    // Non-empty plan: the first frame is pushed and the interpreter loop /
    // retHook chain drives the rest; the object value is pushed as the efun
    // return when the chain completes (caller sees efunTakeover).
    if (!plan->finished) pcb->efunTakeover = true;
    return make_int(0);
}

// clone_object(virtualPath|object, createArgs...): clone the template (a
// fresh instance sharing the read-only program) and construct it through the
// same plan machinery.  ALL clones construct cooperatively — the old core's
// master-NPC-only special case existed because its synchronous path could
// not park; the new core has exactly one path and it can.
Value vmCloneObjectValue(const std::string& vpath,
                         const std::vector<Value>& createArgs) {
    VmPcb* pcb = currentPcb();
    Registry& reg = Registry::instance();
    if (vpath.empty()) return make_int(0);
    // The template may be an existing CLONE INSTANCE (place_char clones the
    // basic object it holds an object value for) — that very instance's
    // dbase is the template data.
    VmObject* tmpl = reg.findByName(vpath);
    if (!tmpl) {
        // Not loaded yet: load it first (safe_load_object semantics), then
        // re-resolve.  The load must complete before cloning.
        Value loaded = vmLoadObjectValue(vpath, {});
        if (loaded.index() != 7) return make_int(0);
        tmpl = objOfValue(loaded);
        if (!tmpl) return make_int(0);
    }
    VmObject* inst = cloneObject(tmpl);
    if (!inst) return make_int(0);
    const std::string ckey = inst->key();
    reg.markConstructing(ckey);
    auto plan = buildConstructPlan(inst, createArgs);
    if (!pcb) {
        if (Scheduler::instance().isSchedulerThread()) {
            VmPcb scratch;
            return vmRunConstructionSync(&scratch, plan);
        }
        plan->resultPromise = std::make_shared<std::promise<Value>>();
        auto fut = plan->resultPromise->get_future();
        Scheduler::instance().spawnConstruct(std::move(plan));
        return fut.get();
    }
    // Inline construction on THIS process (原版 vm_f_construct_object2):
    // frame takeover — the retHook chain finishes and pushes the object value.
    pcb->constructStack.push_back(plan);
    pcb->construct = plan;
    vmStartInlineConstruction(pcb, plan);
    if (!plan->finished) pcb->efunTakeover = true;
    return make_int(0);
}

// ---------------------------------------------------------------------------
// map hooks (move_in_map / put_in_map / remove_from_map)
// ---------------------------------------------------------------------------
static void fireMapHooks(VmPcb* pcb, const std::string& efun,
                         const std::vector<Value>& args, const Value& bres,
                         bool* chainArmed) {
    static thread_local int hookDepth = 0;
    if (hookDepth > 4) return;   // re-entrancy guard (hook moves objects)
    int mapIndex = -1;
    int ox = -1, oy = -1, nx = -1, ny = -1;
    int64_t id = 0;
    if (efun == "move_in_map" && args.size() >= 6) {
        mapIndex = static_cast<int>(toInt(args[0]));
        ox = static_cast<int>(toInt(args[1]));
        oy = static_cast<int>(toInt(args[2]));
        nx = static_cast<int>(toInt(args[3]));
        ny = static_cast<int>(toInt(args[4]));
        id = toInt(args[5]);
    } else if ((efun == "put_in_map" || efun == "remove_from_map") &&
               args.size() >= 4) {
        mapIndex = static_cast<int>(toInt(args[0]));
        nx = ox = static_cast<int>(toInt(args[1]));
        ny = oy = static_cast<int>(toInt(args[2]));
        id = toInt(args[3]);
    }
    if (mapIndex < 0) return;
    auto hooks = lpc::mapModule().hookSnapshot(mapIndex);
    if (hooks.empty()) return;
    ++hookDepth;
    // 原版钩子模式：逐钩子压帧调用（结果丢弃），retHook 链推进；链尾经
    // 末 hook 的 rv 出参交付宿主 efun 的返回值（takeover 吞掉的 bres 在
    // 这里补回）。在 hook 里 pushValue 会与 vmReturnFromFun 的
    // pushValue(returnValue) 双重压栈（同 mapping_array [op8ret] 实锤）。
    // park 安全。
    using HookCall =
        std::pair<std::shared_ptr<ClosureValue>, std::vector<Value>>;
    struct HookCtx {
        VmPcb* pcb;
        std::shared_ptr<std::vector<HookCall>> calls;
        Value bres;
        bool armed = false;   // 至少一个 hook 帧真实压栈（异步链）
        int done = 0;         // 已到链尾的次数（=1 即整链走完）
    };
    auto st = std::make_shared<HookCtx>();
    st->pcb = pcb;
    st->calls = std::make_shared<std::vector<HookCall>>();
    for (const auto& h : hooks) {
        if (h.idFilter != 0 && h.idFilter != id) continue;
        if (efun != "remove_from_map" && nx >= 0 &&
            lpc::MapModule::pointInPoly(nx, ny, h.poly)) {
            if (h.closure.index() == 8)
                st->calls->push_back({std::get<std::shared_ptr<ClosureValue>>(
                                          h.closure),
                                      {make_int(id), make_int(nx), make_int(ny),
                                       make_int(1)}});
        }
        if (efun != "put_in_map" && ox >= 0 && (ox != nx || oy != ny) &&
            lpc::MapModule::pointInPoly(ox, oy, h.poly)) {
            if (h.closure.index() == 8)
                st->calls->push_back({std::get<std::shared_ptr<ClosureValue>>(
                                          h.closure),
                                      {make_int(id), make_int(ox), make_int(oy),
                                       make_int(0)}});
        }
    }
    if (st->calls->empty()) {
        --hookDepth;
        return;
    }
    st->bres = bres;
    static const std::function<void(const std::shared_ptr<HookCtx>&, size_t,
                                    bool)> hookStep =
        [](const std::shared_ptr<HookCtx>& st, size_t i, bool viaHook) {
            if (i >= st->calls->size()) {
                ++st->done;
                --hookDepth;
                return;   // 交付: armed → 末 hook 的 rv; 全同步 → 宿主正常返回
            }
            if (!vmInvokeClosurePush(
                    st->pcb, (*st->calls)[i].first, (*st->calls)[i].second,
                    [st, i](Value& rv) {
                        hookStep(st, i + 1, true);
                        if (st->done ==
                            static_cast<int>(st->calls->size()))
                            rv = st->bres;   // 链在我这条 hook 内走完: 经 rv 交付
                    })) {
                vmInvokeClosure(st->pcb, (*st->calls)[i].first,
                                (*st->calls)[i].second);
                hookStep(st, i + 1, viaHook);
            } else {
                st->armed = true;
            }
        };
    hookStep(st, 0, false);
    if (chainArmed) *chainArmed = st->armed;
}

// ---------------------------------------------------------------------------
// parkOnEvent — wait_event's park (vm_f_wait_event 0x80f097c).  The resume
// hook re-checks the event and only reports success when one of OUR bits was
// actually consumed; a forever wait resumed by anything else RE-PARKS
// (filed.c::do_operate treats 0 as "the reply arrived").
// ---------------------------------------------------------------------------
static void parkOnEvent(VmPcb* pcb, const std::string& ename, const Value& eref,
                        uint32_t mask, double secs) {
    ResumeCond cond;
    cond.kind = 3;
    cond.ename = ename;
    cond.mask = mask;
    if (secs > 0.0 && secs < 2147483647.0) {
        // INT32_MAX and above is the original's "wait forever" idiom — park
        // on the event only, NO wake timer.
        double wakeMs = static_cast<double>(steadyMs()) + secs * 1000.0;
        if (!(wakeMs > static_cast<double>(steadyMs())) || wakeMs > 9.2e18)
            wakeMs = static_cast<double>(steadyMs()) + 100.0;   // garbage guard
        cond.wakeAtMs = static_cast<int64_t>(wakeMs);
    }
    const bool forever = (cond.wakeAtMs == 0);
    pcb->resumeHook = [pcb, eref, mask, ename, forever, secs]() {
        // wakeEvent marks a waiter it actually matched (and the timeout
        // timer its own expiry) via eventWoken; the RAISED BITS are already
        // cleared at raise time, so the flag — not the bits — is the
        // evidence of a legitimate wake.
        if (pcb->eventWoken) {
            // resumeValue already correct (0 for a matching raise, -501 for
            // the timeout timer).
            return;
        }
        // Resumed by something unrelated to this wait: consume bits if any
        // were raised meanwhile; a forever wait re-parks instead of forging
        // wait_event() == 0.
        int64_t rc = lpc::eventWait(eref, mask, 0.0);
        if (rc == 0) {
            pcb->resumeValue = make_int(0);
            return;
        }
        if (forever) {
            parkOnEvent(pcb, ename, eref, mask, secs);   // still waiting
            throw SuspendSignal{};
        }
        pcb->resumeValue = make_int(rc == -101 ? -101 : -501);
    };
    parkAndSuspend(pcb, cond, nullptr, make_int(0));
}

// ---------------------------------------------------------------------------
// dispatch core
// ---------------------------------------------------------------------------
Value dispatchEfunByName(VmPcb* pcb, const std::string& name,
                         std::vector<Value>& args) {
    if (name.empty()) return make_int(0);
    // ---- the FNV-1a hash dispatch (same constants as the verified layer) --
    static std::unordered_map<std::string, uint64_t> s_hash;
    uint64_t h = 0;
    {
        auto it = s_hash.find(name);
        if (it == s_hash.end()) {
            uint64_t hh = 1469598103934665603ull;
            for (char c : name) {
                hh ^= static_cast<unsigned char>(c);
                hh *= 1099511628211ull;
            }
            s_hash[name] = hh;
            h = hh;
        } else {
            h = it->second;
        }
    }
    const VmObject* fobj = frameObj(pcb);
    const std::string fkey = frameObjKey(pcb);
    VmObject* mutableFrameObj = frameObj(pcb);

    // ---- blocking efuns first (they may throw) --------------------------
    if (name == "sleep") {
        // vm_f_sleep (0x80ec9e4): sleep(secs) parks on a timer; resume 1.
        double secs = 0;
        if (!args.empty()) {
            const Value& tv = args[0];
            secs = tv.index() == 3 ? std::get<double>(tv)
                                   : static_cast<double>(toInt(tv));
        }
        if (secs <= 0.0) return make_int(1);
        ResumeCond cond;
        cond.kind = 1;
        double wakeMs = static_cast<double>(steadyMs()) + secs * 1000.0;
        if (!(wakeMs > static_cast<double>(steadyMs())) || wakeMs > 9.2e18) {
            std::cerr << "[sleep] BAD secs=" << secs << " obj=" << fkey
                      << "; clamping to 1s\n";
            wakeMs = static_cast<double>(steadyMs()) + 1000.0;
        }
        cond.wakeAtMs = static_cast<int64_t>(wakeMs);
        parkAndSuspend(pcb, cond, nullptr, make_int(1));
    }
    if (name == "wait_event") {
        // vm_f_wait_event (0x80f097c): wait_event(name, mask, ref, timeout).
        // Probe with timeout 0 first; -101 = unknown event (never park on an
        // event nobody can raise); 0 = already raised; secs == 0 = poll miss
        // -> -501; else park on the event (+ wake timer for a positive secs).
        std::string ename;
        Value eref;
        if (!args.empty()) {
            eref = args[0];
            if (eref.index() == 4) ename = std::get<SharedString>(eref);
            else if (eref.index() == 2)
                ename = "oid:" + std::to_string(std::get<int64_t>(eref));
        }
        uint32_t mask = 0;
        if (args.size() >= 2) mask = static_cast<uint32_t>(toInt(args[1]));
        double secs = -1.0;
        if (args.size() >= 4) {
            const Value& tv = args[3];
            if (tv.index() == 2) secs = static_cast<double>(std::get<int64_t>(tv));
            else if (tv.index() == 3) secs = std::get<double>(tv);
        }
        const int64_t probe = lpc::eventWait(eref, mask, 0.0);
        {
            if (vmcore::traceOn()) {
                std::cerr << "[wait_ev] ev=" << ename << " mask=0x" << std::hex
                          << mask << std::dec << " secs=" << secs
                          << " probe=" << probe
                          << " fn=" << frameFnName(pcb) << " tid=" << pcb->tid
                          << "\n";
            }
        }
        if (probe == -101) return make_int(-101);
        if (probe == 0) return make_int(0);
        if (secs == 0.0) return make_int(-501);
        lpc::flowProbePark(ename.c_str(), frameFnName(pcb).c_str());
        parkOnEvent(pcb, ename, eref, mask, secs);
    }
    if (name == "raise_event") {
        // vm_f_raise_event (0x80f07b8): OR the mask into the driver's event
        // store, then wake parked processes whose wait mask intersects.
        std::string ename;
        Value eref;
        if (!args.empty()) {
            eref = args[0];
            if (eref.index() == 4) ename = std::get<SharedString>(eref);
            else if (eref.index() == 2)
                ename = "oid:" + std::to_string(std::get<int64_t>(eref));
        }
        uint32_t mask = 0;
        if (args.size() >= 2) mask = static_cast<uint32_t>(toInt(args[1]));
        const int64_t rc = lpc::eventRaise(eref, mask);
        if (rc == 0) Scheduler::instance().wakeEvent(ename, mask);
        return make_int(rc);
    }
    if (name == "receive_from_queue" || name == "receive_from_queue_ex") {
        // vm_f_receive_from_queue (0x80f29f8): receive_from_queue(qid, &msg,
        // timeout).  The ref write-through needs the RAW item — the dispatch
        // below re-pops it when called with raw items; from a closure (no
        // refs) the element is returned inline.
        if (args.empty()) return make_int(-101);
        const int64_t qid = lpc::queueResolveId(args[0]);
        if (qid < 0) return make_int(-101);
        bool ok = false;
        Value msg = lpc::queueReceive(qid, 0, ok);
        if (ok) return msg;   // closure flavor: the element itself
        ResumeCond cond;
        cond.kind = 2;
        cond.qkey = "q:" + std::to_string(qid);
        parkAndSuspend(pcb, cond, nullptr, make_int(-402));
    }
    if (name == "send_to_queue") {
        // vm_f_send_to_queue (0x80f2d94): enqueue + wake a parked receiver.
        if (args.empty()) return make_int(-101);
        const int64_t qid = lpc::queueResolveId(args[0]);
        if (qid < 0) return make_int(-101);
        const Value& v = args.size() >= 2 ? args[1] : Value{};
        // Spin breaker (the original's per-process depth guard): a closure
        // whose execution queues the same-named closure again is a
        // self-dispatch loop - 128 consecutive self-sends -> the original's
        // "Too depth (128) function call!" instead of an infinite spin.
        if (v.index() == 8) {
            auto clv = std::get<std::shared_ptr<ClosureValue>>(v);
            const std::string topName = frameFnName(pcb);
            thread_local std::string t_lastSelfName;
            thread_local int t_selfSends = 0;
            if (clv && !clv->functionName.empty() && clv->functionName == topName) {
                if (++t_selfSends >= 128)
                    vmError("Too depth (128) function call!\n");
            } else {
                t_selfSends = 0;
            }
        }
        // A non-closure queue message tripped the old THREAD_D login flow
        // ("Bad message in THREAD_D") — print the sender under trace.  NOTE:
        // filed's srvQ messages are MAPPINGS by design (do_operate msg), so
        // this is diagnostic only, not an error.
        if (v.index() != 8 && vmcore::traceOn())
            std::cerr << "[q-send-noncl] qid=" << qid << " vtype=t"
                      << v.index() << " obj=" << fkey
                      << " fn=" << frameFnName(pcb) << "\n";
        const bool ok = lpc::queueSend(qid, v);
        if (vmcore::traceOn()) {
            std::cerr << "[qsend] qid=" << qid << " ok=" << ok
                      << " mt=" << v.index()
                      << " m=" << to_string(v, false).substr(0, 60)
                      << " pend=" << lpc::queuePendingItems(qid)
                      << " obj=" << fkey
                      << " fn=" << frameFnName(pcb) << " tid=" << pcb->tid << "\n";
        }
        if (ok) Scheduler::instance().wakeQueueOne("q:" + std::to_string(qid));
        return make_int(ok ? 0 : -101);
    }
    if (name == "accept") {
        // vm_f_accept (0x80e53c4): park on the listener socket; the hook
        // accepts when readable (a pending connection is present).
        if (args.empty()) return make_int(-1);
        const int fd = static_cast<int>(toInt(args[0]));
        ResumeCond cond;
        cond.kind = 5;
        cond.fd = fd;
        cond.wantRead = true;
        pcb->resumeHook = [pcb, fd]() {
            pcb->resumeValue = make_int(xdbcAccept(fd));
        };
        parkAndSuspend(pcb, cond, nullptr, make_int(-1));
    }
    if (name == "connect") {
        // vm_f_connect (0x80e83b4): NON-BLOCKING connect; on EINPROGRESS park
        // until the socket is writable, then report the connect result.
        if (args.size() >= 3) {
            const int fd = static_cast<int>(toInt(args[0]));
            const std::string host = to_string(args[1], false);
            const int port = static_cast<int>(toInt(args[2]));
            const int rc = xdbcConnectAsync(fd, host, port);
            if (rc == 0) return make_int(0);
            if (rc == -1) return make_int(-1);
            ResumeCond cond;
            cond.kind = 5;
            cond.fd = fd;
            cond.wantWrite = true;
            pcb->resumeHook = [pcb, fd]() {
                pcb->resumeValue = make_int(xdbcConnectResult(fd));
            };
            parkAndSuspend(pcb, cond, nullptr, make_int(0));
        }
        return make_int(-1);
    }
    if (name == "exit" || name == "exit_thread" || name == "clear_env") {
        // vm_f_exit (0x80d5c4c) / vm_f_exit_thread (0x80ed268) /
        // vm_f_clear_env (0x80d4ee0): the current LPC thread terminates.
        throw ThreadExitSignal{};
    }
    if (name == "relinquish") {
        // vm_f_relinquish: voluntary yield — push 1 NOW (the resumed process
        // sees the value on the stack), then yield; the slice resume pushes
        // nothing (the old core's exact rule).
        pushValue(pcb, make_int(1));
        throw TimeSliceSignal{};
    }
    if (name == "error") {
        // vm_f_error (0x80d64bc): raise an LPC error (catchable).
        // args[0] is a FORMAT string ("Object (id = %d) is not found."), the
        // rest are its arguments — the original formats it (evidence: gs
        // boot logs "[vm-error] Object (id = %d) is not found.\n 14713"
        // whenever we merely concatenated).
        std::string msg;
        if (!args.empty() && args[0].index() == 4) {
            msg = std::get<SharedString>(args[0]).str();
            size_t ai = 1;
            std::string out;
            out.reserve(msg.size() + 16);
            for (size_t i = 0; i < msg.size(); ++i) {
                char c = msg[i];
                if (c != '%' || ai >= args.size()) { out += c; continue; }
                size_t j = i + 1;
                while (j < msg.size() &&
                       (msg[j] == '-' || msg[j] == '+' || msg[j] == ' ' ||
                        msg[j] == '#' || msg[j] == '.' ||
                        (msg[j] >= '0' && msg[j] <= '9')))
                    ++j;
                if (j >= msg.size()) { out += c; continue; }
                char conv = msg[j];
                const Value& av = args[ai++];
                std::string rep;
                switch (conv) {
                case 'd': case 'i': case 'u':
                    rep = std::to_string(toInt(av)); break;
                case 'x':
                    rep = [&] {
                        char b[24];
                        std::snprintf(b, sizeof(b), "%llx",
                                      static_cast<unsigned long long>(toInt(av)));
                        return std::string(b);
                    }();
                    break;
                case 'f': case 'g': {
                    double dv = (av.index() == 3) ? std::get<double>(av)
                                                  : static_cast<double>(toInt(av));
                    rep = std::to_string(dv);
                    // %g style: trim trailing zeros (close enough for error text)
                    auto dot = rep.find('.');
                    if (dot != std::string::npos) {
                        size_t last = rep.find_last_not_of('0');
                        if (last == dot) --last;
                        rep = rep.substr(0, last + 1);
                    }
                    break;
                }
                case 's':
                    rep = to_string(av, false); break;
                case 'c':
                    rep = std::string(1, static_cast<char>(toInt(av))); break;
                case '%':
                    out += '%';
                    --ai;
                    i = j;
                    continue;
                default:
                    out += c;
                    ai = 1;   // not a conversion: leave args untouched
                    break;
                }
                out += rep;
                i = j;
            }
            msg = out;
        } else {
            for (size_t k = 0; k < args.size(); ++k) {
                if (k) msg += " ";
                msg += to_string(args[k], false);
            }
        }
        vmError(msg);
    }

    // ---- object-environment model (mirrors the original driver) ----------
    if (h == 0x174FE94E4CE1BC4Cull) {
        // move_object(container, ob) — THIS game's order (start_gs's
        // update_object does move_object(ob, obs[i]) to put obs[i] INTO ob).
        // move_object(container) - self is the moved object.
        std::string destKey = args.empty() ? std::string() : keyOfArg(args[0]);
        std::string movedKey = (args.size() >= 2) ? keyOfArg(args[1])
                                                  : std::string();
        if (movedKey.empty()) movedKey = fkey;
        if (!movedKey.empty()) envStoreKey(movedKey, destKey);
        return make_int(1);
    }
    if (h == 0x858C9F2913C1E90ull) {
        // vm_f_env_object: the environment of THIS object (or the argument).
        std::string obKey = args.empty() ? fkey : keyOfArg(args[0]);
        if (obKey.empty()) return make_int(0);
        const std::string env = envLookupKey(obKey);
        if (env.empty()) return make_int(0);
        return make_object(env);
    }
    if (h == 0x1FCFC3932B360855ull) {
        // vm_f_get_timer_id (0x80ef6d4)
        std::shared_ptr<TimerEntry> e;
        if (!args.empty()) {
            if (args[0].index() == 4)
                e = timerFindByName(std::get<SharedString>(args[0]));
            else if (args[0].index() == 2)
                e = timerFindById(toInt(args[0]));
        }
        return make_int(e ? e->id : 0);
    }
    if (h == 0x9899B6195332623Bull) {
        // vm_f_previous_function (0x80f62ec): caller's function name.
        int64_t level = args.empty() ? 1 : toInt(args[0]);
        if (level < 1) level = 1;
        if (!pcb || static_cast<size_t>(level) >= pcb->frames.size())
            return make_int(0);
        const VmFrame& fr =
            pcb->frames[pcb->frames.size() - 1 - static_cast<size_t>(level)];
        if (!fr.fn) return make_int(0);
        return make_string(fr.fn->name);
    }
    if (h == 0xEBF3922C7806B2F4ull) {
        // vm_f_object_dir (0x80f63e8): directory part of the program path.
        std::string path = fobj ? fobj->prog()->name() : fkey;
        if (!args.empty()) {
            if (args[0].index() == 4) path = std::get<SharedString>(args[0]);
            else if (args[0].index() == 7) path = keyOfArg(args[0]);
        }
        const size_t slash = path.rfind('/');
        return make_string(slash == std::string::npos
                               ? path : path.substr(0, slash + 1));
    }
    if (h == 0xF7707BBF3A8ED850ull) {
        // vm_f_leave_object (0x80f6454): unlink from the container.
        std::string key = args.empty() ? fkey : keyOfArg(args[0]);
        if (key.empty()) key = fkey;
        if (!key.empty()) envStoreKey(key, "");
        return make_int(0);
    }
    if (h == 0xAF888C6C3C418282ull) {
        // vm_f_lodger_object (0x80f41c8): container's lodgers as an ARRAY
        // (tail-link order = move_object's append order).  Lenient where the
        // original raises: a GM query must not die on a two-way-spelled key.
        if (args.size() > 1) vmError("Too many arguments to lodger_object.");
        std::string key = args.empty() ? fkey : keyOfArg(args[0]);
        if (key.empty() || key == "<this>" || key == ".")
            vmError("Not found the object for lodger_object.\n");
        auto out = std::make_shared<ArrayValue>();
        for (const std::string& k : envLodgerKeysOf(key))
            out->items.push_back(make_object(k));
        return out;
    }
    if (h == 0xD4A2D50BC20098B1ull || h == 0x8DF2E004D5C4C718ull) {
        // vm_f_objects (0x80f4d40) / vm_f_child_objects (0x80f5d9c)
        const bool isObjects = (h == 0xD4A2D50BC20098B1ull);
        if (isObjects && !args.empty())
            vmError("Too many arguments to objects.");
        if (!isObjects && args.size() > 2)
            vmError("Too many arguments to child_objects.");
        if (!isObjects && args.empty())
            vmError("Miss argument 1, expected object | string.");
        auto progOf = [](const std::string& k) {
            std::string s = k;
            if (s.rfind("clone:", 0) == 0) s.erase(0, 6);
            else if (s.rfind("pak:", 0) == 0) s.erase(0, 4);
            const size_t hp = s.find('#');
            if (hp != std::string::npos) s.resize(hp);
            while (!s.empty() && s.front() == '/') s.erase(s.begin());
            if (s.size() > 2 && s.compare(s.size() - 2, 2, ".c") == 0)
                s[s.size() - 1] = 'o';
            return s;
        };
        std::string wantProg;
        bool withoutRaw = false;
        if (!isObjects) {
            const std::string k = keyOfArg(args[0]);
            if (k.empty()) vmError("Not found the object for child_objects.");
            if (!Registry::instance().findByName(k))
                vmError("Not found the object for child_objects.");
            wantProg = progOf(k);
            withoutRaw = args.size() > 1 && !isZero(args[1]);
        }
        auto out = std::make_shared<ArrayValue>();
        for (VmObject* o : Registry::instance().allObjects()) {
            if (!o || o->reallyGone()) continue;
            const std::string& k = o->key();
            const bool isClone = k.find('#') != std::string::npos;
            if (!isObjects) {
                if (progOf(k) != wantProg) continue;
                if (withoutRaw && !isClone) continue;
            }
            out->items.push_back(Registry::makeObjectValue(o));
        }
        return out;
    }
    if (h == 0x678B5DD66DD6F908ull) {
        // vm_f_neighbour_object (0x80f69b4): objects sharing my container.
        const std::string key = args.empty() ? fkey : keyOfArg(args[0]);
        auto out = std::make_shared<ArrayValue>();
        const std::string env = envLookupKey(key);
        if (!env.empty()) {
            for (VmObject* o : Registry::instance().allObjects()) {
                if (!o || o->key() == key) continue;
                if (envLookupKey(o->key()) == env)
                    out->items.push_back(Registry::makeObjectValue(o));
            }
        }
        return out;
    }
    if (h == 0x8E0C7C94CC5355F5ull) {
        // vm_f_get_obj_string (0x80f6dac): the object's string table; arg2
        // selects one entry (out of range -> 0).
        const VmProgram* prog = fobj ? fobj->prog() : nullptr;
        if (!args.empty() && args[0].index() == 4) {
            VmObject* o = Registry::instance().findByName(
                std::get<SharedString>(args[0]));
            if (o) prog = o->prog();
        }
        if (!prog) return make_int(0);
        if (args.size() > 1 && args[1].index() == 2) {
            const int64_t idx = toInt(args[1]);
            if (idx < 0 ||
                static_cast<size_t>(idx) >= prog->src().strings.size())
                return make_int(0);
            return make_string(prog->src().strings[static_cast<size_t>(idx)]);
        }
        auto a = std::make_shared<ArrayValue>();
        for (const std::string& s : prog->src().strings)
            a->items.push_back(make_string(s));
        return a;
    }
    if (h == 0x5928D133C40EDD36ull) {
        // vm_f_bind_value (0x80f5874): a value bound to an object.
        const std::string key = args.empty() ? fkey : keyOfArg(args[0]);
        static std::mutex bvMx;
        static std::unordered_map<std::string, Value> bvMap;
        std::lock_guard<std::mutex> lk(bvMx);
        if (args.size() > 1) bvMap[key] = args[1];
        auto it = bvMap.find(key);
        if (it == bvMap.end()) return Value(UndefinedTag{});
        return it->second;
    }
    if (h == 0xC9B03746F985D2E7ull) {
        // vm_f_find_object_function (0x80f6194): closure for obj|name's
        // function (extra arguments become bound args).
        if (args.size() < 2) return make_int(0);
        VmObject* target = objOfValue(args[0]);
        const std::string fnName =
            (args[1].index() == 4) ? std::get<SharedString>(args[1])
                                   : std::string();
        if (!target || fnName.empty()) return make_int(0);
        const OriginalFunction* fn = target->prog()->findFn(fnName);
        if (!fn) return make_int(0);
        auto cl = std::make_shared<ClosureValue>();
        cl->functionName = fnName;
        cl->func = fn;
        cl->funcIndex = fn->index;
        cl->owner = &target->prog()->src();
        cl->ownerKey = target->key();
        for (size_t i = 2; i < args.size(); ++i)
            cl->boundArgs.push_back(args[i]);
        return cl;
    }
    if (h == 0xF946A1EB35FB4FDEull) {
        // vm_f_get_global_function (0x80f65ec): closure for a function of
        // the driver's GLOBAL program (the master).
        if (args.empty() || args[0].index() != 4) return make_int(0);
        const std::string fnName = std::get<SharedString>(args[0]);
        const VmProgram* master = Interp::masterProgram();
        if (!master) return make_int(0);
        const OriginalFunction* fn = master->findFn(fnName);
        if (!fn) return make_int(0);
        auto cl = std::make_shared<ClosureValue>();
        cl->functionName = fnName;
        cl->func = fn;
        cl->funcIndex = fn->index;
        if (Interp::masterObject())
            cl->owner = &Interp::masterObject()->prog()->src();
        for (size_t i = 1; i < args.size(); ++i)
            cl->boundArgs.push_back(args[i]);
        return cl;
    }
    if (h == 0xE962EDC63ED4F9Bull) {
        // vm_f_get_function_info (0x80d6570)
        if (args.empty() || args[0].index() != 8) return make_int(0);
        auto cl = std::get<std::shared_ptr<ClosureValue>>(args[0]);
        if (!cl) return make_int(0);
        auto m = std::make_shared<MappingValue>();
        if (!cl->func) {
            m->insert(make_string("type"), make_string("external"));
            return m;
        }
        m->insert(make_string("args"), std::make_shared<ArrayValue>());
        m->insert(make_string("vars"), std::make_shared<ArrayValue>());
        if (!cl->ownerKey.empty())
            m->insert(make_string("owner"), make_object(cl->ownerKey));
        else
            m->insert(make_string("owner"), Value(UndefinedTag{}));
        const bool isGlobal =
            Interp::masterObject() && cl->ownerKey == Interp::masterObject()->key();
        m->insert(make_string("is_global"), make_int(isGlobal ? 1 : 0));
        return m;
    }
    if (h == 0x46016A74752945E9ull) {
        // vm_f_size_program (0x80f9a24)
        const VmProgram* prog = fobj ? fobj->prog() : nullptr;
        if (!args.empty()) {
            VmObject* o = objOfValue(args[0]);
            if (o) prog = o->prog();
        }
        if (!prog) return make_int(0);
        const OriginalObject& o = prog->src();
        size_t strBytes = 0;
        for (const std::string& s : o.strings) strBytes += s.size() + 1;
        size_t codeBytes = 0;
        for (const OriginalFunction& f : o.functions)
            codeBytes += f.code.size() * 8;
        auto m = std::make_shared<MappingValue>();
        m->insert(make_string("structure_size"),
                  make_int(static_cast<int64_t>(sizeof(OriginalObject))));
        m->insert(make_string("hash_table_size"), make_int(0));
        m->insert(make_string("consititution_list_size"),
                  make_int(static_cast<int64_t>(o.inherits.size()) * 16));
        m->insert(make_string("init_order_list_size"), make_int(0));
        m->insert(make_string("variable_definition_size"),
                  make_int(static_cast<int64_t>(o.globalVarCount) * 8));
        m->insert(make_string("string_list_size"),
                  make_int(static_cast<int64_t>(strBytes)));
        m->insert(make_string("function_list_size"),
                  make_int(static_cast<int64_t>(codeBytes)));
        m->insert(make_string("program_size"),
                  make_int(static_cast<int64_t>(strBytes + codeBytes +
                                                o.globalVarCount * 8)));
        auto arr = std::make_shared<ArrayValue>();
        for (const OriginalFunction& f : o.functions) {
            auto e = std::make_shared<ArrayValue>();
            e->items.push_back(make_string(f.name));
            e->items.push_back(make_int(static_cast<int64_t>(f.code.size())));
            arr->items.push_back(e);
        }
        m->insert(make_string("all_function"), arr);
        return m;
    }
    if (name == "values" || name == "keys") {
        // vm_f_keys / vm_f_values: the mapping's key/value list.  A missing
        // handler once made every values(map) call return UNDEFINED and
        // whole trade flows silently did nothing.
        if (args.empty() || args[0].index() != 6) return make_array();
        auto out = std::make_shared<ArrayValue>();
        for (auto& kv : std::get<std::shared_ptr<MappingValue>>(args[0])->items())
            out->items.push_back(name == "values" ? kv.second : kv.first);
        return out;
    }
    if (name == "get_function_report") return make_array();
    if (name == "orphan_values") return make_array();
    if (name == "seq_no_to_tid") return make_int(-1);
    if (name == "get_thread_name") return make_string("");
    if (name == "get_thread_seq_no") return make_int(0);
    if (name == "get_thread_priority") return make_int(0);
    if (h == 0x93974224DCC554B0ull) {
        // vm_f_get_thread_info: the CURRENT process's identity.
        auto m = std::make_shared<MappingValue>();
        m->insert(make_string("tid"), make_int(pcb ? pcb->tid : lpcThreadId()));
        m->insert(make_string("euid"), make_int(0));
        m->insert(make_string("attr"), make_int(0));
        return m;
    }
    if (h == 0x341F07FBDC2DEE30ull || h == 0xBEA8D3F7220BDBD9ull) {
        // vm_f_previous_object (0x80f6b28): the object that called into the
        // current function.
        if (!pcb || pcb->previousObjectName.empty()) {
            // [pvdbg] register_trigger 依赖 previous_object() 拿触发器实例;
            // 返回 0 → name_triggers[0]=0 → punishd 永久重试。
            static const std::atomic<bool>& pvd0 =
                dynProbeFlag("LPCVM_OP8DBG");
            if (pvd0.load(std::memory_order_relaxed) && pcb &&
                !pcb->frames.empty() && pcb->frames.back().fn &&
                pcb->frames.back().fn->name == "register_trigger")
                std::cerr << "[pvdbg] previous_object -> 0 (empty prevKey)"
                          << "\n";
            return make_int(0);
        }
        VmObject* pvOb = Registry::instance().findByName(
            pcb->previousObjectName);
        static const std::atomic<bool>& pvd = dynProbeFlag("LPCVM_OP8DBG");
        if (pvd.load(std::memory_order_relaxed) && pcb &&
            !pcb->frames.empty() && pcb->frames.back().fn &&
            pcb->frames.back().fn->name == "register_trigger")
            std::cerr << "[pvdbg] previous_object -> prevKey='"
                      << pcb->previousObjectName
                      << "' resolved="
                      << (pvOb ? pvOb->key() : "<null>")
                      << "\n";
        // [isobj-fix] 返回 Registry 锚定的对象值 (携带真实 alive 标志)。
        // 原 make_object(name) 的 alive==nullptr 在 op36 (typeof 10) 的
        // is_object 语义里被判非对象 (lw62: punishd.do_init 的
        // is_object(triggerd->query_combat_trigger(...)) 恒 0 → 永久重试),
        // 而同值在 builtins is_object (objectValueAlive) 里却是活 —— 两个
        // 实现语义相反。统一锚定到注册表对象。
        if (pvOb) return Registry::makeObjectValue(pvOb);
        return make_object(pcb->previousObjectName);
    }
    if (h == 0xABCB23648594386Bull) {
        // vm_f_object_template (0x80f4b9c): the object this one was cloned
        // from (the REGISTERED TEMPLATE INSTANCE, never a program path —
        // basic_object.c's create() reads my_import_name from it).
        VmObject* t = nullptr;
        if (args.empty()) {
            t = mutableFrameObj ? mutableFrameObj->templateObj : nullptr;
        } else {
            VmObject* a = objOfValue(args[0]);
            t = a ? a->templateObj : nullptr;
        }
        if (t) return Registry::makeObjectValue(t);
        return make_int(0);
    }
    if (name == "object_name" && args.empty()) {
        // vm_f_object_name, no argument: the CURRENT object's full virtual
        // path (".c" spelling; clone keys keep their "#<seq>" suffix).
        std::string n = fkey;
        if (n.compare(0, 4, "pak:") == 0) n = n.substr(4);
        else if (n.compare(0, 6, "clone:") == 0) n = n.substr(6);
        if (!n.empty() && n.front() != '/') n = "/" + n;
        if (n.size() >= 2 && n.compare(n.size() - 2, 2, ".o") == 0)
            n.replace(n.size() - 2, 2, ".c");
        return make_string(n);
    }
    if (name == "object_name" && !args.empty()) {
        // object_name(ob, flag): the argument object's path; flag != 0 strips
        // the "#<seq>" instance suffix (the PROGRAM path — dbase.c's clone
        // fallback reads the template's dbase through it).
        std::string key = keyOfArg(args[0]);
        if (key.empty() || key == "<this>" || key == ".") key = fkey;
        if (key.rfind("oid:", 0) == 0) key.clear();
        if (key.rfind("clone:", 0) == 0) key = key.substr(6);
        else if (key.rfind("pak:", 0) == 0) key = key.substr(4);
        if (args.size() >= 2 && toInt(args[1]) != 0) {
            const size_t hp = key.find('#');
            if (hp != std::string::npos) key = key.substr(0, hp);
        }
        if (!key.empty() && key.front() != '/') key = "/" + key;
        if (key.size() >= 2 && key.compare(key.size() - 2, 2, ".o") == 0)
            key.replace(key.size() - 2, 2, ".c");
        return make_string(key);
    }
    if (name == "call_other" && args.size() >= 2) {
        // vm_f_call_other (0x80f5aa0): call_other(ob, fun, a1..aN).
        if (args[0].index() == 5) {
            return vmCallOtherArray(
                std::get<std::shared_ptr<ArrayValue>>(args[0])->items,
                args[1].index() == 4 ? std::get<SharedString>(args[1]).str()
                                    : std::string(),
                std::vector<Value>(args.begin() + 2, args.end()));
        }
        if (args[0].index() == 7 || args[0].index() == 4) {
            return vmCallOther(
                args[0],
                args[1].index() == 4 ? std::get<SharedString>(args[1]).str()
                                     : std::string(),
                std::vector<Value>(args.begin() + 2, args.end()));
        }
        return make_int(0);
    }
    if (name == "call_other_arg" && args.size() >= 3) {
        // vm_f_call_other_arg (0x80f5afc): the PARAMETER LIST is the LAST
        // argument; an ARRAY there is SPLICED after the fixed arguments:
        //   call_other_arg(file, "func_entry", from_info, args)
        //     -> func_entry(from_info, args[0], args[1], ...)
        const Value& obv = args[0];
        const std::string funName =
            args[1].index() == 4 ? std::get<SharedString>(args[1]) : "";
        std::vector<Value> callArgs(args.begin() + 2, args.end());
        if (!callArgs.empty() && callArgs.back().index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(callArgs.back());
            callArgs.pop_back();
            for (const auto& e : arr->items) callArgs.push_back(e);
        }
        if (obv.index() == 5) {
            return vmCallOtherArray(
                std::get<std::shared_ptr<ArrayValue>>(obv)->items, funName,
                callArgs);
        }
        return vmCallOther(obv, funName, callArgs);
    }
    if (name == "mapping_array" && args.size() >= 2 && args[0].index() == 5) {
        // vm_f_mapping_array: same size, same order; the callable resolves
        // through its owner's CURRENT table (or the efun for an efun closure
        // — re-entering this dispatcher with the same name would recurse).
        auto srcArr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
        auto outArr = std::make_shared<ArrayValue>();
        outArr->items.assign(srcArr->items.size(), Value{});
        if (srcArr->items.empty()) return outArr;
        if (args[1].index() != 8) {
            outArr->items = srcArr->items;
            return outArr;
        }
        auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
        // 原版 vm_f_mapping_array1/2 范本：逐元素 vm_callFunPtr 压帧调用
        // closure，vm_hookCallbackWhenReturnAt(下一元素) 链式推进；全部完成
        // 后 returnFromEfun。这里同构：压帧 + 帧上 retHook 链推进，结果经
        // 帧返回值交付（park 安全——钩子随帧活在 PCB 上，C++ 无续延）。
        // 同步兜底仅限 efun closure（纯 C++，不 park）。
        struct MapArrCtx {
            VmPcb* pcb;
            std::shared_ptr<ClosureValue> cl;
            std::shared_ptr<ArrayValue> src, out;
            size_t nonzero = 0;   // [mapdbg] 收尾统计
            bool completed = false;   // 链在本次 C++ 调用内同步走完(无新帧)
        };
        static const std::atomic<bool>& mapDbg = dynProbeFlag("LPCVM_MAPDBG");
        static const std::function<void(const std::shared_ptr<MapArrCtx>&,
                                        size_t, bool)> mapArrStep =
            [](const std::shared_ptr<MapArrCtx>& st, size_t i, bool viaHook) {
                if (i >= st->src->items.size()) {
                    // [mapdbg] statued 卡死链取证: mapping_array 收尾时统计
                    // 闭包结果的类型分布 —— find_basic_object_by_import_name
                    // 全 0 就是 8160 不开的直接原因。
                    st->completed = true;
                    if (mapDbg.load(std::memory_order_relaxed) &&
                        st->cl && st->cl->functionName.find("find_basic") !=
                                      std::string::npos) {
                        static std::atomic<int> mapLog{0};
                        if (mapLog.fetch_add(1) < 500) {
                            std::string v0 = st->out->items.empty()
                                                 ? "<empty>"
                                                 : to_string(st->out->items[0],
                                                             false)
                                                       .substr(0, 48);
                            std::string s0 = st->src->items.empty()
                                                 ? "<empty>"
                                                 : to_string(st->src->items[0],
                                                             false)
                                                       .substr(0, 48);
                            std::cerr << "[mapdbg] cl=" << st->cl->functionName
                                      << " src=" << st->src->items.size()
                                      << " src0=" << s0
                                      << " nonzero=" << st->nonzero << "/"
                                      << st->out->items.size() << " out0=" << v0
                                      << " caller="
                                      << (st->pcb->frames.empty() ||
                                                  !st->pcb->frames.back().fn
                                              ? "<ctx>"
                                              : st->pcb->frames.back().fn->name)
                                      << "\n";
                        }
                    }
                    // 仅当整条链在 efun 体内同步走完(空数组/全 fallback)时
                    // 由这里压栈交付(viaHook=false); hook 链路径的交付由
                    // hook 的 rv 出参带回 vmReturnFromFun 的单次 pushValue
                    // —— 在 hook 里 pushValue 会让返回路径再 push 一次
                    // returnValue, 双重压栈顶层是闭包返回值(对象),
                    // query_objects 赋值拿到对象而非数组 → sizeof=0 →
                    // statued 永久早退 (lw36 [op8ret] type=7 实锤)。
                    if (!viaHook)
                        Interp::pushValue(st->pcb, Value{st->out});
                    return;
                }
                if (!vmInvokeClosurePush(
                        st->pcb, st->cl, {st->src->items[i]},
                        [st, i](Value& rv) {
                            if (rv.index() != 0 && rv.index() != 1)
                                ++st->nonzero;
                            st->out->items[i] = std::move(rv);
                            if (i + 1 >= st->src->items.size()) {
                                // 末元素: 结果数组经 rv 出参交付
                                // (vmReturnFromFun 的 pushValue(returnValue))
                                rv = Value{st->out};
                                return;
                            }
                            mapArrStep(st, i + 1, true);
                            // 链在 hook 内同步走完(fallback): 交付改由 rv
                            if (st->completed) rv = Value{st->out};
                        })) {
                    st->out->items[i] =
                        vmInvokeClosure(st->pcb, st->cl, {st->src->items[i]});
                    if (st->out->items[i].index() != 0 &&
                        st->out->items[i].index() != 1)
                        ++st->nonzero;
                    mapArrStep(st, i + 1, viaHook);
                }
            };
        auto st = std::make_shared<MapArrCtx>();
        st->pcb = pcb; st->cl = cl; st->src = srcArr; st->out = outArr;
        mapArrStep(st, 0, false);
        pcb->efunTakeover = true;
        return outArr;
    }
    if (name == "is_clone" && args.empty()) {
        // vm_f_is_clone (0x80f7900): the CURRENT object's clone flag.
        return make_int(fkey.find('#') != std::string::npos ? 1 : 0);
    }
    if (h == 0x1931A18DDF58D0D1ull || h == 0x4B4A9D4304B193BAull) {
        // vm_f_find_function (0x80d626c) / vm_f_find_function_arg (0x80d6347)
        // -> vm_findFunction (0x8094ef4): build a bound closure.  The
        // _arg form expands a trailing ARRAY into individual bound arguments.
        bool expandLastArray = false;
        size_t bindFrom = 1;
        if (name == "find_function_arg" && args.size() >= 2 &&
            args.back().index() == 5) {
            expandLastArray = true;
            bindFrom = 1;
        }
        std::vector<Value> bind;
        if (expandLastArray) {
            for (size_t i = bindFrom; i + 1 < args.size(); ++i)
                bind.push_back(args[i]);
            for (const auto& e :
                 std::get<std::shared_ptr<ArrayValue>>(args.back())->items)
                bind.push_back(e);
        } else {
            for (size_t i = bindFrom; i < args.size(); ++i)
                bind.push_back(args[i]);
        }
        if (args.empty()) return Value{};
        const Value& fn0 = args[0];
        auto makeBound = [&](const std::shared_ptr<ClosureValue>& src) {
            auto cl = std::make_shared<ClosureValue>();
            cl->functionName = src->functionName;
            cl->funcIndex = src->funcIndex;
            cl->func = src->func;
            cl->efunClosure = src->efunClosure;
            cl->owner = src->owner;
            cl->ownerKey = src->ownerKey;
            cl->boundArgs = src->boundArgs;
            for (const auto& b : bind) cl->boundArgs.push_back(b);
            return cl;
        };
        if (fn0.index() == 8)
            return makeBound(std::get<std::shared_ptr<ClosureValue>>(fn0));
        if (fn0.index() == 4) {
            const std::string fname = std::get<SharedString>(fn0);
            // Current frame's table first, then the master's (global), then
            // the efun table (an efun name builds an efun closure).
            if (mutableFrameObj) {
                if (const OriginalFunction* fn =
                        mutableFrameObj->prog()->findFn(fname)) {
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = fname;
                    cl->funcIndex = fn->index;
                    cl->func = fn;
                    cl->owner = &mutableFrameObj->prog()->src();
                    cl->ownerKey = mutableFrameObj->key();
                    return makeBound(cl);
                }
            }
            if (Interp::masterProgram() &&
                (!mutableFrameObj ||
                 Interp::masterProgram() != mutableFrameObj->prog())) {
                if (const OriginalFunction* mfn =
                        Interp::masterProgram()->findFn(fname)) {
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = fname;
                    cl->funcIndex = mfn->index;
                    cl->func = mfn;
                    cl->owner = &Interp::masterProgram()->src();
                    if (Interp::masterObject())
                        cl->ownerKey = Interp::masterObject()->key();
                    return makeBound(cl);
                }
            }
            const auto& efn = efunNameTable();
            for (size_t i = 0; i < efn.size(); ++i)
                if (efn[i] == fname) {
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = fname;
                    cl->efunClosure = true;
                    cl->funcIndex = static_cast<int>(i);
                    return makeBound(cl);
                }
            return Value{};   // function not found -> type 0 "undefined"
        }
        return Value{};
    }
    if (h == 0xF24B45A2899A9AF2ull) {
        // vm_f_broadcast: pure log message (object loading is driven by the
        // LPC startup code, not a C++ directory walk).
        if (!args.empty()) {
            const std::string msg = to_string(args[0], false);
            if (!msg.empty()) std::cout << msg << "\n";
        }
        return make_int(1);
    }
    if (h == 0x2F5F8440DCDE401Cull) {
        // vm_f_create_thread (0x80ecbc0): arg0 = name (string or mapping),
        // arg1 = the function (closure, or a string resolved in the CURRENT
        // object), arg2+ = arguments.  Ordinary daemons AND telnet daemons
        // are scheduler processes now — no OS thread.
        std::string funName;
        std::vector<Value> threadArgs;
        VmObject* owner = nullptr;
        if (args.size() >= 2) {
            if (args[1].index() == 8) {
                auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
                funName = cl->functionName;
                threadArgs = cl->boundArgs;
                owner = objOfValue(make_object(cl->ownerKey));
                if (cl->efunClosure) {
                    // A closure wrapping an efun (create_thread((:call_other,
                    // stalld, "stall_daemon":))): run it on its OWN process.
                    // Inline execution chained the CALLER into the efun's
                    // callee — stalld's while(1)+sleep(180) daemon loop
                    // captured the boot master and 8160 never opened (原版
                    // create_thread 的闭包永远跑在新线程上).
                    if (getenv("LPCVM_CTRACE")) {
                        std::cerr << "[ct-efun] fn=" << cl->functionName
                                  << " owner="
                                  << (owner ? owner->key() : std::string("<null>"))
                                  << "\n";
                        std::cerr.flush();
                    }
                    Scheduler::instance().spawnEfunThread(owner, cl);
                    return make_int(1);
                }
                if (getenv("LPCVM_CTRACE")) {
                    std::cerr << "[ct-norm] fn=" << cl->functionName
                              << " efunCl=" << (cl->efunClosure ? 1 : 0)
                              << " owner="
                              << (owner ? owner->key() : std::string("<null>"))
                              << "\n";
                    std::cerr.flush();
                }
            } else if (args[1].index() == 4) {
                funName = std::get<SharedString>(args[1]);
                for (size_t i = 2; i < args.size(); ++i)
                    threadArgs.push_back(args[i]);
                owner = mutableFrameObj;
            }
        } else if (args.size() == 1 && args[0].index() == 8) {
            auto cl = std::get<std::shared_ptr<ClosureValue>>(args[0]);
            funName = cl->functionName;
            threadArgs = cl->boundArgs;
            owner = objOfValue(make_object(cl->ownerKey));
        }
        if (funName.empty() || !owner) return make_int(0);
        // configd's update_config_daemon performs the FIRST config load in
        // the original driver BEFORE the master's post_init: run one
        // update_config() synchronously here so do_init callbacks see
        // populated config (the daemon still re-runs it every 180s).
        if (funName == "update_config_daemon") {
            const OriginalFunction* ufn = owner->prog()->findFn("update_config");
            if (ufn) {
                // 原版模式：压帧调用（update_config 内的阻塞 efun park 后
                // resume 照常走完），不再走 C++ 同步递归（vmApplySync 在
                // park 时会销毁 C++ 续延）。create_thread 的返回值经帧
                // retHook 固定为 1。
                vmPushFunCall(pcb, owner, ufn, {});
                vmHookCallbackWhenReturn(pcb, [](Value& rv) {
                    rv = make_int(1);   // create_thread 返回值
                });
                pcb->efunTakeover = true;
            }
        }
        const bool isTelnetThread =
            funName == "telnet_daemon" || funName == "buildin_telnet_daemon";
        int telnetCno = -1;
        if (isTelnetThread && !threadArgs.empty() && threadArgs[0].index() == 2)
            telnetCno = static_cast<int>(std::get<int64_t>(threadArgs[0]));
        if (isTelnetThread)
            Scheduler::instance().spawnTelnet(owner, funName, threadArgs,
                                              telnetCno);
        else
            Scheduler::instance().spawn(owner, funName, threadArgs);
        return make_int(1);
    }
    if (h == 0xC161F892ED78AE9Cull) {
        // register_post_init is real LPC on the master (its post_init() runs
        // registered closures from the global code); the efun form is a no-op.
        return make_int(1);
    }
    if (name == "set_timer" && args.size() >= 3) {
        // vm_f_set_timer: set_timer(timeout, id|&ref|name, func, arg1, ...)
        // — every argument after func is handed to the callback on fire.
        const Value& fn = args[2];
        std::string fnName;
        std::vector<Value> bound;
        std::shared_ptr<ClosureValue> cl;
        if (fn.index() == 8) {
            cl = std::get<std::shared_ptr<ClosureValue>>(fn);
            fnName = cl->functionName;
            bound = cl->boundArgs;
        } else if (fn.index() == 4) {
            fnName = std::get<SharedString>(fn);
        }
        for (size_t ai = 3; ai < args.size(); ++ai) bound.push_back(args[ai]);
        if (fnName.empty()) return make_int(0);
        // An EFUN closure callback ((: call_other, ob, "fn" :)): schedule the
        // fire as an efun process (spawnDelayedEfun).  Without this the fire
        // resolved "call_other" in the owner's program table, findFn failed
        // and the scheduler chain-aborted every fire of such timers.
        std::shared_ptr<ClosureValue> timerCl;
        if (cl && cl->efunClosure) {
            timerCl = std::make_shared<ClosureValue>(*cl);
            timerCl->boundArgs = bound;
        }
        // The callback runs in the context of the object that owns the
        // closure; a string callback belongs to the CALLING object —
        // this_object() semantics of the original timer fire.
        VmObject* owner = cl ? objOfValue(make_object(cl->ownerKey)) : nullptr;
        if (!owner) owner = mutableFrameObj;
        if (!owner) return make_int(0);
        std::string timerName;
        if (args.size() >= 2 && args[1].index() == 4)
            timerName = std::get<SharedString>(args[1]);
        int64_t timeout = std::max<int64_t>(1, toInt(args[0]));
        if (timeout > 2147483647LL) timeout = 2147483647LL;
        const int64_t id = timerSchedule(timerName, fnName, owner, timeout,
                                         std::move(bound), timerCl);
        {
            // LPCVM_TIMERDBG: the disconnect->save chain hinges on this timer
            // (user.o::disconnected -> set_timer(0/180,&_timer_id,"time_expired")
            //  -> time_expired -> destruct -> userd::destroy_user -> save).
            static const std::atomic<bool>& tmFlag = dynProbeFlag("LPCVM_TIMERDBG");
            if (tmFlag.load(std::memory_order_relaxed) &&
                (fnName == "time_expired" || fnName == "logout" ||
                 timerName == "_timer_id")) {
                std::cerr << "[timer] set_timer id=" << id << " delay=" << timeout
                          << "s fn=" << fnName << " name=" << timerName
                          << " owner=" << owner->key() << "\n";
                std::cerr.flush();
            }
        }
        return make_int(id);
    }
    if (h == 0x807E5C16A0A571A7ull) {
        // vm_f_get_timer (0x80ef470): info mapping or 0.
        std::shared_ptr<TimerEntry> e;
        if (!args.empty()) {
            if (args[0].index() == 4)
                e = timerFindByName(std::get<SharedString>(args[0]));
            else if (args[0].index() == 2)
                e = timerFindById(toInt(args[0]));
        }
        if (!e) return make_int(0);
        const double acc = timerRemainingSecs(e);
        auto tmap = std::make_shared<MappingValue>();
        tmap->insert(make_string("callback"),
                     make_string("(: " + e->callback + " :)"));
        tmap->insert(make_string("timeout"), make_float(acc));
        tmap->insert(make_string("id"), make_int(e->id));
        tmap->insert(make_string("name"), make_string(e->name));
        tmap->insert(make_string("output"), make_string("Disabled"));
        return tmap;
    }
    if (h == 0x684D89F3B1641FFAull) {
        // vm_f_delete_timer (0x80ef404): 0 on success, -601 when not found.
        std::shared_ptr<TimerEntry> e;
        if (!args.empty()) {
            if (args[0].index() == 4)
                e = timerFindByName(std::get<SharedString>(args[0]));
            else if (args[0].index() == 2)
                e = timerFindById(toInt(args[0]));
        }
        if (!e) return make_int(-601);
        timerUnregister(e);
        return make_int(0);
    }
    if (name == "sort" || name == "sort_mapping")
        return efunSort(pcb, args);
    if (h == 0xC70B9A266114A70Cull) {
        // vm_f_take_mutex (0x80f16cc): PROCESS-level named mutex.  free or
        // owned by THIS process -> level++ (recursive, 0); owned by another:
        // timeout omitted -> park until give (wake 0); > 0 -> park + timer
        // (-202); <= 0 -> -202 now.  unknown name -> -101.
        if (args.empty()) return make_int(-101);
        const std::string mname = to_string(args[0], false);
        int tform = -1;
        double tsecs = 0.0;
        if (args.size() >= 2) {
            const Value& t = args[1];
            if (t.index() == 2) {
                const int64_t n = std::get<int64_t>(t);
                if (n == -1) tform = -1;
                else if (n > 0) { tform = 1; tsecs = static_cast<double>(n); }
                else tform = 0;
            } else if (t.index() == 3) {
                const double d = std::get<double>(t);
                if (d > 0.0) { tform = 1; tsecs = d; }
                else tform = 0;
            } else {
                return make_int(-101);
            }
        }
        Scheduler& sched = Scheduler::instance();
        const int rc = sched.mutexTake(mname, -1, Value{});
        if (rc == 0) return make_int(0);
        if (rc == -101) return make_int(-101);
        // rc == 1: must pend.
        if (tform == 0) return make_int(-202);
        ResumeCond cond;
        cond.kind = 6;
        cond.ename = mname;
        if (tform == 1 && tsecs > 0.0)
            cond.wakeAtMs = steadyMs() + static_cast<int64_t>(tsecs * 1000.0);
        parkAndSuspend(pcb, cond, nullptr, make_int(0));
    }
    if (h == 0x41211ABFE4EFFB1Aull) {
        // vm_f_give_mutex (0x80f1564)
        if (args.empty()) return make_int(-101);
        Scheduler::instance().mutexGive(to_string(args[0], false));
        return make_int(0);
    }
    if (h == 0x3594A415072D5B7Full) {
        // vm_f_create_mutex (0x80eed58)
        if (args.empty()) return make_int(0);
        Scheduler::instance().mutexCreate(to_string(args[0], false));
        return make_int(1);
    }
    if (h == 0xCCCCDD05E424173Cull) {
        if (args.empty()) return make_int(0);
        Scheduler::instance().mutexErase(to_string(args[0], false));
        return make_int(1);
    }
    if (name == "clone_object" && !args.empty()) {
        // vm_f_clone_object (0x80f8eac): the argument may be a path or an
        // object value (cloning that very instance); extra args -> create().
        std::string path;
        if (args[0].index() == 7) {
            VmObject* o = objOfValue(args[0]);
            path = o ? o->key() : keyOfArg(args[0]);
        } else {
            path = to_string(args[0], false);
        }
        std::vector<Value> createArgs(args.begin() + 1, args.end());
        return vmCloneObjectValue(path, createArgs);
    }
    if (h == 0x610814AF049B0603ull) {
        // vm_f_load_object (0x80f8cbc): third argument = the program CONTENT
        // (string = LPC source compiled, buffer = a program image) — the
        // anti-cheat challenge script arrives through here.
        if (args.empty()) return make_int(0);
        std::string vpath;
        if (args[0].index() == 4) vpath = std::get<SharedString>(args[0]);
        else vpath = to_string(args[0], false);
        std::vector<Value> createArgs;
        if (args.size() >= 2) createArgs.push_back(args[1]);
        std::string progImage;
        if (args.size() >= 3 && (args[2].index() == 10 || args[2].index() == 4)) {
            if (args[2].index() == 10) {
                auto bv = std::get<std::shared_ptr<BufferValue>>(args[2]);
                if (bv) progImage = bv->data;
            } else {
                progImage = std::get<SharedString>(args[2]).str();
            }
        }
        return vmLoadObjectValue(vpath, createArgs, progImage);
    }
    if (name == "load_object" || name == "safe_load_object") {
        // (safe_)load_object(path[, createArgs]) without the image argument.
        if (args.empty()) return make_int(0);
        std::string vpath;
        if (args[0].index() == 4) vpath = std::get<SharedString>(args[0]);
        else vpath = to_string(args[0], false);
        std::vector<Value> createArgs;
        if (args.size() >= 2) createArgs.push_back(args[1]);
        return vmLoadObjectValue(vpath, createArgs, std::string());
    }

    // ---- everything else: the builtins layer (driver-side asset) ---------
    // Remember WHO is calling so [efun-stub] lines identify object::function
    // and destruct_object(this_object()) resolves the exact instance.
    lpc::stubCallCtx().obj = fkey;
    lpc::stubCallCtx().fn = frameFnName(pcb);
    lpc::stubCallCtx().key = fkey;
    Value bres = call_builtin(nullptr, name, args);
    if (name == "move_in_map" || name == "put_in_map" ||
        name == "remove_from_map") {
        bool chainArmed = false;
        fireMapHooks(pcb, name, args, bres, &chainArmed);
        if (chainArmed) {
            // hook 链真实压帧 → bres 由链尾的 rv 出参交付, 这里必须 takeover
            pcb->efunTakeover = true;
            return Value{};
        }
    }
    return bres;
}

void dispatchEfun(VmPcb* pcb, int efunIndex, int argc) {
    const auto& names = efunNameTable();
    const std::string name =
        (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size())
            ? names[static_cast<size_t>(efunIndex)] : std::string();
    // Pop args; KEEP the raw items for ref-taking efuns, deref for the rest
    // (an lvalue &buf argument must reach the efun as the live container).
    std::vector<StackItem> raw;
    for (int i = 0; i < argc && !pcb->stack.empty(); ++i)
        raw.push_back(Interp::popItem(pcb));

    // [etime] efun 耗时审计 (touch _probe_on/LPCVM_ETIME 热布防): 每次
    // dispatchEfun 计时, 按名聚合 调用数/累计µs/最大µs; 每 20000 次调用
    // dump top10。"整体比原版慢一个量级" — 区分 耗在 efun C++ 实现内 vs
    // interp 指令循环。
    struct EtGuard {
        const std::atomic<bool>& on;
        std::chrono::steady_clock::time_point t0;
        std::string name;
        bool active;
        ~EtGuard() {
            if (!active || !on.load(std::memory_order_relaxed)) return;
            const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now() - t0)
                                .count();
            struct Stat { uint64_t calls = 0, totalUs = 0, maxUs = 0; };
            static std::mutex mx;
            static std::map<std::string, Stat> stats;
            static std::atomic<uint64_t> totalCount{0};
            std::lock_guard<std::mutex> lk(mx);
            auto& s = stats[name];
            s.calls++; s.totalUs += static_cast<uint64_t>(us);
            if (static_cast<uint64_t>(us) > s.maxUs) s.maxUs = static_cast<uint64_t>(us);
            if (totalCount.fetch_add(1) % 20000 == 0) {
                std::vector<std::pair<std::string, Stat>> v(stats.begin(),
                                                            stats.end());
                std::sort(v.begin(), v.end(), [](auto& a, auto& b) {
                    return a.second.totalUs > b.second.totalUs;
                });
                std::cerr << "[etime] dump totalCall="
                          << totalCount.load() << "\n";
                for (size_t i = 0;
                     i < v.size() && i < 10; ++i)
                    std::cerr << "[etime] " << v[i].first
                              << " calls=" << v[i].second.calls
                              << " totalMs="
                              << v[i].second.totalUs / 1000
                              << " avgUs="
                              << (v[i].second.calls
                                      ? v[i].second.totalUs /
                                            v[i].second.calls
                                      : 0)
                              << " maxUs=" << v[i].second.maxUs << "\n";
                std::cerr.flush();
            }
        }
    } etGuard{dynProbeFlag("LPCVM_ETIME"),
              std::chrono::steady_clock::now(), name,
              true};

    // ---- RAW-REF intercepts (the reference must SURVIVE the call) --------
    if (name == "sscanf" && raw.size() >= 2) {
        // sscanf(input, fmt, &a, &b, ...): parse, write each conversion back
        // through its reference, push the match count.
        auto strOf = [&](const StackItem& r) -> std::string {
            Value v = r.isRef ? Interp::derefItem(pcb, r) : r.value;
            if (v.index() == 4) return std::get<SharedString>(v);
            return to_string(v, false);
        };
        const std::string input = strOf(raw[0]);
        const std::string fmtStr = strOf(raw[1]);
        std::vector<Value> outs;
        const int maxOutputs = static_cast<int>(raw.size()) - 2;
        const int n = lpcSscanf(input, fmtStr, outs, maxOutputs);
        for (size_t i = 0; i + 2 < raw.size() && i < outs.size(); ++i)
            Interp::storeItem(pcb, raw[i + 2], outs[i]);
        pushValue(pcb, make_int(n));
        return;
    }
    if (name == "recv" && raw.size() >= 3) {
        // vm_f_recv (0x80e80c8): recv(fd, &buf, maxlen).  Data -> a BUFFER
        // written through the ref + the byte count; no data yet -> park on
        // the socket (kind 5), the hook receives on wake.
        auto valOf = [&](const StackItem& s) {
            return s.isRef ? Interp::derefItem(pcb, s) : s.value;
        };
        const int fd = static_cast<int>(toInt(valOf(raw[0])));
        int len = static_cast<int>(toInt(valOf(raw[2])));
        if (len <= 0)
            vmError("Bad max length (" + std::to_string(len) +
                    ") of buffer to receive from socket.");
        if (len > 0x10000000) len = 0x10000000;
        if (!xdbcSocketExists(fd))
            vmError("Non-existed socket " + std::to_string(fd) + ".");
        bool ok = false;
        const std::string data = xdbcRecvAsync(fd, len, ok);
        if (ok) {
            if (!data.empty())
                Interp::storeItem(pcb, raw[1], make_buffer(data));
            pushValue(pcb, make_int(static_cast<int64_t>(data.size())));
            return;
        }
        ResumeCond cond;
        cond.kind = 5;
        cond.fd = fd;
        cond.wantRead = true;
        StackItem bufRef = raw[1];
        pcb->resumeHook = [pcb, fd, len, bufRef]() mutable {
            bool rok = false;
            const std::string d = xdbcRecvAsync(fd, len, rok);
            if (rok && !d.empty())
                Interp::storeItem(pcb, bufRef, make_buffer(d));
            pcb->resumeValue = make_int(rok ? static_cast<int64_t>(d.size())
                                            : -101);
        };
        parkAndSuspend(pcb, cond, nullptr, make_int(0));
    }
    if (name == "select" && !raw.empty()) {
        // vm_f_select (0x80e7348): select(&read, &write, &except, timeout)
        // (exactly 4 args; each mask is a referenced array of socket ids or
        // 0; timeout SECONDS, <0 -> 0).  The process ALWAYS pends (timer +
        // the FIRST socket); each wake polls with 0 timeout, ready sockets
        // stay in the arrays, the result is the poll count; timer expiry
        // resumes with -126 and leaves the arrays untouched.
        if (raw.size() > 4) vmError("Too many arguments to select.");
        if (raw.size() < 4) vmError("Miss arguments, expected 4 arguments.");
        auto valOf = [&](const StackItem& s) {
            return s.isRef ? Interp::derefItem(pcb, s) : s.value;
        };
        std::vector<int> masks[3];
        bool isArr[3] = {false, false, false};
        for (int k = 0; k < 3; ++k) {
            const Value v = valOf(raw[static_cast<size_t>(k)]);
            if (raw[static_cast<size_t>(k)].isRef && v.index() == 5) {
                isArr[k] = true;
                for (const auto& e :
                     std::get<std::shared_ptr<ArrayValue>>(v)->items) {
                    const int sfd = static_cast<int>(toInt(e));
                    if (!xdbcSocketExists(sfd))
                        vmError("Non-existed socket " + std::to_string(sfd) + ".");
                    masks[k].push_back(sfd);
                }
            } else if (!(v.index() == 2 && std::get<int64_t>(v) == 0)) {
                vmError("Bad type of argument " + std::to_string(k + 1) +
                        ", expected array or 0.");
            }
        }
        const Value tv = valOf(raw[3]);
        double tsecs = 0.0;
        if (tv.index() == 2) tsecs = static_cast<double>(std::get<int64_t>(tv));
        else if (tv.index() == 3) tsecs = std::get<double>(tv);
        else vmError("Bad type of argument 4, expected a float or integer.");
        if (tsecs < 0.0) tsecs = 0.0;
        StackItem refs[3] = {raw[0], raw[1], raw[2]};
        // One vm_doSelect pass: filter each mask, write back when n > 0.
        auto doSelect = [masks, isArr, refs, pcb]() -> int64_t {
            std::vector<int> all;
            for (int k = 0; k < 3; ++k)
                for (int sfd : masks[k])
                    if (std::find(all.begin(), all.end(), sfd) == all.end())
                        all.push_back(sfd);
            std::vector<int> rd, wr;
            if (!all.empty()) xdbcPoll(all, rd, wr, 0);
            auto has = [](const std::vector<int>& v, int x) {
                return std::find(v.begin(), v.end(), x) != v.end();
            };
            std::vector<int> out[3];
            std::vector<int> readyFds;
            for (int k = 0; k < 3; ++k) {
                for (int sfd : masks[k]) {
                    const bool ok = k == 0 ? has(rd, sfd)
                                  : k == 1 ? has(wr, sfd)
                                           : false;
                    if (ok) {
                        out[k].push_back(sfd);
                        if (std::find(readyFds.begin(), readyFds.end(), sfd) ==
                            readyFds.end())
                            readyFds.push_back(sfd);
                    }
                }
            }
            const int64_t n = static_cast<int64_t>(readyFds.size());
            if (n > 0) {
                for (int k = 0; k < 3; ++k) {
                    if (!isArr[k]) continue;
                    auto arr = std::make_shared<ArrayValue>();
                    for (int sfd : out[k]) arr->items.push_back(make_int(sfd));
                    Interp::storeItem(pcb, refs[k], Value(arr));
                }
            }
            return n;
        };
        int wakeFd = !masks[0].empty() ? masks[0][0]
                                       : (!masks[1].empty() ? masks[1][0] : -1);
        ResumeCond cond;
        int64_t waitMs = static_cast<int64_t>(tsecs * 1000.0);
        if (waitMs < 1) waitMs = 1;   // a 0 timeout still pends once
        cond.wakeAtMs = steadyMs() + waitMs;
        if (wakeFd >= 0) {
            cond.kind = 5;
            cond.fd = wakeFd;
            cond.wantRead = !masks[0].empty();
            cond.wantWrite = masks[0].empty();
        } else {
            cond.kind = 1;
        }
        pcb->resumeHook = [pcb, doSelect]() {
            const int64_t n = doSelect();
            pcb->resumeValue = make_int(n > 0 ? n : -126);
        };
        parkAndSuspend(pcb, cond, nullptr, make_int(-126));
    }
    if (name == "receive_from_queue" && raw.size() >= 2) {
        // vm_f_receive_from_queue (0x80f29f8): receive_from_queue(qid, &msg,
        // timeout) — non-blocking receive first; on empty PARK on the queue
        // (send_to_queue wakes it); the hook writes the message through the
        // reference; resume value 0.  -101 when the queue is unknown.
        const bool qTrace = vmcore::traceOn();
        const Value qv = raw[0].isRef ? Interp::derefItem(pcb, raw[0])
                                      : raw[0].value;
        const int64_t qid = lpc::queueResolveId(qv);
        if (qid < 0) {
            pushValue(pcb, make_int(-101));
            return;
        }
        bool ok = false;
        const Value msg = lpc::queueReceive(qid, 0, ok);
        if (qTrace) {
            std::cerr << "[qrecv] nb qid=" << qid << " ok=" << ok
                      << " mt=" << (ok ? msg.index() : -1)
                      << " by=" << frameFnName(pcb) << " tid=" << pcb->tid
                      << "\n";
            std::cerr.flush();
        }
        if (ok) {
            Interp::storeItem(pcb, raw[1], msg);
            pushValue(pcb, make_int(0));
            return;
        }
        ResumeCond cond;
        cond.kind = 2;
        cond.qkey = "q:" + std::to_string(qid);
        StackItem msgRef = raw[1];
        const bool wakeTrace = qTrace;
        pcb->resumeHook = [pcb, qid, msgRef, wakeTrace]() mutable {
            Value m;
            bool rok = false;
            if (pcb->hasQueueDelivery) {
                // Delivered at send time (the reservation model).
                m = std::move(pcb->queueDelivery);
                pcb->queueDelivery = Value{};
                pcb->hasQueueDelivery = false;
                rok = true;
            } else {
                m = lpc::queueReceive(qid, 0, rok);
            }
            if (wakeTrace) {
                std::cerr << "[qrecv] wake qid=" << qid << " ok=" << rok
                          << " mt=" << (rok ? m.index() : -1)
                          << " pend=" << lpc::queuePendingItems(qid)
                          << " tid=" << pcb->tid;
                if (rok && m.index() == 6) {
                    auto mp = std::get<std::shared_ptr<MappingValue>>(m);
                    std::cerr << " mkeys=" << mp->size() << ":";
                    int shown = 0;
                    for (const auto& nd : mp->nodes) {
                        if (nd.next == -2) continue;   // deleted tombstone
                        if (shown++ >= 6) break;
                        std::string k = to_string(nd.key, false);
                        std::cerr << " " << k.substr(0, 10) << "="
                                  << to_string(nd.value, false).substr(0, 14);
                    }
                }
                std::cerr << "\n";
                std::cerr.flush();
            }
            if (rok) Interp::storeItem(pcb, msgRef, m);
        };
        parkAndSuspend(pcb, cond, nullptr, make_int(0));
    }
    if (name == "put_sub_data") {
        efunPutSubData(pcb, raw);
        // 原版 vm_returnFromEfan (0x08093c5c): efun 返回时栈上恒有恰好 1 个
        // 返回值 —— C 实现没压就 vm_pushVoid() 补压 (retType==255 且 v2==0
        // 走 LABEL_16)。字节码侧 [op6 put_sub_data][op51 pop_stack] 成对出
        // 现 (anticheaterd.encrypt_script 循环体 [36][37])，少压 1 个 void
        // 让循环体每轮净 -1，第二轮 op80 读 ctrl 槽 oob ->
        // "Bad loop ctrl" -> 整个 anticheaterd 加载失败。
        pushValue(pcb, Value{});
        return;
    }
    if (name == "new_view_point" && raw.size() >= 7) {
        // vm_f_new_view_point(map, ox, oy, nx, ny, &app, &dis): the sight
        // window diff that drives move_to's walking appear/disappear
        // notifications.  The constant-0 stub silenced every walking reveal.
        std::vector<Value> eargs;
        eargs.reserve(raw.size());
        for (auto& it : raw)
            eargs.push_back(it.isRef ? Interp::derefItem(pcb, it) : it.value);
        Value r = lpc::mapModule().newViewPoint(eargs);
        if (r.index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(r);
            auto get = [&](const char* k) {
                Value out;
                auto v = mp->find(make_string(k));
                if (v) out = *v;
                return out;
            };
            if (raw[5].isRef) Interp::storeItem(pcb, raw[5], get("app"));
            if (raw[6].isRef) Interp::storeItem(pcb, raw[6], get("dis"));
            // non-zero when anything changed
            auto nonEmpty = [&](const Value& v) {
                return v.index() == 5 &&
                       !std::get<std::shared_ptr<ArrayValue>>(v)->items.empty();
            };
            pushValue(pcb, make_int((nonEmpty(get("app")) || nonEmpty(get("dis"))) ? 1 : 0));
        } else {
            if (raw[5].isRef) Interp::storeItem(pcb, raw[5], make_array());
            if (raw[6].isRef) Interp::storeItem(pcb, raw[6], make_array());
            pushValue(pcb, make_int(0));
        }
        return;
    }

    if ((name == "find_reachable" || name == "find_reachable_ex") &&
        raw.size() >= 3) {
        // vm_f_find_reachable / vm_f_find_reachable_ex
        //   find_reachable(map_index, &x, &y [, radius [, plot]])
        // Spiral-searches outward from (x,y) for the first standable cell
        // (is_blocked == 0), writes it back through the by-reference x/y and
        // returns 1; 0 only when nothing standable is within the radius.
        // The constant-0 stub made userd.c's login enter_room treat every
        // restored position as unreachable -> default-town fallback, so no
        // character ever came back where they logged out.
        std::vector<Value> eargs;
        eargs.reserve(raw.size());
        for (auto& it : raw)
            eargs.push_back(it.isRef ? Interp::derefItem(pcb, it) : it.value);
        int x0 = static_cast<int>(toInt(eargs[1]));
        int y0 = static_cast<int>(toInt(eargs[2]));
        int range = eargs.size() >= 4 ? static_cast<int>(toInt(eargs[3])) : 8;
        if (range < 1) range = 1;
        if (range > 128) range = 128;
        auto standable = [&](int x, int y) {
            std::vector<Value> a = {eargs[0], make_int(x), make_int(y)};
            Value b = lpc::mapModule().is_blocked(a);
            return b.index() == 2 && std::get<int64_t>(b) == 0;
        };
        int fx = -1, fy = -1;
        if (standable(x0, y0)) {
            fx = x0; fy = y0;
        } else {
            for (int r = 1; r <= range && fx < 0; ++r) {
                for (int dx = -r; dx <= r && fx < 0; ++dx) {
                    for (int dy = -r; dy <= r && fx < 0; ++dy) {
                        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
                        if (standable(x0 + dx, y0 + dy)) {
                            fx = x0 + dx; fy = y0 + dy;
                        }
                    }
                }
            }
        }
        if (fx >= 0) {
            if (raw[1].isRef) Interp::storeItem(pcb, raw[1], make_int(fx));
            if (raw[2].isRef) Interp::storeItem(pcb, raw[2], make_int(fy));
            pushValue(pcb, make_int(1));
        } else {
            pushValue(pcb, make_int(0));
        }
        return;
    }

    if (name == "set_timer" && raw.size() >= 3) {
        // set_timer(timeout, &timer_id, func, ...): the 2nd parameter is a
        // by-reference OUT slot for the new timer id — dispatch it where the
        // StackItem (and its reference) is still available.
        std::vector<Value> eargs;
        eargs.reserve(raw.size());
        for (auto& it : raw)
            eargs.push_back(it.isRef ? Interp::derefItem(pcb, it) : it.value);
        const Value r = dispatchEfunByName(pcb, name, eargs);
        if (pcb->efunTakeover) {
            pcb->efunTakeover = false;
            return;   // construction frame takeover: the retHook chain pushes
                      // the object value when the chain completes
        }
        if (raw[1].isRef && r.index() == 2)
            Interp::storeItem(pcb, raw[1], r);
        pushValue(pcb, r);
        return;
    }

    std::vector<Value> args;
    args.reserve(raw.size());
    for (auto& it : raw)
        args.push_back(it.isRef ? Interp::derefItem(pcb, it) : it.value);
    Value r = dispatchEfunByName(pcb, name, args);
    if (pcb->efunTakeover) {
        pcb->efunTakeover = false;
        return;   // construction frame takeover (see above)
    }
    // A blocking efun threw (park); the resume value is pushed by vmResume.
    pushValue(pcb, std::move(r));
}

Value dispatchEfunClosure(VmPcb* pcb, const std::shared_ptr<ClosureValue>& cl,
                          int argc) {
    // Efun closure: boundArgs first, then the call args (popped with ref
    // semantics preserved for closures like (: call_other, path, fn :) that
    // forward by-ref arguments).
    std::vector<Value> args = cl->boundArgs;
    // 原版 (vm_callFunPtr 0x809441c): funptr 的 pArgs 经 vm_pushValue 原样
    // 压栈 —— vptr 引用原样进栈; vm_callInOtherFun(0x8094cd8) 不搬运参数,
    // 被调方参数槽就是同一 vptr, 被调方的读写直接落在调用方栈槽上。
    // 跨 vector<Value> 边界时用 CrossVmRef 载体携带【栈槽】引用 (仅限
    // call_other/call_other_arg 型 efun closure —— 只有转发型 efun 需要
    // 保引用; 普通 efun 收参数按值, deref 即原版 vm_popValue 语义)。
    // pushArgs 见到 CrossVmRef 会还原成栈引用槽 (call.cpp)。
    const bool forwarding =
        cl->functionName == "call_other" || cl->functionName == "call_other_arg";
    for (int i = 0; i < argc && !pcb->stack.empty(); ++i) {
        StackItem it = Interp::popItem(pcb);
        if (forwarding && it.isRef && it.slotIndex >= 0) {
            auto r = std::make_shared<CrossVmRef>();
            r->vm = pcb;
            r->slotIndex = it.slotIndex;
            args.push_back(make_crossvm_ref(std::move(r)));
        } else {
            args.push_back(it.isRef ? Interp::derefItem(pcb, it) : it.value);
        }
    }
    Value r = dispatchEfunByName(pcb, cl->functionName, args);
    if (pcb->efunTakeover) {
        // efun closure 也会踩到 takeover 型 efun (sort_array 的异步 key
        // 收集链 / construct / load): 标志置位时结果由 hook 链压栈, 这里
        // 必须消费标志且不再 push —— 否则标志残留, 下一个无关 efun
        // (如 implode) 在 dispatchEfun 尾部被吞掉返回值 (净 -2),
        // create_search_terms 的循环 ctrl 槽被吃掉 → "Bad loop ctrl"。
        pcb->efunTakeover = false;
        return r;
    }
    pushValue(pcb, r);
    return r;
}

// ---------------------------------------------------------------------------
// boot wiring (driver calls once at startup)
// ---------------------------------------------------------------------------
void wireEfunLayer() {
    // The queue store is the driver's (builtins.cpp) real cross-thread queue:
    // a receiver that registered its park AFTER the sender pushed re-checks
    // it instead of sleeping on a non-empty queue.
    Scheduler::queueRecheck_ = [](const std::string& qkey) {
        // qkey = "q:<qid>"
        if (qkey.rfind("q:", 0) != 0) return false;
        const int64_t qid = std::atoll(qkey.c_str() + 2);
        return lpc::queuePendingItems(qid) > 0;
    };
    // The event raised-bit store is the driver's DriverEvent registry.
    Scheduler::eventPeekHook_ = [](const std::string& name) {
        return lpc::eventPeekRaised(name);
    };
    Scheduler::eventConsumeHook_ = [](const std::string& name, uint32_t mask) {
        lpc::eventClearBits(name, mask);
    };
    // The socket poll backend.
    Scheduler::sockPollHook_ =
        [](std::vector<int>& fds, std::vector<int>& rd, std::vector<int>& wr) {
            rd.clear();
            wr.clear();
            return xdbcPoll(fds, rd, wr, 0);
        };
}

// ---------------------------------------------------------------------------
// global-init segment driver: run a program's global code under a CONTEXT
// frame (fn == nullptr — return_from_fun ends the segment without popping).
// ---------------------------------------------------------------------------
void vmRunGlobalInit(VmPcb* pcb, VmObject* obj) {
    VmPcbSetter setPcb(pcb);
    const VmProgram* prog = obj ? obj->prog() : nullptr;
    if (!prog || prog->src().globalCode.empty()) return;
    VmFrame ctx;
    ctx.fn = nullptr;               // context frame: never popped by return
    ctx.obj = obj;
    ctx.prog = prog;
    ctx.strings = &prog->src().strings;
    pcb->frames.push_back(std::move(ctx));
    pcb->code = &prog->src().globalCode;
    pcb->meta = &prog->src().globalMeta;
    pcb->ip = 0;
    pcb->returned = false;
    Interp::run(pcb);
    // The context frame stays for the caller (construction flow pops it).
    if (!pcb->frames.empty() && pcb->frames.back().fn == nullptr)
        pcb->frames.pop_back();
}

} // namespace vmcore
} // namespace lpc
