// ===========================================================================
// vmcore/interp.cpp — 解释器 (全局栈 + 当前 PCB + 帧, 原版模型)
//
// opcode 语义逐个搬运自已验证实现 (编号/操作数布局/栈效应不变), 改动仅限
// 状态来源: this/变量表/函数表/字符串表一律从当前帧取。旧核的探测块
// (LPCVM_* 诊断) 不搬运; CrossVmRef 桥接不存在 — 跨对象调用就是当前 PCB
// 上的帧, by-ref 参数 (slot 引用) 同栈天然有效。
//
// 阻塞 efun 抛 SuspendSignal: 状态原封留在 PCB, 调度器挂起, 唤醒后
// vmResume 从保存的 ip 续跑 (vm_pendCurrentProcess 模型)。
// ===========================================================================
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>

#include "call.h"
#include "interp.h"
#include "object.h"
#include "original_vm.h"
#include "values.h"
#include "vmcore.h"
#include "efun_table.h"
#include "trace.h"

namespace lpc {
namespace vmcore {
namespace Interp {

// ---- diagnostic trace helpers (全套默认开, 见 trace.h) ----------------------
static std::string trcFnName(const VmPcb* pcb) {
    return (pcb->frames.empty() || !pcb->frames.back().fn)
               ? std::string() : pcb->frames.back().fn->name;
}

// 指令级步骤 trace: 每条执行步骤一行 (obj/fn/pc/op/栈深/栈顶值)。
// 死循环压制: 同一 (pc,op) 连续 64 次后每 4096 次报一次。
static void trcStep(VmPcb* pcb, size_t pc, int op, const char* opName) {
    if (!vmcore::traceStepOn() || !vmcore::traceWantOp(op)) return;
    static size_t lastPc = static_cast<size_t>(-1);
    static int lastOp = -1;
    static uint64_t run = 0;
    if (pc == lastPc && op == lastOp) {
        ++run;
        if (run > 64 && (run % 4096) != 0) return;
    } else {
        lastPc = pc;
        lastOp = op;
        run = 0;
    }
    std::cerr << "[step] "
              << (pcb->frames.empty() || !pcb->frames.back().obj
                      ? std::string("<null>")
                      : pcb->frames.back().obj->key())
              << " fn=" << (pcb->frames.empty()
                                ? std::string("<noframe>")
                                : (pcb->frames.back().fn
                                       ? pcb->frames.back().fn->name
                                       : std::string("<ctx>")))
              << " pc=" << pc << " op=" << opName << "(" << op << ")"
              << " depth=" << pcb->stack.size();
    for (int k = 0; k < 3; ++k) {
        size_t at = pcb->stack.size() - 1 - static_cast<size_t>(k);
        if (pcb->stack.size() > static_cast<size_t>(k) && at < pcb->stack.size())
            std::cerr << " t" << k << "="
                      << to_string(pcb->stack[at].value, false).substr(0, 40);
    }
    std::cerr << "\n";
}

// 语义级调用 trace: callee + 全部实参 (p0 在顶)。
static void trcCallStack(VmPcb* pcb, int op, const std::string& callee, int argc) {
    if (!vmcore::traceOn() || !vmcore::traceWant(callee) || !vmcore::traceWantOp(op))
        return;
    std::cerr << "[call] op=" << op << " fn=" << callee << " argc=" << argc
              << " caller=" << trcFnName(pcb)
              << " obj=" << (pcb->frames.empty() || !pcb->frames.back().obj
                                 ? std::string("<null>")
                                 : pcb->frames.back().obj->key())
              << " args=";
    for (int k = 0; k < argc; ++k) {
        size_t at = pcb->stack.size() - 1 - static_cast<size_t>(k);
        if (at < pcb->stack.size())
            std::cerr << "p" << k << "="
                      << to_string(pcb->stack[at].value, false).substr(0, 48) << " ";
    }
    std::cerr << "\n";
}

// 比较指令 trace: 左右操作数 + 结果 (全套数据)
#define TRC_CMP(name, opc, A, B, R)                                            \
    if (vmcore::traceOn()) {                                                   \
        const std::string trc_f = trcFnName(pcb);                              \
        if (vmcore::traceWant(trc_f) && vmcore::traceWantOp(opc))              \
            std::cerr << "[cmp] " << name                                      \
                      << " a=" << to_string(A, false).substr(0, 48)            \
                      << " b=" << to_string(B, false).substr(0, 48)            \
                      << " -> " << (R) << " fn=" << trc_f << "\n";             \
    }

// ---- shared globals (driver's global program) ------------------------------
std::vector<Value>& sharedGlobals() {
    static std::vector<Value> g;
    return g;
}
Value& sharedGlobal(int index) {
    auto& g = sharedGlobals();
    if (static_cast<size_t>(index) >= g.size())
        g.resize(static_cast<size_t>(index) + 1);
    return g[static_cast<size_t>(index)];
}

// ---- master (global) program ------------------------------------------------
static const VmProgram* g_masterProg = nullptr;
static VmObject* g_masterObj = nullptr;
void setMasterProgram(const VmProgram* prog, VmObject* obj) {
    g_masterProg = prog;
    g_masterObj = obj;
}
const VmProgram* masterProgram() { return g_masterProg; }
VmObject* masterObject() { return g_masterObj; }
const OriginalFunction* findMasterFunction(int index) {
    // fn.index IS the DLT link number = the GLOBAL table slot.  The global
    // table = efun slots [0..functionBC=572) followed by the boot object's
    // functions (start_aaa dlt 572..845, start_gs dlt 572..1879) — so a
    // global index is used VERBATIM against the boot object's byIndex_ table
    // (gs closure #1869 -> find_basic_object_by_import_name, dlt 1869).
    // Indices BELOW functionBC are efun slots (原版 funContext 存 pEfun):
    // callers (op7) must dispatch those as efuns, never here.
    return g_masterProg ? g_masterProg->fnAtIndex(index) : nullptr;
}

// ---- action table (operand shape; fromMeta flags) ---------------------------
namespace {
struct ActionInfo {
    const char* name;
    uint8_t parameters;
    uint32_t attrib;
};
const ActionInfo kActions[] = {
    /* 0  */ {"invalid", 0, 0},
    /* 1  */ {"add", 0, 0},
    /* 2  */ {"and", 0, 0},
    /* 3  */ {"assign_value", 0, 0},
    /* 4  */ {"assign_value_reverse", 0, 0},
    /* 5  */ {"call_fun_ptr", 2, 0x60},
    /* 6  */ {"call_in_efun", 2, 0x60},
    /* 7  */ {"call_in_user_fun", 2, 0x60},
    /* 8  */ {"call_in_other_fun_ex", 2, 0x60},
    /* 9  */ {"call_os_fun", 2, 0x80000060},
    /* 10 */ {"case_check_number", 2, 0x80000001},
    /* 11 */ {"case_check_string", 2, 0x80000001},
    /* 12 */ {"case_default", 1, 1},
    /* 13 */ {"cast_type", 2, 0},
    /* 14 */ {"caught_error", 0, 0},
    /* 15 */ {"check_args_num", 1, 1},
    /* 16 */ {"check_is_ref_type", 1, 0},
    /* 17 */ {"check_is_ref_type_i", 0, 0},
    /* 18 */ {"cond_jmp_for_loop", 2, 1},
    /* 19 */ {"convert_to_ptr", 0, 0x40},
    /* 20 */ {"convert_to_value", 0, 0x40},
    /* 21 */ {"create_argument_array", 0, 0},
    /* 22 */ {"create_array", 1, 0},
    /* 23 */ {"create_function_value", 2, 0},
    /* 24 */ {"create_mapping", 1, 0},
    /* 25 */ {"div_value", 0, 0},
    /* 26 */ {"end_thread_catch", 0, 0},
    /* 27 */ {"enter_function", 0, 0},
    /* 28 */ {"eq_value", 0, 0},
    /* 29 */ {"finish_action", 0, 0},
    /* 30 */ {"ge_value", 0, 0},
    /* 31 */ {"goto", 1, 1},
    /* 32 */ {"gt_value", 0, 0},
    /* 33 */ {"if", 2, 1},
    /* 34 */ {"inc_value", 1, 0xC0},
    /* 35 */ {"init_for_loop", 1, 0},
    /* 36 */ {"is_type", 1, 0},
    /* 37 */ {"le_value", 0, 0},
    /* 38 */ {"lsh_value", 0, 0},
    /* 39 */ {"lt_value", 0, 0},
    /* 40 */ {"mod_value", 0, 0},
    /* 41 */ {"mul_value", 0, 0},
    /* 42 */ {"ne_value", 0, 0},
    /* 43 */ {"neg_value", 0, 0},
    /* 44 */ {"new_thread_catch", 1, 1},
    /* 45 */ {"not_value", 0, 0},
    /* 46 */ {"null_action", 0, 0},
    /* 47 */ {"op_eq_value", 1, 0xC0},
    /* 48 */ {"or_value", 0, 0},
    /* 49 */ {"pickup_os_value", 2, 0x80000000},
    /* 50 */ {"pop_n_stack", 1, 0},
    /* 51 */ {"pop_stack", 0, 0},
    /* 52 */ {"push_arg_num", 0, 0},
    /* 53 */ {"push_global_var", 1, 4},
    /* 54 */ {"push_global_vptr", 1, 0x42},
    /* 55 */ {"push_local_value", 1, 4},
    /* 56 */ {"push_local_var_i", 0, 4},
    /* 57 */ {"push_local_vptr", 1, 0x42},
    /* 58 */ {"push_local_vptr_i", 0, 0x42},
    /* 59 */ {"push_number(S)", 1, 8},
    /* 60 */ {"push_number(L)", 1, 0x80000008},
    /* 61 */ {"push_obj_var", 1, 4},
    /* 62 */ {"push_obj_vptr", 1, 0x42},
    /* 63 */ {"push_real_from_word", 2, 0x80000008},
    /* 64 */ {"push_shared_string", 1, 8},
    /* 65 */ {"push_this_object", 0, 8},
    /* 66 */ {"push_undefined", 0, 8},
    /* 67 */ {"query_member", 1, 0x40},
    /* 68 */ {"query_member_ptr", 1, 0x40},
    /* 69 */ {"query_range", 1, 0x40},
    /* 70 */ {"query_range_ptr", 1, 0x40},
    /* 71 */ {"resort_stack", 0, 0},
    /* 72 */ {"return_from_fake_fun", 0, 0},
    /* 73 */ {"return_from_fun", 0, 0},
    /* 74 */ {"rev_value", 0, 0},
    /* 75 */ {"rsh_value", 0, 0},
    /* 76 */ {"set_global_var", 1, 0},
    /* 77 */ {"set_local_var", 1, 0},
    /* 78 */ {"set_obj_var", 1, 0},
    /* 79 */ {"sizeof", 0, 0},
    /* 80 */ {"step_jmp_for_loop", 2, 0},
    /* 81 */ {"sub_value", 0, 0},
    /* 82 */ {"to_reserve_stack_value", 0, 0},
    /* 83 */ {"to_unreserve_stack_value", 0, 0},
    /* 84 */ {"trace", 0, 0},
    /* 85 */ {"xor_value", 0, 0},
    /* 86 */ {"null", 0, 0},
};
const ActionInfo& actionInfo(int op) {
    static const ActionInfo invalid{"unknown", 0, 0};
    if (op < 0 || op >= static_cast<int>(sizeof(kActions) / sizeof(kActions[0])))
        return invalid;
    return kActions[op];
}

int32_t readI32(const std::vector<uint8_t>& m, uint32_t off) {
    if (off + 4 > m.size()) return 0;
    return static_cast<int32_t>(static_cast<uint32_t>(m[off]) |
                                (static_cast<uint32_t>(m[off + 1]) << 8) |
                                (static_cast<uint32_t>(m[off + 2]) << 16) |
                                (static_cast<uint32_t>(m[off + 3]) << 24));
}

// ---- frame-relative helpers --------------------------------------------------

int slotOf(const VmPcb* pcb, int a1) {
    if (pcb->frames.empty()) return a1;
    return pcb->frames.back().bp - a1;
}
static VmObject* curObj(const VmPcb* pcb) {
    if (!pcb->frames.empty()) return pcb->frames.back().obj;
    return pcb->obj ? pcb->obj : masterObject();
}
static const VmProgram* curProg(const VmPcb* pcb) {
    if (!pcb->frames.empty()) return pcb->frames.back().prog;
    return curObj(pcb) ? curObj(pcb)->prog() : masterProgram();
}
static int curVarOffset(const VmPcb* pcb) {
    return pcb->frames.empty() ? 0 : pcb->frames.back().varOffset;
}
static int curFunOffset(const VmPcb* pcb) {
    return pcb->frames.empty() ? 0 : pcb->frames.back().funOffset;
}
static const std::vector<std::string>* curStrings(const VmPcb* pcb) {
    if (!pcb->frames.empty() && pcb->frames.back().strings)
        return pcb->frames.back().strings;
    const VmProgram* p = curProg(pcb);
    return p ? &p->src().strings : nullptr;
}
} // namespace (helpers above are file-local; the reference semantics below

  // are Interp-level — interp.h declares them)

// ---- reference (vptr) semantics ----------------------------------------------
static StackItem makeItemV(Value v) {
    StackItem it;
    it.value = std::move(v);
    return it;
}

// Follow ref-to-ref chains (a ref slot that itself holds a ref).  Self-cycles
// bail out at depth 32 instead of dying on the C++ stack.
Value derefItem(VmPcb* pcb, const StackItem& ref, int depth) {
    if (depth > 32) return make_int(0);
    if (ref.slotIndex >= 0) {
        if (static_cast<size_t>(ref.slotIndex) >= pcb->stack.size())
            return make_int(0);
        const StackItem& it = pcb->stack[static_cast<size_t>(ref.slotIndex)];
        if (it.isRef) return derefItem(pcb, it, depth + 1);
        return it.value;
    }
    if (ref.globalIndex >= 0)
        return sharedGlobal(ref.globalIndex);
    if (ref.objIndex >= 0) {
        // The vptr is an address into the OWNING object's value array
        // (原版 v.info 语义): use the pinned object, not the current frame's.
        VmObject* o = ref.objRef ? ref.objRef : curObj(pcb);
        if (o && static_cast<size_t>(ref.objIndex) < o->vars().size())
            return o->vars()[static_cast<size_t>(ref.objIndex)];
        return make_int(0);
    }
    if (ref.arr) {
        int64_t i = toInt(ref.key);
        const auto& items = ref.arr->items;
        if (i >= 0 && i < static_cast<int64_t>(items.size()))
            return items[static_cast<size_t>(i)];
        return Value{};   // OOB -> original type 0 "undefined"
    }
    if (ref.map) {
        const Value* hit = ref.map->find(ref.key);
        if (hit) return *hit;
        return Value{};   // miss -> original type 0 "undefined"
    }
    return make_int(0);
}

// Follow a ref chain to its ultimate target item (the item whose
// slotIndex/globalIndex/objIndex/arr/map identifies the final destination).
static const StackItem* followRefChain(VmPcb* pcb, const StackItem& ref) {
    const StackItem* cur = &ref;
    int guard = 0;
    while (cur->isRef && guard++ < 64) {
        if (cur->slotIndex >= 0) {
            if (static_cast<size_t>(cur->slotIndex) >= pcb->stack.size())
                return cur;
            const StackItem& next = pcb->stack[static_cast<size_t>(cur->slotIndex)];
            if (next.isRef) cur = &next;
            else return cur;
        } else {
            return cur;   // global / obj / member ref: terminal destination
        }
    }
    return cur;
}

bool storeItem(VmPcb* pcb, const StackItem& ref, const Value& v) {
    const StackItem* target = followRefChain(pcb, ref);
    if (!target) return false;
    if (target->slotIndex >= 0) {
        if (static_cast<size_t>(target->slotIndex) < pcb->stack.size()) {
            // Tripwire: 活跃循环的 ctrl 槽 (Value index 9) 被非 op35 路径
            // 覆写 → post_init "Bad loop ctrl" 的元凶现场。
            // [性能] 必须 dynProbeFlag 门控 (默认关): 构造期帧清理会命中
            // 十几万次, 无缓冲 stderr 直接把 boot 拖慢 2 倍以上 (lw47 实测
            // 23.8 万行日志里 21.3 万条 slot-kill)。
            static const std::atomic<bool>& slotSnag =
                lpc::dynProbeFlag("LPCVM_SLOTSNAG");
            const Value& old = pcb->stack[static_cast<size_t>(target->slotIndex)].value;
            if (slotSnag.load(std::memory_order_relaxed) &&
                old.index() == 9 && v.index() != 9)
                std::cerr << "[loop-slot-kill] storeItem slot="
                          << target->slotIndex << " lastIp=" << pcb->lastIp
                          << " lastOp=" << static_cast<int>(pcb->lastOp)
                          << " frames=" << pcb->frames.size()
                          << " topfn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                               ? std::string("<ctx>")
                                               : pcb->frames.back().fn->name)
                          << " newVal=" << v.index() << "\n";
            pcb->stack[static_cast<size_t>(target->slotIndex)].value = v;
            return true;
        }
        return false;
    }
    if (target->globalIndex >= 0) {
        sharedGlobal(target->globalIndex) = v;
        return true;
    }
    if (target->objIndex >= 0) {
        VmObject* o = target->objRef ? target->objRef : curObj(pcb);
        if (o && static_cast<size_t>(target->objIndex) < o->vars().size()) {
            o->vars()[static_cast<size_t>(target->objIndex)] = v;
            return true;
        }
        return false;
    }
    if (target->arr) {
        int64_t i = toInt(target->key);
        auto& items = target->arr->items;
        if (i >= 0 && i < static_cast<int64_t>(items.size())) {
            items[static_cast<size_t>(i)] = v;
            return true;
        }
        return false;
    }
    if (target->map) {
        const bool trc = vmcore::traceOn() && vmcore::traceWant(trcFnName(pcb));
        const size_t before = trc ? target->map->size() : 0;
        target->map->insert(target->key, v);
        if (trc) {
            std::cerr << "[map] store map=" << target->map.get()
                      << " key=" << to_string(target->key, false).substr(0, 40)
                      << " val=" << to_string(v, false).substr(0, 60)
                      << " size " << before << "->" << target->map->size()
                      << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                       ? std::string("<ctx>")
                                       : pcb->frames.back().fn->name)
                      << " obj="
                      << (pcb->frames.empty() || !pcb->frames.back().obj
                              ? std::string("<null>")
                              : pcb->frames.back().obj->key())
                      << "\n";
        }
        return true;
    }
    if (vmcore::traceOn()) {
        std::cerr << "[store-fail] refType=" << (target->isRef ? 1 : 0)
                  << " slot=" << target->slotIndex << " glob=" << target->globalIndex
                  << " obj=" << target->objIndex << " arr=" << (target->arr ? 1 : 0)
                  << " map=" << (target->map ? 1 : 0)
                  << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                   ? std::string("<ctx>")
                                   : pcb->frames.back().fn->name)
                  << " valType=" << v.index() << "\n";
    }
    return false;
}

static Value slotValue(VmPcb* pcb, int idx) {
    if (idx < 0 || static_cast<size_t>(idx) >= pcb->stack.size())
        return make_int(0);
    const StackItem& it = pcb->stack[static_cast<size_t>(idx)];
    if (it.isRef) return derefItem(pcb, it);
    return it.value;
}

static StackItem makeSlotRef(int idx) {
    StackItem it;
    it.isRef = true;
    it.slotIndex = idx;
    return it;
}
static StackItem makeGlobalRefItem(int idx) {
    StackItem it;
    it.isRef = true;
    it.globalIndex = idx;
    return it;
}
static StackItem makeObjRefItem(VmObject* obj, int idx) {
    StackItem it;
    it.isRef = true;
    it.objIndex = idx;
    it.objRef = obj;
    return it;
}
static StackItem makeMemberRefItem(const Value& container, const Value& key) {
    StackItem it;
    it.isRef = true;
    it.key = key;
    if (container.index() == 5) it.arr = std::get<std::shared_ptr<ArrayValue>>(container);
    else if (container.index() == 6) it.map = std::get<std::shared_ptr<MappingValue>>(container);
    return it;
}

// ---- loop stepping (init_for_loop / cond_jmp / step_jmp) ----------------------
// ---- op trace ring (LPCVM_OPTRACE): last N executed insns, dumped by
// badCtrl on [loop-bad] — pinpoints which insn shrank the stack vs the
// original's accounting without static guesswork.
struct OpTraceEntry { uint32_t ip; uint8_t op; uint32_t size; uint32_t frames; };
static constexpr size_t kOpTraceN = 64;
static OpTraceEntry g_opTrace[kOpTraceN];
static size_t g_opTracePos = 0;
static size_t g_opTraceCount = 0;
static const bool g_opTraceOn = getenv("LPCVM_OPTRACE") != nullptr;
// [性能] CTRACE/LPCVM 环境开关一律启动时缓存 —— construct 链等热路径上
// 每条指令 getenv 会线性扫描环境块并加锁(采样实测占 VM 线程 100% 时间)。
static const bool g_ctraceOn = getenv("LPCVM_CTRACE") != nullptr;
static inline void opTraceRecord(uint32_t ip, uint8_t op, size_t stackSize,
                                 size_t frames) {
    if (!g_opTraceOn) return;
    g_opTrace[g_opTracePos] = {ip, op, static_cast<uint32_t>(stackSize),
                               static_cast<uint32_t>(frames)};
    g_opTracePos = (g_opTracePos + 1) % kOpTraceN;
    if (g_opTraceCount < kOpTraceN) ++g_opTraceCount;
}

static void loopStep(VmPcb* pcb, size_t& ip, int ctrlSlot,
                     int breakOffset, int continueOffset) {
    // 原版 vm_cmpAndJmpInLoop：坏循环状态（栈槽无效/非 LoopCtrl/空 info）
    // 一律 vm_error("Bad loop ctrl, failed to loop (in step).")——显式死。
    // 旧实现的静默 return 让被污染的循环"正常退出"（scand.do_init 的
    // 1.8 万对象插入停在 1200，oper_scan_table_flag=0，post_init 永不
    // 收敛），零日志。
    auto badCtrl = [&]() {
        std::cerr << "[loop-bad] slot=" << ctrlSlot << " stack=" << pcb->stack.size()
                  << " bp=" << (pcb->frames.empty() ? -1 : pcb->frames.back().bp)
                  << " idx=" << (ctrlSlot >= 0 && ctrlSlot < static_cast<int>(pcb->stack.size())
                                     ? std::to_string(pcb->stack[static_cast<size_t>(ctrlSlot)].value.index())
                                     : "oob")
                  << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                    ? std::string("<ctx>")
                                    : pcb->frames.back().fn->name)
                  << " brk=" << breakOffset << " cont=" << continueOffset << "\n";
        for (size_t fi = 0; fi < pcb->frames.size(); ++fi) {
            const VmFrame& fr = pcb->frames[fi];
            std::cerr << "[loop-bad-fr] i=" << fi
                      << " fn=" << (fr.fn ? fr.fn->name : std::string("<ctx>"))
                      << " bp=" << fr.bp << " argNum=" << fr.argNum
                      << " placeholder=" << (fr.zeroArgPlaceholder ? 1 : 0)
                      << " pad=" << fr.padArgs << "\n";
        }
        for (size_t si = 0; si < pcb->stack.size(); ++si)
            std::cerr << "[loop-bad-st] s[" << si << "] idx="
                      << pcb->stack[si].value.index()
                      << (pcb->stack[si].isRef ? " REF" : "") << "\n";
        if (g_opTraceCount) {
            std::cerr << "[op-trace] last " << g_opTraceCount << " insns (ip op stack frames):\n";
            for (size_t i = 0; i < g_opTraceCount; ++i) {
                const OpTraceEntry& e =
                    g_opTrace[(g_opTracePos + kOpTraceN - g_opTraceCount + i) % kOpTraceN];
                std::cerr << "  " << e.ip << " op=" << static_cast<int>(e.op)
                          << " stack=" << e.size << " fr=" << e.frames << "\n";
            }
        }
        std::cerr.flush();
        vmError("Bad loop ctrl, failed to loop (in step).");
    };
    if (ctrlSlot < 0 || static_cast<size_t>(ctrlSlot) >= pcb->stack.size())
        badCtrl();
    StackItem& slotItem = pcb->stack[static_cast<size_t>(ctrlSlot)];
    // 主存储: frames.back().loops (op35 双写)。栈槽镜像被 park 窗口重入搅掉
    // 时以 map 为准继续跑（栈槽内容已不可信, 不能再拿它当循环状态), 并留痕。
    std::shared_ptr<LoopCtrl> c;
    bool haveMap = false;
    if (!pcb->frames.empty()) {
        auto mapIt = pcb->frames.back().loops.find(ctrlSlot);
        if (mapIt != pcb->frames.back().loops.end() && mapIt->second) {
            c = mapIt->second;
            haveMap = true;
        }
    }
    if (haveMap) {
        if (slotItem.value.index() != 9) {
            static std::atomic<int> desyncShown{0};
            if (desyncShown.fetch_add(1) < 3) {
            std::cerr << "[loop-ctrl-desync] slot=" << ctrlSlot
                      << " stackIdx=" << slotItem.value.index()
                      << " val="
                      << to_string(slotItem.value, false).substr(0, 48)
                      << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                        ? std::string("<ctx>")
                                        : pcb->frames.back().fn->name)
                      << " ip=" << pcb->lastOp << "/" << pcb->lastIp;
            for (size_t fi = 0; fi < pcb->frames.size(); ++fi)
                std::cerr << " f" << fi << "="
                          << (pcb->frames[fi].fn ? pcb->frames[fi].fn->name
                                                 : "<ctx>")
                          << ":bp" << pcb->frames[fi].bp
                          << ":a" << pcb->frames[fi].argNum;
            std::cerr << " stack=";
            for (size_t si = 0; si < pcb->stack.size() && si < 8; ++si)
                std::cerr << si << ":" << pcb->stack[si].value.index() << " ";
            std::cerr << "\n";
            }
        }
    } else {
        if (slotItem.value.index() != 9)  // LoopCtrl is variant index 9
            badCtrl();
        c = std::get<std::shared_ptr<LoopCtrl>>(slotItem.value);
    }
    if (!c) badCtrl();
    auto setVar = [&](const Value& v) {
        // Prefer the RELATIVE variable operand (frame.bp - varOff): the ctrl
        // lives in a stack slot, and resort/reorder can shift absolute slots
        // while the frame is live.
        int slot = c->varIsSlot ? slotOf(pcb, c->varOff) : c->slotOfVar;
        if (slot >= 0 && static_cast<size_t>(slot) < pcb->stack.size()) {
            const Value& old = pcb->stack[static_cast<size_t>(slot)].value;
            static const std::atomic<bool>& slotSnag2 =
                lpc::dynProbeFlag("LPCVM_SLOTSNAG");
            if (slotSnag2.load(std::memory_order_relaxed) &&
                old.index() == 9 && v.index() != 9)
                std::cerr << "[loop-slot-kill] setVar slot=" << slot
                          << " lastIp=" << pcb->lastIp << "\n";
            pcb->stack[static_cast<size_t>(slot)].value = v;
        } else
            badCtrl();
    };
    if (c->type == 1 || c->type == -1) {
        int64_t current = c->current;
        c->current = current + c->type;
        setVar(make_int(current));
        bool past = c->done || (c->type > 0 && current > c->to) ||
                    (c->type < 0 && current < c->to);
        ip += past ? static_cast<size_t>(breakOffset)
                   : static_cast<size_t>(continueOffset);
        if (current == c->to) c->done = 1;
        if (past && !pcb->frames.empty()) pcb->frames.back().loops.erase(ctrlSlot);
    } else if (c->type == 901) {
        if (!c->arr) badCtrl();
        size_t iter = c->iter++;
        if (iter < c->arr->items.size()) {
            setVar(c->arr->items[iter]);
            ip += static_cast<size_t>(continueOffset);
        } else {
            if (!pcb->frames.empty()) pcb->frames.back().loops.erase(ctrlSlot);
            ip += static_cast<size_t>(breakOffset);
        }
    } else if (c->type == 902) {
        if (!c->map) badCtrl();
        // foreach over a mapping iterates in vm_mappingIndices order.
        const auto entries = c->map->items();
        if (c->mIdx >= entries.size()) {
            if (!pcb->frames.empty()) pcb->frames.back().loops.erase(ctrlSlot);
            ip += static_cast<size_t>(breakOffset);
        } else {
            const auto& kv = entries[c->mIdx++];
            auto arr = std::make_shared<ArrayValue>();
            arr->items = {kv.first, kv.second, make_int(c->mElementIndex++)};
            setVar(arr);
            ip += static_cast<size_t>(continueOffset);
        }
    }
}

// ---- closure helpers ----------------------------------------------------------
static VmObject* closureOwner(const std::shared_ptr<ClosureValue>& cl) {
    if (cl && !cl->ownerKey.empty()) {
        if (VmObject* o = Registry::instance().findByName(cl->ownerKey))
            return o;
    }
    return masterObject();
}

// ---------------------------------------------------------------------------
// The interpreter loop.
//   run(pcb): until the boundary/context frame ends (chain end).
//   runNested(pcb, depth): until frames.size() returns to depth.
// ---------------------------------------------------------------------------
static void loop(VmPcb* pcb, bool nested, size_t depth) {
    // [loopsplit] loop() 级二分: 每次 run() 的 指令数/耗时 vs 调度间隔。
    // 若每条指令 12.5µs 但 op 本体平凡 → 时间在 run() 之间的调度往返。
    struct LS {
        const std::atomic<bool>& on;
        std::chrono::steady_clock::time_point t0;
        VmPcb* pcb;
        ~LS() {
            if (!on.load(std::memory_order_relaxed)) return;
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::steady_clock::now() - t0)
                          .count();
            static std::mutex mx;
            static uint64_t calls = 0, inNs = 0, insns = 0;
            static std::chrono::steady_clock::time_point lastDump =
                std::chrono::steady_clock::now();
            static std::chrono::steady_clock::time_point firstT =
                std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lk(mx);
            calls++; inNs += static_cast<uint64_t>(ns);
            insns += pcb->instrCount;
            auto since = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - lastDump)
                             .count();
            if (since >= 5000) {
                auto wallMs = std::chrono::duration_cast<
                                  std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - firstT)
                                  .count();
                std::cerr << "[loopsplit] calls=" << calls
                          << " insns=" << insns
                          << " inLoopMs=" << inNs / 1000000
                          << " wallMs=" << wallMs
                          << " insn/call="
                          << (calls ? insns / calls : 0)
                          << " rate="
                          << (wallMs ? static_cast<uint64_t>(insns) *
                                          1000 / wallMs
                                     : 0)
                          << "/s\n";
                std::cerr.flush();
                lastDump = std::chrono::steady_clock::now();
                calls = 0; inNs = 0; insns = 0;
                firstT = std::chrono::steady_clock::now();
            }
        }
    } lsGuard{dynProbeFlag("LPCVM_LOOP2"), std::chrono::steady_clock::now(),
              pcb};
    pcb->instrCount = 0;
    while (pcb->code && pcb->ip < pcb->code->size()) {        const OriginalInsn& in = (*pcb->code)[pcb->ip++];
        ++pcb->instrCount;
        pcb->lastOp = in.op;
        pcb->lastIp = pcb->ip - 1;
        // [opns] 按 op 的执行耗时直方图 (touch _probe_on/LPCVM_OPNS 热布防)。
        // 上一条指令的耗时归因给它自己; 每 4M 条已计指令 dump top10 +
        // 总速率。 Armed 时每指令一次 chrono(~25ns)。用于定位"每指令
        // ~2.3µs、比原版慢 3 个量级"的固定开销落在哪类 op。
        {
            static const std::atomic<bool>& opnsOn =
                lpc::dynProbeFlag("LPCVM_OPNS");
            if (opnsOn.load(std::memory_order_relaxed)) {
                using clk = std::chrono::steady_clock;
                static thread_local clk::time_point opnsT0{};
                static thread_local int opnsLast = -1;
                static std::atomic<uint64_t> opnsNs[96];
                static std::atomic<uint64_t> opnsCnt[96];
                static std::atomic<uint64_t> opnsTotal{0};
                auto now = clk::now();
                if (opnsLast >= 0) {
                    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  now - opnsT0)
                                  .count();
                    opnsNs[opnsLast].fetch_add(
                        static_cast<uint64_t>(ns), std::memory_order_relaxed);
                    auto c = opnsCnt[opnsLast].fetch_add(
                        1, std::memory_order_relaxed);
                    if (opnsTotal.fetch_add(1) % 4000000 == 0) {
                        uint64_t totNs = 0;
                        for (int i = 0; i < 96; ++i)
                            totNs += opnsNs[i].load(std::memory_order_relaxed);
                        std::cerr << "[opns] dump totalOps="
                                  << opnsTotal.load() << " accountedMs="
                                  << totNs / 1000000 << "\n";
                        for (int r = 0; r < 8; ++r) {
                            int best = -1;
                            uint64_t bestNs = 0;
                            for (int i = 0; i < 96; ++i)
                                if (opnsNs[i].load(
                                        std::memory_order_relaxed) > bestNs) {
                                    bestNs = opnsNs[i].load(
                                        std::memory_order_relaxed);
                                    best = i;
                                }
                            if (best < 0 || bestNs == 0) break;
                            opnsNs[best].store(0, std::memory_order_relaxed);
                            std::cerr << "[opns] op" << best << " ("
                                      << actionInfo(best).name
                                      << ") cnt="
                                      << opnsCnt[best].load(
                                             std::memory_order_relaxed)
                                      << " totalMs=" << bestNs / 1000000
                                      << " avgNs="
                                      << (opnsCnt[best].load(
                                              std::memory_order_relaxed)
                                              ? bestNs / opnsCnt[best].load(
                                                    std::memory_order_relaxed)
                                              : 0)
                                      << "\n";
                        }
                        std::cerr.flush();
                    }
                }
                opnsT0 = now;
                opnsLast = static_cast<int>(in.op);
            }
        }
        opTraceRecord(pcb->lastIp, in.op, pcb->stack.size(), pcb->frames.size());
        // ---- time slice (master process; 原版 runTicks) ----
        if (pcb->yieldEvery > 0 && --pcb->yieldCountdown <= 0) {
            pcb->yieldCountdown = pcb->yieldEvery;
            // NESTED loops (vmApplySync/runNested under an efun callback)
            // must NOT throw TimeSliceSignal: the exception would unwind the
            // C++ continuation that owns this nested call (popValueNested /
            // the retHook that finishes the ConstructPlan) — the frames
            // resurface on the chain but the plan's return path is GONE
            // (plans froze at steps=N/N with finished=false and the master
            // boot chain "ended normally", orphaning the boot three runs in
            // a row).  The original's runTicks yields only at the outermost
            // instruction boundary; defer nested slices to the outer loop.
            if (!nested) {
                // 原版 runTicks 在指令完整执行后的边界让出。这里取指已完成
                // (pcb->ip 已 +1) 但指令尚未执行 —— 直接抛出会把取指点上这
                // 条指令整条跳过：convert_to_value 的 [1] enter_function 被
                // 跳掉，5 个局部槽未压，[66] op57 -4 → "No such argument."
                // → 整条 import 链中止 → read_etc 60s 重试不收敛。回退 ip
                // 让下个切片重取并执行同一条指令。
                pcb->ip = pcb->lastIp;
                throw TimeSliceSignal{};
            }
        }
        const ActionInfo& ai = actionInfo(in.op);
        int32_t m0 = 0, m1 = 0;
        const bool fromMeta = (ai.attrib & 0x80000000u) != 0;
        if (fromMeta && pcb->meta) {
            const uint32_t off = static_cast<uint32_t>(in.a) |
                                 (static_cast<uint32_t>(in.b) << 16);
            m0 = readI32(*pcb->meta, off);
            if (ai.parameters == 2) m1 = readI32(*pcb->meta, off + 4);
        }
        trcStep(pcb, pcb->ip - 1, in.op, ai.name);
        // [hotsample] 时间基采样 profiler: 每 64K 条指令看一次钟, 距上次
        // 采样 >=150ms 才打一行 (fn/ip/op)。指令基采样会漏掉"单条 op6 efun
        // 内部烧 10 秒 CPU"的现场 —— 时间基才能如实反映耗时分布。默认关
        // (touch _probe_on/LPCVM_HOTLOOP 热布防)。
        {
            static thread_local uint64_t hotTick = 0;
            if (((++hotTick) & 0xFFFFull) == 0) {
                static const std::atomic<bool>& hotOn =
                    lpc::dynProbeFlag("LPCVM_HOTLOOP");
                if (hotOn.load(std::memory_order_relaxed)) {
                    using clk = std::chrono::steady_clock;
                    static thread_local clk::time_point last =
                        clk::now() - std::chrono::milliseconds(1000);
                    auto now = clk::now();
                    if (now - last >= std::chrono::milliseconds(150)) {
                        last = now;
                        static std::atomic<int> hotLog{0};
                        if (hotLog.fetch_add(1) < 3000 &&
                            !pcb->frames.empty()) {
                            std::string extra;
                            if (in.op == 6) {   // call_in_efun: 解析 efun 名
                                const int efun =
                                    fromMeta ? m0 : static_cast<int16_t>(in.a);
                                const std::vector<std::string>& et =
                                    efunNameTable();
                                if (efun >= 0 &&
                                    static_cast<size_t>(efun) < et.size())
                                    extra =
                                        " efun=" + et[static_cast<size_t>(efun)];
                            }
                            std::cerr << "[hotsample] tid=" << pcb->tid
                                      << " fn="
                                      << (pcb->frames.back().fn
                                              ? pcb->frames.back().fn->name
                                              : "<ctx>")
                                      << " ip=" << (pcb->ip - 1)
                                      << " op=" << static_cast<int>(in.op)
                                      << extra
                                      << " depth=" << pcb->stack.size()
                                      << " ic=" << hotTick
                                      << "\n";
                            std::cerr.flush();
                        }
                    }
                }
            }
        }
        // 环境开关一律启动时缓存 —— 热路径每条指令调 getenv 会线性扫描
        // 整个环境块并加锁，曾把 gs 启动拖到 11 对象/秒 (单核 100%)。
        static const bool g_setupTrace = getenv("LPCVM_SETUPTRACE") != nullptr;
        static const char* const g_fnFilter = getenv("LPCVM_FNFILTER");
        // 调用链上任一帧命中过滤即 trace（否则被调函数体不可见）；
        // fn= 打印栈顶帧名以便区分 caller/callee。
        bool fnHit = false;
        if (g_fnFilter && !pcb->frames.empty()) {
            for (const auto& fr : pcb->frames)
                if (fr.fn && fr.fn->name == g_fnFilter) { fnHit = true; break; }
        }
        // [pi-watch] post_init 4 槽影子: tid=0 主链的 post_init 栈区就
        // [0..3] 四槽 ([3]=ctrl)。op35 布防, 之后每条指令比对 index,
        // 变了立即打印当前指令 —— 必中写手。
        {
            // [性能] 原 g_loopwatchCold 恒 false(env 永未设置) → 每条指令
            // getenv(线性扫描环境块+锁 ≈12µs) + lwOn.store —— 采样器实测
            // VM 线程 100% 时间在 getenv, 指令速率 7.5 万/秒。改 dynProbeFlag
            // (_probe_on/LPCVM_LOOPWATCH 标记热布防), 热路径只剩一次 relaxed load。
            static const std::atomic<bool>& lwFlag =
                lpc::dynProbeFlag("LPCVM_LOOPWATCH");
            const bool lwOn = lwFlag.load(std::memory_order_relaxed);
            static unsigned char piShadow = 0;      // 只盯 ctrl 槽 (bp+3)
            static const VmPcb* piPcb = nullptr;
            static int piBp = 0;
            static bool piPending = false;   // op35 已取指, 下一条再录影子
            static std::atomic<int> piHits{0};
            if (lwOn && piHits.load() < 4) {
                // 激活条件: 该 pcb 帧链上有 post_init (callee 执行期间照样盯)
                const VmFrame* pif = nullptr;
                for (const auto& fr : pcb->frames)
                    if (fr.fn && fr.fn->name == "post_init") { pif = &fr; break; }
                if (pif) {
                    if (piPcb != pcb) { piPcb = pcb; piBp = pif->bp; piPending = false; }
                    if (in.op == 35 && pcb->frames.back().fn &&
                        pcb->frames.back().fn->name == "post_init") {
                        piPending = true;   // ctrl 在本条 switch 里写入
                    } else if (piPending) {
                        piPending = false;
                        size_t sl = static_cast<size_t>(piBp + 3);
                        piShadow = sl < pcb->stack.size()
                            ? static_cast<unsigned char>(pcb->stack[sl].value.index())
                            : 99;
                    } else {
                        size_t sl = static_cast<size_t>(piBp + 3);
                        unsigned char now = sl < pcb->stack.size()
                            ? static_cast<unsigned char>(pcb->stack[sl].value.index())
                            : 99;
                        if (now != piShadow && piHits.fetch_add(1) < 4) {
                            std::cerr << "[pi-watch] slot3 idx "
                                      << static_cast<int>(piShadow) << "->"
                                      << static_cast<int>(now)
                                      << " at ip=" << (pcb->ip - 1)
                                      << " op=" << static_cast<int>(in.op)
                                      << " fn="
                                      << (pcb->frames.back().fn
                                              ? pcb->frames.back().fn->name
                                              : "<ctx>")
                                      << " frames=" << pcb->frames.size()
                                      << " stack=" << pcb->stack.size()
                                      << " tid=" << pcb->tid << "\n";
                            piShadow = now;
                        }
                    }
                } else {
                    piPcb = nullptr;
                }
            }
            // [cst-watch] create_search_terms 的循环 ctrl 槽 (bp+9) 影子:
            // "Bad loop ctrl" 槽在栈内但内容被覆盖 (非越界), 盯写手。
            // 帧链上任一 create_search_terms 都盯 —— callee 执行期间也比对
            // (写手可能经由 by-ref 参数/错位 bp 穿过引用写进 ctrl 槽)。
            {
                static unsigned char cstShadow = 0;
                static int cstSlot = -1;
                static bool cstPending = false;
                static int cstHits = 0;
                if (lwOn && cstHits < 8) {
                    const VmFrame* ctf = nullptr;
                    for (const auto& fr : pcb->frames)
                        if (fr.fn && fr.fn->name == "create_search_terms") {
                            ctf = &fr;
                            break;
                        }
                    if (ctf) {
                        if (cstSlot != ctf->bp + 9) {
                            cstSlot = ctf->bp + 9;
                            cstPending = false;
                        }
                        if (in.op == 35 && pcb->frames.back().fn &&
                            pcb->frames.back().fn->name ==
                                "create_search_terms") {
                            cstPending = true;   // ctrl 在本条写入
                        } else if (cstPending) {
                            cstPending = false;
                            size_t sl = static_cast<size_t>(cstSlot);
                            cstShadow = sl < pcb->stack.size()
                                ? static_cast<unsigned char>(
                                      pcb->stack[sl].value.index())
                                : 99;
                        } else {
                            size_t sl = static_cast<size_t>(cstSlot);
                            unsigned char now = sl < pcb->stack.size()
                                ? static_cast<unsigned char>(
                                      pcb->stack[sl].value.index())
                                : 99;
                            if (now != cstShadow) {
                                // 只报 "LoopCtrl 被覆盖" (9->非9): 进出函数的
                                // 99(越界)/0(重入 undefined) 是正常波动,
                                // 不占配额 —— 否则真写手出现时配额已耗尽。
                                if (cstShadow == 9 && now != 9) {
                                    ++cstHits;
                                    std::cerr << "[cst-watch] slot" << cstSlot
                                              << " idx "
                                              << static_cast<int>(cstShadow)
                                              << "->" << static_cast<int>(now)
                                              << " at ip=" << (pcb->ip - 1)
                                              << " op=" << static_cast<int>(in.op)
                                              << " fn="
                                              << (pcb->frames.back().fn
                                                      ? pcb->frames.back().fn->name
                                                      : "<ctx>")
                                              << " depth=" << pcb->stack.size()
                                              << " tid=" << pcb->tid << "\n";
                                }
                                cstShadow = now;
                            }
                        }
                    } else {
                        cstSlot = -1;
                    }
                }
            }
        }
        if (g_setupTrace && !pcb->frames.empty() &&
            pcb->frames.back().fn && pcb->frames.back().fn->name == "setup")
            std::cerr << "[setup-trace] ip=" << (pcb->ip - 1)
                      << " op=" << static_cast<int>(in.op)
                      << " frames=" << pcb->frames.size() << "\n";
        if (fnHit && !pcb->frames.empty()) {
            size_t s3 = pcb->frames.back().bp + 3;
            unsigned char v3 =
                s3 < pcb->stack.size()
                    ? static_cast<unsigned char>(pcb->stack[s3].value.index())
                    : 99;
            std::cerr << "[fn-trace] fn="
                      << (pcb->frames.back().fn ? pcb->frames.back().fn->name
                                                : std::string("<ctx>"))
                      << " ip=" << (pcb->ip - 1)
                      << " op=" << static_cast<int>(in.op)
                      << " stack=" << pcb->stack.size()
                      << " bp=" << pcb->frames.back().bp
                      << " s3=" << static_cast<int>(v3) << "\n";
        }
        try {
            switch (in.op) {
            case 46: // null_action
                break;
            case 29: { // finish_action
                // finish_action = implicit end-of-function ("return 0"):
                // pop THIS frame and resume the caller — identical to op 73
                // without a popped value.  The old `if (pcb->returned) return`
                // exited the WHOLE loop because `returned` is a chain-level
                // flag that vmReturnFromFun sets on EVERY frame return: after
                // any callee's explicit return, the next frame to fall off
                // its end via op 29 killed the loop with every remaining
                // frame still on the chain — the master boot chain was then
                // marked Done "normally" and the gs froze silently (three
                // runs in a row: xianmolu scripts, packed_items loads,
                // bangpai-mingrentang setup_npc; always frames intact, no
                // error, no log).
                const int rc = vmReturnFromFun(pcb, 29);
                if (rc == Interp::RET_CHAIN_END) {
                    if (nested && pcb->frames.size() > depth)
                        break;   // nested guard: keep draining to depth
                    return;
                }
                break;
            }
            case 72: // return_from_fake_fun (closure return: value always popped)
            case 73: { // return_from_fun
                int rc = vmReturnFromFun(pcb, in.op);
                if (rc == RET_CHAIN_END) {
                    if (nested && pcb->frames.size() > depth)
                        break;   // nested guard: keep draining to depth
                    return;
                }
                break;
            }
            // ---- pushes ----
            case 59: // push_number(S)
                pushValue(pcb, make_int(static_cast<int16_t>(in.a)));
                break;
            case 60: { // push_number(L)
                int32_t v = m0;
                if (!fromMeta) {
                    v = pcb->meta
                        ? readI32(*pcb->meta,
                                  static_cast<uint32_t>(in.a) |
                                  (static_cast<uint32_t>(in.b) << 16))
                        : 0;
                }
                pushValue(pcb, make_int(v));
                break;
            }
            case 63: { // push_real_from_word (float bits in the metadata word)
                float bits = 0.0f;
                std::memcpy(&bits, &m0, 4);
                pushValue(pcb, make_float(bits));
                break;
            }
            case 64: { // push_shared_string (frame's program table)
                const std::vector<std::string>* tbl = curStrings(pcb);
                if (tbl && in.a < tbl->size())
                    pushValue(pcb, make_string((*tbl)[in.a]));
                else
                    pushValue(pcb, make_int(0));
                break;
            }
            case 66: // push_undefined (original type 0)
                pushValue(pcb, Value{});
                break;
            case 65: { // push_this_object (current frame's instance)
                pushValue(pcb, Registry::makeObjectValue(curObj(pcb)));
                break;
            }
            case 52: // push_arg_num
                pushValue(pcb, make_int(pcb->frames.empty()
                                            ? 0 : pcb->frames.back().argNum));
                break;
            case 53: // push_global_var
                pushValue(pcb, sharedGlobal(in.a));
                break;
            case 61: { // push_obj_var
                const int vi = static_cast<int>(in.a) + curVarOffset(pcb);
                VmObject* o = curObj(pcb);
                if (o && static_cast<size_t>(vi) < o->vars().size())
                    pushValue(pcb, o->vars()[static_cast<size_t>(vi)]);
                else
                    pushValue(pcb, make_int(0));
                break;
            }
            case 54: // push_global_vptr
                pushItem(pcb, makeGlobalRefItem(in.a));
                break;
            case 62: // push_obj_vptr
                pushItem(pcb, makeObjRefItem(curObj(pcb),
                                             static_cast<int>(in.a) + curVarOffset(pcb)));
                break;
            case 55: // push_local_value
            case 56: { // push_local_var_i (index popped from stack)
                // [op55t] 解剖 op55: 体前/体后计时, 定位 12.5µs/条 落在
                // case 体 (slotValue+pushValue) 还是循环机构。
                static const std::atomic<bool>& o55 =
                    lpc::dynProbeFlag("LPCVM_LOOP2");
                auto o55t0 = o55.load(std::memory_order_relaxed)
                                 ? std::chrono::steady_clock::now()
                                 : std::chrono::steady_clock::time_point{};
                int idx = static_cast<int16_t>(in.a);
                if (in.op == 56) idx = static_cast<int>(toInt(popValue(pcb)));
                const int slot = slotOf(pcb, idx);
                bool ok = slot >= 0 &&
                          static_cast<size_t>(slot) < pcb->stack.size();
                if (ok && idx >= 0)
                    ok = !pcb->frames.empty() &&
                         idx < pcb->frames.back().argNum;
                // Missing/late arg: the original answers the out-of-range
                // param read with an UNDEFINED value (type 0), not int 0 —
                // mudlib code like time_to_string() { is_int(t) ? t : time() }
                // called with no args relies on is_int(undefined)==0 to fall
                // back to time(); pushing int 0 made is_int(0)==1 and the
                // caller saw 19700101080000 (aaa account UPDATE wrote epoch-0
                // last_login_time, affected=0, and every mall purchase then
                // failed with 余额不足).
                Value o55v =
                    (ok ? slotValue(pcb, slot) : Value{});
                auto o55t1 = o55.load(std::memory_order_relaxed)
                                 ? std::chrono::steady_clock::now()
                                 : std::chrono::steady_clock::time_point{};
                pushValue(pcb, std::move(o55v));
                if (o55.load(std::memory_order_relaxed)) {
                    static std::atomic<uint64_t> aNs{0}, bNs{0}, n{0};
                    aNs.fetch_add(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            o55t1 - o55t0)
                            .count());
                    bNs.fetch_add(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now() - o55t1)
                            .count());
                    if (n.fetch_add(1) % 100000 == 0)
                        std::cerr << "[op55t] n=" << n.load()
                                  << " preNs=" << aNs.load() / 1000
                                  << " pushNs=" << bNs.load() / 1000 << "\n";
                }
                break;
            }
            case 57: // push_local_vptr
            case 58: { // push_local_vptr_i
                int idx = static_cast<int16_t>(in.a);
                if (in.op == 58) idx = static_cast<int>(toInt(popValue(pcb)));
                const int slot = slotOf(pcb, idx);
                // 原版 vm_pushLocalVptr：参数越界 / 槽越界一律
                // vm_error("No such argument.")（可被 LPC catch 捕获）。
                auto noSuchArg = [&]() {
                    if (g_opTraceCount) {
                        std::cerr << "[op-trace] last " << g_opTraceCount
                                  << " insns (ip op stack frames):\n";
                        for (size_t i = 0; i < g_opTraceCount; ++i) {
                            const OpTraceEntry& e = g_opTrace[
                                (g_opTracePos + kOpTraceN - g_opTraceCount + i) % kOpTraceN];
                            std::cerr << "  " << e.ip << " op=" << static_cast<int>(e.op)
                                      << " stack=" << e.size << " fr=" << e.frames << "\n";
                        }
                        std::cerr.flush();
                    }
                    std::cerr << "[no-such-arg] op=" << static_cast<int>(in.op)
                              << " idx=" << idx << " slot=" << slot
                              << " stack=" << pcb->stack.size()
                              << " bp=" << (pcb->frames.empty() ? -1 : pcb->frames.back().bp)
                              << " argc=" << (pcb->frames.empty() ? -1
                                              : pcb->frames.back().argNum)
                              << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                                ? std::string("<ctx>")
                                                : pcb->frames.back().fn->name)
                              << " fnLocals=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                                      ? -1
                                                      : pcb->frames.back().fn->localCount)
                              << " fnNodes=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                                     ? -1
                                                     : static_cast<int>(pcb->frames.back().fn->code.size()))
                              << " runNodes=" << (pcb->code ? static_cast<int>(pcb->code->size()) : -1) << "\n";
                    std::cerr.flush();
                    vmError("No such argument.");
                };
                if (idx >= 0 &&
                    (pcb->frames.empty() ||
                     idx >= pcb->frames.back().argNum))
                    noSuchArg();
                if (slot < 0 || static_cast<size_t>(slot) >= pcb->stack.size())
                    noSuchArg();
                pushItem(pcb, makeSlotRef(slot));
                break;
            }
            // ---- assignments ----
            case 3: { // assign_value: pop ref, keep value on stack
                StackItem ref = popItem(pcb);
                if (!pcb->stack.empty())
                    storeItem(pcb, ref, pcb->stack.back().value);
                break;
            }
            case 4: { // assign_value_reverse: pop value, keep ref
                StackItem val = popItem(pcb);
                if (!pcb->stack.empty())
                    storeItem(pcb, pcb->stack.back(), val.value);
                break;
            }
            case 76: // set_global_var
                sharedGlobal(in.a) = popValue(pcb);
                break;
            case 78: { // set_obj_var
                const int vi = static_cast<int>(in.a) + curVarOffset(pcb);
                Value sv = popValue(pcb);
                VmObject* o = curObj(pcb);
                if (o && static_cast<size_t>(vi) < o->vars().size())
                    o->vars()[static_cast<size_t>(vi)] = sv;
                // OOB write dropped (原版的数组恒为 maxVariableNo 槽, 越界
                // 即活动数组不是本对象的 — 不扩容不落盘)
                break;
            }
            case 77: { // set_local_var
                if (!pcb->stack.empty()) {
                    const int slot = slotOf(pcb, static_cast<int16_t>(in.a));
                    if (slot >= 0 && static_cast<size_t>(slot) < pcb->stack.size()) {
                        StackItem& s = pcb->stack[static_cast<size_t>(slot)];
                        Value sv = popValue(pcb);
                        static const std::atomic<bool>& slotSnag3 =
                            lpc::dynProbeFlag("LPCVM_SLOTSNAG");
                        if (slotSnag3.load(std::memory_order_relaxed) &&
                            s.value.index() == 9 && sv.index() != 9)
                            std::cerr << "[loop-slot-kill] op77 slot=" << slot
                                      << " ip=" << (pcb->ip - 1) << "\n";
                        // 原版 vm_setLocalVar (0x808f6dc): 目标槽是引用
                        // (type 8 / vptr) 时把值写穿到 vptr 指向处 ——
                        // call_other 转发的引用参数 (precost_coin 的
                        // result = ([...])) 靠这条落回调用方栈槽。
                        if (s.isRef)
                            storeItem(pcb, s, sv);
                        else
                            s.value = sv;
                    } else {
                        popValue(pcb);
                    }
                }
                break;
            }
            case 51: // pop_stack
                if (!pcb->stack.empty()) popItem(pcb);
                break;
            case 50: // pop_n_stack
                for (int i = 0; i < in.a && !pcb->stack.empty(); ++i) popItem(pcb);
                break;
            case 34: { // inc_value
                if (!pcb->stack.empty()) {
                    StackItem ref = popItem(pcb);
                    if (ref.isRef) {
                        Value cur = derefItem(pcb, ref);
                        Value nv = make_int(toInt(cur) + static_cast<int16_t>(in.a));
                        storeItem(pcb, ref, nv);
                    }
                }
                break;
            }
            // ---- containers ----
            case 22: { // create_array: elements popped in order (element 0 top)
                const uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !pcb->stack.empty(); ++i)
                    items.push_back(popValue(pcb));
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                pushValue(pcb, arr);
                break;
            }
            case 24: { // create_mapping: (value,key) pairs, reversed back
                const uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !pcb->stack.empty(); ++i)
                    items.push_back(popValue(pcb));
                std::reverse(items.begin(), items.end());
                auto mp = std::make_shared<MappingValue>(
                    static_cast<uint32_t>(items.size() / 2));
                for (size_t i = 0; i + 1 < items.size(); i += 2)
                    mp->insert(items[i + 1], items[i]);
                pushValue(pcb, mp);
                break;
            }
            case 21: { // create_argument_array: the frame's args as array
                std::vector<Value> items;
                if (!pcb->frames.empty()) {
                    const VmFrame& f = pcb->frames.back();
                    for (int i = 0; i < f.argNum; ++i)
                        items.push_back(slotValue(pcb, f.bp - i));
                }
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                pushValue(pcb, arr);
                break;
            }
            case 23: { // create_function_value (bound count = b & 0x3F)
                const int boundCount = in.b & 0x3F;
                const bool global = (in.b & 0x80) != 0;
                const OriginalFunction* fn = nullptr;
                bool fromMaster = false;   // fn resolved from the GLOBAL program
                if (global) {
                    // bit7: the GLOBAL program's table, index verbatim.
                    fn = findMasterFunction(in.a);
                    fromMaster = (fn != nullptr);
                    if (!fn) {
                        // Global closure indices may name builtin EFUN slots
                        // (vm_pGlobalProgram->pFunctions includes them) —
                        // e.g. (: create_thread, ... :) in ccs_clientd create.
                        const auto& names = efunNameTable();
                        if (in.a >= 0 && static_cast<size_t>(in.a) < names.size() &&
                            !names[static_cast<size_t>(in.a)].empty()) {
                            auto cl = std::make_shared<ClosureValue>();
                            cl->functionName = names[static_cast<size_t>(in.a)];
                            cl->funcIndex = in.a;
                            cl->efunClosure = true;
                            cl->owner = nullptr;
                            std::vector<Value> bound;
                            for (int i = 0; i < boundCount && !pcb->stack.empty(); ++i)
                                bound.push_back(popValue(pcb));
                            cl->boundArgs = std::move(bound);
                            pushValue(pcb, cl);
                            break;
                        }
                    }
                } else {
                    // The index is LOCAL to the current function's constitution
                    // (an inherited body emits the PARENT's local index) —
                    // accumulated baseFunOffset applies, exactly as op 7.
                    const int loff = curFunOffset(pcb);
                    const VmProgram* p = curProg(pcb);
                    if (p) {
                        if (loff != 0) fn = p->fnAtIndex(in.a + loff);
                        if (!fn) fn = p->fnAtIndex(in.a);
                    }
                }
                if (fn) {
                    std::vector<Value> bound;
                    for (int i = 0; i < boundCount && !pcb->stack.empty(); ++i)
                        bound.push_back(popValue(pcb));
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = fn->name;
                    cl->funcIndex = in.a;
                    cl->func = fn;          // exact function: invoke directly
                    cl->globalFn = fromMaster;   // body lives in the GLOBAL program
                    cl->boundArgs = bound;
                    cl->owner = nullptr;    // ownerKey pins the instance
                    cl->ownerKey = curObj(pcb) ? curObj(pcb)->key() : std::string();
                    pushValue(pcb, cl);
                } else {
                    // [newfunc0] closure creation FAILED -> silent int 0; a
                    // set_timer(callback=int 0) then never registers and the
                    // gather chest never destructs.  Hot probe to spot it.
                    static const std::atomic<bool>& nfFlag =
                        lpc::dynProbeFlag("LPCVM_NEWFUNC0");
                    if (nfFlag.load(std::memory_order_relaxed))
                        std::cerr << "[newfunc0] op23 index=" << in.a
                                  << " b=" << (int)in.b << " prog="
                                  << (curProg(pcb) ? curProg(pcb)->src().sourceName : std::string("(null)"))
                                  << " obj=" << (curObj(pcb) ? curObj(pcb)->key() : std::string("(null)"))
                                  << " ip=" << (pcb->ip - 1) << "\n";
                    pushValue(pcb, Value{});   // failed closure: void (原版 pushVoid 语义)
                }
                break;
            }
            // ---- arithmetic / comparisons (原版 32 位环绕/float32 语义) ----
            case 1: { Value b = popValue(pcb); Value a = popValue(pcb);
                      pushValue(pcb, addValues(a, b)); break; }
            case 81:
            case 25: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, in.op == 81 ? subValues(a, b) : divValues(a, b)); break; }
            case 41: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, mulValues(a, b)); break; }
            case 40: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, modValues(a, b)); break; }
            case 38: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, lshValues(a, b)); break; }
            case 75: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, rshValues(a, b)); break; }
            case 2: { Value b = popValue(pcb); Value a = popValue(pcb);
                      pushValue(pcb, andValues(a, b)); break; }
            case 48: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, lpcOrValues(a, b)); break; }
            case 85: { Value b = popValue(pcb); Value a = popValue(pcb);
                       pushValue(pcb, xorValues(a, b)); break; }
            case 43: pushValue(pcb, negValue(popValue(pcb))); break;
            case 28: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = value_equal(a, b) ? 1 : 0;
                       TRC_CMP("eq", 28, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 42: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = value_equal(a, b) ? 0 : 1;
                       TRC_CMP("ne", 42, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 30: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = compareValues(a, b) >= 0 ? 1 : 0;
                       TRC_CMP("ge", 30, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 32: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = compareValues(a, b) > 0 ? 1 : 0;
                       TRC_CMP("gt", 32, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 37: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = compareValues(a, b) <= 0 ? 1 : 0;
                       TRC_CMP("le", 37, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 39: { Value b = popValue(pcb); Value a = popValue(pcb);
                       const int64_t r = compareValues(a, b) < 0 ? 1 : 0;
                       TRC_CMP("lt", 39, a, b, r);
                       pushValue(pcb, make_int(r)); break; }
            case 45: // not_value
                pushValue(pcb, make_int(isZero(popValue(pcb)) ? 1 : 0));
                break;
            case 47: { // op_eq_value (compound assignment)
                const int op = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value rhs = popValue(pcb);
                StackItem ref = popItem(pcb);
                // string += fast path: append in place when the LHS block has
                // exactly ONE owner (mirrors the original's refcount==1 append);
                // any alias falls back to the copy path.
                if (op == 2 && rhs.index() == 4) {
                    const StackItem* tgt = followRefChain(pcb, ref);
                    if (tgt && tgt->slotIndex >= 0 &&
                        static_cast<size_t>(tgt->slotIndex) < pcb->stack.size()) {
                        StackItem& slot =
                            pcb->stack[static_cast<size_t>(tgt->slotIndex)];
                        if (!slot.isRef && slot.value.index() == 4) {
                            SharedString& ts = std::get<SharedString>(slot.value);
                            if (ts.size() >= 4096 && ts.blockRefs() == 1) {
                                const SharedString& rs = std::get<SharedString>(rhs);
                                std::string more = rs.str();
                                ts.appendInPlace(more);
                                pushValue(pcb, slot.value);
                                break;
                            }
                        }
                    }
                }
                Value target = derefItem(pcb, ref);
                Value result = make_int(0);
                switch (op) {
                case 2: {
                    // array += array: append in place when the LHS array is
                    // uniquely owned (the plain path deep-copies per append).
                    if (target.index() == 5 && rhs.index() == 5) {
                        auto trg = std::get<std::shared_ptr<ArrayValue>>(target);
                        auto rv = std::get<std::shared_ptr<ArrayValue>>(rhs);
                        if (trg.use_count() <= 2) {
                            trg->items.insert(trg->items.end(),
                                              rv->items.begin(), rv->items.end());
                            storeItem(pcb, ref, Value(trg));
                            pushValue(pcb, Value(trg));
                            break;
                        }
                    }
                    result = addValues(target, rhs);
                    break;
                }
                case 3: result = subValues(target, rhs); break;
                case 4: result = mulValues(target, rhs); break;
                case 5: result = divValues(target, rhs); break;
                case 6: result = andValues(target, rhs); break;
                case 7: result = lpcOrValues(target, rhs); break;
                case 8: result = xorValues(target, rhs); break;
                case 9: result = rshValues(target, rhs); break;
                case 10: result = lshValues(target, rhs); break;
                case 11: result = modValues(target, rhs); break;
                default: break;
                }
                storeItem(pcb, ref, result);
                pushValue(pcb, result);
                break;
            }
            case 79: { // sizeof: string bytes / buffer bytes / array/mapping count
                Value v = popValue(pcb);
                switch (v.index()) {
                case 4:
                    pushValue(pcb, make_int(static_cast<int64_t>(
                                      std::get<SharedString>(v).size())));
                    break;
                case 5:
                    pushValue(pcb, make_int(static_cast<int64_t>(
                                      std::get<std::shared_ptr<ArrayValue>>(v)->items.size())));
                    break;
                case 6:
                    pushValue(pcb, make_int(static_cast<int64_t>(
                                      std::get<std::shared_ptr<MappingValue>>(v)->size())));
                    break;
                case 10:
                    pushValue(pcb, make_int(static_cast<int64_t>(
                                      std::get<std::shared_ptr<BufferValue>>(v)->data.size())));
                    break;
                default:
                    pushValue(pcb, make_int(0));
                    break;
                }
                break;
            }
            case 36: { // is_type
                const int t = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value v = pcb->stack.back().value;
                int got = vmTypeOf(v);
                // A really-freed object behaves as the int 0 (原版引用失效):
                // is_object() must report 0 for it.  alive==nullptr 的对象值
                // (匿名/未锚定, 如旧版 previous_object 的 make_object) 视为
                // 活 —— 与 builtins objectValueAlive 语义一致 (lw62 实锤:
                // 两处语义相反曾致 punishd 永久重试)。
                if (got == t && t == 10 && v.index() == 7) {
                    auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
                    if (!ov || (ov->alive && !ov->alive->load())) got = -1;
                }
                if (vmcore::traceOn() && t == 7 && got != 7) {
                    std::cerr << "[ismp-miss] got=" << got
                              << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                               ? std::string("<ctx>")
                                               : pcb->frames.back().fn->name)
                              << " val=" << to_string(v, false).substr(0, 60)
                              << " obj="
                              << (pcb->frames.empty() || !pcb->frames.back().obj
                                      ? std::string("<null>")
                                      : pcb->frames.back().obj->key())
                              << "\n";
                }
                pcb->stack.back().value = make_int(got == t ? 1 : 0);
                break;
            }
            case 13: { // cast_type
                const int t = fromMeta ? m0 : static_cast<int16_t>(in.a);
                pcb->stack.back().value = castValue(pcb->stack.back().value, t);
                break;
            }
            case 20: { // convert_to_value (dereference the top ref)
                StackItem ref = pcb->stack.back();
                if (ref.isRef) {
                    Value v = derefItem(pcb, ref);
                    pcb->stack.back() = makeItemV(v);
                }
                break;
            }
            case 67: { // query_member (bit0 = index from the end)
                const int mflag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value key = popValue(pcb);
                StackItem c = popItem(pcb);
                Value container = c.isRef ? derefItem(pcb, c) : c.value;
                if (mflag & 1) {
                    size_t len = 0;
                    if (container.index() == 4)
                        len = std::get<SharedString>(container).size();
                    else if (container.index() == 5)
                        len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
                    else if (container.index() == 10)
                        len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
                    key = make_int(static_cast<int64_t>(len) - toInt(key));
                }
                Value got = queryMemberValue(container, key);
                if (vmcore::traceOn()) {
                    const std::string trc_f = trcFnName(pcb);
                    if (vmcore::traceWant(trc_f)) {
                        std::cerr << "[qm] "
                                  << (got.index() == 0 ? "miss" : "hit")
                                  << " ctype=" << container.index()
                                  << " key=" << to_string(key, false).substr(0, 30)
                                  << " got=" << to_string(got, false).substr(0, 40)
                                  << " fn=" << trc_f
                                  << " obj="
                                  << (pcb->frames.empty() || !pcb->frames.back().obj
                                          ? std::string("<null>")
                                          : pcb->frames.back().obj->key())
                                  << "\n";
                    }
                }
                pushValue(pcb, std::move(got));
                break;
            }
            case 68: { // query_member_ptr (member ref for assignment)
                const int mflag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value key = popValue(pcb);
                StackItem c = popItem(pcb);
                Value container = c.isRef ? derefItem(pcb, c) : c.value;
                if (mflag & 1) {
                    size_t len = 0;
                    if (container.index() == 4)
                        len = std::get<SharedString>(container).size();
                    else if (container.index() == 5)
                        len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
                    else if (container.index() == 10)
                        len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
                    key = make_int(static_cast<int64_t>(len) - toInt(key));
                }
                {
                    if (vmcore::traceOn() && container.index() != 5 &&
                        container.index() != 6) {
                        std::cerr << "[mptr-bad] ctype=" << container.index()
                                  << " keyT=" << key.index()
                                  << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                                   ? std::string("<ctx>")
                                                   : pcb->frames.back().fn->name)
                                  << " obj="
                                  << (pcb->frames.empty() || !pcb->frames.back().obj
                                          ? std::string("<null>")
                                          : pcb->frames.back().obj->key())
                                  << "\n";
                    }
                }
                pushItem(pcb, makeMemberRefItem(container, key));
                break;
            }
            case 69: { // query_range
                const int flag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value to = popValue(pcb);
                Value from = popValue(pcb);
                Value container = popValue(pcb);
                pushValue(pcb, queryRangeValue(container, from, to, flag, false));
                break;
            }
            case 70: { // query_range_ptr
                const int flag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value to = popValue(pcb);
                Value from = popValue(pcb);
                Value container = popValue(pcb);
                pushValue(pcb, queryRangeValue(container, from, to, flag, true));
                break;
            }
            // ---- calls ----
            case 6: { // call_in_efun
                const int efun = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int argc = fromMeta ? m1 : in.b;
                {
                    const std::vector<std::string>& et = efunNameTable();
                    std::string en = (efun >= 0 &&
                                      static_cast<size_t>(efun) < et.size())
                                         ? et[static_cast<size_t>(efun)]
                                         : "efun#" + std::to_string(efun);
                    // [op6st] op6 set_timer watch: try_gather's chest timer
                    // hinges on this call actually reaching the efun layer.
                    static const std::atomic<bool>& op6Flag =
                        lpc::dynProbeFlag("LPCVM_OP6ST");
                    if (op6Flag.load(std::memory_order_relaxed) && en == "set_timer")
                        std::cerr << "[op6st] set_timer argc=" << argc
                                  << " obj=" << (curObj(pcb) ? curObj(pcb)->key() : std::string("(null)"))
                                  << " fn=" << (pcb->frames.empty() || !pcb->frames.back().fn ? "(null)" : pcb->frames.back().fn->name)
                                  << " ip=" << (pcb->ip - 1) << "\n";
                    trcCallStack(pcb, 6, en, argc);
                }
                // [efun-bal] 栈平衡断言: efun 弹 argc 压 1 → 净 1-argc。
                // 偏差即 efun 实现弹错 —— create_search_terms 循环头 oob
                // 的最后嫌疑点。
                static const bool w6 = getenv("LPCVM_LOOPWATCH") != nullptr;
                int bal0 = 0;
                bool balWatch = false;
                if (w6) {
                    for (const auto& fr : pcb->frames)
                        if (fr.fn && fr.fn->name == "create_search_terms") {
                            balWatch = true;
                            break;
                        }
                    if (balWatch) bal0 = static_cast<int>(pcb->stack.size());
                }
                dispatchEfun(pcb, efun, argc);   // pushes result (void-aware)
                if (balWatch) {
                    int d = static_cast<int>(pcb->stack.size()) - bal0;
                    if (d != 1 - argc)
                        std::cerr << "[efun-bal] tid=" << pcb->tid
                                  << " efun#" << efun << " argc=" << argc
                                  << " delta=" << d << " expect=" << (1 - argc)
                                  << " ip=" << (pcb->ip - 1) << "\n";
                }
                break;
            }
            case 7: { // call_in_user_fun
                int funIndex = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int flags = fromMeta ? m1 : in.b;
                const int passedArgs = flags & 0x3F;
                // bit7: GLOBAL program's table, operand VERBATIM (never add
                // baseFunOffset — call_global_fn(#630) must stay #630);
                // otherwise LOCAL to the current function's constitution.
                VmObject* obj = nullptr;
                const VmProgram* prog = nullptr;
                const OriginalFunction* fn = nullptr;
                if (flags & 0x80) {
                    // Global fn runs INLINE in the current context (原版
                    // vm_callInUserFunBody(bit7, ..., vm_pGlobalProgram)).
                    // The GLOBAL table = efun slots [0..functionBC) followed by
                    // the boot object's functions; an efun-slot index is run as
                    // an EFUN (原版 funContext 存 pEfun, 走 efun 执行路径) —
                    // e.g. aaa configd's call_global_fn(#569) =
                    // get_telnet_password.
                    const uint32_t gbase = masterProgram()
                                               ? masterProgram()->src().functionBC
                                               : 0;
                    if (gbase > 0 && funIndex < static_cast<int>(gbase)) {
                        dispatchEfun(pcb, funIndex, passedArgs);
                        break;
                    }
                    fn = findMasterFunction(funIndex);
                    obj = curObj(pcb);
                    prog = masterProgram();
                } else {
                    const int off = curFunOffset(pcb);
                    if (off != 0) funIndex += off;
                    prog = curProg(pcb);
                    obj = curObj(pcb);
                    if (prog) fn = prog->fnAtIndex(funIndex);
                }
                if (fn) {
                    {
                        // [skilldbg] global-fn path (call_in_user_fun):
                        // register_skill_ob/get_skill_ob run through HERE, not
                        // op8 — probe the actual no values.
                        static const std::atomic<bool>& skG =
                            lpc::dynProbeFlag("LPCVM_SKILLDBG");
                        if (skG.load(std::memory_order_relaxed) &&
                            (fn->name == "register_skill_ob" ||
                             fn->name == "get_skill_ob") &&
                            !pcb->stack.empty()) {
                            std::cerr << "[skilldbg-g] " << fn->name
                                      << "(arg0="
                                      << to_string(pcb->stack.back().value, false)
                                      .substr(0, 40)
                                      << ") argc=" << (flags & 0x3F) << "\n";
                            std::cerr.flush();
                            if (fn->name == "get_skill_ob") {
                                vmcore::vmHookCallbackWhenReturn(
                                    pcb, [](Value& rv) {
                                        std::cerr << "[skilldbg-g]   -> idx"
                                                  << rv.index() << "\n";
                                        std::cerr.flush();
                                    });
                            }
                        }
                    }
                    // [rpidbg] post_init 永不排空取证: register_post_init 的
                    // 每次调用都打印闭包目标 + 调用者 —— 每轮循环重复出现的
                    // 名字就是卡死 boot 的重注册者。
                    {
                        static const std::atomic<bool>& rpi =
                            lpc::dynProbeFlag("LPCVM_RPIDBG");
                        if (rpi.load(std::memory_order_relaxed) &&
                            fn->name == "register_post_init" &&
                            !pcb->stack.empty()) {
                            // 只报重复注册者: boot 期每个守护只注册一次,
                            // 同一闭包第 2+ 次出现 = post_init 轮询的重注册者
                            // (每轮 register → catch 报错/条件不满足 → 永转)。
                            static std::mutex rpiMx;
                            static std::map<std::string, int> rpiCnt;
                            const Value& top = pcb->stack.back().value;
                            std::string cdesc =
                                "<idx" + std::to_string(top.index()) + ">";
                            if (top.index() == 8) {
                                auto cl = std::get<
                                    std::shared_ptr<ClosureValue>>(top);
                                if (cl)
                                    cdesc = cl->functionName + "@" +
                                            cl->ownerKey;
                            }
                            int cnt = 0;
                            {
                                std::lock_guard<std::mutex> lk(rpiMx);
                                cnt = ++rpiCnt[cdesc];
                            }
                            if (cnt >= 2)
                                std::cerr << "[rpidbg] #" << cnt
                                          << " reg_post_init " << cdesc
                                          << " caller="
                                          << (obj ? obj->key() : "<null>")
                                          << "::"
                                          << (pcb->frames.empty() ||
                                                      !pcb->frames.back().fn
                                                  ? "<ctx>"
                                                  : pcb->frames.back().fn->name)
                                          << "\n";
                        }
                    }
                    // [op7w] scand 调用链栈深审计: 第二次 create_search_terms
                    // 循环头 ctrl 槽 oob(栈浅 1), 对比两次链的进入栈深定位。
                    // 平衡判据: 返回侧 [retw] 的 newSize 必须 == 这里 preStack,
                    // 差 -1 的 (调用,返回) 对就是栈漂移现场。
                    static const bool w7 = getenv("LPCVM_LOOPWATCH") != nullptr;
                    if (w7) {
                        const char* nm = fn->name.c_str();
                        if (!std::strcmp(nm, "create_search_terms") ||
                            !std::strcmp(nm, "query_objects") ||
                            !std::strcmp(nm, "sql_select") ||
                            !std::strcmp(nm, "do_init") ||
                            !std::strcmp(nm, "create_bit_search") ||
                            !std::strcmp(nm, "do_equipment_filter") ||
                            !std::strcmp(nm, "mdb_select") ||
                            !std::strcmp(nm, "revise_scan_items") ||
                            !std::strcmp(nm, "covert_string") ||
                            !std::strcmp(nm, "build_xiaobssb_equip_buf"))
                            std::cerr << "[op7w] tid=" << pcb->tid << " "
                                      << nm << " caller="
                                      << (pcb->frames.empty() ||
                                                  !pcb->frames.back().fn
                                              ? "<ctx>"
                                              : pcb->frames.back().fn->name)
                                      << " preStack=" << pcb->stack.size()
                                      << " args=" << passedArgs;
                            if (passedArgs >= 2 && pcb->stack.size() >=
                                                       static_cast<size_t>(
                                                           passedArgs)) {
                                // param1 (index preStack-args+1): types 数组
                                const StackItem& p1 = pcb->stack[pcb->stack
                                                                      .size() -
                                                                  static_cast<size_t>(passedArgs) + 1];
                                std::cerr << " p1="
                                          << to_string(p1.value, false)
                                                 .substr(0, 60);
                            }
                            std::cerr << "\n";
                    }
                    trcCallStack(pcb, 7, fn->name, passedArgs);
                    if (pushFrameInPlace(pcb, obj, prog, fn, passedArgs, true, 0))
                        break;   // inline start: the callee pushes its own result
                }
                if (vmcore::traceOn()) {
                    std::cerr << "[op7-miss] idx=" << funIndex << " flags=" << flags
                              << " bit7=" << ((flags & 0x80) ? 1 : 0)
                              << " fn=" << (fn ? fn->name.c_str() : "<null>")
                              << " code=" << (fn ? fn->code.size() : 0)
                              << " cur=" << (pcb->frames.empty() || !pcb->frames.back().fn
                                                 ? std::string("<ctx>")
                                                 : pcb->frames.back().fn->name)
                              << "\n";
                }
                pushValue(pcb, Value{});   // missing fn: void (对齐 vm_pushVoid)
                break;
            }
            case 8: { // call_in_other_fun_ex
                const int argn = fromMeta ? m1 : in.b;
                Value pathVal = popValue(pcb);
                Value funVal = popValue(pcb);
                std::string funName;
                int funIndex = -1;
                if (funVal.index() == 4)
                    funName = std::get<SharedString>(funVal).str();
                else if (funVal.index() == 2)
                    funIndex = static_cast<int>(std::get<int64_t>(funVal));
                trcCallStack(pcb, 8, funName.empty()
                                          ? ("fn#" + std::to_string(funIndex))
                                          : funName, argn);
                // Args popped as ITEMS: by-ref arguments must reach the callee
                // as live references (same stack — the callee's
                // push_local_vptr/assign_value writes through to OUR slots).
                std::vector<StackItem> items;
                for (int i = 0; i < argn && !pcb->stack.empty(); ++i)
                    items.push_back(popItem(pcb));
                {
                    // [trg-dbg] punishd 轮询取证: query_combat_trigger 的名字
                    // 参数原样(含 hex)打印 —— 与 register_trigger 注册键比对。
                    static const std::atomic<bool>& td =
                        lpc::dynProbeFlag("LPCVM_OP8DBG");
                    if (td.load(std::memory_order_relaxed) &&
                        (funName == "query_combat_trigger" ||
                         funName == "register_trigger" ||
                         funName == "register_skill_ob" ||
                         funName == "get_skill_ob" ||
                         funName == "set_attrib") &&
                        !items.empty() && items.back().value.index() == 4) {
                        const std::string s = std::get<SharedString>(
                                                  items.back().value)
                                                  .str();
                        std::string hex;
                        for (unsigned char ch : s) {
                            char b[4];
                            snprintf(b, sizeof(b), "%02x", ch);
                            hex += b;
                        }
                        std::cerr << "[trg-dbg] " << funName << " arg0='"
                                  << s.substr(0, 40) << "' hex=" << hex
                                  << "\n";
                    }
                }
                {
                    // [skilldbg] 0x21c1 empty skill list: all_skills[no] stays
                    // empty when register_skill_ob gets no==0 (importd
                    // set_attrib never landed "no" in the skill's dbase) or
                    // get_skill_ob misreads the global mapping.  Log the
                    // actual no values and lookup results.
                    static const std::atomic<bool>& skFlag =
                        lpc::dynProbeFlag("LPCVM_SKILLDBG");
                    if (skFlag.load(std::memory_order_relaxed) &&
                        (funName == "register_skill_ob" ||
                         funName == "get_skill_ob")) {
                        std::string a0;
                        if (!items.empty())
                            a0 = to_string(items.back().value, false);
                        std::cerr << "[skilldbg] " << funName << "(" << a0
                                  << ") argc=" << items.size() << "\n";
                        std::cerr.flush();
                        if (funName == "get_skill_ob") {
                            vmcore::vmHookCallbackWhenReturn(pcb, [](Value& rv) {
                                std::cerr << "[skilldbg]   -> idx"
                                          << rv.index() << "\n";
                                std::cerr.flush();
                            });
                        }
                    }
                }
                auto pushItemsBack = [&]() {
                    for (auto it = items.rbegin(); it != items.rend(); ++it)
                        pushItem(pcb, *it);
                };
                if (pathVal.index() == 5) {
                    // ARRAY broadcast: call fn on every object element,
                    // nested arrays flatten one level, non-objects skipped,
                    // result = array of results (hook-driven so a blocking
                    // efun inside an element parks mid-broadcast).
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(pathVal);
                    std::vector<Value> targets;
                    for (auto& el : arr->items) {
                        if (el.index() == 5) {
                            auto inner = std::get<std::shared_ptr<ArrayValue>>(el);
                            for (auto& e2 : inner->items) targets.push_back(e2);
                        } else {
                            targets.push_back(el);
                        }
                    }
                    std::vector<Value> vargs;
                    vargs.reserve(items.size());
                    for (auto& it : items) vargs.push_back(it.value);
                    vmCallOtherArrayPush(pcb, targets, funName, vargs);
                    break;
                }
                VmObject* target = nullptr;
                if (pathVal.index() == 7) {
                    auto ov = std::get<std::shared_ptr<ObjectValue>>(pathVal);
                    if (ov) {
                        target = ov->vmObj;
                        // Reconstructed reference (get_object_by_id / DBFS
                        // restore): no live pointer, only the registry key.
                        // Resolve it — guardd's npc scan and every other
                        // id-based lookup depend on this working.
                        if (!target && !ov->programName.empty())
                            target = Registry::instance().findByName(
                                ov->programName);
                    }
                } else if (pathVal.index() == 4) {
                    const std::string p =
                        std::get<SharedString>(pathVal).str();
                    target = Registry::instance().findByName(p);
                    if (!target) {
                        // mudlib by-path call_other spelling
                        // ("/aaa/daemons/configd.c" / "/daemons/configd.c"):
                        // 原版 vm_callInOtherFunEx -> vm_lookupObject 按路径
                        // 查找。Registry keys are "pak:<rel>.o" spellings, so
                        // normalize: strip the leading '/', .c -> .o, then
                        // retry bare and with the boot-pak "pak:" prefix.
                        std::string r = p;
                        if (!r.empty() && r.front() == '/') r.erase(r.begin());
                        if (r.size() > 2 &&
                            r.compare(r.size() - 2, 2, ".c") == 0)
                            r.replace(r.size() - 2, 2, ".o");
                        if (!r.empty()) {
                            target = Registry::instance().findByName(r);
                            if (!target)
                                target = Registry::instance().findByName(
                                    "pak:" + r);
                        }
                    }
                }
                if (!target || target->reallyGone()) {
                    // Probe BOTH miss flavors: string paths (index 4) and
                    // object values whose vmObj/registry lookup failed
                    // (index 7) — the silent drop here is what shrinks
                    // scand.do_init's scan table (ob->query("type") -> 0).
                    if (vmcore::traceOn() || g_ctraceOn) {
                        static std::atomic<int> op8MissLog{0};
                        if (op8MissLog.fetch_add(1) < 40) {
                            std::string p = pathVal.index() == 4
                                ? std::get<SharedString>(pathVal).str()
                                : (pathVal.index() == 7 && std::get<std::shared_ptr<ObjectValue>>(pathVal)
                                       ? "ov:" + std::get<std::shared_ptr<ObjectValue>>(pathVal)->programName
                                       : "idx" + std::to_string(pathVal.index()));
                            std::cerr << "[op8-miss] path=" << p
                                      << " fn=" << funName
                                      << " caller="
                                      << (pcb->frames.empty() || !pcb->frames.back().obj
                                              ? std::string("<null>")
                                              : pcb->frames.back().obj->key())
                                      << " keys=";
                            for (auto* ko : Registry::instance().allObjects())
                                if (ko->key().find("configd") != std::string::npos)
                                    std::cerr << ko->key()
                                              << (ko->destructed() ? "(dead)" : "")
                                              << ",";
                            std::cerr << "\n";
                            std::cerr.flush();
                        }
                    }
                    pushValue(pcb, Value{});   // missing/destructed: call dropped (void, 非 int 0)
                    break;
                }
                const OriginalFunction* fn = funName.empty()
                    ? target->prog()->fnAtIndex(funIndex)
                    : target->prog()->findFn(funName);
                if (!fn) {
                    // 原版 vm_callInOtherFun (0x8094cd8): 函数缺失 →
                    // flags&4 (静默) 时 popNStack(argn) + vm_pushVoid() ——
                    // 压 **undefined** 而非 0。mudlib 依赖 undefined!=0:
                    // easy_task.go_to_next_state 的
                    // `0 == easy_taskd->prepare_for_state(...)` 在
                    // sub_ob(easy_task 模板)缺该函数时 (flags=4 静默) 必须为
                    // 假, 压 int 0 使 0==0 判真 → "获取任务信息失败"弹窗
                    // (lw114 fn-trace: call_other 后零 mudlib 帧进入=缺失实锤)。
                    // 无 flags&4 时原版 vm_error("Function is not found.");
                    // 本 VM 一律静默 (历史行为), 仅对齐返回值类型。
                    pushValue(pcb, Value{});   // missing function: void
                    break;
                }
                VmObject* caller = pcb->frames.empty() ? nullptr
                                                       : pcb->frames.back().obj;
                pcb->previousObjectName = caller ? caller->key() : std::string();
                {
                    // LPCVM_SAVEDBG: op8 is THE mudlib call_other path (inline
                    // frame push, vmCallOtherPush is only the C++ entry) —
                    // trace the login/logout save chain hops here.
                    static const std::atomic<bool>& svFlag = dynProbeFlag("LPCVM_SAVEDBG");
                    if (svFlag.load(std::memory_order_relaxed) && caller) {
                        const std::string& ck = caller->key();
                        const std::string tk = target->key();
                        // Full outgoing call graph of the user object (logout
                        // window) + every userd.o -> user.o hop (destroy_user
                        // died mid-body; the killer hop is one of these).
                        if (ck.find("char/user.o#") != std::string::npos ||
                            (ck.find("daemons/userd.o") != std::string::npos &&
                             tk.find("char/user.o#") != std::string::npos) ||
                            (funName == "destroy_user" || funName == "save_to_text" ||
                             funName == "write_content_to" || funName == "create_content_to" ||
                             funName == "enter_world" || funName == "start_game" ||
                             funName == "select_char" || funName == "restore_from_text" ||
                             funName == "restore_raw" || funName == "disconnected")) {
                            std::cerr << "[save] " << ck << " -> "
                                      << tk << "::" << funName << "\n";
                            std::cerr.flush();
                        }
                        // Capture get_start_room's actual result — the save's
                        // "position" comes verbatim from it; shows which branch
                        // produced the persisted room/x/y/dir.
                        if (funName == "get_start_room") {
                            const std::string tk0 = target->key();
                            vmcore::vmHookCallbackWhenReturn(pcb, [tk0](Value& rv) {
                                std::string s = to_string(rv, false);
                                size_t p = s.find("position");
                                std::cerr << "[startroom] " << tk0 << " -> "
                                          << (p == std::string::npos
                                                  ? s.substr(0, 200)
                                                  : s.substr(p, 120))
                                          << "\n";
                                std::cerr.flush();
                            });
                        }
                        // Login-room redirect decision inputs: userd enter_room
                        // switches to the default town when
                        // is_in_mini_client() && mini_clientd::refuse_enter(room).
                        if (funName == "refuse_enter" || funName == "is_in_mini_client") {
                            const std::string tk1 = target->key();
                            const std::string fk1 = funName;
                            vmcore::vmHookCallbackWhenReturn(pcb, [tk1, fk1](Value& rv) {
                                std::cerr << "[minic] " << fk1 << " on " << tk1
                                          << " -> idx" << rv.index()
                                          << (rv.index() == 2
                                                  ? "=" + std::to_string(std::get<int64_t>(rv))
                                                  : "")
                                          << "\n";
                                std::cerr.flush();
                            });
                        }
                    }
                }
                pushItemsBack();
                if (!pushFrameInPlace(pcb, target, target->prog(), fn, argn, true, 0))
                    pushValue(pcb, Value{});   // frame refused: void
                else {
                    // LPCVM_NSDBG: capture what query("not_start_room") returns
                    // on rooms — move.c::enter_room gates start_room updates on
                    // it, and a wrongly-truthy result keeps the stale start
                    // room in the logout save (position never persists).
                    static const std::atomic<bool>& nsDbg = dynProbeFlag("LPCVM_NSDBG");
                    if (nsDbg.load(std::memory_order_relaxed) && funName == "query" &&
                        !pcb->stack.empty() && pcb->stack.back().value.index() == 4 &&
                        std::get<SharedString>(pcb->stack.back().value).str() ==
                            "not_start_room") {
                        const std::string tk = target->key();
                        vmcore::vmHookCallbackWhenReturn(pcb, [tk](Value& rv) {
                            std::cerr << "[nsroom] " << tk << " -> "
                                      << (rv.index() == 2
                                              ? std::to_string(std::get<int64_t>(rv))
                                              : "idx" + std::to_string(rv.index()))
                                      << "\n";
                            std::cerr.flush();
                        });
                    }
                }
                break;
            }
            case 5: { // call_fun_ptr
                const int total = fromMeta ? m1 : in.b;
                StackItem funItem = popItem(pcb);
                const int argc = total - 1;
                {
                    static const bool lw5 = getenv("LPCVM_LOOPWATCH") != nullptr;
                    if (lw5 && !pcb->frames.empty() &&
                        pcb->frames.back().fn &&
                        pcb->frames.back().fn->name == "post_init") {
                        std::cerr << "[op5x] p16=" << in.a << " p8="
                                  << static_cast<int>(in.b)
                                  << " total=" << total
                                  << " funIdx=" << funItem.value.index()
                                  << " fun="
                                  << to_string(funItem.value, false).substr(0, 40)
                                  << " preStack=" << (pcb->stack.size() + 1)
                                  << " top=";
                        for (size_t si = pcb->stack.size() > 4
                                             ? pcb->stack.size() - 4 : 0;
                             si < pcb->stack.size(); ++si)
                            std::cerr << pcb->stack[si].value.index() << "/";
                        std::cerr << "\n";
                    }
                }
                if (funItem.value.index() != 8) {
                    // 原版 vm_callFunPtr(0x809441c) type==4 分支: 栈顶是
                    // 函数名字符串 → 在 this 对象 program 按名查找
                    // (vm_findProgramFunction), 找不到再查 master ident;
                    // isGlobal 命中时用 master program 的函数体调用。
                    // 找不到且 mayNull(p8) → pop args + pushVoid; 否则报错。
                    // (此前这里直接 push 0 顶替 —— post_init 对每个 daemon
                    // 的 do_init 按名调用全部被吞, daemon 永不初始化。)
                    if (funItem.value.index() == 4) {
                        const auto& nameS =
                            std::get<SharedString>(funItem.value);
                        std::vector<StackItem> callItems;
                        for (int i = 0; i < argc && !pcb->stack.empty(); ++i)
                            callItems.push_back(popItem(pcb));
                        VmObject* thisObj = curObj(pcb);
                        const OriginalFunction* sfn = nullptr;
                        const VmProgram* sprog = nullptr;
                        if (thisObj) {
                            sfn = thisObj->prog()->findFn(nameS.str());
                            if (sfn) sprog = thisObj->prog();
                        }
                        if (!sfn && masterProgram()) {
                            sfn = masterProgram()->findFn(nameS.str());
                            if (sfn) sprog = masterProgram();
                        }
                        if (sfn) {
                            // 原版 vm_callFunPtr(0x0809441c) 弹 funItem 后把该
                            // 槽"清零保留"(vm_stack[vm_sp].type=0; ++vm_sp) ——
                            // 种子槽在参数之下, 让 callee 的 bp 落在 caller 数据
                            // 之上(argNum=0 时无种子则 callee bp 直接压进 caller
                            // 栈区, op27 压局部覆盖 caller ctrl 槽)。返回收缩
                            // newSize = bp-argNum+1 恰为种子槽+1, resize 天然把
                            // 种子裁掉, 栈深精确复原 —— 公式无需任何改动。
                            pushValue(pcb, Value{});
                            for (auto it = callItems.rbegin();
                                 it != callItems.rend(); ++it)
                                pushItem(pcb, *it);
                            pcb->previousObjectName =
                                curObj(pcb) ? curObj(pcb)->key()
                                            : std::string();
                            if (!pushFrameInPlace(pcb, thisObj, sprog, sfn,
                                                  argc, true, 0))
                                pushValue(pcb, Value{});   // frame refused: void
                            else
                                pcb->frames.back().zeroArgPlaceholder = true;
                            break;
                        }
                        // 原版 mayNull 时 pop args + pushVoid; 这里按宽限
                        // 处理 (post_init 调的 daemon 函数都在 master 里,
                        // 走不到这条), 留痕以便观察。
                        std::cerr << "[op5-name-miss] fn=" << nameS.str()
                                  << " thisObj="
                                  << (thisObj ? thisObj->key() : "<null>")
                                  << "\n";
                        for (int i = 0; i < argc && !pcb->stack.empty(); ++i)
                            popItem(pcb);
                        pushValue(pcb, Value{});
                        break;
                    }
                    // Non-closure: consume args, result 0.
                    for (int i = 0; i < argc && !pcb->stack.empty(); ++i) popItem(pcb);
                    pushValue(pcb, Value{});   // non-closure: void
                    break;
                }
                auto cl = std::get<std::shared_ptr<ClosureValue>>(funItem.value);
                if (!cl) {
                    for (int i = 0; i < argc && !pcb->stack.empty(); ++i) popItem(pcb);
                    pushValue(pcb, Value{});   // null closure: void
                    break;
                }
                // Collect the call args as ITEMS (refs stay live: the callee
                // writes through &args into OUR slots — same stack).
                std::vector<StackItem> callItems;
                for (int i = 0; i < argc && !pcb->stack.empty(); ++i)
                    callItems.push_back(popItem(pcb));
                if (cl->efunClosure) {
                    // Efun closure: re-push items, let the efun layer pop them
                    // with ref semantics (dispatchEfunClosure).
                    for (auto it = callItems.rbegin(); it != callItems.rend(); ++it)
                        pushItem(pcb, *it);
                    dispatchEfunClosure(pcb, cl, argc);
                    break;
                }
                // Resolve the target function: exact func first, then by
                // name in the owner program.
                VmObject* ownerObj = closureOwner(cl);
                const OriginalFunction* fn = nullptr;
                if (cl->func) fn = cl->func;
                else if (ownerObj && !cl->functionName.empty())
                    fn = ownerObj->prog()->findFn(cl->functionName);
                else if (!cl->functionName.empty())
                    fn = findMasterFunction(cl->funcIndex);
                {
                    if (vmcore::traceOn()) {
                        const std::string caller = trcFnName(pcb);
                        if (vmcore::traceWant(cl->functionName) ||
                            vmcore::traceWant(caller)) {
                            std::cerr << "[call] op=5 fn=" << cl->functionName
                                      << " found=" << (fn ? 1 : 0)
                                      << " owner="
                                      << (ownerObj ? ownerObj->key() : "<null>")
                                      << " bound=";
                            for (auto& b : cl->boundArgs)
                                std::cerr << to_string(b, false).substr(0, 32) << ",";
                            std::cerr << " args=";
                            for (auto& it : callItems)
                                std::cerr << to_string(it.value, false).substr(0, 32)
                                          << ",";
                            std::cerr << " caller=" << caller << "\n";
                        }
                    }
                }
                if (!fn || !ownerObj) {
                    pushValue(pcb, Value{});   // missing fn/owner: void
                    break;
                }
                // Parameter order: bound[0..N-1] then the call args.  The
                // frame convention puts param0 ON TOP of the stack (pushArgs
                // pushes deepest-first).  callItems were popped TOP-first, so
                // restoring them means pushing in REVERSE (same as op-8's
                // pushItemsBack) — a forward push flipped the call args
                // internally (operate_done saw cookie==got_result swapped and
                // the ne check always reported a mismatch).
                // 原版 funptr 槽"清零保留"(0x0809441c: type=0; ++sp): 种子槽
                // 在参数之下隔离 callee 帧与 caller 栈区(argNum=0 时无种子则
                // callee bp 直接指进 caller 数据, op27 压局部覆盖 caller ctrl)。
                // 返回收缩 newSize = bp-argNum+1 恰为种子槽+1, resize 天然裁掉
                // 种子、栈深复原 —— 帧约定 bp=最后参数 与 resort/[bp-i] 取参
                // 全不受影响(种子在参数区之下一个槽, 相对位置不变)。
                // 此前一次补种子失败(load_all 循环变量 undefined)是种子补在
                // 栈深敏感路径之外/次序不对所致, 现按原版语义在回压参数前补。
                pushValue(pcb, Value{});
                for (auto it = callItems.rbegin(); it != callItems.rend(); ++it)
                    pushItem(pcb, *it);
                for (int i = static_cast<int>(cl->boundArgs.size()) - 1; i >= 0; --i)
                    pushValue(pcb, cl->boundArgs[static_cast<size_t>(i)]);
                {
                    static const bool lw = getenv("LPCVM_LOOPWATCH") != nullptr;
                    if (lw && !pcb->frames.empty()) {
                        const VmFrame& cf = pcb->frames.back();
                        // 弹掉调用项是否会吃掉当前帧栈区之下的槽: 弹前栈深
                        // 与弹项数的差 < 1 就是吃到了更低的槽(局部/ctrl)。
                        int before = static_cast<int>(pcb->stack.size()) +
                                     static_cast<int>(callItems.size());
                        std::cerr << "[op5] caller="
                                  << (cf.fn ? cf.fn->name : "<ctx>")
                                  << " bp=" << cf.bp
                                  << " stackBefore=" << before
                                  << " argc=" << argc
                                  << " bound=" << cl->boundArgs.size()
                                  << " callee="
                                  << (fn ? fn->name : std::string("?"))
                                  << " calleeLocals="
                                  << (fn ? fn->localCount : -1)
                                  << " ctrlSlots=";
                        for (const auto& kv : cf.loops)
                            std::cerr << kv.first << " ";
                        std::cerr << "\n";
                    }
                }
                const int passed = static_cast<int>(cl->boundArgs.size()) + argc;
                pcb->previousObjectName = curObj(pcb) ? curObj(pcb)->key()
                                                      : std::string();
                // 注意: 原版 funptr 槽"清零保留"(vm_stack[vm_sp].type=0;
                // ++vm_sp) —— 但本 VM 的帧约定是 bp=最后参数(参数=[bp-argNum
                // ..bp-1], 局部=[bp+1..]), resort_stack/check_args 全按此写。
                // 补种子会把 bp 推到参数区外, resort 取参 [bp-i] 全体错位
                // (load_all 循环变量变 undefined, daemons 永远加载不出)。
                // globalFn closure: the BODY comes from the master program
                // (原版 vm_callInUserFunBody(bit7,...,vm_pGlobalProgram)),
                // with the resolved owner object as this-object context.
                if (!pushFrameInPlace(pcb, ownerObj,
                                      cl->globalFn ? masterProgram()
                                                   : ownerObj->prog(),
                                      fn, passed, true, 0))
                    pushValue(pcb, Value{});   // frame refused: void
                else {
                    pcb->frames.back().zeroArgPlaceholder = true;
                    // [menucond] "求师傅指点" 卡菜单战斗取证: create_menu 的
                    // 返回值决定 start_new_round 走 wait_menu_commands(弹菜单,
                    // 无操作面板) 还是 wait_commands(普通战斗)。打印
                    // popup_menu_func (test_popup_menu) 的实际返回值。
                    static const std::atomic<bool>& mcFlag =
                        lpc::dynProbeFlag("LPCVM_MENUCOND");
                    if (mcFlag.load(std::memory_order_relaxed) &&
                        cl->functionName == "test_popup_menu") {
                        vmcore::vmHookCallbackWhenReturn(
                            pcb, [](Value& rv) {
                                std::cerr << "[menucond] test_popup_menu rv idx="
                                          << rv.index() << " val="
                                          << to_string(rv, false).substr(0, 80)
                                          << "\n";
                                std::cerr.flush();
                            });
                    }
                }
                break;
            }
            // ---- jumps / conditionals / switch ----
            case 33: { // if: jump when isZero != bit0; pop when bit1 set
                const int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int flags = fromMeta ? m1 : in.b;
                const bool zero = isZero(pcb->stack.back().value);
                const bool jump = (zero != ((flags & 1) != 0));
                if (vmcore::traceOn()) {
                    const std::string trc_f = trcFnName(pcb);
                    if (vmcore::traceWant(trc_f) && vmcore::traceWantOp(33))
                        std::cerr << "[if] cond="
                                  << to_string(pcb->stack.back().value, false).substr(0, 48)
                                  << " -> " << (jump ? "jump" : "fall")
                                  << " fn=" << trc_f << "\n";
                }
                if (jump) {
                    pcb->ip += static_cast<size_t>(static_cast<int64_t>(offset));
                    if (pcb->ip > pcb->code->size() && g_ctraceOn) {
                        const VmFrame& fr = pcb->frames.back();
                        std::cerr << "[if-overflow] fn="
                                  << (fr.fn ? fr.fn->name : std::string("<ctx>"))
                                  << " ip=" << pcb->ip << " seglen="
                                  << pcb->code->size() << " a=" << in.a
                                  << " b=" << (int)in.b
                                  << " fromMeta=" << (fromMeta ? 1 : 0)
                                  << " m0=" << m0 << " m1=" << m1 << "\n";
                        std::cerr.flush();
                    }
                }
                if (flags & 2) popItem(pcb);
                break;
            }
            case 31: // goto
                pcb->ip += static_cast<size_t>(
                    static_cast<int64_t>(static_cast<int16_t>(in.a)));
                break;
            case 12: { // case_default
                const int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                if (!pcb->stack.empty()) popItem(pcb);
                pcb->ip += static_cast<size_t>(static_cast<int64_t>(offset));
                break;
            }
            case 10: { // case_check_number
                const int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int number = fromMeta ? m1 : 0;
                const Value& v = pcb->stack.back().value;
                if (v.index() == 2 && std::get<int64_t>(v) == number) {
                    popItem(pcb);
                    pcb->ip += static_cast<size_t>(static_cast<int64_t>(offset));
                }
                break;
            }
            case 11: { // case_check_string
                const int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int strIndex = fromMeta ? m1 : 0;
                const Value& v = pcb->stack.back().value;
                const std::vector<std::string>* tbl = curStrings(pcb);
                if (v.index() == 4 && tbl && strIndex >= 0 &&
                    static_cast<size_t>(strIndex) < tbl->size() &&
                    std::get<SharedString>(v) == (*tbl)[static_cast<size_t>(strIndex)]) {
                    popItem(pcb);
                    pcb->ip += static_cast<size_t>(static_cast<int64_t>(offset));
                }
                break;
            }
            // ---- prologue / arg defaults ----
            case 27: { // enter_function: push localCount undefineds
                if (!pcb->frames.empty()) {
                    const VmFrame& f = pcb->frames.back();
                    // 写手探测: 新帧压局部会从当前栈顶向上占槽 —— 若下层
                    // 帧的循环 ctrl 槽号 >= 压入前栈深, 就是帧重叠实锤。
                    if (f.fn) {
                        const int base = static_cast<int>(pcb->stack.size());
                        for (size_t fi = 0; fi + 1 < pcb->frames.size(); ++fi)
                            for (const auto& kv : pcb->frames[fi].loops)
                                if (kv.first >= base)
                                    std::cerr << "[op27-clobbers-lower] newFn="
                                              << f.fn->name
                                              << " locals=" << f.fn->localCount
                                              << " base=" << base
                                              << " lowerF" << fi << "="
                                              << (pcb->frames[fi].fn
                                                      ? pcb->frames[fi].fn->name
                                                      : "<ctx>")
                                              << " ctrlSlot=" << kv.first
                                              << "\n";
                        static const bool w27 =
                            getenv("LPCVM_LOOPWATCH") != nullptr;
                        if (w27) {
                            const char* nm27 = f.fn->name.c_str();
                            if (!std::strcmp(nm27, "create_search_terms") ||
                                !std::strcmp(nm27, "query_objects") ||
                                !std::strcmp(nm27, "do_init"))
                                std::cerr << "[op27w] " << nm27
                                          << " locals=" << f.fn->localCount
                                          << " before=" << base
                                          << " after="
                                          << (base + f.fn->localCount)
                                          << " bp=" << f.bp << "\n";
                        }
                        for (int i = 0; i < f.fn->localCount; ++i)
                            pushValue(pcb, Value{});
                    }
                }
                break;
            }
            case 15: { // check_args_num (vm_checkArgNum 0x809521c)
                const int skipAll = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int passed = pcb->frames.empty() ? 0
                                                       : pcb->frames.back().argNum;
                const int declared =
                    (pcb->frames.empty() || !pcb->frames.back().fn)
                        ? 0 : pcb->frames.back().fn->argCount;
                const int maxArgNo = declared;
                if (!pcb->frames.empty()) pcb->frames.back().maxArgNo = maxArgNo;
                {
                    // [defdbg] build_msg_message's server/show_extra came out
                    // 0/"" on the wire while the time() default applied —
                    // watch which functions skip the default block and with
                    // what passed/declared counts.
                    static const std::atomic<bool>& dd =
                        lpc::dynProbeFlag("LPCVM_DEFDBG");
                    if (dd.load(std::memory_order_relaxed) && !pcb->frames.empty() &&
                        pcb->frames.back().fn) {
                        const std::string& fnn = pcb->frames.back().fn->name;
                        if (fnn.rfind("build_msg_message", 0) == 0 ||
                            fnn.rfind("build_msg_long_message", 0) == 0)
                            std::cerr << "[defdbg] checkargs " << fnn
                                      << " passed=" << passed
                                      << " declared=" << declared
                                      << " skip=" << skipAll << "\n",
                                std::cerr.flush();
                    }
                }
                // All declared args passed -> skip the inline default block;
                // otherwise fall through (defaults + resort_stack run).
                if (passed >= maxArgNo)
                    pcb->ip += static_cast<size_t>(static_cast<int64_t>(skipAll));
                break;
            }
            case 71: { // resort_stack: move inline defaults below the real args
                if (pcb->frames.empty()) break;
                VmFrame& f = pcb->frames.back();
                const int defaultPushed =
                    static_cast<int>(pcb->stack.size()) - 1 - f.bp;
                if (defaultPushed > 0) {
                    // The default block is emitted for ALL defaulted params, so
                    // its LAST `missing` entries are the ones that belong to
                    // parameters the caller did NOT supply — keep only those.
                    int keep = defaultPushed;
                    if (f.maxArgNo > 0) {
                        int missing = f.maxArgNo - f.argNum;
                        if (missing < 0) missing = 0;
                        if (missing < defaultPushed) keep = missing;
                    }
                    if (keep < 0) keep = 0;
                    {
                        static const std::atomic<bool>& dd =
                            lpc::dynProbeFlag("LPCVM_DEFDBG");
                        if (dd.load(std::memory_order_relaxed) && f.fn) {
                            const std::string& fnn = f.fn->name;
                            if (fnn.rfind("build_msg_message", 0) == 0 ||
                                fnn.rfind("build_msg_long_message", 0) == 0)
                                std::cerr << "[defdbg] resort " << fnn
                                          << " pushed=" << defaultPushed
                                          << " keep=" << keep
                                          << " argNum=" << f.argNum
                                          << " maxArgNo=" << f.maxArgNo << "\n",
                                    std::cerr.flush();
                        }
                    }
                    std::vector<StackItem> defaults;
                    for (int i = defaultPushed - keep; i < defaultPushed; ++i)
                        defaults.push_back(
                            pcb->stack[static_cast<size_t>(f.bp + 1 + i)]);
                    std::vector<StackItem> args;
                    for (int i = f.argNum - 1; i >= 0; --i)
                        args.push_back(pcb->stack[static_cast<size_t>(f.bp - i)]);
                    pcb->stack.resize(static_cast<size_t>(f.bp - f.argNum + 1));
                    // Defaults re-pushed in REVERSE so param(argNum) ends up
                    // closest to the real args (original vm_resortStack).
                    for (auto it = defaults.rbegin(); it != defaults.rend(); ++it)
                        pcb->stack.push_back(*it);
                    for (auto& a : args) pcb->stack.push_back(a);
                    f.argNum += keep;
                    f.bp = static_cast<int>(pcb->stack.size()) - 1;
                }
                break;
            }
            // ---- loops ----
            case 35: { // init_for_loop
                const int varNo = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int slot = slotOf(pcb, varNo);
                Value typeV = popValue(pcb);
                Value second = popValue(pcb);
                StackItem varRef = popItem(pcb);
                const int number = static_cast<int>(toInt(typeV));
                auto ctrl = std::make_shared<LoopCtrl>();
                ctrl->slotOfVar = varRef.slotIndex >= 0 ? varRef.slotIndex : slot;
                ctrl->varIsSlot = (varRef.slotIndex >= 0);
                ctrl->varOff = ctrl->varIsSlot && !pcb->frames.empty()
                                   ? (pcb->frames.back().bp - varRef.slotIndex) : 0;
                if (number == 1 || number == -1) {
                    ctrl->type = number;
                    ctrl->to = toInt(second);
                    ctrl->current = toInt(derefItem(pcb, varRef));
                } else if (number == 997) {
                    if (second.index() == 5) {
                        ctrl->type = 901;
                        ctrl->arr = std::get<std::shared_ptr<ArrayValue>>(second);
                        ctrl->iter = 0;
                    } else if (second.index() == 6) {
                        ctrl->type = 902;
                        ctrl->map = std::get<std::shared_ptr<MappingValue>>(second);
                        ctrl->mIdx = 0;
                        ctrl->mElementIndex = 0;
                    }
                }
                if (slot >= 0 && static_cast<size_t>(slot) < pcb->stack.size())
                    pcb->stack[static_cast<size_t>(slot)].value = ctrl;
                // ctrl 的主存储放 pcb->loops（按槽号）：post_init 的 daemon
                // do_init 通过 op5 调用后内部 db 查询异步 park 整条链，park
                // 窗口内同 pcb 被其他事件重入会搅掉栈槽镜像 —— 只认栈槽会让
                // op80 读到 undefined → "Bad loop ctrl" → 主 boot 链中止。
                if (slot >= 0 && !pcb->frames.empty())
                    pcb->frames.back().loops[slot] = ctrl;
                break;
            }
            case 18: { // cond_jmp_for_loop
                const int breakOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(pcb, pcb->ip, slotOf(pcb, varNo), breakOffset, 0);
                break;
            }
            case 80: { // step_jmp_for_loop
                const int continueOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                const int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(pcb, pcb->ip, slotOf(pcb, varNo), 0, continueOffset);
                break;
            }
            case 82: // to_reserve_stack_value (placeholder slot)
                pushValue(pcb, Value{});
                break;
            case 83: // to_unreserve_stack_value
                if (!pcb->stack.empty()) popItem(pcb);
                break;
            // ---- catch ----
            case 14: // caught_error: the VmError handler already restored the
                // stack and pushed the error string — continue at the handler.
                break;
            case 26: // end_thread_catch: guarded region completed — drop record
                if (!pcb->catches.empty()) pcb->catches.pop_back();
                break;
            case 44: { // new_thread_catch (vm_newThreadCatch 0x80aaf84)
                const int64_t off = static_cast<int16_t>(in.a);
                CatchRecord rec;
                rec.ipTarget = pcb->ip + static_cast<size_t>(off);
                rec.stackSize = pcb->stack.size();
                rec.frameCount = pcb->frames.size();
                pcb->catches.push_back(rec);
                break;
            }
            case 16: { // check_is_ref_type (vm_checkIsRefType)
                // 检查参数 a 是否为引用（vptr）：argNum <= a 或槽越界时
                // 推 0（非引用），否则推 (stack[slot].isRef ? 1 : 0)。
                // expand_privilege_group 的可选引用出参 (void|ref) 靠这条
                // 指令在 2 参调用时安全跳过 param2 赋值。
                const int idx = static_cast<int16_t>(in.a);
                bool isRef = false;
                if (!pcb->frames.empty() && idx >= 0 &&
                    idx < pcb->frames.back().argNum) {
                    const int slot = slotOf(pcb, idx);
                    if (slot >= 0 &&
                        static_cast<size_t>(slot) < pcb->stack.size())
                        isRef = pcb->stack[static_cast<size_t>(slot)].isRef;
                }
                pushValue(pcb, make_int(isRef ? 1 : 0));
                break;
            }
            case 17: case 19: case 74: case 84:
                break;   // convert_to_ptr/rev/trace: no-ops
            default:
                break;   // 9/49/86 and unknown: no-ops (matches the old engine)
            }
        } catch (const TimeSliceSignal&) {
            throw;   // state stays in the PCB; the scheduler requeues
        } catch (const SuspendSignal&) {
            throw;   // state stays in the PCB; the scheduler parks
        } catch (const ThreadExitSignal&) {
            // vm_returnToBackend: end this chain NOW (exit/exit_thread/clear_env).
            // Probe: when this fires INSIDE a construct chain (master boot /
            // inline script construction) the whole boot chain dies silently —
            // identify the thrower (fn + object + remaining plans).
            if (!pcb->constructStack.empty() && g_ctraceOn) {
                const VmFrame* tf =
                    pcb->frames.empty() ? nullptr : &pcb->frames.back();
                std::cerr << "[threadexit-in-construct] tid=" << pcb->tid
                          << " fn="
                          << (tf && tf->fn ? tf->fn->name : std::string("<noframe>"))
                          << " obj="
                          << (tf && tf->obj ? tf->obj->key() : std::string("<null>"))
                          << " plansLeft=" << pcb->constructStack.size()
                          << "\n";
                std::cerr.flush();
            }
            pcb->frames.clear();
            pcb->stack.clear();
            pcb->catches.clear();
            pcb->code = nullptr;
            pcb->meta = nullptr;
            pcb->ip = 0;
            pcb->returnValue = make_int(0);
            pcb->returned = true;
            return;
        } catch (const std::exception& e) {
            // VmError (and any C++ exception): unwind to the innermost catch.
            if (pcb->catches.empty()) throw;
            const CatchRecord rec = pcb->catches.back();
            if (rec.frameCount > pcb->frames.size()) throw;
            // 错误展开穿过构造帧 = 该构造链已死 (原版语义: 失败的 load_object
            // 析构半成品并从对象表移除, Constructing 标记清除, 下次重新构造)。
            // retHook 随帧销毁, 计划必须显式终结 — 否则 Constructing 悬挂,
            // 后续加载 join-park 等一个永不完成的构造 = boot 死锁。
            auto killConstructsThrough = [&](VmObject* fobj) {
                auto& st = pcb->constructStack;
                for (int si = static_cast<int>(st.size()) - 1; si >= 0; --si) {
                    if (st[si]->obj != fobj) continue;
                    for (int sj = static_cast<int>(st.size()) - 1; sj >= si;
                         --sj) {
                        auto& pl = st[static_cast<size_t>(sj)];
                        pl->failed = true;
                        pl->finished = true;
                        Registry::instance().clearConstructing(
                            pl->obj->key());
                        if (pl->resultPromise) {
                            pl->resultPromise->set_value(make_int(0));
                            pl->resultPromise.reset();
                        }
                        if (!pl->doneEvent.empty())
                            Scheduler::instance().wakeEventFront(
                                pl->doneEvent);
                        // 析构半成品并移出对象表 (失败构造不能留在表里,
                        // 原版 load_object 失败即 destruct)。
                        if (pl->obj && !pl->obj->destructed()) {
                            pl->obj->markDestructed();
                            Registry::instance().unregisterObject(
                                pl->obj->key());
                        }
                    }
                    st.resize(static_cast<size_t>(si));
                    break;
                }
                pcb->construct = st.empty() ? nullptr : st.back();
            };
            // Pop frames opened inside the guarded region exactly like a
            // return (restore caller stream, drop their slots), closing their
            // catch records.
            while (pcb->frames.size() > rec.frameCount) {
                if (!pcb->catches.empty() &&
                    pcb->catches.back().frameCount >= pcb->frames.size())
                    pcb->catches.pop_back();
                const VmFrame fr = pcb->frames.back();
                int newSize = fr.bp - fr.argNum + 1 -
                              (fr.zeroArgPlaceholder ? 1 : 0) - fr.padArgs;
                if (newSize < 0) newSize = 0;
                static const std::atomic<bool>& slotSnagU =
                    lpc::dynProbeFlag("LPCVM_SLOTSNAG");
                for (size_t i = static_cast<size_t>(newSize); i < pcb->stack.size(); ++i)
                    if (slotSnagU.load(std::memory_order_relaxed) &&
                        pcb->stack[i].value.index() == 9)
                        std::cerr << "[loop-slot-kill] unwind-resize slot=" << i
                                  << " newSize=" << newSize
                                  << " fn=" << (fr.fn ? fr.fn->name : std::string("<ctx>"))
                                  << " bp=" << fr.bp << " argNum=" << fr.argNum << "\n";
                pcb->frames.pop_back();
                // 帧清栈只缩不扩 (扩=核算错误, 扩出的空槽即 residual 泄漏)。
                pcb->stack.resize(static_cast<size_t>(
                    newSize > static_cast<int>(pcb->stack.size())
                        ? pcb->stack.size() : newSize));
                if (fr.retCode) {
                    pcb->code = fr.retCode;
                    pcb->meta = fr.retMeta;
                }
                if (fr.obj && !pcb->constructStack.empty())
                    killConstructsThrough(fr.obj);
            }
            if (pcb->catches.empty() ||
                pcb->catches.back().frameCount != pcb->frames.size())
                throw;
            pcb->catches.pop_back();
            static const std::atomic<bool>& slotSnagC =
                lpc::dynProbeFlag("LPCVM_SLOTSNAG");
            for (size_t ki = static_cast<size_t>(rec.stackSize); ki < pcb->stack.size(); ++ki)
                if (slotSnagC.load(std::memory_order_relaxed) &&
                    pcb->stack[ki].value.index() == 9)
                    std::cerr << "[loop-slot-kill] catch-resize slot=" << ki
                              << " recSize=" << rec.stackSize
                              << " recIp=" << rec.ipTarget << "\n";
            pcb->stack.resize(rec.stackSize);
            pushValue(pcb, make_string(std::string(e.what())));
            pcb->ip = rec.ipTarget;
            // 被捕获的错误也必须留痕 (vm_traceError 语义 — 不存在静默吞错)。
            {
                std::string ch;
                for (const auto& fr : pcb->frames) {
                    if (!ch.empty()) ch += "<-";
                    ch += fr.fn ? fr.fn->name : std::string("<ctx>");
                    if (ch.size() > 200) break;
                }
                std::cerr << "[lpc-catch] \"" << e.what() << "\" tid=" << pcb->tid
                          << " chain=[" << ch << "] ip=" << rec.ipTarget << "\n";
                std::cerr.flush();
            }
        }
        if (nested && pcb->frames.size() <= depth)
            return;   // the pushed frame chain drained: back to the C++ caller
    }
    // REACHED ONLY when the main while condition failed (code==nullptr or ip
    // ran past the segment) with the loop returning "normally": the original
    // treats ip-out-of-range as -8102 deleteProcess; frames still on the
    // chain here mean state corruption (the silent boot killer) — surface it.
    if (!pcb->frames.empty() && g_ctraceOn) {
        const VmFrame* tf = &pcb->frames.back();
        std::cerr << "[loop-exit-midchain] tid=" << pcb->tid
                  << " ip=" << pcb->ip
                  << " fn=" << (tf && tf->fn ? tf->fn->name : std::string("<ctx>"))
                  << " frames=" << pcb->frames.size()
                  << " code=" << (pcb->code ? "set" : "null");
        if (tf && tf->fn) {
            const auto& c = tf->fn->code;
            std::cerr << " seglen=" << c.size() << " retIp=" << tf->retIp
                      << " retCode=" << (tf->retCode ? "set" : "null")
                      << " codeIsTop=" << (pcb->code == &c ? 1 : 0)
                      << " ops:";
            if (c.size() <= 80) {
                for (size_t k = 0; k < c.size(); ++k)
                    std::cerr << " " << static_cast<int>(c[k].op);
            }
        }
        std::cerr << " || frames:";
        for (size_t fi = 0; fi < pcb->frames.size(); ++fi) {
            const VmFrame& fr = pcb->frames[fi];
            std::cerr << " [" << (fr.fn ? fr.fn->name : std::string("<ctx>"))
                      << " ip=" << fr.retIp << "]";
        }
        std::cerr << "\n";
        std::cerr.flush();
    }
}

void run(VmPcb* pcb) { loop(pcb, false, 0); }
void runNested(VmPcb* pcb, size_t depth) { loop(pcb, true, depth); }

// ---- efun dispatch hooks (bodies live in the stage-5 efun adapter) ----------

} // namespace Interp
} // namespace vmcore
} // namespace lpc
