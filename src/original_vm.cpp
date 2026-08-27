#include "original_vm.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "builtins.h"
#include "efun_table.h"
#include "value.h"

namespace fs = std::filesystem;

namespace lpc {

namespace {

// ---------------------------------------------------------------------------
// Action table recovered from the original driver (actionInfoTable).
// {name, parameters, attrib} - parameters: 0=entry(), 1=entry(par16),
// 2=entry(par16,par8); attrib bit31 = operands come from the metadata pool.
// ---------------------------------------------------------------------------
struct ActionInfo {
    const char* name;
    uint8_t parameters;
    uint32_t attrib;
};

static const ActionInfo kActions[] = {
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

// ---------------------------------------------------------------------------
// Value helpers (original VM type codes)
// ---------------------------------------------------------------------------
int vmTypeOf(const Value& v) {
    switch (v.index()) {
    case 0: return 0;            // void/nil
    case 1: return 1;            // undefined
    case 2: return 2;            // int
    case 3: return 3;            // real
    case 4: return 4;            // string
    case 5: return 6;            // array
    case 6: return 7;            // mapping
    case 7: return 10;           // object
    case 8: return 9;            // function value
    default: return 0;
    }
}

int64_t toInt(const Value& v) {
    switch (v.index()) {
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: {
        const std::string& s = std::get<std::string>(v);
        return s.empty() ? 0 : std::strtoll(s.c_str(), nullptr, 10);
    }
    case 5: return static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(v)->items.size());
    case 6: return static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(v)->entries.size());
    default: return 0;
    }
}

double toReal(const Value& v) {
    switch (v.index()) {
    case 2: return static_cast<double>(std::get<int64_t>(v));
    case 3: return std::get<double>(v);
    case 4: return std::strtod(std::get<std::string>(v).c_str(), nullptr);
    default: return 0.0;
    }
}

bool isZero(const Value& v) {
    switch (v.index()) {
    case 0:
    case 1: return true;
    case 2: return std::get<int64_t>(v) == 0;
    case 3: return std::get<double>(v) == 0.0;
    case 4: return std::get<std::string>(v).empty();
    case 5: return std::get<std::shared_ptr<ArrayValue>>(v)->items.empty();
    case 6: return std::get<std::shared_ptr<MappingValue>>(v)->entries.empty();
    case 8: {
        auto c = std::get<std::shared_ptr<ClosureValue>>(v);
        return c->functionName.empty();
    }
    default: return false;
    }
}

// Concatenate / add two values following vm_addValue semantics.
Value addValues(const Value& a, const Value& b) {
    if (a.index() == 5 && b.index() == 5) {
        auto r = std::make_shared<ArrayValue>();
        const auto& aa = std::get<std::shared_ptr<ArrayValue>>(a)->items;
        const auto& bb = std::get<std::shared_ptr<ArrayValue>>(b)->items;
        r->items.reserve(aa.size() + bb.size());
        r->items.insert(r->items.end(), aa.begin(), aa.end());
        r->items.insert(r->items.end(), bb.begin(), bb.end());
        return r;
    }
    if (a.index() == 6 && b.index() == 6) {
        auto r = std::make_shared<MappingValue>();
        const auto& aa = std::get<std::shared_ptr<MappingValue>>(a)->entries;
        const auto& bb = std::get<std::shared_ptr<MappingValue>>(b)->entries;
        r->entries = aa;
        for (const auto& kv : bb) {
            bool found = false;
            for (auto& e : r->entries)
                if (value_equal(e.first, kv.first)) { e.second = kv.second; found = true; break; }
            if (!found) r->entries.push_back(kv);
        }
        return r;
    }
    if (a.index() == 4 || b.index() == 4)
        return make_string(to_string(a, false) + to_string(b, false));
    if (a.index() == 3 || b.index() == 3)
        return make_float(toReal(a) + toReal(b));
    return make_int(toInt(a) + toInt(b));
}

Value subValues(const Value& a, const Value& b) {
    if (a.index() == 3 || b.index() == 3) return make_float(toReal(a) - toReal(b));
    return make_int(toInt(a) - toInt(b));
}
Value mulValues(const Value& a, const Value& b) {
    if (a.index() == 3 || b.index() == 3) return make_float(toReal(a) * toReal(b));
    return make_int(toInt(a) * toInt(b));
}
Value divValues(const Value& a, const Value& b) {
    if (a.index() == 3 || b.index() == 3) {
        double d = toReal(b);
        return make_float(d == 0.0 ? 0.0 : toReal(a) / d);
    }
    int64_t d = toInt(b);
    return make_int(d == 0 ? 0 : toInt(a) / d);
}
Value modValues(const Value& a, const Value& b) {
    int64_t d = toInt(b);
    return make_int(d == 0 ? 0 : toInt(a) % d);
}
Value lshValues(const Value& a, const Value& b) { return make_int(toInt(a) << (toInt(b) & 63)); }
Value rshValues(const Value& a, const Value& b) { return make_int(toInt(a) >> (toInt(b) & 63)); }
Value andValues(const Value& a, const Value& b) { return make_int(toInt(a) & toInt(b)); }
Value orValues(const Value& a, const Value& b) { return make_int(toInt(a) | toInt(b)); }
Value xorValues(const Value& a, const Value& b) { return make_int(toInt(a) ^ toInt(b)); }
Value negValue(const Value& a) { return make_int(-toInt(a)); }

int compareValues(const Value& a, const Value& b) {
    if (a.index() == 3 || b.index() == 3) {
        double x = toReal(a), y = toReal(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    if (a.index() == 4 && b.index() == 4) {
        const auto& x = std::get<std::string>(a);
        const auto& y = std::get<std::string>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    int64_t x = toInt(a), y = toInt(b);
    return x < y ? -1 : (x > y ? 1 : 0);
}

// Cast a value in place to the target type (2 int, 3 real, 4 string, 6 array, 7 mapping).
Value castValue(Value v, int targetType, bool setConstAttr = false) {
    switch (targetType) {
    case 2:
        if (v.index() == 3) return make_int(static_cast<int64_t>(std::get<double>(v)));
        if (v.index() == 4) return make_int(std::strtoll(std::get<std::string>(v).c_str(), nullptr, 10));
        if (v.index() == 1 || v.index() == 0) return make_int(0);
        return v;
    case 3:
        if (v.index() == 2) return make_float(static_cast<double>(std::get<int64_t>(v)));
        if (v.index() == 4) return make_float(std::strtod(std::get<std::string>(v).c_str(), nullptr));
        if (v.index() == 1 || v.index() == 0) return make_float(0.0);
        return v;
    case 4:
        if (v.index() == 4) return v;
        if (v.index() == 5 || v.index() == 6 || v.index() == 7 || v.index() == 8) {
            // arrays/mappings/functions stringify
            return make_string(to_string(v, false));
        }
        return make_string(to_string(v, false));
    case 6:
        if (v.index() == 1 || v.index() == 0) return make_array();
        return v;
    case 7:
        if (v.index() == 1 || v.index() == 0) return make_mapping();
        return v;
    default:
        return v;
    }
}

// ---------------------------------------------------------------------------
// Stack model
// ---------------------------------------------------------------------------
struct StackItem {
    bool isRef = false;                       // reference (vptr) item
    int slotIndex = -1;                       // ref to a stack slot
    int globalIndex = -1;                     // ref to a shared global
    int objIndex = -1;                        // ref to an object instance variable
    std::shared_ptr<ArrayValue> arr;          // member ref container
    std::shared_ptr<MappingValue> map;
    Value key;
    Value value;
};

StackItem makeItem(Value v) {
    StackItem it;
    it.value = std::move(v);
    return it;
}
StackItem makeSlotRef(int idx) {
    StackItem it;
    it.isRef = true;
    it.slotIndex = idx;
    return it;
}
StackItem makeGlobalRef(int idx) {
    StackItem it;
    it.isRef = true;
    it.globalIndex = idx;
    return it;
}
StackItem makeObjRef(int idx) {
    StackItem it;
    it.isRef = true;
    it.objIndex = idx;
    return it;
}
StackItem makeMemberRef(const Value& container, const Value& key) {
    StackItem it;
    it.isRef = true;
    it.key = key;
    if (container.index() == 5) it.arr = std::get<std::shared_ptr<ArrayValue>>(container);
    else if (container.index() == 6) it.map = std::get<std::shared_ptr<MappingValue>>(container);
    return it;
}

// Loop control block (value type 20 in the original VM).
struct Frame {
    const OriginalFunction* fn = nullptr;
    int bp = 0;                       // stack index of arg0 (param0 = top)
    int argNum = 0;
    int localCount = 0;
};

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
Value tryCallOtherObject(const std::string& searchRoot, const std::string& objectPath,
                         const std::string& funName, const std::vector<Value>& args);
Value handleBroadcastEfun(const std::string& searchRoot, const std::vector<Value>& args);
bool hasSuffix(const std::string& s, const std::string& suffix);
struct PendingThread {
    const OriginalObject* obj;
    std::string fun;
    std::vector<Value> args;
};
class OriginalVm;
struct ObjectRegistry {
    std::unordered_map<std::string, std::shared_ptr<OriginalObject>> objects;
    std::unordered_map<std::string, std::shared_ptr<OriginalVm>> vms;
};
ObjectRegistry& objectRegistry();
std::vector<PendingThread>& pendingThreads();
std::vector<PendingThread>& postInitFuncs();
void runPendingOriginalThreads(const std::string& searchRoot);
void runPostInitFunctions(const std::string& searchRoot);

int32_t readI32(const std::vector<uint8_t>& meta, uint32_t off) {
    if (off + 4 > meta.size()) throw std::runtime_error("original_vm: bad metadata reference");
    return static_cast<int32_t>(meta[off]) |
           (static_cast<int32_t>(meta[off + 1]) << 8) |
           (static_cast<int32_t>(meta[off + 2]) << 16) |
           (static_cast<int32_t>(meta[off + 3]) << 24);
}

// Runtime error that stops the current execution (like vm_error).
struct VmError : std::runtime_error {
    explicit VmError(const std::string& m) : std::runtime_error(m) {}
};

// The "global variable" space is shared across all objects (it belongs to the
// driver's global program). push_global_var/vptr access this space; object
// instance variables (push_obj_var/vptr) stay per-object.
std::vector<Value>& sharedGlobalVars() {
    static std::vector<Value> g;
    return g;
}

Value& sharedGlobal(int index) {
    auto& g = sharedGlobalVars();
    if (static_cast<size_t>(index) >= g.size()) g.resize(static_cast<size_t>(index) + 1);
    return g[static_cast<size_t>(index)];
}

class OriginalVm {
public:
    explicit OriginalVm(const OriginalObject& obj, std::string searchRoot = "")
        : obj_(obj), searchRoot_(std::move(searchRoot)) {
        objVars_.resize(obj.globalVarCount);
        for (auto& g : objVars_) g = make_int(0);
        for (const auto& fn : obj.functions) {
            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
        }
    }

    const OriginalObject& object() const { return obj_; }
    std::string functionNameByIndex(int index) const {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return "";
        return it->second->name;
    }
    std::vector<Value>& objVars() { return objVars_; }

    Value callFunctionByName(const std::string& name, const std::vector<Value>& args) {
        for (const auto& fn : obj_.functions) {
            if (fn.name == name && fn.index >= 0) {
                auto it = funcsByIndex_.find(fn.index);
                if (it != funcsByIndex_.end()) return executeFunctionFromOutside(*it->second, args);
            }
        }
        return make_int(0);
    }

    Value callFunctionByIndex(int index) {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return make_int(0);
        return executeFunctionOnStack(*it->second);
    }

    int runInit() {
        try {
            execute(obj_.globalCode, obj_.globalMeta);
            callCreate();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_) + " (" + actionInfo(lastOp_).name + ")");
        }
        return 0;
    }

    int run() {
        try {
            execute(obj_.globalCode, obj_.globalMeta);
            callCreate();
            runMainLoop(10, std::chrono::milliseconds(50));
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_) + " (" + actionInfo(lastOp_).name + ")");
        }
        return 0;
    }

    void callCreate() {
        for (const auto& fn : obj_.functions) {
            if (fn.name == "create" && fn.index >= 0) {
                auto it = funcsByIndex_.find(fn.index);
                if (it != funcsByIndex_.end()) {
                    std::vector<Value> args;
                    for (int i = 0; i < it->second->argCount; ++i) args.push_back(UndefinedTag{});
                    executeFunctionFromOutside(*it->second, args);
                }
                break;
            }
        }
    }

    void callHeartBeat(int ticks) {
        const OriginalFunction* hb = findFunction("heart_beat");
        if (!hb) return;
        std::vector<Value> args;
        for (int i = 0; i < hb->argCount; ++i) args.push_back(make_int(0));
        for (int i = 0; i < ticks; ++i) executeFunctionFromOutside(*hb, args);
    }

    void runMainLoop(int ticks, std::chrono::milliseconds interval) {
        const OriginalFunction* hb = findFunction("heart_beat");
        if (!hb) return;
        std::vector<Value> args;
        for (int i = 0; i < hb->argCount; ++i) args.push_back(make_int(0));
        for (int i = 0; i < ticks; ++i) {
            executeFunctionFromOutside(*hb, args);
            if (i + 1 < ticks) std::this_thread::sleep_for(interval);
        }
    }

private:
    const OriginalObject& obj_;
    std::string searchRoot_;
    std::vector<StackItem> stack_;
    std::vector<Value> objVars_;    // per-object instance variables
    std::vector<Frame> frames_;
    std::map<int, const OriginalFunction*> funcsByIndex_;
    int lastOp_ = -1;
    size_t lastIp_ = 0;
    int stepCounter_ = 0;
    Value returnValue_ = make_int(0);
    bool returned_ = false;
    int catchDepth_ = 0;

    const OriginalFunction* findFunction(const std::string& name) {
        for (const auto& fn : obj_.functions)
            if (fn.name == name && fn.index >= 0) return &fn;
        return nullptr;
    }

    // ---- stack primitives (top = back of vector) ----
    void push(const StackItem& it) { stack_.push_back(it); }
    void pushValue(const Value& v) { push(makeItem(v)); }
    StackItem pop() {
        if (stack_.empty()) throw VmError("original_vm: stack underflow");
        StackItem it = std::move(stack_.back());
        stack_.pop_back();
        return it;
    }
    StackItem& top() {
        if (stack_.empty()) throw VmError("original_vm: stack underflow");
        return stack_.back();
    }

    // Absolute stack slot for a local/arg operand (a1 may be negative).
    int slotOf(int a1) const {
        if (frames_.empty()) return a1;
        const Frame& f = frames_.back();
        return f.bp - a1;
    }

    // Read the value a slot ref / global ref / obj ref / member ref points to.
    Value deref(const StackItem& ref) const {
        if (ref.slotIndex >= 0) {
            if (static_cast<size_t>(ref.slotIndex) < stack_.size())
                return stack_[ref.slotIndex].value;
            return make_int(0);
        }
        if (ref.globalIndex >= 0) {
            return sharedGlobal(ref.globalIndex);
        }
        if (ref.objIndex >= 0) {
            if (static_cast<size_t>(ref.objIndex) < objVars_.size()) return objVars_[ref.objIndex];
            return make_int(0);
        }
        if (ref.arr) {
            int64_t i = toInt(ref.key);
            const auto& items = ref.arr->items;
            if (i >= 0 && i < static_cast<int64_t>(items.size())) return items[static_cast<size_t>(i)];
            return UndefinedTag{};
        }
        if (ref.map) {
            for (const auto& kv : ref.map->entries)
                if (value_equal(kv.first, ref.key)) return kv.second;
            return UndefinedTag{};
        }
        return make_int(0);
    }

    void writeThrough(const StackItem& ref, const Value& v) {
        if (ref.slotIndex >= 0) {
            if (static_cast<size_t>(ref.slotIndex) < stack_.size()) stack_[ref.slotIndex].value = v;
        } else if (ref.globalIndex >= 0) {
            sharedGlobal(ref.globalIndex) = v;
        } else if (ref.objIndex >= 0) {
            if (static_cast<size_t>(ref.objIndex) < objVars_.size()) objVars_[ref.objIndex] = v;
        } else if (ref.arr) {
            int64_t i = toInt(ref.key);
            auto& items = ref.arr->items;
            if (i >= 0 && i < static_cast<int64_t>(items.size())) items[static_cast<size_t>(i)] = v;
        } else if (ref.map) {
            for (auto& kv : ref.map->entries) {
                if (value_equal(kv.first, ref.key)) { kv.second = v; return; }
            }
            ref.map->entries.emplace_back(ref.key, v);
        }
    }

    // Read the value at a stack slot; if the slot holds a ref, dereference it.
    Value slotValue(int idx) const {
        if (idx < 0 || static_cast<size_t>(idx) >= stack_.size()) return make_int(0);
        const StackItem& it = stack_[idx];
        if (it.isRef) return deref(it);
        return it.value;
    }

    int32_t metaOperand(const std::vector<uint8_t>& meta, uint16_t a, uint8_t b) const {
        return readI32(meta, static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 16));
    }

    // ------------------------------------------------------------------
    // Efun / function dispatch
    // ------------------------------------------------------------------
    Value callEfun(int efunIndex, int argCount) {
        std::vector<Value> args;
        for (int i = 0; i < argCount && !stack_.empty(); ++i) args.push_back(pop().value);
        // The compiler pushes args in reverse (param0 last, on top), so the
        // pop order already yields param0..paramN-1 - no reversal needed.
        const auto& names = efunNameTable();
        if (std::getenv("LPCVM_TRACE")) {
            std::string nm = (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size())
                                 ? names[efunIndex] : "?";
            std::string fn = frames_.empty() ? "<global>" : frames_.back().fn->name;
            std::cerr << "[vm] efun idx=" << efunIndex << " name=" << nm << " argc=" << argCount
                      << " in=" << fn << "\n";
        }
        if (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size() && !names[efunIndex].empty()) {
            const std::string& nm = names[efunIndex];
            if (nm == "broadcast" && !searchRoot_.empty()) return handleBroadcastEfun(searchRoot_, args);
            if (nm == "create_thread" && args.size() >= 2 && args[1].index() == 8) {
                auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
                pendingThreads().push_back({&obj_, cl->functionName, cl->boundArgs});
                return make_int(static_cast<int64_t>(pendingThreads().size()));
            }
            if (nm == "register_post_init" && args.size() >= 1 && args[0].index() == 8) {
                auto cl = std::get<std::shared_ptr<ClosureValue>>(args[0]);
                postInitFuncs().push_back({&obj_, cl->functionName, cl->boundArgs});
                return make_int(1);
            }
            return call_builtin(nullptr, nm, args);
        }
        return make_int(0);
    }

    Value callUserFunction(int index, int flags) {
        auto it = funcsByIndex_.find(index);
        if (it != funcsByIndex_.end()) {
            const OriginalFunction& fn = *it->second;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[vm] call user fun index=" << index << " name=" << fn.name
                          << " args=" << fn.argCount << "\n";
            return executeFunctionOnStack(fn);
        }
        // Cross-object function table fallback.
        for (auto& kv : objectRegistry().vms) {
            std::string name = kv.second->functionNameByIndex(index);
            if (!name.empty()) {
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[vm] cross-object user fun index=" << index << " name=" << name << "\n";
                return kv.second->callFunctionByIndex(index);
            }
        }
        return make_int(0);
    }

    Value executeFunctionFromOutside(const OriginalFunction& fn, const std::vector<Value>& args) {
        // The original VM pushes args in reverse (param0 last, on top).
        int n = fn.argCount;
        for (int i = n - 1; i >= 0; --i) {
            Value a = (i < static_cast<int>(args.size())) ? args[i] : Value(UndefinedTag{});
            pushValue(a);
        }
        return executeFunctionOnStack(fn);
    }

    Value executeFunctionOnStack(const OriginalFunction& fn) {
        if (static_cast<int>(stack_.size()) < fn.argCount) return make_int(0);
        int bp = static_cast<int>(stack_.size()) - 1;
        Frame frame{&fn, bp, fn.argCount, fn.localCount};
        frames_.push_back(frame);
        Value savedReturn = returnValue_;
        bool savedReturned = returned_;
        returnValue_ = make_int(0);
        returned_ = false;
        try {
            execute(fn.code, fn.metaData);
        } catch (...) {
            frames_.pop_back();
            returnValue_ = savedReturn;
            returned_ = savedReturned;
            throw;
        }
        Value result = returnValue_;
        int newSize = bp - fn.argCount + 1;
        if (newSize < 0) newSize = 0;
        stack_.resize(static_cast<size_t>(newSize));
        frames_.pop_back();
        returnValue_ = savedReturn;
        returned_ = savedReturned;
        return result;
    }

    // ------------------------------------------------------------------
    // The main bytecode executor (mirrors vm_regularProcessor)
    // ------------------------------------------------------------------
    void execute(const std::vector<OriginalInsn>& code, const std::vector<uint8_t>& meta) {
        size_t ip = 0;
        while (ip < code.size()) {
            const OriginalInsn& in = code[ip++];
            lastOp_ = in.op;
            lastIp_ = ip - 1;
            if (++stepCounter_ > 20000000) return;
            const ActionInfo& ai = actionInfo(in.op);
            int32_t m0 = 0, m1 = 0;
            bool fromMeta = (ai.attrib & 0x80000000u) != 0;
            if (fromMeta) {
                uint32_t off = static_cast<uint32_t>(in.a) | (static_cast<uint32_t>(in.b) << 16);
                m0 = readI32(meta, off);
                if (ai.parameters == 2) m1 = readI32(meta, off + 4);
            }
            switch (in.op) {
            case 46: // null_action
                break;
            case 29: // finish_action
                if (returned_) return;
                returnValue_ = make_int(0);
                return;
            case 72: // return_from_fake_fun
            case 73: // return_from_fun
                if (!stack_.empty()) returnValue_ = pop().value;
                else returnValue_ = make_int(0);
                returned_ = true;
                return;
            case 59: // push_number(S)
                pushValue(make_int(static_cast<int16_t>(in.a)));
                break;
            case 60: // push_number(L)
                pushValue(make_int(fromMeta ? m0 : metaOperand(meta, in.a, in.b)));
                break;
            case 63: // push_real_from_word (float bits in the first metadata word)
                pushValue(make_float(*reinterpret_cast<const float*>(&m0)));
                break;
            case 64: // push_shared_string
                if (in.a < obj_.strings.size()) pushValue(make_string(obj_.strings[in.a]));
                else pushValue(make_int(0));
                break;
            case 66: // push_undefined
                pushValue(UndefinedTag{});
                break;
            case 65: // push_this_object
                pushValue(make_object("<this>"));
                break;
            case 52: // push_arg_num
                pushValue(make_int(frames_.empty() ? 0 : frames_.back().argNum));
                break;
            case 53: // push_global_var
                pushValue(sharedGlobal(in.a));
                break;
            case 61: // push_obj_var
                if (in.a < objVars_.size()) pushValue(objVars_[in.a]);
                else pushValue(make_int(0));
                break;
            case 54: // push_global_vptr
                push(makeGlobalRef(in.a));
                break;
            case 62: // push_obj_vptr
                push(makeObjRef(in.a));
                break;
            case 55: // push_local_value
            case 56: { // push_local_var_i (index popped from stack)
                int idx = static_cast<int16_t>(in.a);
                if (in.op == 56) idx = static_cast<int>(toInt(pop().value));
                pushValue(slotValue(slotOf(idx)));
                break;
            }
            case 57: // push_local_vptr
            case 58: { // push_local_vptr_i (index popped from stack)
                int idx = static_cast<int16_t>(in.a);
                if (in.op == 58) idx = static_cast<int>(toInt(pop().value));
                push(makeSlotRef(slotOf(idx)));
                break;
            }
            case 3: { // assign_value: pop ref, keep value on stack
                StackItem ref = pop();
                if (!stack_.empty()) writeThrough(ref, stack_.back().value);
                break;
            }
            case 4: { // assign_value_reverse: pop value, keep ref
                StackItem val = pop();
                if (!stack_.empty()) writeThrough(stack_.back(), val.value);
                break;
            }
            case 76: // set_global_var
                sharedGlobal(in.a) = pop().value;
                break;
            case 78: // set_obj_var
                if (in.a < objVars_.size()) objVars_[in.a] = pop().value;
                else pop();
                break;
            case 77: // set_local_var
                if (!stack_.empty()) stack_[static_cast<size_t>(slotOf(static_cast<int16_t>(in.a)))].value = pop().value;
                break;
            case 51: // pop_stack
                pop();
                break;
            case 50: // pop_n_stack
                for (int i = 0; i < in.a && !stack_.empty(); ++i) pop();
                break;
            case 34: // inc_value
                if (!stack_.empty()) {
                    StackItem ref = pop();
                    if (ref.isRef) {
                        Value cur = deref(ref);
                        Value nv = make_int(toInt(cur) + static_cast<int16_t>(in.a));
                        writeThrough(ref, nv);
                    }
                }
                break;
            case 22: { // create_array
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.push_back(pop().value);
                std::reverse(items.begin(), items.end());
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                pushValue(arr);
                break;
            }
            case 23: { // create_function_value (bound count = par8 & 0x3F)
                auto it = funcsByIndex_.find(in.a);
                if (it != funcsByIndex_.end()) {
                    std::vector<Value> bound;
                    int boundCount = in.b & 0x3F;
                    for (int i = 0; i < boundCount && !stack_.empty(); ++i) bound.push_back(pop().value);
                    pushValue(make_closure(it->second->name, bound));
                } else {
                    pushValue(make_int(0));
                }
                break;
            }
            case 24: { // create_mapping
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.push_back(pop().value);
                std::reverse(items.begin(), items.end());
                auto mp = std::make_shared<MappingValue>();
                for (size_t i = 0; i + 1 < items.size(); i += 2)
                    mp->entries.emplace_back(items[i], items[i + 1]);
                pushValue(mp);
                break;
            }
            case 21: { // create_argument_array
                std::vector<Value> items;
                if (!frames_.empty()) {
                    const Frame& f = frames_.back();
                    for (int i = 0; i < f.argNum; ++i) items.push_back(slotValue(f.bp - i));
                }
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                pushValue(arr);
                break;
            }
            case 1: { // add
                Value b = pop().value;
                Value a = pop().value;
                pushValue(addValues(a, b));
                break;
            }
            case 81: // sub_value
            case 25: { // div_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(in.op == 81 ? subValues(a, b) : divValues(a, b));
                break;
            }
            case 41: { // mul_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(mulValues(a, b));
                break;
            }
            case 40: { // mod_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(modValues(a, b));
                break;
            }
            case 38: { // lsh_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(lshValues(a, b));
                break;
            }
            case 75: { // rsh_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(rshValues(a, b));
                break;
            }
            case 2: { // and_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(andValues(a, b));
                break;
            }
            case 48: { // or_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(orValues(a, b));
                break;
            }
            case 85: { // xor_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(xorValues(a, b));
                break;
            }
            case 43: { // neg_value
                pushValue(negValue(pop().value));
                break;
            }
            case 28: { // eq_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(value_equal(a, b) ? 1 : 0));
                break;
            }
            case 42: { // ne_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(value_equal(a, b) ? 0 : 1));
                break;
            }
            case 30: { // ge_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(compareValues(a, b) >= 0 ? 1 : 0));
                break;
            }
            case 32: { // gt_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(compareValues(a, b) > 0 ? 1 : 0));
                break;
            }
            case 37: { // le_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(compareValues(a, b) <= 0 ? 1 : 0));
                break;
            }
            case 39: { // lt_value
                Value b = pop().value;
                Value a = pop().value;
                pushValue(make_int(compareValues(a, b) < 0 ? 1 : 0));
                break;
            }
            case 47: { // op_eq_value (compound assignment)
                int op = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value rhs = pop().value;
                StackItem ref = pop();
                Value target = deref(ref);
                Value result = make_int(0);
                switch (op) {
                case 2: result = addValues(target, rhs); break;
                case 3: result = subValues(target, rhs); break;
                case 4: result = mulValues(target, rhs); break;
                case 5: result = divValues(target, rhs); break;
                case 6: result = andValues(target, rhs); break;
                case 7: result = orValues(target, rhs); break;
                case 8: result = xorValues(target, rhs); break;
                case 9: result = rshValues(target, rhs); break;
                case 10: result = lshValues(target, rhs); break;
                case 11: result = modValues(target, rhs); break;
                default: break;
                }
                writeThrough(ref, result);
                pushValue(result);
                break;
            }
            case 45: // not_value
                pushValue(make_int(isZero(pop().value) ? 1 : 0));
                break;
            case 79: { // sizeof
                Value v = pop().value;
                pushValue(make_int(toInt(v)));
                break;
            }
            case 36: { // is_type
                int t = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value v = top().value;
                top().value = make_int(vmTypeOf(v) == t ? 1 : 0);
                break;
            }
            case 13: { // cast_type
                int t = fromMeta ? m0 : static_cast<int16_t>(in.a);
                top().value = castValue(top().value, t);
                break;
            }
            case 20: { // convert_to_value (dereference the top ref)
                StackItem ref = top();
                if (ref.isRef) {
                    Value v = deref(ref);
                    stack_.back() = makeItem(v);
                }
                break;
            }
            case 67: { // query_member
                Value key = pop().value;
                Value container = pop().value;
                pushValue(queryMemberValue(container, key));
                break;
            }
            case 68: { // query_member_ptr
                Value key = pop().value;
                Value container = pop().value;
                push(makeMemberRef(container, key));
                break;
            }
            case 69: { // query_range
                int flag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value to = pop().value;
                Value from = pop().value;
                Value container = pop().value;
                pushValue(queryRangeValue(container, from, to, flag, false));
                break;
            }
            case 70: { // query_range_ptr
                int flag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value to = pop().value;
                Value from = pop().value;
                Value container = pop().value;
                pushValue(queryRangeValue(container, from, to, flag, true));
                break;
            }
            case 6: { // call_in_efun
                int efun = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int argc = fromMeta ? m1 : in.b;
                pushValue(callEfun(efun, argc));
                break;
            }
            case 7: { // call_in_user_fun
                int funIndex = fromMeta ? m0 : static_cast<int16_t>(in.a);
                pushValue(callUserFunction(funIndex, fromMeta ? m1 : in.b));
                break;
            }
            case 8: { // call_in_other_fun_ex
                Value pathVal = pop().value;
                Value funVal = pop().value;
                int argn = fromMeta ? m1 : in.b;
                std::vector<Value> args;
                for (int i = 0; i < argn && !stack_.empty(); ++i) args.push_back(pop().value);
                std::string funName = funVal.index() == 4 ? std::get<std::string>(funVal) : "";
                Value result = make_int(0);
                bool found = false;
                for (const auto& fn : obj_.functions) {
                    if (fn.name == funName && fn.index >= 0) {
                        auto fit = funcsByIndex_.find(fn.index);
                        if (fit != funcsByIndex_.end()) {
                            result = executeFunctionFromOutside(*fit->second, args);
                            found = true;
                        }
                        break;
                    }
                }
                if (!found && pathVal.index() == 4) {
                    result = tryCallOtherObject(searchRoot_, std::get<std::string>(pathVal), funName, args);
                }
                pushValue(result);
                break;
            }
            case 5: { // call_fun_ptr
                int mayNull = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int total = fromMeta ? m1 : in.b;
                StackItem funItem = pop();
                std::vector<Value> args;
                for (int i = 1; i < total && !stack_.empty(); ++i) args.push_back(pop().value);
                Value result = make_int(0);
                if (funItem.value.index() == 8) {
                    auto cl = std::get<std::shared_ptr<ClosureValue>>(funItem.value);
                    std::vector<Value> all = cl->boundArgs;
                    all.insert(all.end(), args.begin(), args.end());
                    const OriginalFunction* fn = findFunction(cl->functionName);
                    if (fn) result = executeFunctionFromOutside(*fn, all);
                } else if (funItem.value.index() == 4) {
                    const std::string& fnName = std::get<std::string>(funItem.value);
                    const OriginalFunction* fn = findFunction(fnName);
                    if (fn) result = executeFunctionFromOutside(*fn, args);
                }
                if (funItem.value.index() == 0 || funItem.value.index() == 1) {
                    // null function value
                }
                pushValue(result);
                break;
            }
            case 33: { // if: jump when isZero != bit0; pop when bit1 set
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int flags = fromMeta ? m1 : in.b;
                bool zero = isZero(top().value);
                if (zero != ((flags & 1) != 0)) ip += offset;
                if (flags & 2) pop();
                break;
            }
            case 31: // goto
                ip += static_cast<int16_t>(in.a);
                break;
            case 27: { // enter_function
                const Frame& f = frames_.back();
                for (int i = 0; i < f.localCount; ++i) pushValue(UndefinedTag{});
                break;
            }
            case 15: { // check_args_num
                int skipAll = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int passed = frames_.empty() ? 0 : frames_.back().argNum;
                int declared = frames_.empty() ? 0 : frames_.back().fn->argCount;
                // If every declared argument was passed, skip the default-code block.
                if (passed >= declared) ip += skipAll;
                // otherwise fall through and let the inline defaults + resort_stack run
                break;
            }
            case 71: { // resort_stack: move inline default arguments below the
                       // real arguments so they become the trailing parameters.
                if (frames_.empty()) break;
                Frame& f = frames_.back();
                int defaultPushed = static_cast<int>(stack_.size()) - 1 - f.bp;
                if (defaultPushed > 0) {
                    std::vector<StackItem> defaults;
                    for (int i = 0; i < defaultPushed; ++i)
                        defaults.push_back(stack_[static_cast<size_t>(f.bp + 1 + i)]);
                    std::vector<StackItem> args;
                    for (int i = f.argNum - 1; i >= 0; --i)
                        args.push_back(stack_[static_cast<size_t>(f.bp - i)]);
                    stack_.resize(static_cast<size_t>(f.bp - f.argNum + 1));
                    for (auto& d : defaults) stack_.push_back(d);
                    for (auto& a : args) stack_.push_back(a);
                    f.argNum += defaultPushed;
                    f.bp = static_cast<int>(stack_.size()) - 1;
                }
                break;
            }
            case 35: { // init_for_loop
                int varNo = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int slot = slotOf(varNo);
                Value typeV = pop().value;
                Value second = pop().value;
                StackItem varRef = pop();
                int number = static_cast<int>(toInt(typeV));
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[loop] init varNo=" << varNo << " slot=" << slot
                              << " typeV=" << number << " second.type=" << second.index()
                              << " varRef.isRef=" << varRef.isRef << " varSlot=" << varRef.slotIndex << "\n";
                auto ctrl = std::make_shared<LoopCtrl>();
                ctrl->slotOfVar = varRef.slotIndex >= 0 ? varRef.slotIndex : slot;
                if (number == 1 || number == -1) {
                    ctrl->type = number;
                    ctrl->to = toInt(second);
                    ctrl->current = toInt(deref(varRef));
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
                if (slot >= 0 && static_cast<size_t>(slot) < stack_.size())
                    stack_[static_cast<size_t>(slot)].value = ctrl;
                else if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[loop] init ctrl slot out of range: " << slot << " size=" << stack_.size() << "\n";
                break;
            }
            case 18: { // cond_jmp_for_loop
                int breakOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(ip, slotOf(varNo), breakOffset, 0);
                break;
            }
            case 80: { // step_jmp_for_loop
                int continueOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(ip, slotOf(varNo), 0, continueOffset);
                break;
            }
            case 82: // to_reserve_stack_value
                pushValue(UndefinedTag{});
                break;
            case 83: // to_unreserve_stack_value
                if (!stack_.empty()) pop();
                break;
            case 14: // caught_error
            case 26: // end_thread_catch
            case 44: // new_thread_catch
            case 16: // check_is_ref_type
            case 17: // check_is_ref_type_i
            case 19: // convert_to_ptr
            case 74: // rev_value
            case 84: // trace
                break;
            case 12: { // case_default
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                if (!stack_.empty()) pop();
                ip += offset;
                break;
            }
            case 10: { // case_check_number
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int number = fromMeta ? m1 : 0;
                Value v = top().value;
                if (v.index() == 2 && std::get<int64_t>(v) == number) {
                    pop();
                    ip += offset;
                }
                break;
            }
            case 11: { // case_check_string
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int strIndex = fromMeta ? m1 : 0;
                Value v = top().value;
                if (v.index() == 4 && strIndex >= 0 &&
                    static_cast<size_t>(strIndex) < obj_.strings.size() &&
                    std::get<std::string>(v) == obj_.strings[static_cast<size_t>(strIndex)]) {
                    pop();
                    ip += offset;
                }
                break;
            }
            default:
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[vm] unhandled op " << in.op << " " << ai.name
                              << " at ip=" << (ip - 1) << "\n";
                break;
            }
        }
    }

    // ------------------------------------------------------------------
    // Loop step (mirrors vm_cmpAndJmpInLoop)
    // ------------------------------------------------------------------
    void loopStep(size_t& ip, int ctrlSlot, int breakOffset, int continueOffset) {
        if (ctrlSlot < 0 || static_cast<size_t>(ctrlSlot) >= stack_.size()) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[loop] ctrl slot out of range: " << ctrlSlot << " size=" << stack_.size() << "\n";
            return;
        }
        StackItem& slotItem = stack_[static_cast<size_t>(ctrlSlot)];
        if (slotItem.value.index() != 9) {    // LoopCtrl is variant index 9
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[loop] ctrl slot " << ctrlSlot << " not a LoopCtrl (index="
                          << slotItem.value.index() << ")\n";
            return;
        }
        auto c = std::get<std::shared_ptr<LoopCtrl>>(slotItem.value);
        if (!c) return;
        auto setVar = [&](const Value& v) {
            if (c->slotOfVar >= 0 && static_cast<size_t>(c->slotOfVar) < stack_.size())
                stack_[static_cast<size_t>(c->slotOfVar)].value = v;
        };
        if (c->type == 1 || c->type == -1) {
            int64_t current = c->current;
            c->current = current + c->type;
            setVar(make_int(current));
            bool past = c->done || (c->type > 0 && current > c->to) || (c->type < 0 && current < c->to);
            ip += past ? breakOffset : continueOffset;
            if (current == c->to) c->done = 1;
        } else if (c->type == 901) {
            if (!c->arr) return;
            size_t iter = c->iter++;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[loop] 901 iter=" << iter << " size=" << c->arr->items.size()
                          << " break=" << breakOffset << " cont=" << continueOffset << "\n";
            if (iter < c->arr->items.size()) {
                setVar(c->arr->items[iter]);
                ip += continueOffset;
            } else {
                ip += breakOffset;
            }
        } else if (c->type == 902) {
            if (!c->map) return;
            const auto& entries = c->map->entries;
            if (c->mIdx >= entries.size()) {
                ip += breakOffset;
            } else {
                const auto& kv = entries[c->mIdx++];
                auto arr = std::make_shared<ArrayValue>();
                arr->items = {kv.first, kv.second, make_int(c->mElementIndex++)};
                setVar(arr);
                ip += continueOffset;
            }
        } else {
            // Unknown loop control type: fall through (mirrors the original VM).
            return;
        }
    }

    Value queryMemberValue(const Value& container, const Value& key) {
        if (container.index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(container);
            for (auto& kv : mp->entries)
                if (value_equal(kv.first, key)) return kv.second;
            return UndefinedTag{};
        }
        if (container.index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(container);
            int64_t i = toInt(key);
            if (i >= 0 && i < static_cast<int64_t>(arr->items.size())) return arr->items[static_cast<size_t>(i)];
            return UndefinedTag{};
        }
        if (container.index() == 4) {
            const std::string& s = std::get<std::string>(container);
            int64_t i = toInt(key);
            if (i >= 0 && i < static_cast<int64_t>(s.size())) return make_string(std::string(1, s[static_cast<size_t>(i)]));
            return UndefinedTag{};
        }
        return UndefinedTag{};
    }

    Value queryRangeValue(const Value& container, const Value& from, const Value& to,
                          int flag, bool asPtr) {
        int64_t a = toInt(from);
        int64_t b = toInt(to);
        size_t len = 0;
        if (container.index() == 5) len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
        else if (container.index() == 4) len = std::get<std::string>(container).size();
        if (flag & 1) { if (b <= 0) b = 1; b = static_cast<int64_t>(len) - b; }
        if (flag & 2) { if (a <= 0) a = 1; a = static_cast<int64_t>(len) - a; }
        if (a < 0) a = 0;
        if (static_cast<int64_t>(len) <= b) b = static_cast<int64_t>(len) - 1;
        if (a > b) {
            if (container.index() == 4) return make_string("");
            return make_array();
        }
        if (container.index() == 4) {
            const std::string& s = std::get<std::string>(container);
            return make_string(s.substr(static_cast<size_t>(a), static_cast<size_t>(b - a + 1)));
        }
        if (container.index() == 5) {
            const auto& items = std::get<std::shared_ptr<ArrayValue>>(container)->items;
            auto arr = std::make_shared<ArrayValue>();
            for (int64_t i = a; i <= b; ++i) arr->items.push_back(items[static_cast<size_t>(i)]);
            return arr;
        }
        return make_array();
    }
};

ObjectRegistry& objectRegistry() {
    static ObjectRegistry reg;
    return reg;
}

std::vector<PendingThread>& pendingThreads() {
    static std::vector<PendingThread> threads;
    return threads;
}

std::vector<PendingThread>& postInitFuncs() {
    static std::vector<PendingThread> funcs;
    return funcs;
}

Value handleBroadcastEfun(const std::string& searchRoot, const std::vector<Value>&) {
    static bool inBroadcast = false;
    if (inBroadcast) return make_int(1);
    inBroadcast = true;
    struct Section { const char* title; const char* dir; };
    Section sections[] = {
        {"Load all global feature.", "/feature"},
        {"Load all global inherits.", "/inherit"},
        {"Load all global objects.", "/clone"},
        {"Load all global cmds.", "/cmds"},
        {"", "/daemons"},
        {"", "/aaa/daemons"},
        {"", "/aaa/clone"},
    };
    auto& reg = objectRegistry();
    for (auto& sec : sections) {
        std::string title = sec.title;
        if (!title.empty()) std::cout << title << "\n";
        fs::path dir = fs::path(searchRoot) / (sec.dir + 1);
        std::error_code ec;
        if (!fs::exists(dir, ec)) continue;
        std::cout << "Load sub dir: " << sec.dir << "\n";
        std::vector<std::string> files;
        for (auto& p : fs::recursive_directory_iterator(dir, ec)) {
            if (!p.is_regular_file()) continue;
            if (p.path().extension() != ".o") continue;
            fs::path rel = fs::relative(p.path(), searchRoot);
            files.push_back(rel.generic_string());
        }
        std::sort(files.begin(), files.end());
        for (auto& rel : files) {
            std::string virt = "/" + rel;
            if (hasSuffix(virt, ".o")) virt = virt.substr(0, virt.size() - 2) + ".c";
            std::cout << "Load: " << virt << "... OK\n";
            std::string full = searchRoot + "/" + rel;
            if (reg.vms.find(full) == reg.vms.end()) {
                try {
                    auto objPtr = std::make_shared<OriginalObject>(loadOriginalObject(full));
                    auto vmPtr = std::make_shared<OriginalVm>(*objPtr, searchRoot);
                    reg.objects[full] = objPtr;
                    reg.vms[full] = vmPtr;
                    vmPtr->runInit();
                } catch (const std::exception& e) {
                    std::cout << "Load: " << virt << "... FAILED: " << e.what() << "\n";
                }
            }
        }
    }
    runPostInitFunctions(searchRoot);
    inBroadcast = false;
    return make_int(1);
}

bool hasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Value tryCallOtherObject(const std::string& searchRoot, const std::string& objectPath,
                         const std::string& funName, const std::vector<Value>& args) {
    if (searchRoot.empty() || objectPath.empty()) return make_int(0);
    std::string rel = objectPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::vector<std::string> candidates;
    candidates.push_back(searchRoot + "/" + rel);
    if (rel.compare(0, 4, "aaa/") == 0) candidates.push_back(searchRoot + "/" + rel.substr(4));
    for (const auto& c : candidates) {
        std::error_code ec;
        if (!std::filesystem::exists(c, ec)) continue;
        auto& reg = objectRegistry();
        auto it = reg.vms.find(c);
        if (it != reg.vms.end()) {
            return it->second->callFunctionByName(funName, args);
        }
        try {
            auto objPtr = std::make_shared<OriginalObject>(loadOriginalObject(c));
            auto vmPtr = std::make_shared<OriginalVm>(*objPtr, searchRoot);
            reg.objects[c] = objPtr;
            reg.vms[c] = vmPtr;
            return vmPtr->callFunctionByName(funName, args);
        } catch (...) {
        }
    }
    return make_int(0);
}

void runPendingOriginalThreads(const std::string& searchRoot) {
    auto& threads = pendingThreads();
    if (std::getenv("LPCVM_TRACE")) std::cerr << "[orig] pending threads=" << threads.size() << "\n";
    for (auto& th : threads) {
        try {
            OriginalVm vm(*th.obj, searchRoot);
            vm.callFunctionByName(th.fun, th.args);
        } catch (...) {
        }
    }
    threads.clear();
}

void runPostInitFunctions(const std::string& searchRoot) {
    auto& funcs = postInitFuncs();
    if (std::getenv("LPCVM_TRACE")) std::cerr << "[orig] post_init funcs=" << funcs.size() << "\n";
    for (auto& fn : funcs) {
        try {
            OriginalVm vm(*fn.obj, searchRoot);
            vm.callFunctionByName(fn.fun, fn.args);
        } catch (...) {
        }
    }
    funcs.clear();
}

} // namespace

int runOriginalObject(const OriginalObject& obj, std::ostream& out, const std::string& searchRoot) {
    OriginalVm vm(obj, searchRoot);
    vm.run();
    runPendingOriginalThreads(searchRoot);
    out << "Original .o VM finished.\n";
    return 0;
}

int runOriginalObjectInit(const OriginalObject& obj, const std::string& searchRoot) {
    OriginalVm vm(obj, searchRoot);
    vm.runInit();
    return 0;
}

Value callOriginalObjectFunction(const std::string& oPath, const std::string& funName,
                                 const std::vector<Value>& args, const std::string& searchRoot) {
    return tryCallOtherObject(searchRoot, oPath, funName, args);
}

} // namespace lpc
