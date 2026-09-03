#include "original_vm.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "builtins.h"
#include "efun_table.h"
#include "o_loader.h"
#include "pak_loader.h"
#include "value.h"
#include "xdbc.h"

namespace fs = std::filesystem;

namespace lpc {

namespace {
// ---------------------------------------------------------------------------
// Timer registry backing set_timer / get_timer / delete_timer, faithfully
// mirroring the original driver's scheduler (vm_scheduler.c):
//   - vm_putTimerIntoTimerList (0x80aa12c) inserts each timer into an ordered
//     list sorted by ABSOLUTE trigger time; each node's timeoutTicks holds the
//     delta from the previous node (so a walk accumulating timeoutTicks yields
//     the absolute trigger offset from the list head).
//   - vm_f_get_timer (0x80ef470) returns timeout = accumulated ticks / 1000.
//   - Same-name timers may COEXIST; get_timer/delete_timer act on the FIRST
//     match in the list (vm_getTimerByName), they never replace.
//   - delete_timer returns 0 on success, -601 when not found (0x80ef404).
// ---------------------------------------------------------------------------
struct TimerEntry {
    int64_t id = 0;
    std::string name;         // empty when registered with 0/ref as arg2
    std::string callback;     // function name, for the get_timer mapping
    int64_t timeoutSecs = 0;  // delay in seconds
    int64_t triggerAt = 0;    // absolute trigger time (seconds since epoch)
    std::shared_ptr<std::atomic<bool>> cancelled;
};
std::mutex g_timerMtx;
// Ordered by triggerAt (the original's sorted timer list); name lookups walk
// this list for the first match (same-name timers coexist).
std::vector<std::shared_ptr<TimerEntry>> g_timerList;
std::map<int64_t, std::shared_ptr<TimerEntry>> g_timerById;
std::atomic<int64_t> g_timerSeq{1};

// First timer whose name matches (vm_getTimerByName semantics), or nullptr.
std::shared_ptr<TimerEntry> timerFindByName(const std::string& name) {
    std::lock_guard<std::mutex> lk(g_timerMtx);
    for (const auto& e : g_timerList)
        if (e->name == name) return e;
    return nullptr;
}
std::shared_ptr<TimerEntry> timerFindById(int64_t id) {
    std::lock_guard<std::mutex> lk(g_timerMtx);
    auto it = g_timerById.find(id);
    return it == g_timerById.end() ? nullptr : it->second;
}
void timerUnregister(const std::shared_ptr<TimerEntry>& e) {
    if (!e) return;
    std::lock_guard<std::mutex> lk(g_timerMtx);
    if (e->cancelled) *e->cancelled = true;
    for (auto it = g_timerList.begin(); it != g_timerList.end(); ++it)
        if (*it == e) { g_timerList.erase(it); break; }
    auto it = g_timerById.find(e->id);
    if (it != g_timerById.end() && it->second == e) g_timerById.erase(it);
}

// Resolve a mudlib-relative .o path (e.g. "feature/common/daemon.o") exactly
// like the original driver's binary-directory lookup: the mounted pak first
// (the -b directory /aaa/pack_data/lib_aaa32.pak), then the extracted tree
// on disk.  Returns nullptr when neither holds the file.
std::shared_ptr<OriginalObject> loadOriginalObjectByRelPath(const std::string& rel) {
    // Parse-once cache.  runInheritedInitsOf / mergeInheritedFunctions reload
    // the SAME inherited programs for every object construction; with no cache
    // each construction re-read the object bytes from the pak and re-parsed
    // them, and basic_objectd's create constructs thousands of templates
    // (item/npc/monster basic.o) - the crawl's native profile is dominated by
    // pak file reads (filebuf::xsgetn) + runPakOriginal + parse churn.
    // Parsed programs are immutable after loading, so sharing is safe (the
    // registry already shares template programs the same way).
    static std::recursive_mutex cacheMx;
    static std::unordered_map<std::string, std::shared_ptr<OriginalObject>> cache;
    {
        std::lock_guard<std::recursive_mutex> lk(cacheMx);
        auto it = cache.find(rel);
        if (it != cache.end()) return it->second;
    }
    std::shared_ptr<OriginalObject> obj;
    if (pakMounted()) {
        std::vector<uint8_t> bytes;
        if (pakReadFile(pakVirtualPrefix() + "/" + rel, bytes)) {
            try {
                obj = std::make_shared<OriginalObject>(loadOriginalObjectFromBytes(bytes));
                obj->sourceName = "/" + rel;
            } catch (...) {
            }
        }
    }
    if (obj) {
        std::lock_guard<std::recursive_mutex> lk(cacheMx);
        auto& slot = cache[rel];
        if (!slot) slot = obj;
        else obj = slot;
        if (std::getenv("LPCVM_LOAD_PROF")) {
            static int64_t loadCalls = 0;
            if ((++loadCalls % 5000) == 0)
                std::cerr << "[load-prof] calls=" << loadCalls
                          << " cachedDistinct=" << cache.size() << "\n";
        }
        return obj;
    }
    return nullptr;
}

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
    case 10: return 5;           // buffer (vm_castType writes type 5)
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
    case 6: return static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(v)->size());
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
    // vm_isZero (0x808cae6): ONLY number-0, real-0.0, undefined(void) and
    // nil are zero.  Arrays, mappings, buffers, strings and functions are
    // ALWAYS non-zero (probed live: if(({})) and if("") are truthy).
    // Treating an empty array as zero made dba's cmd_read_content skip its
    // send_message reply ("if (!arr)" with a ({}) result -> return 0 without
    // replying), hanging ccs's filed do_operate forever.
    switch (v.index()) {
    case 0:
    case 1: return true;
    case 2: return std::get<int64_t>(v) == 0;
    case 3: return std::get<double>(v) == 0.0;
    default: return false;
    }
}

// ---------------------------------------------------------------------------
// Original-VM numeric semantics, probed live on 192.168.200.128:1024 shell:
//   * the engine is 32-bit: int arithmetic wraps mod 2^32 (1000000*1000000
//     == -727379968); reals are IEEE single precision (100.0/3.0 -> 33.333332,
//     0.1+0.2 == 0.3 is true after per-op float32 rounding).
//   * a mixed int/real binary op keeps the LEFT operand's type: with an int on
//     the left it computes in real and truncates toward zero back to int
//     (3*1.5==4, -3*1.5==-4, 5/2.0==2, 3+1.5==4, 3-1.5==1, 7%2.0==1); with a
//     real on the left the result stays real (1.5*3==4.5, 2.5%1==0.5,
//     1.2*3==3.6).  Unary - keeps the operand's type (-3.9 stays real).
//   * ints are promoted to real through float32 (single precision), so big
//     ints lose low bits before mixing (matches the original's float casts).
static int64_t i32(int64_t v) { return static_cast<int64_t>(static_cast<int32_t>(v)); }
static double f32(double v) { return static_cast<double>(static_cast<float>(v)); }
// real result -> int: truncate toward zero (C++ double->int64 cast), then wrap
// to the original's 32-bit int range.
static int64_t truncWrap(double v) { return i32(static_cast<int64_t>(v)); }

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
        auto r = std::make_shared<MappingValue>(static_cast<uint32_t>(std::get<std::shared_ptr<MappingValue>>(a)->size()));
        for (const auto& kv : std::get<std::shared_ptr<MappingValue>>(a)->items())
            r->insert(kv.first, kv.second);
        for (const auto& kv : std::get<std::shared_ptr<MappingValue>>(b)->items())
            r->insert(kv.first, kv.second);
        return r;
    }
    if (a.index() == 10 || b.index() == 10) {
        // buffer concatenation: buffer + anything = raw byte concat.
        auto cat = [](const Value& v) -> std::string {
            if (v.index() == 10) return std::get<std::shared_ptr<BufferValue>>(v)->data;
            return to_string(v, false);
        };
        return make_buffer(cat(a) + cat(b));
    }
    if (a.index() == 4 || b.index() == 4)
        return make_string(to_string(a, false) + to_string(b, false));
    if (a.index() == 3) return make_float(f32(toReal(a)) + f32(toReal(b)));
    if (b.index() == 3) return make_int(truncWrap(f32(toReal(a)) + f32(toReal(b))));
    return make_int(i32(toInt(a) + toInt(b)));
}

Value subValues(const Value& a, const Value& b) {
    // array - array: remove elements of b from a (vm_subValue / vm_subArray)
    if (a.index() == 5 && b.index() == 5) {
        auto r = std::make_shared<ArrayValue>();
        const auto& bb = std::get<std::shared_ptr<ArrayValue>>(b)->items;
        for (const auto& item : std::get<std::shared_ptr<ArrayValue>>(a)->items) {
            bool found = false;
            for (const auto& x : bb)
                if (value_equal(item, x)) { found = true; break; }
            if (!found) r->items.push_back(item);
        }
        return r;
    }
    // mapping - mapping/array: remove keys present in b
    if (a.index() == 6) {
        auto r = std::make_shared<MappingValue>();
        auto am = std::get<std::shared_ptr<MappingValue>>(a);
        for (const auto& kv : am->items()) {
            bool found = false;
            if (b.index() == 6) {
                found = std::get<std::shared_ptr<MappingValue>>(b)->find(kv.first) != nullptr;
            } else if (b.index() == 5) {
                for (const auto& x : std::get<std::shared_ptr<ArrayValue>>(b)->items)
                    if (value_equal(kv.first, x)) { found = true; break; }
            }
            if (!found) r->insert(kv.first, kv.second);
        }
        return r;
    }
    if (a.index() == 3) return make_float(f32(toReal(a)) - f32(toReal(b)));
    if (b.index() == 3) return make_int(truncWrap(f32(toReal(a)) - f32(toReal(b))));
    return make_int(i32(toInt(a) - toInt(b)));
}
Value mulValues(const Value& a, const Value& b) {
    if (a.index() == 3) return make_float(f32(toReal(a)) * f32(toReal(b)));
    if (b.index() == 3) return make_int(truncWrap(f32(toReal(a)) * f32(toReal(b))));
    return make_int(i32(toInt(a) * toInt(b)));
}
Value divValues(const Value& a, const Value& b) {
    if (a.index() == 3) {
        double d = f32(toReal(b));
        return make_float(d == 0.0 ? 0.0 : f32(toReal(a)) / d);
    }
    if (b.index() == 3) {
        double d = f32(toReal(b));
        return make_int(d == 0.0 ? 0 : truncWrap(f32(toReal(a)) / d));
    }
    int64_t d = toInt(b);
    return make_int(i32(d == 0 ? 0 : toInt(a) / d));
}
Value modValues(const Value& a, const Value& b) {
    if (a.index() == 3) {
        double d = f32(toReal(b));
        return make_float(d == 0.0 ? 0.0 : std::fmod(f32(toReal(a)), d));
    }
    if (b.index() == 3) {
        double d = f32(toReal(b));
        return make_int(d == 0.0 ? 0 : truncWrap(std::fmod(f32(toReal(a)), d)));
    }
    int64_t d = toInt(b);
    return make_int(i32(d == 0 ? 0 : toInt(a) % d));
}
Value lshValues(const Value& a, const Value& b) {
    // vm_lshValue (0x808df54): mapping << mapping is a MERGE done IN PLACE on
    // the LEFT operand - vm_concatMapping(left.mapping, right.mapping) - the
    // left mapping absorbs the right's keys, the right overriding on conflict,
    // then the (now-mutated) left is left on the stack as the result.
    // LPC code relies on this aliasing everywhere:
    //   register_daemon_data:  m << ([ index: val ]);      // result discarded
    //   itemd:                 prop << ([ attrib_name: value ]);
    //   partywar:              cs_info << ([ "matchday_key": ... ]);
    // Copying the left would silently DISCARD every such merge (the statement
    // result is dropped, so the accumulated mapping never grows) and would
    // copy the whole growing mapping on each call (O(n^2) memory churn).
    if (a.index() == 6 && b.index() == 6) {
        auto left = std::get<std::shared_ptr<MappingValue>>(a);
        auto right = std::get<std::shared_ptr<MappingValue>>(b);
        if (std::getenv("LPCVM_DBG_OBJVAR")) {
            auto li = left->items(), ri = right->items();
            std::cerr << "[lsh-map] left=" << li.size() << " right=" << ri.size() << " rkeys=[";
            int show = std::min<int>(12, static_cast<int>(ri.size()));
            for (int i = 0; i < show; ++i)
                std::cerr << to_string(ri[i].first, false).substr(0, 24) << " ";
            std::cerr << "]\n";
        }
        for (const auto& kv : right->items()) left->insert(kv.first, kv.second);
        return left;   // same shared_ptr, mutated in place
    }
    return make_int(i32(static_cast<uint32_t>(toInt(a)) << (static_cast<uint32_t>(toInt(b)) & 31)));
}
Value rshValues(const Value& a, const Value& b) {
    return make_int(static_cast<int32_t>(toInt(a)) >> (static_cast<uint32_t>(toInt(b)) & 31));
}
Value andValues(const Value& a, const Value& b) { return make_int(static_cast<int32_t>(toInt(a)) & static_cast<int32_t>(toInt(b))); }
Value orValues(const Value& a, const Value& b) { return make_int(static_cast<int32_t>(toInt(a)) | static_cast<int32_t>(toInt(b))); }
Value xorValues(const Value& a, const Value& b) { return make_int(static_cast<int32_t>(toInt(a)) ^ static_cast<int32_t>(toInt(b))); }
Value negValue(const Value& a) {
    if (a.index() == 3) return make_float(-f32(toReal(a)));
    return make_int(i32(-toInt(a)));
}

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
        if (v.index() == 4) {
            // vm_castType: (int)string -> strtol; (int)buffer -> big-endian
            // first 4 bytes.  Buffers are byte strings in this VM: when the
            // content is not a decimal number treat it as a buffer.
            const std::string& s = std::get<std::string>(v);
            char* end = nullptr;
            long long parsed = std::strtoll(s.c_str(), &end, 10);
            if (end && *end == '\0' && end != s.c_str()) return make_int(parsed);
            if (s.size() >= 4) {
                uint32_t be = (static_cast<unsigned char>(s[0]) << 24) |
                              (static_cast<unsigned char>(s[1]) << 16) |
                              (static_cast<unsigned char>(s[2]) << 8) |
                              static_cast<unsigned char>(s[3]);
                return make_int(static_cast<int32_t>(be));
            }
            return make_int(0);
        }
        if (v.index() == 10) {
            // vm_castType case 2, type 5 (0x8090344): (int)buffer accumulates
            // every byte big-endian (v22 = byte | (v22 << 8)).
            const std::string& s = std::get<std::shared_ptr<BufferValue>>(v)->data;
            uint64_t be = 0;
            for (unsigned char c : s) be = (be << 8) | c;
            return make_int(static_cast<int64_t>(be));
        }
        if (v.index() == 1 || v.index() == 0) return make_int(0);
        return v;
    case 3:
        if (v.index() == 2) return make_float(static_cast<double>(std::get<int64_t>(v)));
        if (v.index() == 4) return make_float(std::strtod(std::get<std::string>(v).c_str(), nullptr));
        if (v.index() == 1 || v.index() == 0) return make_float(0.0);
        return v;
    case 4:
        if (v.index() == 4) return v;
        if (v.index() == 10) {
            // (string)buffer cast (vm_castType 0x8090212): raw bytes as a
            // string, stripping a leading UTF-8 BOM (EF BB BF) like the
            // original (0x809021a / 0x80903b7).  Returning ":HEX:" here
            // corrupted dba's `content_text = (string)content`.
            std::string s = std::get<std::shared_ptr<BufferValue>>(v)->data;
            if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
                static_cast<unsigned char>(s[1]) == 0xBB &&
                static_cast<unsigned char>(s[2]) == 0xBF)
                s.erase(0, 3);
            return make_string(s);
        }
        if (v.index() == 0) return make_string("(undefined)");
        if (v.index() == 1) return make_string("(void)");
        if (v.index() == 5 || v.index() == 6 || v.index() == 7 || v.index() == 8) {
            // arrays/mappings/functions stringify
            return make_string(to_string(v, false));
        }
        return make_string(to_string(v, false));
    case 5:
        // (buffer) casts (vm_castType case 5, 0x8090120): ALWAYS allocate a
        // real Vm_Buffer_T (type 5):
        //   int -> 4-byte big-endian buffer (0x80902d1)
        //   string -> buffer with the same bytes (0x809029c)
        //   nil/undefined -> empty buffer (0x8090278)
        //   buffer -> itself
        // Returning plain strings here made filed's `(buffer)save_string(...)`
        // content serialize with a spurious u8 length prefix in pad_buffer,
        // corrupting the write_content content field (3-byte garbage in
        // dl_ddb_1.data instead of "({...})").
        if (v.index() == 2) {
            uint32_t n = static_cast<uint32_t>(static_cast<int32_t>(std::get<int64_t>(v)));
            std::string s;
            s += static_cast<char>((n >> 24) & 0xFF);
            s += static_cast<char>((n >> 16) & 0xFF);
            s += static_cast<char>((n >> 8) & 0xFF);
            s += static_cast<char>(n & 0xFF);
            return make_buffer(s);
        }
        if (v.index() == 4) return make_buffer(std::get<std::string>(v));
        if (v.index() == 1 || v.index() == 0) return make_buffer("");
        if (v.index() == 10) return v;
        return make_buffer(to_string(v, false));
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
    // String table of the PROGRAM the function belongs to.  Inherited
    // functions carry their own program's strings (shared-string indexes are
    // resolved against the callee's table, not the current object's).
    const std::vector<std::string>* strings = nullptr;
    // Accumulated baseVarOffset of the function's constitution.  The original
    // driver adds consititution->baseVarOffset to every obj-var access
    // (vm_pushObjVar 0x8091224 / vm_pushObjVptr 0x80911a8 /
    // vm_setObjVar 0x808f7f8).  0 for the object's own functions.
    int varOffset = 0;
    // Accumulated baseFunOffset of the function's constitution.  call_in_user_fun
    // resolves local function indexes as baseFunOffset + index
    // (vm_callInUserFun 0x8094080).  0 for the object's own functions.
    int funOffset = 0;
    // Explicit-stack-machine return context: the caller's code/meta/ip that
    // execution resumes at when this function returns (vm_returnFromFun pops
    // the caller's funContext and jumps back).  For the top-level frame pushed
    // by executeFunctionOnStack these hold the OUTER context that must be
    // restored when that external call completes; ip_ points at the
    // instruction AFTER the calling opcode (ip was pre-incremented).
    const std::vector<OriginalInsn>* retCode = nullptr;
    const std::vector<uint8_t>* retMeta = nullptr;
    size_t retIp = 0;
    // True when this frame was entered with executeFunctionOnStack's
    // zero-arg placeholder slot (one extra slot pushed so bp >= 0); the
    // return cleanup must drop that slot with the frame.
    bool zeroArgPlaceholder = false;
    // Cross-object inline calls (call_other to another object) run the callee
    // on THIS VM's stack with objVars_ temporarily switched to the target
    // object's variable table.  savedObjVars restores the caller's table when
    // the frame pops (set only on such frames; nullptr = same object).
    std::shared_ptr<std::vector<Value>> savedObjVars;
    // The same switch for the FUNCTION-index table (the callee's constitution):
    // while a cross-object inline frame is active, call_in_user_fun resolves
    // LOCAL indexes against the TARGET program's table (filed.o's read_content
    // calls its private do_operate by index 111 - resolving that against the
    // caller's merged table found an unrelated function).  nullptr = use this
    // VM's own funcsByIndex_.
    const std::map<int, const OriginalFunction*>* savedFuncLookup = nullptr;
    // Loop control blocks.  The original VM keeps a loop's control value in a
    // local slot, but expression-stack pops can descend into the local region
    // in this interpreter (the prologue of load_comm_desc does); storing the
    // control separately keeps foreach loops from losing their state.
    std::map<int, std::shared_ptr<LoopCtrl>> loops;
};

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
Value tryCallOtherObject(const std::string& searchRoot, const std::string& objectPath,
                         const std::string& funName, const std::vector<Value>& args,
                         const std::string& callerThisName = "");
Value tryCallOtherObjectByIndex(const std::string& searchRoot, const std::string& objectPath,
                                int funIndex, const std::vector<Value>& args,
                                const std::string& callerThisName = "");
Value handleBroadcastEfun(const std::string& searchRoot, const std::vector<Value>& args);
bool hasSuffix(const std::string& s, const std::string& suffix);
struct PendingThread {
    const OriginalObject* obj;
    std::string fun;
    std::vector<Value> args;
    // For register_post_init closures that wrap a builtin efun (e.g.
    // ccs_clientd/tts_clientd create(): register_post_init((: create_thread,
    // "tccsclient", (: daemon :) :))).  Spawning such an entry by LPC name
    // fails ("create_thread" is an efun, not a master function), so keep the
    // original closure and invoke it through the efun fallback.
    std::shared_ptr<ClosureValue> closure;
};
class OriginalVm;
// ---- efun watchdog globals (filled by callEfunWithArgs, reported by a
// scheduler watchdog thread when one efun call runs >3s). ----
namespace {
extern std::string g_curEfunName, g_curEfunObj, g_curEfunFn;
extern std::mutex g_curEfunMx;
extern std::atomic<int64_t> g_curEfunStartMs;
// Per-efun CPU profiler (LPCVM_EFUN_PROF=1): efun name -> accumulated time.
struct EfunProfEntry {
    int64_t ns = 0;
    int64_t calls = 0;
};
extern std::mutex g_efunProfMx;
extern std::unordered_map<std::string, EfunProfEntry> g_efunProf;
} // namespace
struct ObjectRegistry {
    std::unordered_map<std::string, std::shared_ptr<OriginalObject>> objects;
    std::unordered_map<std::string, std::shared_ptr<OriginalVm>> vms;
    // Objects whose global initializer + create() have already run.  Threads
    // spawned later (daemon/echo_daemon on the master program) share this
    // initialized state; re-running it would restart the whole driver.
    std::unordered_map<std::string, bool> initialized;
    // The same, keyed by object pointer (the master's local VM uses the
    // original object, not the registry copy).
    std::unordered_map<const OriginalObject*, bool> initializedObj;
    // Registry keys whose object-construction process (__init__) is in flight
    // (load_object spawned it; runConstruction raises the done event when it
    // finishes).  A second load_object for the same key must join that wait,
    // not spawn a second construction.
    std::unordered_set<std::string> constructing;
    // The driver's global program object (start_aaa), as used by the running
    // master VM.  Threads/timers on it must not re-run the full startup.
    const OriginalObject* masterObj = nullptr;
};
ObjectRegistry& objectRegistry();
// Forward declaration (defined below the class; used by the VM internals).
std::recursive_mutex& registryMutex();
std::vector<PendingThread>& pendingThreads();
std::mutex& pendingThreadsMutex();
std::vector<PendingThread>& postInitFuncs();
void runPendingOriginalThreads(const std::string& searchRoot);
void runPostInitFunctions(const std::string& searchRoot);
bool isObjectInitialized(const OriginalObject* obj);
bool isMasterObject(const OriginalObject* obj);
// Register the object's VM in the registry WITHOUT running its initializer
// (used by load_object's cooperative construction).  Returns the key ("" on
// failure); defined below the class alongside loadObjectIntoRegistry.
std::string registerObjectVM(const std::string& virtualPath, const std::string& searchRoot);

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

// ---------------------------------------------------------------------------
// Cooperative-scheduler suspend signal (stage B/C).
//
// A blocking efun (sleep / wait_event / receive_from_queue / ...) consumes its
// arguments, registers the wake-up condition with the scheduler, and throws
// SuspendSignal.  All execution state (stack_/frames_/code_/meta_/ip_/
// returnValue_/returned_) lives in VM members, so runLoop simply unwinds and
// the scheduler parks the VM; when the condition fires the scheduler re-enters
// the VM via resumeProcess(), which pushes the efun's result and continues the
// SAME loop from the saved ip_.  This is the original driver's
// vm_pendCurrentProcess model - no OS threads, no Fiber, no C++ recursion.
//
// What a parked process waits for, and the value to push on resume.
struct ResumeCond {
    // kind: 0 = not parked, 1 = timer (wakeAtMs), 2 = queue (qkey),
    //       3 = event (ename), 4 = forever (only woken externally),
    //       5 = socket readable/writable (fd; polled by the scheduler)
    int kind = 0;
    int64_t wakeAtMs = 0;       // kind 1
    std::string qkey;           // kind 2
    std::string ename;          // kind 3
    int fd = -1;                // kind 5
    bool wantRead = false;      // kind 5
    bool wantWrite = false;     // kind 5
    int64_t timeoutAtMs = 0;    // kind 5: wake anyway at this time (connect timeout)
    Value resumeValue;          // pushed onto the stack when resumed
};

// SuspendSignal intentionally does NOT derive from std::exception, so the
// runLoop catch(VmError)/catch(std::exception) handlers never see it.  Every
// C++ boundary that uses catch(...) must re-throw it unchanged (and must NOT
// destroy the parked execution state).
//
// It carries the parking VM and its ResumeCond because a blocking efun may run
// on a DIFFERENT VM than the scheduler-process VM (cross-object call_other
// executes on the target's registry VM; wait_event inside it parks THAT VM,
// but the SuspendSignal propagates to the scheduler which owns the CALLER's
// process).  The scheduler must park/register the cond-owning VM so resume
// re-enters the right execution state.
struct SuspendSignal {
    OriginalVm* vm = nullptr;
    ResumeCond cond;
};

// Time-slice yield for the MASTER process (the original driver's runTicks:
// the master's startup code - load_all, post_init - shares the CPU with the
// daemon processes it creates, so "THREAD_D.service daemon start..." appears
// interleaved with object loading, exactly like the reference log).  When the
// master has run a slice of instructions it yields (throws TimeSliceSignal);
// the scheduler requeues it at the tail of ready_ and runs the next process.
// Execution state stays in the VM members (like SuspendSignal), so resume
// continues from the saved ip_.
//
// It carries the VM that yielded: the master's post_init() loop calls each
// daemon's init/do_init closure INLINE on the OWNER's registry VM (see the
// call_fun_ptr inPostInit branch).  A relinquish inside such a closure must
// park the OWNER's frame chain - its load_all loop continues from the saved
// ip_ when the scheduler resumes it, exactly like the original single-thread
// cooperative scheduler.  Treating it as a plain master yield would requeue
// the master and re-enter runAsMaster(), silently dropping the owner VM's
// in-flight load (this is what lost all /gs/zone and /gs/daemons/tasks
// top-level loads during startup).
struct TimeSliceSignal {
    OriginalVm* vm = nullptr;
};

// The "global variable" space is shared across all objects (it belongs to the
// driver's global program). push_global_var/vptr access this space; object
// instance variables (push_obj_var/vptr) stay per-object.
std::vector<Value>& sharedGlobalVars() {
    static std::vector<Value> g;
    return g;
}

// The master program's (start_aaa) global function table, shared across every
// object.  call_in_user_fun bit7 / create_function_value bit7 / call_fun_ptr
// all resolve global function indices against THIS table, because vm_callInUserFun
// reads vm_pGlobalProgram->pFunctions[index] regardless of the calling object.
// Each entry carries its own strings (the master's string table) and 0
// varOffset/funOffset so it can be executed inline on any VM's stack.
std::unordered_map<int, std::shared_ptr<OriginalFunction>>& sharedMasterFunctions() {
    static std::unordered_map<int, std::shared_ptr<OriginalFunction>> m;
    return m;
}

const OriginalFunction* findMasterFunction(int index) {
    auto& m = sharedMasterFunctions();
    auto it = m.find(index);
    return it == m.end() ? nullptr : it->second.get();
}

Value& sharedGlobal(int index) {
    auto& g = sharedGlobalVars();
    if (static_cast<size_t>(index) >= g.size()) g.resize(static_cast<size_t>(index) + 1);
    return g[static_cast<size_t>(index)];
}

// ---------------------------------------------------------------------------
// Cooperative scheduler (stage B).  A single driver-level event loop that
// drives LPC processes: ready queue, wake timers, queue/event wakeups.
//
// A "process" here is a shared OriginalVm plus the entry it should run.  The
// VM's execution state (stack_/frames_/code_/meta_/ip_) lives in the VM; a
// blocking efun parks the VM (SuspendSignal), the scheduler registers the
// wake condition (ResumeCond) and parks the process; when the condition fires
// the scheduler resumes the VM via resumeProcess() and keeps driving it until
// it finishes or parks again.
//
// The scheduler's run() loop is the replacement for main()'s idle sleep: it
// runs ready processes, fires due timers, and waits for external wakeups
// (queue push / event raise / network data / timer expiry).
// ---------------------------------------------------------------------------
class Scheduler {
public:
    struct Process {
        std::shared_ptr<OriginalVm> vm;
        std::string entry;              // function to call on first run
        std::vector<Value> args;
        Value resumeValue;              // pushed by resumeProcess on wake
        bool firstRun = true;           // false => resume an existing call
        bool done = false;
        // Per-process thread id (the original driver's PCB->tid).  get_thread_id()
        // must return a DIFFERENT value per LPC process - configd's read_etc uses
        // it to build the event name "ev_readetcd" + tid/32 and mask 1 << tid%32,
        // so two processes waiting on read_etc must not share the same event (the
        // master's wait would be woken by a reply meant for the __init__ process
        // and vice versa - it then times out and returns -1, so lgxy_server etc.
        // config reads fail and the lgxy/douchong standard data is never written).
        // The scheduler runs all processes on ONE OS thread, so a thread_local id
        // (what builtins' lpcThreadId() returns) is identical for every process;
        // assign a fresh id per Process instead and expose the CURRENT process.
        int64_t tid = 0;
        std::shared_ptr<TimerEntry> timerEntry;   // set_timer callback (cancellation)
        // The VM whose execution state was parked by a blocking efun.  Usually
        // == vm, but a cross-object call_other executes on the TARGET's
        // registry VM, so the parked state (stack_/frames_/cond_) lives there.
        std::shared_ptr<OriginalVm> parkedVm;
        // For a CROSS-VM blocking call: the process's OWN VM (whose frames -
        // e.g. [do_init, start_service] - were detached by runDetached while
        // the callee ran on parkedVm).  When the callee's top-level frame
        // returns, the scheduler resumes THIS VM's suspended chain with the
        // callee's result, otherwise the caller never continues.
        // NESTED cross-object blocking (master post_init -> daemon do_init on
        // its owner VM -> configd read_etc): each runDetached layer restores
        // its own VM's frames (do_init frame etc.) but only the OUTERMOST
        // process VM (p->vm) was recorded.  Resume must walk every intermediate
        // detached caller in order, innermost first, so do_init's remaining
        // code (init_all_npc etc.) actually runs after read_etc returns.
        // callers_ holds them inner->outer; callerVm kept as a convenience for
        // the common single-layer case.
        std::shared_ptr<OriginalVm> callerVm;
        std::vector<std::shared_ptr<OriginalVm>> callers_;
        // The process's permanent VM (set at spawn, never overwritten by the
        // suspend handler's parkedVm reassignment).
        std::shared_ptr<OriginalVm> homeVm;
        // True after a TimeSliceSignal requeue: resume the master via
        // runAsMaster() (phase continuation), not resumeProcess().
        bool sliceResume = false;
        // True after a CROSS-VM TimeSliceSignal requeue: a post_init closure
        // yielded on its OWNER's registry VM (call_fun_ptr inPostInit runs it
        // inline on that VM).  Resume that VM's frame chain directly via
        // resumeRunLoop() (relinquish already pushed its result), then hand
        // the closure's result back to the caller (the master's post_init
        // loop) with resumeCaller() - the original single-thread scheduler's
        // behaviour for a yield inside a do_init closure.
        bool resumeRunLoop = false;
        // Object-construction process (entry "__init__"): the event raised
        // when the object's init/create finished, waking the parked caller.
        std::string doneEvent;
        // A timed wait_event (kind 3 with wakeAtMs) is registered in BOTH
        // eventWaiters_ and timers_.  Whichever fires first marks eventWoken;
        // the other path then drops the stale wakeup instead of double-running
        // the process.  timedEvent marks such processes so the timer path can
        // resume them with the timeout result (-501).
        bool eventWoken = false;
        bool timedEvent = false;
        // Cancellation token for the wake timer this process parked with.  A
        // process can RE-PARK on a different condition while its previous
        // (timed) wake timer is still pending in timers_; the stale timer must
        // not resume the new park.  parkCurrent sets the old token before
        // registering the new one, and the timer path drops cancelled entries.
        std::shared_ptr<std::atomic<bool>> parkTimerCancelled;
        // External-thread call (execOnMain): the calling thread blocks on this
        // promise until the process's whole call chain completes; runProcess
        // sets it (exactly once) when the process finishes.
        std::shared_ptr<std::promise<Value>> resultPromise;
    };

    // Spawn a new process (create_thread / set_timer / post_init / network
    // callback).  Called from any OS thread; the process runs on the
    // scheduler thread via run().
    void spawn(std::shared_ptr<OriginalVm> vm, const std::string& entry,
               std::vector<Value> args);

    // Spawn an object-construction process (entry "__init__"): runs the
    // object's global init code + create() cooperatively (yields/park resume
    // through the scheduler), then raises `doneEvent` to wake the caller that
    // parked inside load_object.
    void spawnConstruction(std::shared_ptr<OriginalVm> vm, std::vector<Value> args,
                           std::string doneEvent);

    // Schedule a process to run once at absolute time atMs (set_timer).  The
    // optional TimerEntry allows delete_timer to cancel it before it fires.
    void spawnDelayed(std::shared_ptr<OriginalVm> vm, const std::string& entry,
                      std::vector<Value> args, int64_t atMs,
                      std::shared_ptr<TimerEntry> timerEntry = nullptr);

    // Re-queue a process whose wake condition fired.
    void wake(std::shared_ptr<Process> p);

    // Wake a process to the FRONT of the ready queue (load_object's parked
    // caller resumes before processes spawned during construction - the
    // original continues the loading process inline, so "Load: ... OK" prints
    // before the freshly created daemons get their turn).
    void wakeFront(std::shared_ptr<Process> p);

    // Time-slice yield: put the process back at the TAIL of the ready queue
    // (master process after a TimeSliceSignal; its VM state is preserved so
    // the next run continues from the saved ip_).
    void requeue(std::shared_ptr<Process> p);

    // Park the CURRENTLY RUNNING process (called from run() while executing a
    // process whose VM threw SuspendSignal).  Registers the wake condition.
    void parkCurrent(std::shared_ptr<Process> p);

    // Wake every process waiting on queue `qkey` (send_to_queue side).
    void wakeQueue(const std::string& qkey);

    // Wake every process waiting on event `ename` (raise_event side).
    void wakeEvent(const std::string& ename);

    // True when some parked process is currently suspended ON `vm` (its
    // parkedVm).  Used by cross-object call dispatch: a target whose registry
    // VM is already servicing one caller must not take a second concurrent
    // serviced call (single-flight collision); the caller inlines instead.
    bool isVmParked(const OriginalVm* vm);

    // Wake the event's waiters to the FRONT of ready_ (construction done).
    void wakeEventFront(const std::string& ename);

    // Wake processes whose socket (kind-5 wait) became readable/writable.
    void wakeSocket(int fd, bool readable, bool writable);

    // Main loop: run ready processes / fire due timers / wait for wakeups.
    void run();

    // True when called from the scheduler's own thread (run()).  External
    // threads (network accept/recv, telnet, mysql worker) MUST route VM
    // execution through execOnMain so the driver stays single-threaded.
    bool isSchedulerThread() const { return std::this_thread::get_id() == schedThreadId_; }

    // Run an LPC function call on the scheduler thread.  When already there
    // (nested call from a running process) it executes directly; otherwise it
    // spawns a scheduler process for the call and BLOCKS on its result, so the
    // VM's execution state is never touched concurrently by another thread.
    Value execOnMain(const std::shared_ptr<OriginalVm>& vm, const std::string& funName,
                     const std::vector<Value>& args);

    void requestShutdown();

    // If the CURRENTLY RUNNING process should stop the whole driver, park it
    // forever so run() returns when no other work exists.
    void parkForever(std::shared_ptr<Process> p);

private:
    static int64_t nowMs();
    void runProcess(std::shared_ptr<Process> p);

public:
    // The tid of the LPC process currently executing (get_thread_id()).
    int64_t currentProcessTid() const {
        if (currentProcess_) return currentProcess_->tid;
        return -1;
    }
    // A cross-object runDetached layer just restored ITS OWN VM's detached
    // frames while the (blocking) callee parks on another VM.  That VM must be
    // resumed (resumeCaller) after the callee returns, before the process's
    // own VM continues.  Push it onto the current process's caller chain
    // (innermost first).  Called from OriginalVm::runDetached, which runs on
    // the scheduler thread while currentProcess_ is set.
    void noteParkedCaller(OriginalVm* vm) {
        if (!currentProcess_) return;
        if (!vm || vm == currentProcess_->vm.get() ||
            vm == currentProcess_->parkedVm.get()) return;
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        for (auto& kv : reg.vms) {
            if (kv.second.get() == vm) {
                currentProcess_->callers_.push_back(kv.second);
                return;
            }
        }
    }

    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<std::shared_ptr<Process>> ready_;
    // wakeAtMs -> {process, cancellation token}.  A process can re-park on a
    // new condition while an older wake timer is still pending; each timer
    // entry carries its OWN token so the old timer can be cancelled without
    // touching the process's current park state.
    struct TimerEntry2 {
        std::shared_ptr<Process> p;
        std::shared_ptr<std::atomic<bool>> cancelled;
    };
    std::multimap<int64_t, TimerEntry2> timers_;       // wakeAtMs -> entry
    std::unordered_map<std::string, std::deque<std::shared_ptr<Process>>> queueWaiters_;
    std::unordered_map<std::string, std::deque<std::shared_ptr<Process>>> eventWaiters_;
    // fd -> processes parked in connect()/recv() (kind 5).  Polled by run().
    std::unordered_map<int, std::deque<std::shared_ptr<Process>>> sockWaiters_;
    std::vector<std::shared_ptr<Process>> parked_;   // no automatic wakeup
    // The process currently executing (set by runProcess / execOnMain).  Used
    // by get_thread_id() to return a per-PROCESS id on the single scheduler
    // thread (a thread_local would be identical for every process).
    std::shared_ptr<Process> currentProcess_;
    static std::atomic<int64_t> nextTid_;
    bool shutdown_ = false;
    uint64_t step_ = 0;
    int64_t lastPollMs_ = 0;      // last socket poll tick (20ms throttle)
    std::thread::id schedThreadId_;   // the thread running run() (set on entry)

    // Tid pool: a Process keeps its tid from spawn until it finishes (run()'s
    // p->done branch calls releaseTid).  Tids 0..127 are the original driver's
    // process-id space; the old nextTid_ % 128 counter reused an id after 128
    // spawns even while the earlier process was still alive (parked), which
    // made configd's read_etc wait_event (ev_readetcd<tid/32>, bit tid%32)
    // get woken by replies meant for a same-tid sibling - timing out and
    // returning -1, so lgxy_server/douchong_dahui reads failed and the
    // standard data was never written.  Pool the ids instead.
    std::mutex tidMtx_;
    std::set<int64_t> freeTids_;
    bool tidPoolInit_ = false;
    // Allocate a free process tid (0..127), or a wrapped fallback if all 128
    // are live (should not happen under the original's process discipline).
    int64_t allocTid() {
        std::lock_guard<std::mutex> lk(tidMtx_);
        if (freeTids_.empty() && !tidPoolInit_) {
            for (int64_t i = 0; i < 128; ++i) freeTids_.insert(i);
            tidPoolInit_ = true;
        }
        if (freeTids_.empty()) {
            // pool exhausted: hand out the next counter value (wrapped); the
            // caller's read_etc will contend but this is the degenerate case.
            return nextTid_.fetch_add(1) % 128;
        }
        auto it = freeTids_.begin();
        int64_t t = *it;
        freeTids_.erase(it);
        return t;
    }
    void releaseTid(int64_t t) {
        std::lock_guard<std::mutex> lk(tidMtx_);
        freeTids_.insert(t);
    }
};

// The driver-level scheduler instance (defined below; declared here so VM
// blocking efuns can wake parked processes).
Scheduler& scheduler();

class OriginalVm {
public:
    explicit OriginalVm(const OriginalObject& obj, std::string searchRoot = "")
        : obj_(obj), searchRoot_(std::move(searchRoot)) {
        objVars_ = std::make_shared<std::vector<Value>>(static_cast<size_t>(obj.globalVarCount));
        ownObjVars_ = objVars_;
        for (auto& g : *objVars_) g = make_int(0);
        for (const auto& fn : obj.functions) {
            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
        }
        mergeInheritedFunctions();
        objName_ = obj.sourceName.empty() ? "<obj>" : obj.sourceName;
    }
    // Scheduler-process constructor: keeps the source object alive so the VM
    // (whose obj_ is a reference) never dangles after the caller's temporary
    // shared_ptr goes out of scope.
    explicit OriginalVm(std::shared_ptr<const OriginalObject> obj, std::string searchRoot = "")
        : obj_(*obj), objKeepAlive_(std::move(obj)), searchRoot_(std::move(searchRoot)) {
        objVars_ = std::make_shared<std::vector<Value>>(static_cast<size_t>(obj_.globalVarCount));
        ownObjVars_ = objVars_;
        for (auto& g : *objVars_) g = make_int(0);
        for (const auto& fn : obj_.functions) {
            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
        }
        mergeInheritedFunctions();
        objName_ = obj_.sourceName.empty() ? "<obj>" : obj_.sourceName;
    }
    // Clone constructor: shares the template's (read-only) program and its
    // already-merged inherited function table, allocating ONLY a fresh
    // instance-variable table.  The old path deep-copied the whole
    // OriginalObject (all functions + code) and re-ran mergeInheritedFunctions
    // per clone - with thousands of NPC clones that cost ~1.5GB.  The program
    // and merged table are immutable after construction, so sharing them is
    // safe (funcsByIndex_ points into obj_ / inheritedFuncs_, both shared).
    explicit OriginalVm(std::shared_ptr<const OriginalObject> obj, OriginalVm* tmpl)
        : obj_(*obj), objKeepAlive_(std::move(obj)), searchRoot_(tmpl ? tmpl->searchRoot_ : std::string()) {
        objVars_ = std::make_shared<std::vector<Value>>(static_cast<size_t>(obj_.globalVarCount));
        ownObjVars_ = objVars_;
        for (auto& g : *objVars_) g = make_int(0);
        if (tmpl) {
            funcsByIndex_ = tmpl->funcsByIndex_;
            inheritedFuncs_ = tmpl->inheritedFuncs_;
        } else {
            for (const auto& fn : obj_.functions)
                if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
            mergeInheritedFunctions();
        }
        objName_ = obj_.sourceName.empty() ? "<obj>" : obj_.sourceName;
    }

    const OriginalObject& object() const { return obj_; }
    // The shared program backing this VM.  Clones reuse the template's
    // read-only program instead of deep-copying it (saves ~1.5GB with
    // thousands of NPC clones).  VMs constructed from a reference keep their
    // own copy; the scheduler-process constructor already holds one.
    std::shared_ptr<const OriginalObject> sharedObject() const {
        if (objKeepAlive_) return objKeepAlive_;
        return std::make_shared<OriginalObject>(obj_);
    }
    const std::string& objName() const { return objName_; }
    const std::vector<OriginalInsn>* code() const { return code_; }
    size_t ip() const { return ip_; }
    // Public lookups for cross-object inline calls (case 8).
    const OriginalFunction* findFunctionAny(const std::string& name) const {
        for (const auto& fn : obj_.functions)
            if (fn.name == name && fn.index >= 0) return &fn;
        // Inherited functions: prefer a REAL implementation over a forward
        // declaration.  Inherit order gives each parent a distinct
        // baseFunOffset, so name.c's empty `mapping query_entire_dbase();`
        // (key = 0) and dbase.c's real implementation (key = 20) BOTH live in
        // funcsByIndex_ under different keys; a plain key-ascending scan
        // returned the forward, making room objects' query_entire_dbase()
        // return int 0 (empty body) and import/room.c's
        // `dbase << room_info` silently no-op - no room ever got its
        // npc/exits/alias data.  Take the LAST (largest key) non-empty
        // implementation: later inherits override earlier ones, matching the
        // original driver's name resolution.
        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_)
            if (kv.second->name == name && kv.second->index >= 0 && !kv.second->code.empty())
                impl = kv.second;   // keep the last (highest-key) implementation
        return impl;
    }
    // Public efun dispatch used by runPostInitFunctions for post-init closures
    // that wrap a builtin efun (ccs_clientd/tts_clientd create_thread).
    Value callEfunPublic(int efunIndex, const std::vector<Value>& args) {
        return callEfunWithArgs(efunIndex, args);
    }
    const OriginalFunction* functionByIndex(int index) const {
        auto it = funcsByIndex_.find(index);
        return it == funcsByIndex_.end() ? nullptr : it->second;
    }
    // The function-index table (used as the target "constitution" when a
    // cross-object inline frame switches to this object's functions).
    const std::map<int, const OriginalFunction*>& functionTable() const {
        return funcsByIndex_;
    }
    // True when this VM's function table contains fn (used to find the owning
    // VM of an inline cross-object frame for closure index resolution).
    bool ownsFunction(const OriginalFunction* fn) const {
        for (const auto& f : obj_.functions)
            if (&f == fn) return true;
        auto it = funcsByIndex_.find(fn->index);
        return it != funcsByIndex_.end() && it->second == fn;
    }
    // The shared object-variable table (cross-object inline calls switch to
    // the target object's table for the duration of the call).
    std::shared_ptr<std::vector<Value>> objVarsShared() { return objVars_; }
    // Mark this VM as the driver's master process (time-sliced startup).
    void setMasterProcess(bool v) { isMasterProcess_ = v; }
    // baseVarOffset of the currently executing function's constitution.
    // Global init code runs with no frame -> offset 0 (its own program).
    int currentVarOffset() const {
        return frames_.empty() ? 0 : frames_.back().varOffset;
    }
    std::string functionNameByIndex(int index) const {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return "";
        return it->second->name;
    }
    std::vector<Value>& objVars() { return *objVars_; }
    // Point this VM's instance-variable storage at another VM's (original
    // create_thread semantics: the thread shares the creator's thisObject,
    // so writes to instance vars are visible to other threads/callers).
    void shareObjVars(OriginalVm* other) {
        objVars_ = other->objVars_;
        // A shared table IS this VM's object context (create_thread copies
        // share the owner's instance variables); the cross-object entry
        // switch must treat it as "own" or it would always misfire on
        // daemon-process VMs.
        ownObjVars_ = objVars_;
    }
    // Set the object identity this_object() resolves to (clone instances get
    // their "clone:<rel>#<seq>" registry key so cross-object calls and
    // set_client_agent store the REAL object, not the "<this>" placeholder).
    void setThisObjectName(const std::string& n) { thisObjectName_ = n; }
    // Record the VM this clone was created from (object_template()).
    void setTemplateVm(const std::shared_ptr<OriginalVm>& t) { templateVm_ = t; }
    std::shared_ptr<OriginalVm> templateVm() const { return templateVm_; }
    const std::string& searchRoot() const { return searchRoot_; }
    // Record the caller's object identity before a cross-object entry runs on
    // this VM (previous_object() reads it back).  The value is the caller's
    // thisObjectName_ (registry key / clone key), so previous_object() can
    // build an object value resolving to that VM.
    void setPreviousObjectName(const std::string& n) { previousObjectName_ = n; }
    const std::string& previousObjectName() const { return previousObjectName_; }

    Value callFunctionByName(const std::string& name, const std::vector<Value>& args) {
        // Resolve through own + inherited functions (like vm_lookupIdentInProgram
        // on the global program used by vm_apply): a command function such as
        // msg_l_auth may live in an inherited program, not the master itself.
        // Cross-object entry: previous_object() inside the callee must see the
        // CALLER.  callFunctionByName is entered from call_other / case-8
        // owner-resolve paths where the caller's identity is known; record it
        // here (the caller sets previousObjectName_ on this VM before
        // invoking, see tryCallOtherObject and the case-8 owner branch).
        if (std::getenv("LPCVM_TRACE") &&
            (name == "lite_query_temp" || name == "lite_set_temp" || name == "set_temp" ||
             name == "query_temp" || name == "set"))
            std::cerr << "[lqt] callFunctionByName vm=" << this << " obj=" << objName_
                      << " fn=" << name << " args=" << args.size()
                      << " thisObj=" << thisObjectName_ << " a0=["
                      << (args.empty() ? "" : to_string(args[0], false).substr(0, 40)) << "]\n";
        if (std::getenv("LPCVM_TRACE") && (name == "register_entry" || name == "query_entry"))
            std::cerr << "[reg] callFunctionByName vm=" << this << " obj=" << objName_
                      << " fn=" << name << " args=" << args.size() << " a0=["
                      << (args.empty() ? "" : to_string(args[0], false).substr(0, 50)) << "]\n";
        if (const OriginalFunction* fn = findFunctionAny(name)) {
            if (std::getenv("LPCVM_TRACE") && name == "lite_query_temp")
                std::cerr << "[lqt] found idx=" << fn->index << " code=" << fn->code.size()
                          << " argCount=" << fn->argCount << "\n";
            if (std::getenv("LPCVM_TRACE") && name == "account_login")
                std::cerr << "[cfv-enter] account_login on obj=" << objName_
                          << " fnidx=" << fn->index << " code=" << fn->code.size()
                          << " args=" << args.size() << "\n";
            if (std::getenv("LPCVM_DBG_OBJVAR") && name == "query_entire_dbase") {
                std::string keys;
                if (objVars_->size() > 0 && (*objVars_)[0].index() == 6) {
                    auto m = std::get<std::shared_ptr<MappingValue>>((*objVars_)[0]);
                    auto items = m->items();
                    int show = std::min<int>(16, static_cast<int>(items.size()));
                    for (int i = 0; i < show; ++i)
                        keys += to_string(items[i].first, false).substr(0, 30) + " ";
                }
                std::cerr << "[qed-enter] obj=" << objName_
                          << " objVars=" << objVars_->size()
                          << " ownObjVars=" << ownObjVars_->size()
                          << " same=" << (objVars_ == ownObjVars_)
                          << " v0=" << (objVars_->size() > 0 ? (*objVars_)[0].index() : -1)
                          << " v1=" << (objVars_->size() > 1 ? (*objVars_)[1].index() : -1)
                          << " fnCode=" << fn->code.size()
                          << " fnIdx=" << fn->index
                          << " varOff=" << fn->varOffset
                          << " keys=[" << keys << "]\n";
            }
            // Cross-object entry on this VM: if an active cross-object INLINE
            // frame switched objVars_ to another object's table (e.g. a cfg
            // handler main inline on configd's VM calling configd's set_config
            // via the case-8 runDetached fallback), the callee must run in
            // THIS VM's own object context.  The frame records the switched
            // table (savedObjVarsForNextCall_) so its pop restores it - even
            // across a park/resume (the parked image keeps objVars_ = own).
            std::shared_ptr<std::vector<Value>> savedVars;
            if (objVars_ != ownObjVars_) {
                savedVars = objVars_;
                objVars_ = ownObjVars_;
                savedObjVarsForNextCall_ = savedVars;
            }
            Value r = executeFunctionFromOutside(*fn, args);
            if (savedVars && !parked_)
                objVars_ = savedVars;
            if (std::getenv("LPCVM_TRACE") && name == "mysql_select") {
                std::string msg = "[xret] mysql_select result=" + to_string(r, false) + "\n";
                std::cerr.write(msg.data(), static_cast<std::streamsize>(msg.size()));
                std::cerr.flush();
            }
            if (std::getenv("LPCVM_TRACE") && name == "account_login")
                std::cerr << "[cfv-exit] account_login done\n";
            return r;
        }
        if (std::getenv("LPCVM_TRACE") && name == "account_login")
            std::cerr << "[cfv-miss] account_login NOT FOUND on obj=" << objName_ << "\n";
        return make_int(0);
    }

    // vm_f_load_object cooperative path (stage B): register the object's VM
    // in the registry, spawn an "__init__" construction scheduler process that
    // runs the object's global init code + create() with full cooperative
    // yields, and PARK this VM on "obj-init:<key>" until construction done.
    // The original runs the construction inline on the loading process (so
    // create()'s blocking/yielding efuns park the whole process and it resumes
    // inside create later); a scheduler process + park reproduces that with
    // the single cooperative scheduler.
    Value loadObjectCooperative(const std::string& vpath, const std::vector<Value>& createArgs) {
        if (searchRoot_.empty() || vpath.empty()) return make_int(0);
        std::string rel = vpath;
        if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
        if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
        auto& reg = objectRegistry();
        std::string key;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                if (hasSuffix(kv.first, rel)) { key = kv.first; break; }
            }
        }
        if (!key.empty()) {
            // Already registered: if initialized, return the object now; if a
            // construction is in flight, join its wait (do not double-spawn).
            bool initd = false;
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                auto it = reg.initialized.find(key);
                initd = (it != reg.initialized.end() && it->second);
            }
            if (initd) return make_object(vpath);
            // Construction in flight (or failed): park on the same event.  The
            // resume hook re-checks the registry so a failed construction
            // resumes with 0 instead of a phantom object.
            ResumeCond cond;
            cond.kind = 3;
            cond.ename = "obj-init:" + key;
            auto hook = [this, vpath, key]() {
                bool ok = false;
                {
                    auto& reg2 = objectRegistry();
                    std::lock_guard<std::recursive_mutex> rl2(registryMutex());
                    auto it2 = reg2.initialized.find(key);
                    ok = (it2 != reg2.initialized.end() && it2->second);
                }
                resumeValue_ = ok ? make_object(vpath) : make_int(0);
            };
            parkAndSuspend(cond, std::move(hook), make_int(0));
            return make_int(0);   // unreachable (SuspendSignal thrown)
        }
        key = registerObjectVM(vpath, searchRoot_);
        if (key.empty()) return make_int(0);
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            reg.constructing.insert(key);
        }
        std::shared_ptr<OriginalVm> vm;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            auto it = reg.vms.find(key);
            if (it != reg.vms.end()) vm = it->second;
        }
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[loadobj-cop] " << vpath << " key=" << key
                      << " spawn __init__ createArgs=" << createArgs.size() << "\n";
        scheduler().spawnConstruction(vm, createArgs, "obj-init:" + key);
        // Park the caller; runConstruction raises the event when done.  The
        // resume hook re-checks the registry: a failed create() yields 0.
        ResumeCond cond;
        cond.kind = 3;
        cond.ename = "obj-init:" + key;
        auto hook = [this, vpath, key]() {
            bool ok = false;
            {
                auto& reg2 = objectRegistry();
                std::lock_guard<std::recursive_mutex> rl2(registryMutex());
                auto it2 = reg2.initialized.find(key);
                ok = (it2 != reg2.initialized.end() && it2->second);
            }
            resumeValue_ = ok ? make_object(vpath) : make_int(0);
        };
        parkAndSuspend(cond, std::move(hook), make_int(0));
        return make_int(0);   // unreachable (SuspendSignal thrown)
    }

    // Run a specific function pointer on THIS VM (with this VM's object vars).
    // Used by call_fun_ptr to run a closure's captured function on its owner VM.
    Value callFunctionPointer(const OriginalFunction* fn, const std::vector<Value>& args) {
        if (!fn) return make_int(0);
        return executeFunctionFromOutside(*fn, args);
    }

    Value callFunctionByIndex(int index) {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return make_int(0);
        return executeFunctionOnStack(*it->second);
    }

    int runInit(const std::vector<Value>& createArgs = {}) {
        std::lock_guard<std::recursive_mutex> lock(execMutex_);
        if (std::getenv("LPCVM_LOCK_TRACE"))
            std::cerr << "[runinit] enter obj=" << obj_.sourceName
                      << " t=" << std::this_thread::get_id() << "\n";
        try {
            // The original runs EVERY constitution's initCodeSegment in
            // initOrderList order (vm_callInInitFun 0x8094258 sets
            // consititution per program, so obj-var accesses inside the init
            // use that program's baseVarOffset).  The child's own global code
            // first (its constitution, offset 0), then each inherited
            // program's global code under its own accumulated offset.
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[runinit] global code obj=" << obj_.sourceName
                          << " t=" << std::this_thread::get_id() << "\n";
            execute(obj_.globalCode, obj_.globalMeta);
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[runinit] inherited inits obj=" << obj_.sourceName
                          << " t=" << std::this_thread::get_id() << "\n";
            runInheritedInits(0, 0);
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[runinit] create obj=" << obj_.sourceName
                          << " t=" << std::this_thread::get_id() << "\n";
            callCreate(createArgs);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_) + " (" + actionInfo(lastOp_).name + ")");
        }
        return 0;
    }

    int run() {
        try {
            execute(obj_.globalCode, obj_.globalMeta);
            runInheritedInits(0, 0);
            callCreate();
            runMainLoop(10, std::chrono::milliseconds(50));
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_) + " (" + actionInfo(lastOp_).name + ")");
        }
        return 0;
    }

    // Master startup run as a SCHEDULER process (stage B): executes the global
    // code (load_all), inherited inits and create() with a time slice so the
    // daemon processes created during load get to run interleaved (the
    // original driver's runTicks).  TimeSliceSignal propagates up to the
    // scheduler which requeues this VM at the tail of ready_; the VM's
    // execution state (stack_/frames_/ip_) is preserved, so the next
    // runAsMaster() call continues from the saved ip_ instead of restarting.
    // Returns normally only when the whole startup finished.
    void runAsMaster() {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[master-enter] phase=" << masterPhase_
                      << " code=" << (code_ ? code_->size() : -1)
                      << " ip=" << ip_ << " frames=" << frames_.size() << "\n";
        // Phase 0 (globalCode / load_all) may yield via relinquish; the
        // scheduler requeues this VM and runAsMaster() is called again with
        // code_/ip_ preserved - runLoop() continues where it left off.
        // Phases 1/2 (runInheritedInits / callCreate) are C++ loops that
        // cannot resume mid-way, so masterNoYield_ makes relinquish return
        // immediately and they run to completion synchronously.
        if (masterPhase_ <= 0) {
            if (!code_) {
                execute(obj_.globalCode, obj_.globalMeta);
            } else {
                runLoop();   // resumed after a yield
            }
            code_ = nullptr;
            if (masterPhase_ == 0) {
                masterPhase_ = 1;
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[master-phase] 0->1 inherited\n";
            }
        }
        if (masterPhase_ == 1) {
            masterNoYield_ = true;
            runInheritedInits(0, 0);
            masterNoYield_ = false;
            code_ = nullptr;
            masterPhase_ = 2;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[master-phase] 1->2 create\n";
        }
        if (masterPhase_ == 2) {
            masterNoYield_ = true;
            if (!masterCreateStarted_) {
                masterCreateStarted_ = true;
                callCreate();
            }
            masterNoYield_ = false;
            code_ = nullptr;
            masterPhase_ = 3;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[master-phase] 2->3 post_init\n";
            runPostInitFunctions(searchRoot_);
            masterInitDone_ = true;
        }
    }

    // Object construction as a SCHEDULER process (entry "__init__"), spawned
    // by load_object: run the object's global init code + create() exactly
    // like the original driver's vm_f_construct_object2 chain (inline on the
    // loading process, so create() can yield/park and resume through the
    // scheduler).  The phase machine mirrors runAsMaster: phase 0 (global
    // code) may yield and resumes via runLoop; phases 1/2 (inherited inits /
    // create) are C++ calls that cannot resume mid-way, so masterNoYield_
    // keeps relinquish from yielding inside them (create() that must yield
    // uses the SuspendSignal parking of its own blocking efuns instead).
    // When all phases finish, raise initDoneEvent_ so the parked load_object
    // caller resumes.
    void runConstruction(const std::vector<Value>& createArgs, const std::string& doneEvent) {
        initCreateArgs_ = createArgs;
        initDoneEvent_ = doneEvent;
        if (initPhase_ <= 0) {
            if (!code_) {
                execute(obj_.globalCode, obj_.globalMeta);
            } else {
                runLoop();   // resumed after a yield in the global code
            }
            code_ = nullptr;
            initPhase_ = 1;
        }
        if (initPhase_ == 1) {
            masterNoYield_ = true;
            runInheritedInits(0, 0);
            masterNoYield_ = false;
            code_ = nullptr;
            initPhase_ = 2;
        }
        if (initPhase_ == 2) {
            if (!initCreateStarted_) {
                initCreateStarted_ = true;
                masterNoYield_ = true;
                callCreate(initCreateArgs_);
                masterNoYield_ = false;
            }
            // create() finished (synchronously, or after a park-resume which
            // ran the rest of the create via resumeProcess).
            code_ = nullptr;
            initPhase_ = 3;
        }
        if (initPhase_ == 3) {
            // Construction finished: mark the object initialized in the
            // registry and wake the load_object caller parked on this event.
            markObjectInitialized();
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[construct-done] " << objName_ << " event=" << initDoneEvent_ << "\n";
            scheduler().wakeEventFront(initDoneEvent_);
            initPhase_ = 4;
        }
    }

    // Mark this VM's object as initialized in the registry (construction
    // process completion).  The registry keys for this VM's object get
    // initialized[key] = true so isObjectInitialized() / later load_object
    // calls short-circuit.
    void markObjectInitialized() {
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        for (auto& kv : reg.vms) {
            if (kv.second.get() == this || &kv.second->object() == &obj_) {
                reg.initialized[kv.first] = true;
                reg.initializedObj[&obj_] = true;
                reg.constructing.erase(kv.first);
            }
        }
    }

    // Execute the global initializer of every inherited program, each under
    // its own constitution (accumulated baseVarOffset + its own strings).
    // Recurses through the inherit chain; mirrors mergeInheritedFunctions.
    void runInheritedInits(int accFun, int accVar) {
        runInheritedInitsOf(obj_, accFun, accVar);
    }

    void runInheritedInitsOf(const OriginalObject& parent, int accFun, int accVar) {
        for (const auto& inh : parent.inherits) {
            std::shared_ptr<OriginalObject> parentObj;
            try {
                std::string rel = inh.path;
                if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
                if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
                if (auto p = loadOriginalObjectByRelPath(rel)) parentObj = p;
                else {
                    std::string cand = searchRoot_.empty() ? rel : searchRoot_ + "/" + rel;
                    parentObj = std::make_shared<OriginalObject>(loadOriginalObject(cand));
                }
            } catch (...) {
                if (std::getenv("LPCVM_DBG_OBJVAR"))
                    std::cerr << "[inhinit] FAIL load " << inh.path << " for " << obj_.sourceName << "\n";
                continue;
            }
            if (std::getenv("LPCVM_DBG_OBJVAR") && obj_.sourceName.find("/zone/") != std::string::npos)
                std::cerr << "[inhinit] " << obj_.sourceName << " <- " << inh.path
                          << " gc=" << parentObj->globalCode.size() << " vars=" << objVars_->size() << "\n";
            int funOff = accFun + inh.baseFunOffset;
            int varOff = accVar + inh.baseVarOffset;
            runInheritedInitsOf(*parentObj, funOff, varOff);
            // Execute the parent's own global init code under its constitution.
            Frame frame{nullptr, 0, 0, 0, &parentObj->strings};
            frame.varOffset = varOff;
            frames_.push_back(frame);
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[inhinit] executing " << parentObj->sourceName
                          << " varOff=" << varOff << " globalCode=" << parentObj->globalCode.size()
                          << " t=" << std::this_thread::get_id() << "\n";
            try {
                if (std::getenv("LPCVM_LOCK_TRACE"))
                    std::cerr << "[inhinit] BEFORE exec " << parentObj->sourceName
                              << " frames=" << frames_.size() << " t=" << std::this_thread::get_id() << "\n";
                execute(parentObj->globalCode, parentObj->globalMeta);
                if (std::getenv("LPCVM_LOCK_TRACE"))
                    std::cerr << "[inhinit] AFTER exec " << parentObj->sourceName
                              << " frames=" << frames_.size() << " ip=" << ip_
                              << " t=" << std::this_thread::get_id() << "\n";
            } catch (...) {
                frames_.pop_back();
                throw;
            }
            frames_.pop_back();
        }
    }

    void callCreate(const std::vector<Value>& createArgs = {}) {
        // The create() initializer may live in MULTIPLE programs along the
        // inherit chain, and the original driver runs EACH program's create()
        // during object construction (vm_f_construct_object2 walks the
        // initOrderList constitutions; every inherited program's create that
        // is not shadowed runs).  clone/npc/basic.o inherits
        // /gs/inherit/char/npc.c whose create() does lite_set("type", 4) -
        // without it importd.import() fails the type dispatch and the NPC's
        // npc.list attributes (including file_id) are never applied, so
        // find_basic_object_by_import_name("tianyongcheng/shouhutianshen.c")
        // misses and place_char can't clone the guard NPC.  Collect ALL
        // non-empty creates from the merged table, run them in inherit order
        // (ascending key = parent constitutions first, the object's own create
        // last), skipping duplicates of the same program.
        std::vector<const OriginalFunction*> creates;
        std::set<int> seenKeys;
        for (const auto& fn : obj_.functions) {
            if (fn.name == "create" && fn.index >= 0 && !fn.code.empty() && seenKeys.insert(fn.index).second)
                creates.push_back(&fn);
        }
        for (const auto& kv : funcsByIndex_) {
            if (kv.second->name == "create" && kv.second->index >= 0 &&
                !kv.second->code.empty() && seenKeys.insert(kv.first).second)
                creates.push_back(kv.second);
        }
        if (creates.empty()) return;
        std::sort(creates.begin(), creates.end(), [](const OriginalFunction* a, const OriginalFunction* b) {
            return a->index < b->index;
        });
        if (std::getenv("LPCVM_TRACE") && objName_.find("npc/basic") != std::string::npos) {
            std::string dbg;
            for (auto* c : creates) { if (!dbg.empty()) dbg += ","; dbg += std::to_string(c->index); }
            std::cerr << "[create-multi] obj=" << objName_ << " n=" << creates.size() << " idx=[" << dbg << "]\n";
        }
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_CREATE") && objName_.find("npc/basic") != std::string::npos) {
            for (auto* c : creates) {
                std::string src = c->strings && !c->strings->empty() ? "[strtable]" : "?";
                std::cerr << "[create-src] idx=" << c->index << " args=" << c->argCount
                          << " code=" << c->code.size() << " varOff=" << c->varOffset
                          << " funOff=" << c->funOffset << "\n";
            }
        }
        for (const OriginalFunction* createFn : creates) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[create] object=" << obj_.magic << " fn=" << createFn->name
                          << " index=" << createFn->index << " args=" << createFn->argCount
                          << " extArgs=" << createArgs.size() << "\n";
            auto it = funcsByIndex_.find(createFn->index);
            if (it != funcsByIndex_.end()) {
                std::vector<Value> args;
                // External (load_object) arguments are the object-creation
                // context; only pass them to creates that declare parameters
                // (handler.c's create(arg) reads arg["module"]).  A zero-arg
                // create ignores them, so keep its stack clean.
                if (!createArgs.empty() && it->second->argCount >= 1) {
                    for (size_t i = 0; i < createArgs.size(); ++i) args.push_back(createArgs[i]);
                }
                for (int i = static_cast<int>(args.size()); i < it->second->argCount; ++i)
                    args.push_back(Value{});   // missing create arg -> undefined (type 0)
                executeFunctionFromOutside(*it->second, args);
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
    // The cooperative scheduler resumes parked VMs (stage B); it must reach
    // resumeProcess/resumeCond/parkedResumeValue/parkAndSuspend.
    friend class Scheduler;

    // The original driver is a single-threaded program: one global program and
    // cooperatively scheduled lightweight threads.  We model threads with real
    // OS threads, so a shared VM (the registry copies used for cross-object
    // calls, e.g. the "<main>" copy) can be entered concurrently by several
    // threads.  Serialize execution per VM with a recursive mutex (recursive
    // because one VM's execute() may re-enter itself through closures/calls).
    mutable std::recursive_mutex execMutex_;
    // Recursion depth of execMutex_ for the CURRENT holder thread (only the
    // holder touches it).  runDetached() releases all of these to make a
    // cross-object call without holding this VM's lock, avoiding the AB-BA
    // deadlock between two threads calling into each other's objects.
    int execDepth_ = 0;

    const OriginalObject& obj_;
    // Keeps the referenced object alive when this VM was built from a
    // temporary copy (create_thread/set_timer scheduler processes): obj_ is a
    // reference member, so the shared_ptr here must outlive the VM.  Registry
    // VMs leave this null (the registry owns the object).
    std::shared_ptr<const OriginalObject> objKeepAlive_;
    std::string objName_;
    std::string searchRoot_;
    std::string thisObjectName_ = "<this>";   // object identity for this_object()
    // For CLONE instances: the VM this clone was created FROM (the original
    // driver's pTemplateObject).  object_template() returns it, and
    // basic_object.c's create() uses it to inherit my_import_name from the
    // template so get_name()/query() resolve through the basic object's dbase
    // (e.g. a place_char'd NPC clone reports its basic object's name).
    std::shared_ptr<OriginalVm> templateVm_;
    // The object that called INTO this VM's current function (previous_object).
    // Set at the cross-object call sites (call_other / callFunctionByName /
    // case-8 owner calls) to the caller's thisObjectName_ before the callee
    // runs; previous_object() returns it as an object value.  Empty when the
    // current entry was not a cross-object call (previous_object() then
    // returns 0, matching the original driver for a top-level call).
    std::string previousObjectName_;
    std::vector<StackItem> stack_;
    std::shared_ptr<std::vector<Value>> objVars_;  // per-object instance variables (shared across threads of the same object)
    // This VM's OWN object-variable table (never switched by cross-object
    // inline frames).  A top-level function call (external entry) must run in
    // this table even when an active inline frame switched objVars_ to another
    // object's table - otherwise the callee writes into the wrong table.
    std::shared_ptr<std::vector<Value>> ownObjVars_;
    // Cross-object entry switch (callFunctionByName): when an active inline
    // frame switched objVars_ and a cross-object call runs on THIS VM, the
    // next executeFunctionOnStack frame records the switched table here so
    // its pop restores it (survives park/resume via the frame's savedObjVars).
    std::shared_ptr<std::vector<Value>> savedObjVarsForNextCall_;
    std::vector<Frame> frames_;
    std::map<int, const OriginalFunction*> funcsByIndex_;
    // Active function-index table.  While a cross-object inline frame (from a
    // call_other to another object, e.g. filed.c read_content running on the
    // master's stack) is active this points at the TARGET object's
    // funcsByIndex_ so the callee's internal local-index calls resolve in its
    // own program; nullptr means this VM's own funcsByIndex_.
    const std::map<int, const OriginalFunction*>* funcLookup_ = nullptr;
    std::vector<std::shared_ptr<OriginalFunction>> inheritedFuncs_;  // owned copies of merged parents
    int lastOp_ = -1;
    size_t lastIp_ = 0;
    int stepCounter_ = 0;
    // Time-slice state (master process): after this many instructions the
    // runLoop yields (TimeSliceSignal) so daemon processes created during
    // load_all/post_init get scheduled interleaved (original runTicks).
    int yieldEvery_ = 0;
    int yieldCountdown_ = 0;
    bool isMasterProcess_ = false;
    bool masterInitDone_ = false;   // master startup (globalCode+create) finished
    int masterPhase_ = 0;           // 0=globalCode,1=inherited,2=create,3=done
    // While TRUE the master is inside runInheritedInits/callCreate (C++
    // functions that cannot be resumed mid-way after a yield); relinquish()
    // returns immediately instead of throwing TimeSliceSignal, so those
    // phases run to completion synchronously.
    bool masterNoYield_ = false;
    // The master's own create() already started (park-resume must not re-run
    // it; callCreate can park on load_object now that loading is cooperative).
    bool masterCreateStarted_ = false;
    // Object-construction state (entry "__init__" scheduler process, spawned
    // by load_object): runs the object's global init code + create() with the
    // same phase machine as the master, then raises initDoneEvent_ to wake the
    // parked load_object caller.
    int initPhase_ = 0;             // 0=globalCode,1=inherited,2=create,3=done
    bool initCreateStarted_ = false;   // create() began (park-resume must not re-run it)
    std::string initDoneEvent_;     // "obj-init:<key>" raised when construction finished
    std::vector<Value> initCreateArgs_;
    Value returnValue_ = make_int(0);
    bool returned_ = false;
    int catchDepth_ = 0;
    struct CatchRecord {
        size_t ipTarget = 0;     // resume ip when an error is caught
        size_t stackSize = 0;    // stack to unwind to
        size_t frameCount = 0;   // frames to keep
    };
    std::vector<CatchRecord> catches_;
    // Explicit-stack-machine execution context.  These are the current
    // instruction stream (code), its metadata and the next instruction index.
    // They live as members so a blocking efun can SAVE the whole execution
    // state (stack_/frames_/code_/meta_/ip_ are all members) and the
    // scheduler can resume by re-entering execute() from the saved point -
    // exactly like the original PCB's sp/funCp/ip restore.  Function calls
    // no longer recurse into a second execute(): they push a Frame carrying
    // the caller's (retCode/retMeta/retIp) and switch these members.
    const std::vector<OriginalInsn>* code_ = nullptr;
    const std::vector<uint8_t>* meta_ = nullptr;
    size_t ip_ = 0;
    // Cooperative-scheduler park state (stage B).  When a blocking efun throws
    // SuspendSignal, cond_.kind != 0 records what to wait for and the VM is
    // parked; resumeProcess(v) pushes v and re-enters runLoop at ip_.  All
    // execution state stays in the members above - nothing is moved or copied.
    ResumeCond cond_;
    // While parked, the top-level entry that owns this VM (callFunctionByName
    // from a create_thread/network thread) must NOT run cleanup; it re-throws
    // SuspendSignal and the thread exits, leaving the VM object owned by the
    // scheduler.  This flag is cleared by resumeProcess.
    bool parked_ = false;
    // Set by an efun that started a callee INLINE (frame pushed, stream
    // switched) - the callee's return op pushes the result, so the case-6
    // default path must NOT push the efun's return value.
    bool efunInlineStarted_ = false;
    // Value pushed when this parked VM resumes (parkAndSuspend stores it).
    Value resumeValue_ = make_int(0);

    const OriginalFunction* findFunction(const std::string& name) {
        for (const auto& fn : obj_.functions)
            if (fn.name == name && fn.index >= 0) return &fn;
        return nullptr;
    }

    // Search the object's own functions AND the inherited ones merged into
    // funcsByIndex_ (vm_findProgramFunction walks the inherit chain too).
    // Returns the merged-table pointer; its .index is the merged index when
    // the function came from funcsByIndex_ (inherited) - own functions keep
    // their original index which equals the merged one.

    // Resolve a function by name to its MERGED table entry, fixing the merged
    // index on inherited copies (their .index field still holds the parent
    // program's local index).  Returns nullptr when not found.
    const OriginalFunction* findMergedFunction(const std::string& name) const {
        for (const auto& fn : obj_.functions)
            if (fn.name == name && fn.index >= 0) return &fn;
        for (const auto& kv : funcsByIndex_)
            if (kv.second->name == name && kv.second->index >= 0) {
                if (kv.second->index != kv.first) {
                    // fix up the copied function's index so callers that
                    // re-look-up by index hit the right entry
                    auto mut = const_cast<OriginalFunction*>(kv.second);
                    mut->index = kv.first;
                }
                return kv.second;
            }
        return nullptr;
    }

    // Merge inherited functions into this object's function table.  The
    // compiler assigns each inherit a baseFunOffset / baseVarOffset (from the
    // INHERITS table) and indexes inherited functions as baseFunOffset +
    // <function index in the parent program>.  The parent may itself inherit
    // other programs, so the merge walks the inherit chain recursively and
    // ACCUMULATES both offsets (vm_loadBinaryInherit 0x80a2b58 does the same:
    // v25->baseFunOffset = pConsititutions->baseFunOffset + v29,
    // v25->baseVarOffset = pConsititutions->baseVarOffset + v27).
    void mergeInheritedFunctions() {
        // Resolve an inherit path (a mudlib-relative virtual path like
        // "/feature/daemon/register.c"): the mounted pak (binary directory)
        // first, then the extracted tree on disk.
        auto loadInheritFile = [this](const std::string& vpath) -> std::shared_ptr<OriginalObject> {
            std::string rel = vpath;
            if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
            if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
            if (auto p = loadOriginalObjectByRelPath(rel)) return p;
            std::string cand = searchRoot_.empty() ? rel : searchRoot_ + "/" + rel;
            return std::make_shared<OriginalObject>(loadOriginalObject(cand));
        };
        std::map<std::string, std::shared_ptr<OriginalObject>> loaded;
        std::function<void(const OriginalObject&, int, int)> walk;
        walk = [&](const OriginalObject& parent, int accFun, int accVar) {
            for (const auto& inh : parent.inherits) {
                std::shared_ptr<OriginalObject> parentObj;
                auto lit = loaded.find(inh.path);
                if (lit != loaded.end()) {
                    parentObj = lit->second;
                } else {
                    try {
                        parentObj = loadInheritFile(inh.path);
                    } catch (...) {
                        if (std::getenv("LPCVM_DBG_OBJVAR"))
                            std::cerr << "[inh-merge] FAIL load " << inh.path << " for "
                                      << obj_.sourceName << "\n";
                        continue;   // failed to load the inherited file
                    }
                    loaded[inh.path] = parentObj;
                }
                if (std::getenv("LPCVM_DBG_OBJVAR") && obj_.sourceName.find("/zone/") != std::string::npos)
                    std::cerr << "[inh-merge] " << obj_.sourceName << " <- " << inh.path
                              << " fns=" << parentObj->functions.size() << "\n";
                int funOff = accFun + inh.baseFunOffset;
                int varOff = accVar + inh.baseVarOffset;
                // Parent's own inheritance comes first (its function indexes
                // already include them), then the parent's functions.
                walk(*parentObj, funOff, varOff);
                // One shared copy of the parent's string table for ALL of its
                // inherited functions (copying per function was wasteful).
                auto parentTable = std::make_shared<const std::vector<std::string>>(parentObj->strings);
                for (const auto& fn : parentObj->functions) {
                    if (fn.index < 0) continue;
                    int childIndex = funOff + fn.index;
                    auto existing = funcsByIndex_.find(childIndex);
                    if (existing == funcsByIndex_.end()) {
                        auto inhFn = std::make_shared<OriginalFunction>(fn);
                        // inherited functions resolve shared-string indexes
                        // against the PARENT's string table, obj-var accesses
                        // against the accumulated baseVarOffset, and local
                        // function-index calls against the accumulated
                        // baseFunOffset
                        inhFn->strings = parentTable;
                        inhFn->varOffset = varOff;
                        inhFn->funOffset = funOff;
                        inheritedFuncs_.push_back(inhFn);
                        funcsByIndex_[childIndex] = inhFn.get();
                    } else if (!fn.code.empty() && existing->second->code.empty()) {
                        // An inherited forward declaration (empty code, e.g.
                        // name.c/import.c declaring `mapping query_entire_dbase();`
                        // with dlt index 0) merged BEFORE the real implementation
                        // (dbase.c's query_entire_dbase, also index 0) because
                        // inherit order in room.c lists name.c first.  The
                        // forward must NOT shadow the implementation: replace it
                        // with the concrete function (same index, real code).
                        // Without this, room objects' query_entire_dbase returns
                        // int 0 (empty body) and `dbase << room_info` in
                        // import/room.c silently no-ops, so no room ever gets
                        // its npc/exits/alias data.
                        auto inhFn = std::make_shared<OriginalFunction>(fn);
                        inhFn->strings = parentTable;
                        inhFn->varOffset = varOff;
                        inhFn->funOffset = funOff;
                        inheritedFuncs_.push_back(inhFn);
                        existing->second = inhFn.get();
                    }
                }
            }
        };
        walk(obj_, 0, 0);
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
    // Follow ref-to-ref chains (a ref slot that itself holds a ref, e.g. a
    // callee writing through an argument that is a reference to the caller's
    // local).
    Value deref(const StackItem& ref) const {
        if (ref.slotIndex >= 0) {
            if (static_cast<size_t>(ref.slotIndex) >= stack_.size()) return make_int(0);
            const StackItem& it = stack_[ref.slotIndex];
            if (it.isRef) return deref(it);
            return resolveCrossVmRef(it.value);
        }
        if (ref.globalIndex >= 0) {
            return sharedGlobal(ref.globalIndex);
        }
        if (ref.objIndex >= 0) {
            if (static_cast<size_t>(ref.objIndex) < objVars_->size()) return (*objVars_)[ref.objIndex];
            return make_int(0);
        }
        if (ref.arr) {
            int64_t i = toInt(ref.key);
            const auto& items = ref.arr->items;
            if (i >= 0 && i < static_cast<int64_t>(items.size())) return items[static_cast<size_t>(i)];
            return Value{};   // OOB -> original type 0 "undefined"
        }
        if (ref.map) {
            const Value* hit = ref.map->find(ref.key);
            if (hit) return *hit;
            return Value{};   // miss -> original type 0 "undefined"
        }
        return make_int(0);
    }

    // Follow a ref chain to its ultimate target StackItem (the innermost
    // non-ref slot).  Used by writeThrough so an assignment to a reference
    // argument updates the caller's variable, not the ref slot itself.
    // Returns the LAST ref item in the chain (or the input ref itself), i.e.
    // the item whose slotIndex/globalIndex/objIndex/arr/map identifies the
    // final destination slot.  A plain value slot has slotIndex == -1, so
    // writeThrough must NOT be pointed at it.
    const StackItem* followRefChain(const StackItem& ref) const {
        const StackItem* cur = &ref;
        int guard = 0;
        while (cur->isRef && guard++ < 64) {
            if (cur->slotIndex >= 0) {
                if (static_cast<size_t>(cur->slotIndex) >= stack_.size()) return cur;
                const StackItem& next = stack_[static_cast<size_t>(cur->slotIndex)];
                if (next.isRef) {
                    cur = &next;   // ref-to-ref: keep walking
                } else {
                    return cur;    // target is a plain value slot: stop here
                }
            } else {
                return cur;        // global / obj / member ref: terminal destination
            }
        }
        return cur;
    }

    void writeThrough(const StackItem& ref, const Value& v) {
        const StackItem* target = followRefChain(ref);
        if (!target) return;
        if (target->slotIndex >= 0) {
            if (static_cast<size_t>(target->slotIndex) < stack_.size()) {
                StackItem& slot = stack_[static_cast<size_t>(target->slotIndex)];
                if (slot.value.index() == 11) {
                    // The slot holds a CrossVmRef argument: the assignment
                    // must land in the CALLER VM's object variable, and the
                    // slot keeps the CrossVmRef so later `+=`/`=` writes keep
                    // forwarding (importd.create_data's attrib_index).
                    auto r = std::get<std::shared_ptr<CrossVmRef>>(slot.value);
                    OriginalVm* vm = static_cast<OriginalVm*>(r->vm);
                    if (vm && r->objIndex >= 0 && static_cast<size_t>(r->objIndex) < vm->objVars().size())
                        vm->objVars()[static_cast<size_t>(r->objIndex)] = v;
                    // leave slot.value as the CrossVmRef
                } else {
                    slot.value = v;
                }
            }
        } else if (target->globalIndex >= 0) {
            sharedGlobal(target->globalIndex) = v;
        } else if (target->objIndex >= 0) {
            if (static_cast<size_t>(target->objIndex) < objVars_->size()) (*objVars_)[target->objIndex] = v;
        } else if (target->arr) {
            int64_t i = toInt(target->key);
            auto& items = target->arr->items;
            if (i >= 0 && i < static_cast<int64_t>(items.size())) items[static_cast<size_t>(i)] = v;
        } else if (target->map) {
            target->map->insert(target->key, v);
        }
    }

    // Read the value at a stack slot; if the slot holds a ref, dereference it.
    Value slotValue(int idx) const {
        if (idx < 0 || static_cast<size_t>(idx) >= stack_.size()) return make_int(0);
        const StackItem& it = stack_[idx];
        if (it.isRef) return deref(it);
        return resolveCrossVmRef(it.value);
    }

    // Resolve a CrossVmRef argument to the value in the CALLER VM's object
    // variable (the callee reads the ref parameter through this).  Non-ref
    // values pass through unchanged.
    Value resolveCrossVmRef(const Value& v) const {
        if (v.index() == 11) {
            auto r = std::get<std::shared_ptr<CrossVmRef>>(v);
            OriginalVm* vm = static_cast<OriginalVm*>(r->vm);
            if (vm && r->objIndex >= 0 && static_cast<size_t>(r->objIndex) < vm->objVars().size())
                return vm->objVars()[static_cast<size_t>(r->objIndex)];
            return make_int(0);
        }
        return v;
    }

    int32_t metaOperand(const std::vector<uint8_t>& meta, uint16_t a, uint8_t b) const {
        return readI32(meta, static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 16));
    }

    // ------------------------------------------------------------------
    // Efun / function dispatch
    // ------------------------------------------------------------------
    // Call an efun by index, popping its arguments from the stack.
    Value callEfun(int efunIndex, int argCount) {
        std::vector<Value> args;
        for (int i = 0; i < argCount && !stack_.empty(); ++i) args.push_back(pop().value);
        Value r = callEfunWithArgs(efunIndex, args);
        return r;
    }

    // Call an efun by index with already-collected arguments.
    Value callEfunWithArgs(int efunIndex, const std::vector<Value>& args) {
        const auto& names = efunNameTable();
        // Watchdog record for a single efun call that runs >3s (a C++ spin
        // inside the efun implementation; LPC-level [vm-stuck] never fires).
        struct EfunWatch {
            std::string name, obj, fn;
            EfunWatch(std::string n, std::string o, std::string f)
                : name(std::move(n)), obj(std::move(o)), fn(std::move(f)) {
                using namespace std::chrono;
                std::lock_guard<std::mutex> lk(g_curEfunMx);
                g_curEfunName = name; g_curEfunObj = obj; g_curEfunFn = fn;
                g_curEfunStartMs = duration_cast<milliseconds>(
                    steady_clock::now().time_since_epoch()).count();
            }
            ~EfunWatch() {
                std::lock_guard<std::mutex> lk(g_curEfunMx);
                g_curEfunStartMs = 0;
            }
        };
        // The consumer thread in Scheduler::run() only starts under
        // LPCVM_STUCK_TRACE, so keep the producer (steady_clock stamp + two
        // mutex locks + several string copies, ~0.3us per efun call) gated the
        // same way - unconditional it measurably slows the boot/import phases
        // (millions of efun calls, e.g. importd create_data row parsing).
        static const bool efunWatchOn = (std::getenv("LPCVM_STUCK_TRACE") != nullptr);
        if (efunWatchOn) {
            std::string nm0 = (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size())
                                  ? names[efunIndex] : "?";
            std::string fn0 = "<global>";
            if (!frames_.empty() && frames_.back().fn) fn0 = frames_.back().fn->name;
            EfunWatch efunWatch(nm0, objName_, fn0);
        }
        // Per-efun CPU profiler (LPCVM_EFUN_PROF=1): accumulate wall time per
        // efun name and dump the top consumers every ~5s.  When the env var is
        // unset the cost is a single static-bool test per call.
        static const bool efunProfOn = (std::getenv("LPCVM_EFUN_PROF") != nullptr);
        if (efunProfOn) {
            using namespace std::chrono;
            struct EfunProf {
                std::string name;
                int64_t t0;
                EfunProf(std::string n)
                    : name(std::move(n)),
                      t0(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count()) {}
                ~EfunProf() {
                    int64_t dt = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count() - t0;
                    {
                        std::lock_guard<std::mutex> lk(g_efunProfMx);
                        EfunProfEntry& e = g_efunProf[name];
                        e.ns += dt;
                        e.calls += 1;
                    }
                    static int64_t lastDumpMs = 0;
                    int64_t nowMs =
                        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
                    if (nowMs - lastDumpMs < 5000) return;
                    lastDumpMs = nowMs;
                    std::vector<std::pair<std::string, EfunProfEntry>> all;
                    {
                        std::lock_guard<std::mutex> lk(g_efunProfMx);
                        all.assign(g_efunProf.begin(), g_efunProf.end());
                        g_efunProf.clear();
                    }
                    std::sort(all.begin(), all.end(),
                              [](const auto& a, const auto& b) { return a.second.ns > b.second.ns; });
                    std::cerr << "[efun-prof] top " << std::min<size_t>(14, all.size()) << " of "
                              << all.size() << ":\n";
                    for (size_t i = 0; i < all.size() && i < 14; ++i)
                        std::cerr << "  " << all[i].first << " " << all[i].second.ns / 1000000 << "ms ("
                                  << all[i].second.calls << " calls, avg "
                                  << (all[i].second.calls ? all[i].second.ns / all[i].second.calls : 0)
                                  << "ns)\n";
                }
            } efunProf((efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size())
                           ? names[efunIndex] : std::string("?"));
        }
        if (std::getenv("LPCVM_TRACE")) {
            std::string nm = (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size())
                                 ? names[efunIndex] : "?";
            std::string fn = "<global>";
            if (!frames_.empty() && frames_.back().fn) fn = frames_.back().fn->name;
            std::cerr << "[vm] efun idx=" << efunIndex << " name=" << nm << " argc=" << args.size()
                      << " in=" << fn << " obj=" << objName_ << "\n";
        }
        if (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size() && !names[efunIndex].empty()) {
            const std::string& nm = names[efunIndex];
            if (nm == "previous_object" || nm == "previous_objects") {
                // vm_f_previous_object (0x80f6b28): returns the object that
                // called into the current function (the caller's thisObject).
                // xlibd's safe_set uses `drama = previous_object()` to get the
                // task object whose dbase it validates before set() - a 0 stub
                // made every safe_set hit the "already set" error path.
                if (previousObjectName_.empty())
                    return make_int(0);
                return make_object(previousObjectName_);
            }
            if (nm == "error") {
                // vm_f_error (0x80d64bc): raise an LPC error.  The original
                // throws (aborting the current function, catchable by
                // new_thread_catch); a stub returning 0 let safe_set's error
                // path "succeed" silently, so the value was never stored and
                // the error string (with a large sprintf desc) leaked per
                // call - hundreds of calls ballooned GS to 2GB.
                std::string msg;
                for (size_t ai = 0; ai < args.size(); ++ai) {
                    if (ai) msg += " ";
                    msg += to_string(args[ai], false);
                }
                throw VmError(msg);
            }
            if (nm == "object_template") {
                // vm_f_object_template (0x80f4b9c): return the object this one
                // was cloned from (pTemplateObject); 0 when not a clone or no
                // template.  basic_object.c's create() uses it on clones to
                // inherit my_import_name from the template, which is what lets
                // a place_char'd NPC clone's get_name()/query() fall through to
                // its basic object's dbase (name 守护天神 etc.).
                if (templateVm_) {
                    std::string tp = templateVm_->thisObjectName_;
                    if (tp == "<this>" || tp.empty()) tp = templateVm_->objName();
                    return make_object(tp);
                }
                return make_int(0);
            }
            if (nm == "object_name" && args.empty()) {
                // vm_f_object_name with no argument returns the CURRENT
                // object's path (vm_currentContext->thisObject).  handler.c's
                // create() calls object_name() then explode("/") and takes
                // segment[1]; for that to yield the command name the name must
                // be "/<basename>", so return "/" + the file's basename.
                std::string n = thisObjectName_;
                if (n == "<this>" || n.empty()) n = objName_;
                size_t slash = n.find_last_of('/');
                std::string base = slash == std::string::npos ? n : n.substr(slash + 1);
                std::string res = "/" + base;
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[obj-name] this=" << thisObjectName_ << " -> " << res << "\n";
                return make_string(res);
            }
            if (nm == "call_other" && args.size() >= 2) {
                // vm_f_call_other (0x80f5aa0): call_other(ob, fun, a1..aN) -
                // the extra args are DIRECT arguments (vm_callInOtherFunEx
                // with type 5 and argNum-2 extra args).  ccs' master post_init
                // invokes it with 4 args.  Same inline cross-object machinery
                // as call_other_arg below, but the args arrive one-by-one.
                auto& reg0 = objectRegistry();
                const Value& obv = args[0];
                const Value& funv = args[1];
                std::string funName = funv.index() == 4 ? std::get<std::string>(funv) : "";
                std::vector<Value> callArgs(args.begin() + 2, args.end());
                std::shared_ptr<OriginalVm> targetVm;
                std::string path;
                if (obv.index() == 7) {
                    auto ov = std::get<std::shared_ptr<ObjectValue>>(obv);
                    path = ov->programName;
                    if ((path == "<this>" || path == "." || path.empty()) && ov->owner) {
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg0.vms)
                            if (&kv.second->object() == ov->owner) { targetVm = kv.second; break; }
                        if (targetVm) path = targetVm->objName();
                    }
                } else if (obv.index() == 4) {
                    path = std::get<std::string>(obv);
                }
                if (path.empty() || path == "<this>" || path == ".") {
                    return callFunctionByName(funName, callArgs);
                }
                if (!targetVm) {
                    {
                        std::string rel = path;
                        if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
                        if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg0.vms)
                            if (hasSuffix(kv.first, rel)) { targetVm = kv.second; break; }
                    }
                }
                const OriginalFunction* targetFn = nullptr;
                if (targetVm) targetFn = targetVm->findFunctionAny(funName);
                if (targetFn && targetFn->index >= 0) {
                    if (objName_.find('#') != std::string::npos) {
                        // Clone callers (NPC master-fire's save chain calling
                        // filed) must not run the callee's bytecode on the
                        // clone VM: filed's globals (srvQ etc.) would resolve
                        // against the clone's table.  Run on the target's
                        // registry VM instead.
                        return tryCallOtherObject(searchRoot_, path, funName, callArgs, thisObjectName_);
                    }
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[co-inline] " << objName_ << " -> "
                                  << (targetVm ? targetVm->objName() : "?") << "::" << funName
                                  << " argc=" << callArgs.size() << "\n";
                    const OriginalFunction& fn = *targetFn;
                    int declared = fn.argCount;
                    int pad = (declared > static_cast<int>(callArgs.size()))
                                  ? declared - static_cast<int>(callArgs.size()) : 0;
                    for (int i = 0; i < pad; ++i) pushValue(Value{});
                    for (int i = static_cast<int>(callArgs.size()) - 1; i >= 0; --i)
                        pushValue(callArgs[static_cast<size_t>(i)]);
                    if (declared == 0 && callArgs.empty()) {
                        pushValue(Value{});
                        if (startInlineFunction(fn, true, 0, targetVm->objVarsShared(),
                                                &targetVm->object().strings)) {
                            efunInlineStarted_ = true;
                            return make_int(0);   // callee's return op pushes the result
                        }
                        // Inline failed (shallow stack): run synchronously, but
                        // the callee is a CROSS-OBJECT function (e.g. filed.o's
                        // do_operate) - its globals (srvQ) must resolve in ITS
                        // object table.  startInlineFunction only switches the
                        // table when it succeeds, so switch here; the frame's
                        // savedObjVars restores the caller's table on pop
                        // (park-safe: the parked image keeps the callee table).
                        if (objVars_ != targetVm->objVarsShared() && !savedObjVarsForNextCall_) {
                            savedObjVarsForNextCall_ = objVars_;
                            objVars_ = targetVm->objVarsShared();
                        }
                        return executeFunctionOnStack(fn, true, 0);
                    }
                    bool inlOk = startInlineFunction(fn, false, static_cast<int>(callArgs.size()),
                                                     targetVm->objVarsShared(),
                                                     &targetVm->object().strings);
                    if (inlOk) {
                        efunInlineStarted_ = true;
                        return make_int(0);
                    }
                    if (objVars_ != targetVm->objVarsShared()) {
                        savedObjVarsForNextCall_ = objVars_;
                        objVars_ = targetVm->objVarsShared();
                    }
                    return executeFunctionOnStack(fn, false, static_cast<int>(callArgs.size()));
                }
                return tryCallOtherObject(searchRoot_, path, funName, callArgs, thisObjectName_);
            }
            if (nm == "call_other_arg" && args.size() >= 3) {
                // vm_f_call_other_arg (0x80f5afc): call_other(ob, fun,
                // para_list...) where para_list is an ARRAY expanded into
                // individual arguments (handler_module's process_command calls
                // call_other_arg(entry, "main", [agent, ...cmdargs])).  The
                // array is the LAST argument; the caller passes ob + fun.
                // Cross-object: run the callee INLINE on this VM's stack with
                // objVars switched (original driver switches constitution on
                // the same process; a blocking efun parks the whole process).
                auto& reg0 = objectRegistry();
                const Value& obv = args[0];
                const Value& funv = args[1];
                const Value& listv = args[2];
                std::string funName = funv.index() == 4 ? std::get<std::string>(funv) : "";
                std::vector<Value> callArgs;
                if (listv.index() == 5) {
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(listv);
                    callArgs = arr->items;
                } else {
                    for (size_t i = 2; i < args.size(); ++i) callArgs.push_back(args[i]);
                }
                std::shared_ptr<OriginalVm> targetVm;
                std::string path;
                if (obv.index() == 7) {
                    auto ov = std::get<std::shared_ptr<ObjectValue>>(obv);
                    path = ov->programName;
                    if ((path == "<this>" || path == "." || path.empty()) && ov->owner) {
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg0.vms)
                            if (&kv.second->object() == ov->owner) { targetVm = kv.second; break; }
                        if (targetVm) path = targetVm->objName();
                    }
                } else if (obv.index() == 4) {
                    path = std::get<std::string>(obv);
                }
                if (path.empty() || path == "<this>" || path == ".") {
                    return callFunctionByName(funName, callArgs);
                }
                if (true) {   // A/B: call_other_arg inline ON
                    if (!targetVm) {
                        {
                            std::string rel = path;
                            if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
                            if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            for (auto& kv : reg0.vms)
                                if (hasSuffix(kv.first, rel)) { targetVm = kv.second; break; }
                        }
                    }
                    const OriginalFunction* targetFn = nullptr;
                    if (targetVm) targetFn = targetVm->findFunctionAny(funName);
                    if (targetFn && targetFn->index >= 0) {
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[coa-inline] " << objName_ << " -> "
                                      << (targetVm ? targetVm->objName() : "?") << "::" << funName
                                      << " argc=" << callArgs.size() << "\n";
                        const OriginalFunction& fn = *targetFn;
                        int declared = fn.argCount;
                        int pad = (declared > static_cast<int>(callArgs.size()))
                                      ? declared - static_cast<int>(callArgs.size()) : 0;
                        for (int i = 0; i < pad; ++i) pushValue(Value{});
                        for (int i = static_cast<int>(callArgs.size()) - 1; i >= 0; --i)
                            pushValue(callArgs[static_cast<size_t>(i)]);
                        if (declared == 0 && callArgs.empty()) {
                            pushValue(Value{});
                            if (startInlineFunction(fn, true, 0, targetVm->objVarsShared(),
                                                    &targetVm->object().strings)) {
                                efunInlineStarted_ = true;
                                return make_int(0);   // callee's return op pushes the result
                            }
                            if (objVars_ != targetVm->objVarsShared()) {
                                savedObjVarsForNextCall_ = objVars_;
                                objVars_ = targetVm->objVarsShared();
                            }
                            return executeFunctionOnStack(fn, true, 0);
                        }
                        bool inlOk = startInlineFunction(fn, false, static_cast<int>(callArgs.size()),
                                                         targetVm->objVarsShared(),
                                                         &targetVm->object().strings);
                        if (inlOk) {
                            efunInlineStarted_ = true;
                            return make_int(0);
                        }
                        if (objVars_ != targetVm->objVarsShared() && !savedObjVarsForNextCall_) {
                            savedObjVarsForNextCall_ = objVars_;
                            objVars_ = targetVm->objVarsShared();
                        }
                        return executeFunctionOnStack(fn, false, static_cast<int>(callArgs.size()));
                    }
                }
                return tryCallOtherObject(searchRoot_, path, funName, callArgs, thisObjectName_);
            }
            if (nm == "is_clone" && args.empty()) {
                // vm_f_is_clone (0x80f7900) with no argument inspects the
                // CURRENT object (vm_currentContext->thisObject) and returns
                // (thisObject->flags & 1) - the clone flag.  account.o's
                // create() does `if (!is_clone()) return;` and only when the
                // object IS a clone proceeds to replace_entire_dbase() and
                // set_cmd_module(); returning 0 here skipped those, leaving
                // cmd_module unset so process_command could not forward.
                bool isClone = thisObjectName_.find('#') != std::string::npos;
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[is_clone] no-arg this=" << thisObjectName_
                              << " -> " << (isClone ? 1 : 0) << "\n";
                return make_int(isClone ? 1 : 0);
            }
            if (nm == "find_function" || nm == "find_function_arg") {
                // vm_f_find_function (0x80d626c) / vm_f_find_function_arg
                // (0x80d6347) -> vm_findFunction (0x8094ef4).
                // find_function(fn, extraArg, ...): extra args are BOUND onto a
                // new closure.
                // find_function_arg(fn, midArg..., lastArray): the mid args are
                // bound AND the last ARRAY is EXPANDED into individual bound
                // arguments (connectiond's process_command calls
                // (*find_function_arg(auth_func[cmd], agent, args))() where
                // args is the command argument array).
                if (nm == "find_function_arg" && args.size() >= 2 && args.back().index() == 5) {
                    std::vector<Value> bind;
                    for (size_t i = 1; i + 1 < args.size(); ++i) bind.push_back(args[i]);
                    auto arr = std::get<std::shared_ptr<ArrayValue>>(args.back());
                    for (const auto& e : arr->items) bind.push_back(e);
                    const Value& fn0 = args[0];
                    auto makeBound = [&](const std::shared_ptr<ClosureValue>& src) {
                        auto cl = std::make_shared<ClosureValue>();
                        cl->functionName = src->functionName;
                        cl->funcIndex = src->funcIndex;
                        cl->func = src->func;
                        cl->owner = src->owner;
                        cl->boundArgs = src->boundArgs;
                        for (const auto& b : bind) cl->boundArgs.push_back(b);
                        return cl;
                    };
                    if (fn0.index() == 8) return makeBound(std::get<std::shared_ptr<ClosureValue>>(fn0));
                    if (fn0.index() == 4) {
                        const std::string& fname = std::get<std::string>(fn0);
                        if (const OriginalFunction* fn = findMergedFunction(fname)) {
                            auto cl = std::make_shared<ClosureValue>();
                            cl->functionName = fname;
                            cl->funcIndex = fn->index;
                            cl->func = fn;
                            cl->owner = &obj_;
                            return makeBound(cl);
                        }
                        std::shared_ptr<OriginalVm> master;
                        const OriginalObject* masterObj = nullptr;
                        {
                            auto& reg = objectRegistry();
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            std::string key = searchRoot_.empty() ? "<main>" : searchRoot_ + "/<main>";
                            auto mit = reg.vms.find(key);
                            if (mit != reg.vms.end()) {
                                master = mit->second;
                                masterObj = &mit->second->object();
                            }
                        }
                        if (master && masterObj && masterObj != &obj_) {
                            const OriginalFunction* mfn = master->findFunctionAny(fname);
                            if (mfn) {
                                auto cl = std::make_shared<ClosureValue>();
                                cl->functionName = fname;
                                cl->funcIndex = mfn->index;
                                cl->func = mfn;
                                cl->owner = masterObj;
                                return makeBound(cl);
                            }
                        }
                        const auto& efn = efunNameTable();
                        for (size_t i = 0; i < efn.size(); ++i)
                            if (efn[i] == fname) return make_closure(fname, {});
                        return Value{};   // function not found -> type 0 "undefined"
                    }
                    return Value{};   // bad bind target -> type 0 "undefined"
                }
                auto bindClosure = [&](const std::shared_ptr<ClosureValue>& src) {
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = src->functionName;
                    cl->funcIndex = src->funcIndex;
                    cl->func = src->func;
                    cl->owner = src->owner;
                    cl->boundArgs = src->boundArgs;
                    for (size_t i = 1; i < args.size(); ++i)
                        cl->boundArgs.push_back(args[i]);
                    return cl;
                };
                if (!args.empty()) {
                    const Value& a = args[0];
                    if (a.index() == 8) {
                        return bindClosure(std::get<std::shared_ptr<ClosureValue>>(a));
                    }
                    if (a.index() == 4) {
                        const std::string& fname = std::get<std::string>(a);
                        if (const OriginalFunction* fn = findFunctionAny(fname)) {
                            auto cl = std::make_shared<ClosureValue>();
                            cl->functionName = fname;
                            cl->funcIndex = fn->index;
                            cl->func = fn;
                            cl->owner = &obj_;
                            return bindClosure(cl);
                        }
                        std::shared_ptr<OriginalVm> master;
                        const OriginalObject* masterObj = nullptr;
                        {
                            auto& reg = objectRegistry();
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            std::string key = searchRoot_.empty() ? "<main>" : searchRoot_ + "/<main>";
                            auto mit = reg.vms.find(key);
                            if (mit != reg.vms.end()) {
                                master = mit->second;
                                masterObj = &mit->second->object();
                            }
                        }
                        if (master && masterObj && masterObj != &obj_) {
                            const OriginalFunction* mfn = master->findFunctionAny(fname);
                            if (mfn) {
                                auto cl = std::make_shared<ClosureValue>();
                                cl->functionName = fname;
                                cl->funcIndex = mfn->index;
                                cl->func = mfn;
                                cl->owner = masterObj;
                                return bindClosure(cl);
                            }
                        }
                        const auto& efn = efunNameTable();
                        for (size_t i = 0; i < efn.size(); ++i)
                            if (efn[i] == fname) return make_closure(fname, {});
                        return Value{};   // function not found -> type 0 "undefined"
                    }
                    // bad type: option 0 -> undefined (no error)
                    return Value{};
                }
                return Value{};
            }
            if (nm == "broadcast") {
                // Pure log message (vm_f_broadcast).  Object loading is driven
                // by the LPC startup code (load_all_path/load_all -> load_object
                // in the master's global code), NOT by a C++ directory walk.
                std::string msg;
                if (!args.empty()) msg = to_string(args[0], false);
                if (!msg.empty()) std::cout << msg << "\n";
                return make_int(1);
            }
            if (nm == "create_thread") {
                if (std::getenv("LPCVM_TRACE")) {
                    std::string dbg;
                    for (auto& a : args) { if (!dbg.empty()) dbg += ","; dbg += to_string(a, false); }
                    std::cerr << "[cthread] args=[" << dbg << "] argc=" << args.size() << "\n";
                }
                // Original vm_f_create_thread (0x80ecbc0):
                //   arg0 = thread name (string) or mapping with
                //          name/stack_size/fun_context_size/catch_context_size,
                //   arg1 = the function to run, found via vm_findFunction(1,..)
                //          - a closure, or a string function name resolved in
                //            the CURRENT object (e.g. "daemon")
                //   arg2+ = arguments passed to the spawned function.
                std::string funName;
                std::vector<Value> threadArgs;
                const OriginalObject* owner = nullptr;
                if (args.size() >= 2) {
                    if (args[1].index() == 8) {
                        auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
                        funName = cl->functionName;
                        threadArgs = cl->boundArgs;
                        owner = cl->owner ? cl->owner : &obj_;
                    } else if (args[1].index() == 4) {
                        funName = std::get<std::string>(args[1]);
                        for (size_t i = 2; i < args.size(); ++i) threadArgs.push_back(args[i]);
                        owner = &obj_;
                    }
                } else if (args.size() == 1 && args[0].index() == 8) {
                    auto cl = std::get<std::shared_ptr<ClosureValue>>(args[0]);
                    funName = cl->functionName;
                    threadArgs = cl->boundArgs;
                    owner = cl->owner ? cl->owner : &obj_;
                }
                if (funName.empty()) return make_int(0);
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[cthread] fun=" << funName << " args=" << threadArgs.size() << "\n";
                // The original driver spawns the process immediately; the new
                // process shares the creator object's context (globals are
                // process-wide, instance vars are copied from the initialized
                // object).
                if (!owner) owner = &obj_;
                auto objPtr = std::make_shared<OriginalObject>(*owner);
                std::vector<Value> objVarsSnapshot;
                {
                    auto& reg = objectRegistry();
                    std::lock_guard<std::recursive_mutex> rl(registryMutex());
                    for (auto& kv : reg.vms) {
                        if (&kv.second->object() == owner) {
                            objVarsSnapshot = kv.second->objVars();
                            break;
                        }
                    }
                }
                std::string root = searchRoot_;
                bool isTelnetThread = funName == "telnet_daemon" || funName == "buildin_telnet_daemon";
                int telnetCno = -1;
                if (isTelnetThread && !threadArgs.empty() && threadArgs[0].index() == 2) {
                    telnetCno = static_cast<int>(std::get<int64_t>(threadArgs[0]));
                }
                // configd's update_config_daemon performs the first config load
                // (aaa/ccs/... tables from MySQL).  In the original driver the
                // cooperative scheduler ran it before the master's post_init(),
                // so do_init callbacks always saw populated config.  Our real
                // threads can finish update_config AFTER post_init starts,
                // making do_init's retry loop print (it only prints while the
                // MySQL data is not yet fetched).  Synchronously run the FIRST
                // update_config() on the owner's shared VM here so the data is
                // fetched before post_init - the daemon process still re-runs
                // update_config() every 180s as usual, so this does not change
                // the loop semantics, only the ordering.
                if (funName == "update_config_daemon") {
                    std::shared_ptr<OriginalVm> ownerVm;
                    {
                        auto& reg = objectRegistry();
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg.vms) {
                            if (&kv.second->object() == owner) { ownerVm = kv.second; break; }
                        }
                    }
                    if (ownerVm) {
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[config] sync first update_config before post_init\n";
                        try {
                            ownerVm->callFunctionByName("update_config", {});
                        } catch (const std::exception& e) {
                            std::cerr << "[config] sync update_config error: " << e.what() << "\n";
                        } catch (...) {
                        }
                    }
                }
                // Cooperative scheduler (stage B): ordinary daemon threads are
                // scheduler PROCESSES now - no OS thread.  The VM runs on the
                // scheduler thread; blocking efuns park it (SuspendSignal) and
                // the scheduler resumes it.  Telnet daemons stay OS threads for
                // now (they block on socket recv; stage F makes that event
                // driven too).
                if (!isTelnetThread) {
                    auto vm = std::make_shared<OriginalVm>(objPtr, root);
                    {
                        auto& reg = objectRegistry();
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg.vms) {
                            if (&kv.second->object() == owner) {
                                vm->shareObjVars(kv.second.get());
                                break;
                            }
                        }
                    }
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[sched] spawn " << funName << " args=" << threadArgs.size()
                                  << " owner=[" << (owner ? owner->sourceName : "NULL")
                                  << "] copyObj=[" << objPtr->sourceName << "]\n";
                    scheduler().spawn(vm, funName, threadArgs);
                    return make_int(1);
                }
                std::thread([objPtr, owner, funName, threadArgs, root, isTelnetThread, telnetCno,
                             objVarsSnapshot = std::move(objVarsSnapshot)]() {
                    if (isTelnetThread && telnetCno >= 0)
                        setThreadTelnetClient(telnetCno);
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[thread] run " << funName << " args=" << threadArgs.size() << "\n";
                    // The original driver's create_thread shares the creator's
                    // thisObject: the new PCB runs on the same object, so
                    // instance variables (configd's config_list, etc.) are
                    // shared with the creator and with other threads of the
                    // same object.  Each process still has its OWN stack /
                    // frame context, so we run on a fresh VM whose objVars
                    // SHARE the owner's objVars storage instead of copying.
                    try {
                        OriginalVm vm(*objPtr, root);
                        // Share the owner's instance-variable storage: point
                        // this VM's objVars at the registry owner's vector so
                        // update_config() writes are visible to get_my_info()
                        // callers (post_init do_init etc.).
                        {
                            auto& reg = objectRegistry();
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            for (auto& kv : reg.vms) {
                                if (&kv.second->object() == owner) {
                                    vm.shareObjVars(kv.second.get());
                                    break;
                                }
                            }
                        }
                        // First update_config_daemon iteration populates the
                        // shared config tables; post_init's do_init callbacks
                        // wait for that data with their own retry loops
                        // (while(!is_mapping) sleep(3)) exactly like the
                        // original driver - they only keep printing when the
                        // MySQL data cannot be fetched, not due to a race.
                        vm.callFunctionByName(funName, threadArgs);
                    } catch (const TelnetClosedError&) {
                        // Client disconnected: end the daemon thread quietly
                        // (matches the original driver killing the pended
                        // process on session close).
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[thread] " << funName << " client closed\n";
                    } catch (const std::exception& e) {
                        std::cerr << "[thread] " << funName << " error: " << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[thread] " << funName << " error: unknown\n";
                    }
                    setThreadTelnetClient(-1);
                }).detach();
                return make_int(1);
            }  // nm == "create_thread"
            if (nm == "register_post_init") {
                // Not intercepted (see callUserFunction): the master's LPC
                // post_init() runs registered closures inline from its global
                // code; the scheduler parks the master on blocking closures.
                return make_int(1);
            }
            if (nm == "set_timer" && args.size() >= 3) {
                // set_timer(timeout, id, function) - run the function once after
                // timeout seconds (the game re-registers repeating timers itself).
                const Value& fn = args[2];
                std::string fnName;
                std::vector<Value> bound;
                std::shared_ptr<ClosureValue> cl;
                if (fn.index() == 8) {
                    cl = std::get<std::shared_ptr<ClosureValue>>(fn);
                    fnName = cl->functionName;
                    bound = cl->boundArgs;
                } else if (fn.index() == 4) {
                    fnName = std::get<std::string>(fn);
                }
                if (!fnName.empty()) {
                    int64_t timeout = std::max<int64_t>(1, toInt(args[0]));
                    // The timer callback runs in the context of the object that
                    // owns the closure (cl->owner); string callbacks belong to
                    // the current VM's object.  We NEVER re-run the object's
                    // initializer here - that would restart the whole driver
                    // (create() re-registers threads and timers).  Instead the
                    // callback VM gets the initialized instance variables.
                    const OriginalObject* owner = cl ? cl->owner : nullptr;
                    if (!owner) owner = &obj_;
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[timer] schedule " << fnName << " in " << timeout << "s\n";
                    // arg2 is the timer name (string) or 0/&ref (anonymous).
                    std::string timerName;
                    if (args.size() >= 2 && args[1].index() == 4)
                        timerName = std::get<std::string>(args[1]);
                    auto entry = std::make_shared<TimerEntry>();
                    entry->id = g_timerSeq.fetch_add(1);
                    entry->name = timerName;
                    entry->callback = fnName;
                    entry->timeoutSecs = timeout;
                    entry->triggerAt = static_cast<int64_t>(std::time(nullptr)) + timeout;
                    entry->cancelled = std::make_shared<std::atomic<bool>>(false);
                    {
                        // Insert into the trigger-time-ordered list (original
                        // vm_putTimerIntoTimerList).  Same-name timers coexist;
                        // the game's delete-then-set pattern is driven by LPC.
                        std::lock_guard<std::mutex> lk(g_timerMtx);
                        auto it = std::lower_bound(
                            g_timerList.begin(), g_timerList.end(), entry,
                            [](const std::shared_ptr<TimerEntry>& a,
                               const std::shared_ptr<TimerEntry>& b) {
                                return a->triggerAt < b->triggerAt;
                            });
                        g_timerList.insert(it, entry);
                        g_timerById[entry->id] = entry;
                    }
                    auto objPtr = std::make_shared<OriginalObject>(*owner);
                    // The timer callback runs in the object's OWN context like
                    // the original driver's cooperative timer fire: the SAME
                    // process/Vm that owns the object (searchRoot for
                    // call_other resolution, shared instance vars, and closure
                    // owners pointing at the registry object so a queued
                    // closure (save_info -> threadd dispatch_service ->
                    // save_info_to_db) resolves back to this VM).  Stage B:
                    // the callback is a SCHEDULER process fired at triggerAt
                    // (the original driver's event loop runs timer callbacks
                    // on the single thread); delete_timer cancels it via the
                    // entry's cancelled flag.
                    std::shared_ptr<OriginalVm> ownerVm;
                    {
                        auto& reg = objectRegistry();
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : reg.vms) {
                            if (&kv.second->object() == owner) { ownerVm = kv.second; break; }
                        }
                    }
                    std::shared_ptr<OriginalVm> timerVm = ownerVm;
                    if (!timerVm) {
                        // Object not registered (rare): fall back to a fresh
                        // VM whose obj vars mirror the snapshot.
                        timerVm = std::make_shared<OriginalVm>(objPtr);
                    }
                    if (std::getenv("LPCVM_LOCK_TRACE"))
                        std::cerr << "[timer] scheduling " << fnName
                                  << " on scheduler t=" << std::this_thread::get_id() << "\n";
                    using namespace std::chrono;
                    int64_t atMs = duration_cast<milliseconds>(
                        steady_clock::now().time_since_epoch()).count() + timeout * 1000;
                    scheduler().spawnDelayed(timerVm, fnName, bound, atMs, entry);
                    return make_int(entry->id);
                }
            }
            if (nm == "get_timer") {
                // vm_f_get_timer (0x80ef470): get_timer(name|id) -> info
                // mapping {callback,timeout,output,id,name}, or 0.  The game
                // tests truthiness or reads ["timeout"].
                std::shared_ptr<TimerEntry> e;
                if (!args.empty()) {
                    if (args[0].index() == 4)
                        e = timerFindByName(std::get<std::string>(args[0]));
                    else if (args[0].index() == 2)
                        e = timerFindById(toInt(args[0]));
                }
                if (!e) return make_int(0);
                // Original vm_f_get_timer walks the sorted list from the head,
                // accumulating each node's timeoutTicks (relative delta); the
                // matched timer's timeout = accumulated ticks / 1000.  Mirror
                // that: head delta = first trigger - now, later deltas = the
                // trigger-time gap, so timeout is the absolute trigger offset.
                double acc = 0.0;
                {
                    // Accumulate every node's timeoutTicks up to the match
                    // (v3 += FirstTimer->timeoutTicks), exactly like the
                    // original: head delta = trigger - now, later deltas =
                    // trigger-time gaps.
                    std::lock_guard<std::mutex> lk(g_timerMtx);
                    int64_t now = static_cast<int64_t>(std::time(nullptr));
                    int64_t prev = 0;
                    for (const auto& ent : g_timerList) {
                        int64_t delta = (prev == 0) ? (ent->triggerAt - now)
                                                     : (ent->triggerAt - prev);
                        acc += static_cast<double>(delta);
                        if (ent == e) break;
                        prev = ent->triggerAt;
                    }
                }
                auto tmap = std::make_shared<MappingValue>();
                // Original callback is "(: fn@/path/file.c :)"; we only keep
                // the function name, render the closest form.
                tmap->insert(make_string("callback"),
                             make_string("(: " + e->callback + " :)"));
                tmap->insert(make_string("timeout"), make_float(acc));
                tmap->insert(make_string("id"), make_int(e->id));
                tmap->insert(make_string("name"), make_string(e->name));
                // Timers created via set_timer have outFd = 0 -> "Disabled"
                // (probed on the original: output is "Disabled").
                tmap->insert(make_string("output"), make_string("Disabled"));
                return tmap;
            }
            if (nm == "delete_timer") {
                // vm_f_delete_timer (0x80ef404): 0 on success, -601 when the
                // timer does not exist.
                std::shared_ptr<TimerEntry> e;
                if (!args.empty()) {
                    if (args[0].index() == 4)
                        e = timerFindByName(std::get<std::string>(args[0]));
                    else if (args[0].index() == 2)
                        e = timerFindById(toInt(args[0]));
                }
                if (!e) return make_int(-601);
                timerUnregister(e);
                return make_int(0);
            }
            if (nm == "sort" && args.size() >= 2 && args[0].index() == 5) {
                // vm_f_sort (0x80dbf4c): sort(array, cmp_func) sorts ascending by
                // the comparison function's result.  cmd_l_get_server_list does
                //   sort(values(server_list), (: sort_by_server_id :))
                // where sort_by_server_id(data) returns data["id"].
                auto src = std::get<std::shared_ptr<ArrayValue>>(args[0]);
                std::vector<Value> items = src->items;
                Value cmp = args[1];
                auto invokeCmp = [&](const Value& v) -> Value {
                    if (cmp.index() != 8) return make_int(0);
                    auto cl = std::get<std::shared_ptr<ClosureValue>>(cmp);
                    std::vector<Value> all = cl->boundArgs;
                    all.push_back(v);
                    const OriginalObject* master = objectRegistry().masterObj;
                    if (cl->func && (cl->owner == nullptr || cl->owner == &obj_ || cl->owner == master)) {
                        return executeFunctionFromOutside(*cl->func, all);
                    }
                    if (cl->owner && cl->owner != &obj_) {
                        auto& reg = objectRegistry();
                        std::shared_ptr<OriginalVm> ownerVm;
                        {
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            for (auto& kv : reg.vms)
                                if (&kv.second->object() == cl->owner) { ownerVm = kv.second; break; }
                        }
                        if (ownerVm) {
                            if (cl->func) return ownerVm->callFunctionPointer(cl->func, all);
                            return ownerVm->callFunctionByName(cl->functionName, all);
                        }
                    }
                    return make_int(0);
                };
                std::stable_sort(items.begin(), items.end(), [&](const Value& a, const Value& b) {
                    return toInt(invokeCmp(a)) < toInt(invokeCmp(b));
                });
                auto out = std::make_shared<ArrayValue>();
                out->items = std::move(items);
                return out;
            }
            if (nm == "relinquish") {
                // vm_f_relinquish: the LPC startup code (load_all etc.) calls
                // relinquish() to yield the CPU to other processes (daemon
                // processes created so far) - the original cooperative
                // scheduler's voluntary yield.  We throw TimeSliceSignal; the
                // scheduler requeues this process at the tail of ready_ and
                // its VM state (stack_/frames_/ip_) is preserved, so it
                // resumes here (returning 1) after the other processes ran.
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[relinquish] vm=" << this << " obj=" << objName_
                              << " ip=" << ip_ << " frames=" << frames_.size() << "\n";
                if (masterNoYield_) {
                    // Inside runInheritedInits/callCreate (C++ loops that
                    // cannot resume mid-way): run synchronously, no yield.
                    return make_int(1);
                }
                // The caller expects relinquish() to return; push 1 now, then
                // yield AFTER the push so the resumed process sees the value.
                pushValue(make_int(1));
                throw TimeSliceSignal{this};
            }
            if (nm == "accept") {
                // vm_f_accept (0x80e53c4): accept(fd, &addr, &port) -> new fd
                // or -1.  ccs_serverd's accept_daemon runs `while(1) { if ((fd
                // = accept(...)) < 0) continue; ... }` as a SCHEDULER process.
                // The builtins accept() BLOCKS on the scheduler thread, which
                // freezes the whole driver once no connection is pending
                // (ccs accepted the gs once, then its log stopped dead at the
                // next accept()).  Park on the listener socket (kind 5,
                // wantRead) and call the real accept only when readable.
                if (args.empty()) return make_int(-1);
                int fd = static_cast<int>(toInt(args[0]));
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[accept] fd=" << fd << " park-wait\n";
                ResumeCond cond;
                cond.kind = 5;
                cond.fd = fd;
                cond.wantRead = true;
                auto hook = [this, fd]() {
                    // Listener readable: accept now (non-blocking enough - a
                    // pending connection is present).
                    resumeValue_ = make_int(xdbcAccept(fd));
                };
                parkAndSuspend(cond, std::move(hook), make_int(-1));
                return make_int(-1);   // unreachable (SuspendSignal thrown)
            }
            if (nm == "connect") {
                // vm_f_connect (0x80e83b4): connect(fd, host, port, timeout).
                // The original performs a NON-BLOCKING connect: on EINPROGRESS
                // the process parks on the socket and the cooperative scheduler
                // wakes it when the socket becomes writable (or the timeout
                // fires).  Returns 0 on success, -1 on failure.
                if (args.size() >= 3) {
                    int fd = static_cast<int>(toInt(args[0]));
                    std::string host = to_string(args[1], false);
                    int port = static_cast<int>(toInt(args[2]));
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[connect] fd=" << fd << " " << host << ":" << port << "\n";
                    int rc = xdbcConnectAsync(fd, host, port);
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[connect] rc=" << rc << "\n";
                    if (rc == 0) return make_int(0);          // connected immediately
                    if (rc == -1) return make_int(-1);        // hard failure
                    // EINPROGRESS: park until the socket is writable.
                    ResumeCond cond;
                    cond.kind = 5;
                    cond.fd = fd;
                    cond.wantWrite = true;
                    auto hook = [this, fd]() {
                        // Socket writable: check the connect result.
                        resumeValue_ = make_int(xdbcConnectResult(fd));
                    };
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[connect] parking fd=" << fd << " kind=" << cond.kind << "\n";
                    parkAndSuspend(cond, std::move(hook), make_int(0));
                    return make_int(0);   // unreachable (SuspendSignal thrown)
                }
                return make_int(-1);
            }
            if (nm == "load_object") {
                // vm_f_load_object (0x80f8cbc): load + construct the object.
                // The original runs the construction (global init code +
                // create()) INLINE on the current process, so a create() that
                // blocks (queue/event/socket) parks the WHOLE process and the
                // scheduler resumes it inside create() later - the "Load: ...
                // OK" line prints only AFTER create() finished (reference
                // dba.log: 143-161 configd create + handlers, then 162 "Load:
                // /daemons/configd.c... OK").  Mirror that with a construction
                // SCHEDULER process (entry "__init__"): register the object's
                // VM, spawn the construction, and PARK this VM on the
                // "obj-init:<key>" event until construction finished, then
                // resume with the object value.
                if (args.empty()) return make_int(0);
                std::string vpath;
                if (args[0].index() == 4) vpath = std::get<std::string>(args[0]);
                else if (args[0].index() == 2) vpath = std::to_string(std::get<int64_t>(args[0]));
                else vpath = to_string(args[0], false);
                std::vector<Value> createArgs;
                if (args.size() >= 2) createArgs.push_back(args[1]);
                return loadObjectCooperative(vpath, createArgs);
            }
            // Remember WHO is calling before falling through to builtins.cpp,
            // so [efun-stub] lines identify object::function.
            stubCallCtx().obj = objName_;
            stubCallCtx().fn = frames_.empty() || !frames_.back().fn
                                   ? "<global>" : frames_.back().fn->name;
            return call_builtin(nullptr, nm, args);
        }
        return make_int(0);
    }

    // vm_callInUserFun (0x8094080): bit 7 of the flags selects the driver's
    // GLOBAL program (start_aaa) function table, NOT this object's table.
    // vm_callInUserFunBody then runs that function INLINE on the current
    // stack/process (thisObject=NULL, consititution = the global program's),
    // so it never switches object and never takes another VM's execution
    // lock.  The old path forwarded it to the master VM as a cross-object
    // call, which deadlocked account_login (game thread -> accountd
    // execMutex_) against _check_client_data (threadd thread -> accountd
    // execMutex_ -> master execMutex_ for set_dynamic_key).
    // Returns true when the callee was started INLINE on the current stack
    // (a frame was pushed and code_/meta_/ip_ switched; the callee's
    // return_from_fun will push its own result, so the caller must NOT push
    // one).  Returns false when the call COMPLETED synchronously and result
    // holds the return value.
    bool callUserFunction(int index, int flags, Value& result) {
        result = make_int(0);
        if (flags & 0x80) {
            if (const OriginalFunction* gfn = findMasterFunction(index)) {
                // register_post_init is NOT intercepted: the master runs
                // LPC post_init() from its global code on aaa/ccs/dba/gs
                // alike, iterating the LPC _post_init_funcs array.  The
                // master is a SCHEDULER process, so a blocking closure
                // (wait_config -> wait_event) throws SuspendSignal which
                // the scheduler catches and parks - exactly the original
                // cooperative driver.  Stealing the closure into the C++
                // postInitFuncs() (which only drains after the master's
                // global code RETURNS) starved the LPC array on gs, whose
                // global code never returns (post_init() then
                // init_service_module() run inline, and init blocks on
                // try_to_sync_time waiting for the ccs_clientd daemon that
                // the stolen create_thread closure was supposed to spawn).
                if (std::getenv("LPCVM_TRACE")) {
                    std::string cur = frames_.empty() || !frames_.back().fn ? "<global>" : frames_.back().fn->name;
                    std::cerr << "[gfn] inline global fn=" << gfn->name << " idx=" << index
                              << " argc=" << (flags & 0x3F) << " vm=" << this
                              << " obj=" << objName_ << " cur=" << cur
                              << " frames=" << frames_.size() << " stack=" << stack_.size() << "\n";
                    std::cerr.flush();
                }
                if (startInlineFunction(*gfn, false, flags & 0x3F)) return true;
                if (gfn->name == "mysql_select" || gfn->name == "mixed_mysql_select")
                    std::cerr << "[gfn] " << gfn->name << " START FAILED (stack too shallow)\n";
                return false;
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[gfn] global fn idx=" << index << " NOT FOUND in master\n";
            return false;
        }
        const std::map<int, const OriginalFunction*>& tab =
            funcLookup_ ? *funcLookup_ : funcsByIndex_;
        auto it = tab.find(index);
        if (it != tab.end()) {
            const OriginalFunction& fn = *it->second;
            if (std::getenv("LPCVM_TRACE") &&
                ((index >= 50 && index <= 60) || (objName_.find("connectiond") != std::string::npos && index <= 15) ||
                 objName_.find("config") != std::string::npos))
                std::cerr << "[cfu] local fn=" << fn.name << " idx=" << index
                          << " obj=" << objName_ << " code=" << fn.code.size() << "\n";
            // A nodes==0 entry is a forward declaration (dlt reference) whose
            // bytecode lives in the program that actually implements it (e.g.
            // feature/common/agent.o declares get_client_no/set_client_no/
            // relay_comm but the real bodies are comm.c's).  Resolve to the
            // implementing copy by name so relay_agent's local calls work.
            const OriginalFunction* impl = &fn;
            if (fn.code.empty()) {
                for (const auto& kv : tab) {
                    if (kv.second->name == fn.name && !kv.second->code.empty() && kv.second != &fn) {
                        impl = kv.second;
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[cfu] fwd " << fn.name << " idx=" << index
                                      << " -> impl idx=" << kv.first
                                      << " varOff=" << kv.second->varOffset
                                      << " funOff=" << kv.second->funOffset << "\n";
                        break;
                    }
                }
                if (impl == &fn) {
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[cfu] fwd " << fn.name << " idx=" << index << " NO IMPL\n";
                    // No implementation anywhere: keep a zero result but still
                    // consume the declared arguments.
                    for (int i = 0; i < fn.argCount && !stack_.empty(); ++i) pop();
                    return false;
                }
            }
            bool traceMe = impl->name == "mixed_mysql_select" || impl->name == "get_info_by_type" ||
                           impl->name == "get_aaa_info" || impl->name == "get_my_info" ||
                           impl->name == "fetch_config_list" || impl->name == "assure_connection" ||
                           impl->name == "mysql_select" || impl->name == "update_config" ||
                           impl->name == "choose_one_url" || impl->name == "do_init";
            bool authTrace = impl->name == "register_auth" || impl->name == "register_ban" ||
                             impl->name == "do_when_new_connection" || impl->name == "auth_connection" ||
                             impl->name == "register_disconnect" || impl->name == "auth_in_config_list" ||
                             impl->name == "auth_in_config_info" || impl->name == "mysql_select" ||
                             impl->name == "mixed_mysql_select";
            if (authTrace) {
                int ac = flags & 0x3F;
                std::string adbg;
                for (int k = 0; k < ac && k < 6; ++k) {
                    if (k) adbg += ",";
                    size_t si = stack_.size() - static_cast<size_t>(ac) + static_cast<size_t>(k);
                    adbg += (si < stack_.size()) ? to_string(stack_[si].value, false).substr(0, 40) : "?";
                }
                std::cerr << "[auth] " << impl->name << " obj=" << objName_
                          << " argc=" << ac << " args=[" << adbg << "]\n";
            }
            // Pad missing DECLARED arguments with undefined so the callee's
            // param slots never alias stale caller stack data.  case 7 pops
            // nothing for the callee - the bytecode pushed only the args it
            // had (fill_pic_pool calls generate_hanzi_info() with 0 args while
            // it declares char_arr); without padding, bp = stack.size()-1
            // points at the CALLER's stale top and the callee's first param
            // reads garbage (a dangling array crashed picd's second
            // generate_hanzi_info call in `loop (str in chars)`).
            {
                int ac = flags & 0x3F;
                int declared = impl->argCount;
                int pad = (declared > ac) ? declared - ac : 0;
                if (pad) {
                    // param0 must sit on top; missing declared params are the
                    // LOW slots below the passed ones, exactly like
                    // pushArgsForFunction.  The bytecode already pushed the
                    // real args (param0 on top): pull them off, push the pad
                    // undefineds, then push the real args back so param0 ends
                    // on top and bp = stack.size()-1 lands on it.
                    std::vector<StackItem> args2;
                    for (int i = 0; i < ac && !stack_.empty(); ++i) args2.push_back(pop());
                    for (int i = 0; i < pad; ++i) pushValue(Value{});
                    for (auto it = args2.rbegin(); it != args2.rend(); ++it) stack_.push_back(*it);
                }
                // The callee inherits the CURRENT frame's string table (a
                // cross-object inline frame switched to the target program; a
                // local private call like filed.do_operate must resolve its
                // shared strings in filed's table, not this VM's own object).
                const std::vector<std::string>* curTbl =
                    frames_.empty() ? nullptr : frames_.back().strings;
                if (std::getenv("LPCVM_NO_CONST")) curTbl = nullptr;
                {
                    static const char* filedFns[] = {"do_operate", "do_operate_ex", "read_content",
                                                     "read_content_to", "write_content", "write_content_to",
                                                     "create_content", "create_content_to", "process_request",
                                                     "fetch_data", "write_data"};
                    for (auto* ff : filedFns) {
                        if (impl->name == ff && std::getenv("LPCVM_SVC_TRACE")) {
                            std::string chain;
                            for (int fi = 0; fi < static_cast<int>(frames_.size()) && fi < 5; ++fi) {
                                if (!chain.empty()) chain += "<-";
                                chain += frames_[static_cast<size_t>(fi)].fn
                                             ? frames_[static_cast<size_t>(fi)].fn->name : "?";
                            }
                            std::cerr << "[filed-call] " << impl->name << " obj=" << objName_
                                      << " vm=" << this
                                      << " frames=" << frames_.size()
                                      << " chain=[" << chain << "]"
                                      << " lookup=" << (funcLookup_ ? "func" : "byIndex")
                                      << " varOff=" << impl->varOffset << "\n";
                            break;
                        }
                    }
                }
                if (startInlineFunction(*impl, false, ac + pad, nullptr, curTbl)) return true;
            }
            if (authTrace)
                std::cerr << "[auth] " << impl->name << " START FAILED (stack too shallow)\n";
            return false;
        }
        // Cross-object function table fallback (inherited/global functions).
        // The arguments are still on this stack: transfer them to the callee.
        int argCount = flags & 0x3F;
        auto& reg = objectRegistry();
        // Global functions (mysql_select, mixed_mysql_select, load_all, ...)
        // live in the driver's master program (start_aaa).  Resolve them in
        // the master VM FIRST: several daemon objects embed their own copies
        // of the same global function indexes (e.g. a sqlite variant of
        // mixed_mysql_select), and picking an arbitrary VM runs the wrong
        // bytecode (seen as "sql=0" / immediate return from the wrapper).
        std::shared_ptr<OriginalVm> target;
        std::string name;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            std::string key = searchRoot_.empty() ? "<main>" : searchRoot_ + "/<main>";
            auto mit = reg.vms.find(key);
            if (mit != reg.vms.end()) {
                name = mit->second->functionNameByIndex(index);
                if (!name.empty()) target = mit->second;
            }
            if (!target) {
                for (auto& kv : reg.vms) {
                    name = kv.second->functionNameByIndex(index);
                    if (!name.empty()) {
                        target = kv.second;
                        break;
                    }
                }
            }
        }
        if (target) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[vm] cross-object user fun index=" << index << " name=" << name << "\n";
            std::vector<Value> args;
            for (int i = 0; i < argCount && !stack_.empty(); ++i) args.push_back(pop().value);
            if (std::getenv("LPCVM_TRACE")) {
                std::string dbg;
                for (auto& a : args) { if (!dbg.empty()) dbg += ","; dbg += to_string(a, false); }
                std::cerr << "[fallback] vm=" << this << " index=" << index << " name=" << name
                          << " argc=" << argCount << " args=[" << dbg << "] stackLeft=" << stack_.size() << "\n";
            }
            result = runDetached([&] { return target->callFunctionByName(name, args); });
            return false;
        }
        // Some global function indices refer to builtin efuns (e.g. the telnet
        // helpers). The low 6 bits of the flags carry the argument count.
        const auto& names = efunNameTable();
        if (index >= 0 && static_cast<size_t>(index) < names.size() && !names[index].empty()) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[vm] global efun fallback index=" << index << " name=" << names[index]
                          << " argc=" << argCount << "\n";
            result = callEfun(index, argCount);
            return false;
        }
        return false;
    }

    // RAII reentrant execution lock that also tracks the recursion depth so
    // runDetached() can fully release it around a cross-object call.
    struct ExecGuard {
        std::recursive_mutex& m;
        int& depth;
        std::string tag;
        ExecGuard(std::recursive_mutex& m_, int& d_, std::string tag_ = "")
            : m(m_), depth(d_), tag(std::move(tag_)) {
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] try " << tag << " t=" << std::this_thread::get_id() << "\n";
            m.lock();
            ++depth;
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] got " << tag << " depth=" << depth
                          << " t=" << std::this_thread::get_id() << "\n";
        }
        ~ExecGuard() {
            --depth;
            m.unlock();
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock-] rel " << tag << " depth=" << depth
                          << " t=" << std::this_thread::get_id() << "\n";
        }
        ExecGuard(const ExecGuard&) = delete;
        ExecGuard& operator=(const ExecGuard&) = delete;
    };

    // Run a cross-object call WITHOUT holding this VM's execution lock, so two
    // threads that call into each other's objects (A->B on the game thread,
    // B->A on the threadd thread) can't deadlock on the two exec mutexes.
    // The paused stack/frames/return state are detached first, so a thread that
    // enters this VM during the window gets a fresh empty stack instead of
    // clobbering the paused caller's.
    template <typename F>
    Value runDetached(F&& fn) {
        int d = execDepth_;
        std::vector<StackItem> s = std::move(stack_);
        std::vector<Frame> f = std::move(frames_);
        Value rv = returnValue_;
        bool rd = returned_;
        returnValue_ = make_int(0);
        returned_ = false;
        execDepth_ = 0;
        // The caller's continuation (this VM's stream at the call site): a
        // SAME-VM detached callee that parks must return into it (see the
        // SuspendSignal branch below).
        const std::vector<OriginalInsn>* savedCode = code_;
        const std::vector<uint8_t>* savedMeta = meta_;
        size_t savedIp = ip_;
        for (int i = 0; i < d; ++i) execMutex_.unlock();
        Value r;
        try {
            r = fn();
        } catch (const SuspendSignal&) {
            // The detached call PARKED on THIS VM: merge the callee's frame
            // chain (built on the detached empty stack) back ON TOP of the
            // caller's restored frames so the parked process resumes
            // coherently; the outermost callee frame returns into the caller's
            // stream (savedCode/savedIp).  For a CROSS-VM callee the parked
            // state lives on the other VM and this VM's frames are simply
            // restored (the scheduler resumes the caller via callerVm).
            for (int i = 0; i < d; ++i) execMutex_.lock();
            execDepth_ = d;
            if (!frames_.empty()) {
                size_t calleeStart = f.size();
                int shift = static_cast<int>(s.size());
                for (auto& fr : frames_) fr.bp += shift;
                s.insert(s.end(), stack_.begin(), stack_.end());
                f.insert(f.end(), frames_.begin(), frames_.end());
                if (calleeStart < f.size()) {
                    f[calleeStart].retCode = savedCode;
                    f[calleeStart].retMeta = savedMeta;
                    f[calleeStart].retIp = savedIp;
                }
                stack_ = std::move(s);
                frames_ = std::move(f);
            } else {
                stack_ = std::move(s);
                frames_ = std::move(f);
                // CROSS-VM blocking park: this VM's caller frames (e.g. a
                // daemon's do_init running on ITS owner VM, called from the
                // master's post_init loop) were detached and are now restored
                // here, waiting for the callee (running on another VM) to
                // return.  Record this VM so the scheduler resumes it (via
                // resumeCaller) after the callee finishes - otherwise only the
                // process's own VM (p->vm, the master) is resumed and the
                // do_init's remaining code after the blocking call never runs.
                scheduler().noteParkedCaller(this);
            }
            returnValue_ = rv;
            returned_ = rd;
            throw;
        } catch (...) {
            for (int i = 0; i < d; ++i) execMutex_.lock();
            execDepth_ = d;
            stack_ = std::move(s);
            frames_ = std::move(f);
            returnValue_ = rv;
            returned_ = rd;
            throw;
        }
        for (int i = 0; i < d; ++i) execMutex_.lock();
        execDepth_ = d;
        stack_ = std::move(s);
        frames_ = std::move(f);
        returnValue_ = rv;
        returned_ = rd;
        return r;
    }

    Value executeFunctionFromOutside(const OriginalFunction& fn, const std::vector<Value>& args) {
        // The whole call - pushing the arguments AND running the body - must
        // hold this VM's execution lock.  Several OS threads can call into the
        // same registry VM concurrently; pushing outside the lock let one
        // thread's arguments land on another thread's stack (seen as a
        // shifted mixed_mysql_select parameter list -> bogus cno/sql).
        bool diag = std::getenv("LPCVM_TRACE") != nullptr;
        bool diagMe = diag && (objName_ == "<obj>" || objName_ == "/aaa/daemons/accountd.o" ||
                               objName_ == "/clone/misc/unauthed_agent.o" ||
                               fn.name == "account_login" || fn.name == "set_dynamic_key" ||
                               fn.name == "mysql_select" || fn.name == "_check_client_data");
        if (diagMe)
            std::cerr << "[lock] TRY obj=" << objName_ << " fn=" << fn.name
                      << " t=" << std::this_thread::get_id() << "\n";
        ExecGuard lock(execMutex_, execDepth_, ("fromOutside " + objName_ + "/" + fn.name).c_str());
        if (diagMe)
            std::cerr << "[lock] GOT obj=" << objName_ << " fn=" << fn.name
                      << " t=" << std::this_thread::get_id() << "\n";
        return executeFunctionFromOutsideLocked(fn, args);
    }

    // Push the arguments for fn onto the stack the way the original VM does
    // (param0 on top; missing declared args padded with undefined type 0).
    // Returns true when a zero-arg call pushed one placeholder slot (so
    // bp >= 0).  Mirrors executeFunctionFromOutsideLocked's inline layout.
    bool pushArgsForFunction(const OriginalFunction& fn, const std::vector<Value>& args) {
        int n = fn.argCount;
        int passed = static_cast<int>(args.size());
        bool placeholder = false;
        if (n == 0 && passed == 0) {
            pushValue(Value{});   // frame slot for bp (original type 0)
            placeholder = true;
        }
        int pushCount = (passed > n) ? passed : n;
        for (int i = pushCount - 1; i >= 0; --i) {
            Value a = (i < static_cast<int>(args.size())) ? args[i] : Value{};
            pushValue(a);
        }
        return placeholder;
    }

    Value executeFunctionFromOutsideLocked(const OriginalFunction& fn, const std::vector<Value>& args) {
        // The original VM pushes args in reverse (param0 last, on top).
        // For a zero-arg function the stack top would be -1; reserve one slot
        // so bp >= 0 and local/loop-control slots (bp + n) stay in bounds.
        if (std::getenv("LPCVM_TRACE") && fn.name == "save_to_text")
            std::cerr << "[efo-dbg] save_to_text args=" << args.size()
                      << " stackBefore=" << stack_.size()
                      << " caller=" << (frames_.empty() || !frames_.back().fn
                                            ? "<top>" : frames_.back().fn->name) << "\n";
        // all declared arguments were pushed (missing ones as undefined)
        bool placeholder = pushArgsForFunction(fn, args);
        return executeFunctionOnStack(fn, placeholder, static_cast<int>(args.size()));
    }

    // Push a Frame for fn and switch the instruction stream to it, WITHOUT
    // recursing into a new execute()/runLoop.  The caller (a case 7/8/5
    // handler inside runLoop) continues the SAME loop; when the callee hits
    // return_from_fun (case 72/73) it pops this frame and restores
    // code_/meta_/ip_ from the frame's ret* fields, then pushes the return
    // value.  This is the explicit-stack-machine equivalent of the
    // original's vm_callInUserFunBody running the callee INLINE.
    // The arguments must already be on the stack (param0 on top), exactly as
    // executeFunctionFromOutsideLocked leaves them.  Returns false (and
    // leaves the stack untouched) when the stack is too shallow.
    bool startInlineFunction(const OriginalFunction& fn, bool zeroArgPlaceholder, int passedArgs,
                             std::shared_ptr<std::vector<Value>> switchObjVars = nullptr,
                             const std::vector<std::string>* targetStrings = nullptr,
                             const std::map<int, const OriginalFunction*>* switchFuncs = nullptr) {
        // Only the REAL arguments must be on the stack.  Missing declared
        // parameters are filled by the callee's check_args_num default-value
        // block (inline defaults + resort_stack), so requiring fn.argCount
        // slots here wrongly rejected calls that pass fewer args (read_etc_file
        // declares 3 params with defaults but is called with 1 - startInline
        // returned false, the call yielded 0, and load_valid_chinese errored).
        int need = (passedArgs >= 0) ? passedArgs : fn.argCount;
        if (static_cast<int>(stack_.size()) < need) return false;
        int bp = static_cast<int>(stack_.size()) - 1;
        // Cross-object inline call: the callee's push_shared_string indexes its
        // OWN program's string table, not the caller's.  fn.strings carries the
        // per-function override when the .o has one; otherwise fall back to the
        // CALLEE object's table (targetStrings), never to this VM's object.
        const std::vector<std::string>* tbl = fn.strings ? fn.strings.get() : nullptr;
        if (!tbl || tbl->empty()) {
            tbl = targetStrings ? targetStrings : &obj_.strings;
            if (tbl->empty()) tbl = fn.strings ? fn.strings.get() : nullptr;
        }
        Frame frame{&fn, bp, passedArgs >= 0 ? passedArgs : fn.argCount, fn.localCount, tbl};
        frame.varOffset = fn.varOffset;
        frame.funOffset = fn.funOffset;
        frame.zeroArgPlaceholder = zeroArgPlaceholder;
        // Cross-object inline call: temporarily switch the object variable
        // table to the callee's object; restore when the frame pops.
        if (switchObjVars) {
            frame.savedObjVars = objVars_;
            objVars_ = std::move(switchObjVars);
        }
        // ... and the function-index table (the callee's constitution): its
        // internal local-index calls (private do_operate etc.) must resolve in
        // ITS program, not the caller's merged table.
        if (switchFuncs) {
            frame.savedFuncLookup = funcLookup_;
            funcLookup_ = switchFuncs;
        }
        // Save the caller's stream so return_from_fun can resume it.
        frame.retCode = code_;
        frame.retMeta = meta_;
        frame.retIp = ip_;   // ip_ already points at the instruction AFTER the call
        frames_.push_back(frame);
        code_ = &fn.code;
        meta_ = &fn.metaData;
        ip_ = 0;
        returnValue_ = make_int(0);
        returned_ = false;
        return true;
    }

    // Start a function call from OUTSIDE the bytecode loop (external entry:
    // callFunctionByName / callFunctionPointer / callFunctionByIndex / efun
    // embedded calls).  Pushes a TOP-LEVEL frame (retCode == nullptr) and
    // runs the loop until that frame returns.  Nested uses (an efun that
    // calls a function on the same VM) recurse one level per efun, exactly
    // like the original's vm_apply from inside an efun.
    Value executeFunctionOnStack(const OriginalFunction& fn, bool zeroArgPlaceholder = false,
                                 int passedArgs = -1) {
        ExecGuard lock(execMutex_, execDepth_, "onStack " + objName_ + "/" + fn.name);
        // Only the REAL arguments must be present; declared-but-defaulted
        // params are filled by the callee's check_args_num default block.
        int need = (passedArgs >= 0) ? passedArgs : fn.argCount;
        if (static_cast<int>(stack_.size()) < need) return make_int(0);

        int bp = static_cast<int>(stack_.size()) - 1;
        if (std::getenv("LPCVM_TRACE") && (fn.name == "register_entry" || fn.name == "query_entry")) {
            std::string dbg;
            for (int i = 0; i < fn.argCount; ++i)
                dbg += (i ? "," : "") + to_string(slotValue(bp - i), false);
            std::cerr << "[reg] exec " << fn.name << " obj=" << objName_ << " varOff=" << fn.varOffset
                      << " params=[" << dbg << "] stackSize=" << stack_.size() << "\n";
            // dump the entry table (obj var at the register.c constitution)
            int tableVar = fn.varOffset;  // register.c var 0
            if (static_cast<size_t>(tableVar) < objVars_->size()) {
                const Value& tv = (*objVars_)[static_cast<size_t>(tableVar)];
                std::string keys;
                if (tv.index() == 6) {
                    for (auto& kv : std::get<std::shared_ptr<MappingValue>>(tv)->items()) {
                        if (!keys.empty()) keys += ",";
                        keys += to_string(kv.first, false);
                    }
                }
                std::cerr << "[reg]   table var" << tableVar << " type=" << tv.index()
                          << " keys=[" << keys.substr(0, 120) << "]\n";
            }
        }
        if (std::getenv("LPCVM_TRACE") && (fn.name == "load_comm_desc" || fn.name == "load_file_dependence")) {
            std::cerr << "[xfn] " << fn.name << " bp=" << bp << " argc=" << fn.argCount
                      << " argsPassed=" << fn.argCount << " stackSize=" << stack_.size()
                      << " localCount=" << fn.localCount << " frames=" << frames_.size() << "\n";
        } else if (std::getenv("LPCVM_TRACE") && (fn.name == "mixed_mysql_select" || fn.name == "mysql_select" ||
                                           fn.name == "fetch_config_list" || fn.name == "update_config" ||
                                           fn.name == "assure_connection" || fn.name == "choose_one_url" ||
                                           fn.name == "do_init")) {
            std::string dbg;
            for (int i = 0; i < fn.argCount; ++i)
                dbg += (i ? "," : "") + to_string(slotValue(bp - i), false);
            std::string stack0;
            if (fn.name == "fetch_config_list")
                stack0 = stack_.size() > 0 ? to_string(stack_[0].value, false) : "<empty>";
            std::cerr << "[xfn] vm=" << this << " " << fn.name << " params=[" << dbg << "] bp=" << bp
                      << " argNum=" << fn.argCount << " stackSize=" << stack_.size()
                      << " frames=" << frames_.size()
                      << (stack0.empty() ? "" : (" stack0=" + stack0)) << "\n";
        }
        // argNum records how many arguments were actually PASSED: functions
        // with default arguments compare it against the declared count in
        // check_args_num and push the missing defaults inline.
        Frame frame{&fn, bp, passedArgs >= 0 ? passedArgs : fn.argCount, fn.localCount,
                    fn.strings ? fn.strings.get() : &obj_.strings};
        // A cross-object entry call (callFunctionByName) pre-switched
        // objVars_ to this VM's own table and parked the previous (switched)
        // table here: record it on the frame so the pop restores it.
        if (savedObjVarsForNextCall_) {
            frame.savedObjVars = std::move(savedObjVarsForNextCall_);
            savedObjVarsForNextCall_.reset();
        }
        // This VM hosts many scheduler processes (execOnMain) and their frames
        // share the members.  efunInlineStarted_ is set by a call_other_arg
        // that started its callee INLINE and is meant to be consumed by the
        // NEXT call_in_efun within the SAME execution; if the callee returns
        // without another efun call the flag stays set and leaks into the NEXT
        // process, which then skips its own efun dispatch.  Reset it here so
        // every external entry starts clean.
        efunInlineStarted_ = false;
        frame.varOffset = fn.varOffset;
        frame.funOffset = fn.funOffset;
        frame.zeroArgPlaceholder = zeroArgPlaceholder;
        // Top-level frame: retCode stays nullptr.  When case 72/73 pops this
        // frame it sees retCode == nullptr and exits runLoop with the result
        // in returnValue_ (no caller to resume).
        frames_.push_back(frame);
        Value savedReturn = returnValue_;
        bool savedReturned = returned_;
        returnValue_ = make_int(0);
        returned_ = false;
        // The calling context (may be an outer runLoop on this VM, e.g. an
        // efun embedded call, or an idle VM when entered externally).  Save
        // and restore it so the explicit stack machine never loses it.
        const std::vector<OriginalInsn>* savedCode = code_;
        const std::vector<uint8_t>* savedMeta = meta_;
        size_t savedIp = ip_;
        try {
            code_ = &fn.code;
            meta_ = &fn.metaData;
            ip_ = 0;
            runLoop();
        } catch (const SuspendSignal&) {
            // Parked by a blocking efun.  ALL execution state (stack_/frames_/
            // code_/meta_/ip_/returnValue_/returned_) stays in the members as
            // the parked process image; the scheduler resumes it via
            // resumeProcess().  Do NOT restore savedCode/savedMeta/savedIp and
            // do NOT pop the frame - that would destroy the parked state.
            parked_ = true;
            throw;
        } catch (const TimeSliceSignal&) {
            // Master time-slice yield: keep ALL state (stack_/frames_/
            // code_/meta_/ip_) so the scheduler can resume from ip_ later.
            throw;
        } catch (...) {
            code_ = savedCode;
            meta_ = savedMeta;
            ip_ = savedIp;
            if (!frames_.empty() && frames_.back().fn == &fn) {
                if (frames_.back().savedObjVars) objVars_ = frames_.back().savedObjVars;
                if (frames_.back().savedFuncLookup) funcLookup_ = frames_.back().savedFuncLookup;
                frames_.pop_back();
            }
            returnValue_ = savedReturn;
            returned_ = savedReturned;
            throw;
        }
        Value result = returnValue_;
        // Normal path: case 72/73 already popped our frame and cleaned the
        // stack.  Defensive: if runLoop exited abnormally (finish_action,
        // step limit, code exhaustion) the frame may still be on the stack.
        if (!frames_.empty() && frames_.back().fn == &fn) {
            // The cleanup must be based on the LIVE frame: resort_stack moves
            // the frame base (inline default arguments become trailing
            // parameters) and updates argNum to passed + defaults.
            int liveBp = frames_.back().bp;
            int usedArgs = frames_.back().argNum;
            int newSize = liveBp - usedArgs + 1 - (frames_.back().zeroArgPlaceholder ? 1 : 0);
            if (newSize < 0) newSize = 0;
            stack_.resize(static_cast<size_t>(newSize));
            if (frames_.back().savedObjVars) objVars_ = frames_.back().savedObjVars;
            if (frames_.back().savedFuncLookup) funcLookup_ = frames_.back().savedFuncLookup;
            frames_.pop_back();
        }
        code_ = savedCode;
        meta_ = savedMeta;
        ip_ = savedIp;
        returnValue_ = savedReturn;
        returned_ = savedReturned;
        return result;
    }

    // ------------------------------------------------------------------
    // The main bytecode executor (mirrors vm_regularProcessor)
    // ------------------------------------------------------------------
    // Top-level entry: adopt the instruction stream and run it to completion
    // (global init code, or an externally-started function body).  In the
    // original driver this is vm_doActions on the process's current
    // constitution; function calls inside switch to a new stream via
    // startInlineFunction and RETURN by restoring (code_/meta_/ip_) from the
    // Frame, so this loop never recurses.
    void execute(const std::vector<OriginalInsn>& code, const std::vector<uint8_t>& meta) {
        if (std::getenv("LPCVM_LOCK_TRACE"))
            std::cerr << "[exec] obj=" << objName_ << " codeSize=" << code.size()
                      << " metaSize=" << meta.size() << " ip0=" << (code.empty() ? -1 : code[0].op)
                      << " t=" << std::this_thread::get_id() << "\n";
        code_ = &code;
        meta_ = &meta;
        ip_ = 0;
        runLoop();
    }

    // ------------------------------------------------------------------
    // Cooperative-scheduler resume entry (stage B).  A parked VM (a blocking
    // efun threw SuspendSignal; all execution state stayed in the members)
    // is resumed here by the scheduler: run the efun's resume hook (writes
    // back through references etc.), push the efun's result (the parked
    // call site consumed its args but never pushed), then continue the SAME
    // loop from the saved ip_.  Returns when the whole call chain finishes,
    // or re-throws SuspendSignal if it parks again (the scheduler re-parks).
    // resumeValue must be a fresh value; the caller owns any stack effects.
    void resumeProcess(Value /*resumeValue ignored*/) {
        ExecGuard lock(execMutex_, execDepth_, "resume " + objName_);
        parked_ = false;
        ResumeCond c = cond_;
        cond_ = ResumeCond{};
        if (onResume_) {
            auto hook = std::move(onResume_);
            onResume_ = nullptr;
            hook();     // may update resumeValue_ (e.g. connect's result check)
        }
        pushValue(std::move(resumeValue_));
        runLoop();
        // runLoop returned normally: the whole call chain finished and the
        // top-level frame was popped by case 72/73 (retCode == nullptr).
        // All state is already clean; nothing to restore.
    }

    // Resume a VM that yielded via relinquish while it was being called
    // synchronously from ANOTHER VM (the master's post_init loop running a
    // daemon's init/do_init closure inline on this VM).  The efun already
    // pushed its result (1), so continue the saved stream directly without
    // pushing another value (unlike resumeProcess).  Mirrors the original
    // single-thread cooperative scheduler: a yield inside a closure parks
    // that closure's own execution and resumes the SAME frames later.
    void resumeRunLoop() {
        ExecGuard lock(execMutex_, execDepth_, "resumeRunLoop " + objName_);
        runLoop();
    }

    // The result a just-completed top-level call left in returnValue_
    // (used by runProcess to hand a resumed process's result to execOnMain).
    Value lastReturnValue() const { return returnValue_; }

    // Resume a CALLER VM whose frame chain was detached by runDetached while
    // a cross-object callee ran (and blocked) on another VM.  Push the
    // callee's result and continue the caller's stream from the saved ip_
    // (the op after the case-8 call site).
    void resumeCaller(Value result) {
        ExecGuard lock(execMutex_, execDepth_, "resumeCaller " + objName_);
        // Nothing to resume here (the detached callee was merged back into
        // THIS VM's frame chain, so the continuation already happened inside
        // the parked VM's resume) - skip.
        if (frames_.empty()) return;
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[resume-caller] vm=" << this << " obj=" << objName_
                      << " frames=" << frames_.size() << " ip=" << ip_ << "\n";
        pushValue(std::move(result));
        runLoop();
    }

    // True while this VM is parked (a blocking efun suspended it).
    bool isParked() const { return parked_; }
    // The condition the parked VM is waiting for (owned by the scheduler).
    ResumeCond& resumeCond() { return cond_; }
    // Resume hook run just before resumeValue is pushed (write-backs etc.).
    std::function<void()> onResume_;

    // Park this VM on a wake condition and unwind to the scheduler.  cond
    // records what to wait for; hook runs on resume before the efun result
    // is pushed; resumeValue is pushed on resume.
    void parkAndSuspend(const ResumeCond& cond, std::function<void()> hook, Value resumeValue) {
        cond_ = cond;
        onResume_ = std::move(hook);
        parked_ = true;
        resumeValue_ = std::move(resumeValue);
        SuspendSignal sig;
        sig.vm = this;
        sig.cond = cond_;
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[suspend] vm=" << this << " obj=" << objName_
                      << " kind=" << cond.kind << " ip=" << ip_ << " frames=" << frames_.size() << "\n";
        throw sig;
    }
    // The value to push when this parked VM resumes.
    Value& parkedResumeValue() { return resumeValue_; }

    // The single cooperative instruction loop.  Executes code_/meta_/ip_
    // until finish_action, a top-level return (frame with no caller), or the
    // end of the stream.  All execution state (stack_/frames_/code_/meta_/ip_)
    // lives in members, so a blocking efun can later park the process by
    // returning from here and resume by re-entering with the saved ip_ -
    // exactly like the original scheduler restoring a PCB.
    void runLoop() {
        // Step-throughput meter (LPCVM_STEP_PROF=1): prints obj/fn/ip plus
        // cumulative instruction and runLoop-activation counts once per second
        // of wall time while instructions execute.  When the env var is unset
        // the only cost is a static-bool test per instruction.
        static const bool stepProfOn = (std::getenv("LPCVM_STEP_PROF") != nullptr);
        static int64_t stepProfInsns = 0;
        static int64_t stepProfActs = 0;
        static int64_t stepProfLastMs = 0;
        if (stepProfOn) ++stepProfActs;
        while (code_ && ip_ < code_->size()) {
            const OriginalInsn& in = (*code_)[ip_++];
            lastOp_ = in.op;
            lastIp_ = ip_ - 1;
            if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_FN")) {
                std::string cur = "<global>";
                if (!frames_.empty() && frames_.back().fn) cur = frames_.back().fn->name;
                if (cur == std::getenv("LPCVM_TRACE_FN"))
                    std::cerr << "[fn] vm=" << this << " " << cur << " ip=" << (ip_ - 1) << " op=" << in.op
                              << " " << actionInfo(in.op).name << " a=" << in.a << " b=" << in.b << "\n";
            }
            if (++stepCounter_ > 20000000) return;
            if (stepProfOn) {
                ++stepProfInsns;
                using namespace std::chrono;
                int64_t now2 = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
                if (now2 - stepProfLastMs >= 1000) {
                    stepProfLastMs = now2;
                    std::string cur = "<global>";
                    if (!frames_.empty() && frames_.back().fn) cur = frames_.back().fn->name;
                    std::cerr << "[step-prof] obj=" << objName_ << " fn=" << cur
                              << " ip=" << ip_ << "/" << (code_ ? code_->size() : 0)
                              << " op=" << in.op << " insns=" << stepProfInsns
                              << " acts=" << stepProfActs << "\n";
                }
            }
            if ((std::getenv("LPCVM_LOCK_TRACE") || std::getenv("LPCVM_STUCK_TRACE")) &&
                (stepCounter_ % 500000) == 0) {
                std::string cur = "<global>";
                if (!frames_.empty() && frames_.back().fn) cur = frames_.back().fn->name;
                std::cerr << "[vm-stuck] obj=" << objName_ << " fn=" << cur
                          << " ip=" << ip_ << "/" << (code_ ? code_->size() : 0)
                          << " op=" << in.op << " frames=" << frames_.size()
                          << " stack=" << stack_.size();
                // Full call chain (fn -> fn -> ...), most recent LAST.  Frame
                // fns are the callee's program functions; cross-object inline
                // calls show savedObjVars (different object variable table).
                for (size_t fi = 0; fi < frames_.size(); ++fi) {
                    const Frame& fr = frames_[fi];
                    std::string fn = (fr.fn ? fr.fn->name : "<ctx/global>");
                    std::string a0 = "?";
                    if (fr.bp >= 0 && static_cast<size_t>(fr.bp) < stack_.size()) {
                        a0 = to_string(stack_[static_cast<size_t>(fr.bp)].value, false);
                        if (a0.size() > 28) a0 = a0.substr(0, 28) + "...";
                    }
                    std::cerr << " [" << fi << (fr.savedObjVars ? "|x" : "") << "]"
                              << fn.substr(0, 40) << "@" << a0;
                }
                // Show each frame's live loop-control blocks: type/current
                // (or iter+array size) reveal whether a foreach/counter is
                // stuck or advancing.
                for (size_t fi = 0; fi < frames_.size(); ++fi) {
                    const Frame& fr = frames_[fi];
                    if (fr.loops.empty()) continue;
                    std::string fn = (fr.fn ? fr.fn->name : "<ctx>");
                    std::cerr << " f" << fi << "(" << fn.substr(0, 24) << "){";
                    for (auto& lc : fr.loops) {
                        auto c = lc.second;
                        if (!c) continue;
                        if (c->type == 1 || c->type == -1)
                            std::cerr << " L" << lc.first << "=" << c->current << "/" << c->to;
                        else if (c->type == 901)
                            std::cerr << " L" << lc.first << "=it" << c->iter << "/"
                                      << (c->arr ? c->arr->items.size() : 0);
                        else if (c->type == 902)
                            std::cerr << " L" << lc.first << "=m" << c->mIdx << "/"
                                      << (c->map ? c->map->items().size() : 0);
                        else
                            std::cerr << " L" << lc.first << "=t" << c->type;
                    }
                    std::cerr << "}";
                }
                if (!frames_.empty()) {
                    const Frame& tf = frames_.back();
                    std::cerr << " bp=" << tf.bp << " argc=" << tf.argNum;
                }
                size_t showN = stack_.size() > 8 ? 8 : stack_.size();
                std::cerr << " top=[";
                for (size_t si = stack_.size() - showN; si < stack_.size(); ++si) {
                    if (si > stack_.size() - showN) std::cerr << " ";
                    std::string vs = to_string(stack_[si].value, false);
                    if (vs.size() > 40) vs = vs.substr(0, 40) + "...";
                    std::cerr << "s" << si << "=" << vs;
                }
                // Any loop-control blocks parked on the stack (foreach state
                // lives in a local slot as a LoopCtrl value).
                for (size_t si = 0; si < stack_.size(); ++si) {
                    if (stack_[si].value.index() != 9) continue;
                    auto c = std::get<std::shared_ptr<LoopCtrl>>(stack_[si].value);
                    if (!c) continue;
                    if (c->type == 1 || c->type == -1)
                        std::cerr << " lc@[" << si << "]cnt=" << c->current << "/" << c->to;
                    else if (c->type == 901)
                        std::cerr << " lc@[" << si << "]iter=" << c->iter << "/"
                                  << (c->arr ? c->arr->items.size() : 0);
                    else if (c->type == 902)
                        std::cerr << " lc@[" << si << "]mIter=" << c->mIdx << "/"
                                  << (c->map ? c->map->items().size() : 0);
                    else
                        std::cerr << " lc@[" << si << "]t" << c->type;
                }
                std::cerr << "]\n";
            }
            const ActionInfo& ai = actionInfo(in.op);
            int32_t m0 = 0, m1 = 0;
            bool fromMeta = (ai.attrib & 0x80000000u) != 0;
            if (fromMeta) {
                uint32_t off = static_cast<uint32_t>(in.a) | (static_cast<uint32_t>(in.b) << 16);
                m0 = readI32(*meta_, off);
                if (ai.parameters == 2) m1 = readI32(*meta_, off + 4);
            }
            try {
                switch (in.op) {
            case 46: // null_action
                break;
            case 29: // finish_action
                if (returned_) return;
                returnValue_ = make_int(0);
                return;
            case 72: // return_from_fake_fun (lambda/closure return: value always popped)
            case 73: { // return_from_fun
                // Original vm_returnFromFun (0x8094978): a value-returning
                // function leaves exactly one value on the stack (sp ==
                // bp - args + 1); a void function must have sp == bp - args
                // and gets a VOID value pushed by the VM - it must NEVER pop
                // the caller's stack (that would consume the caller's locals,
                // e.g. update_config's cno slot).  In our vector model bp is
                // the top slot at entry, so:
                //   void:   size == bp + 1            -> push void
                //   valued: size == bp + 2            -> pop the return value
                // A global init code segment ends with return_from_fake_fun
                // (register.o's `all_entries = ([ ])` compiles to create_mapping
                // / push_obj_vptr / assign_value / pop_stack / null_action /
                // return_from_fake_fun / finish_action).  Such code runs under
                // a CONTEXT frame (fn == nullptr) pushed by runInheritedInits,
                // not a real call frame: the return op must NOT pop that
                // context frame (runInheritedInits pops it itself) and must not
                // resume a caller stream - it just ends the init segment.
                if (frames_.empty()) { returned_ = true; return; }
                const Frame& f = frames_.back();
                if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_FN") &&
                    f.fn && f.fn->name == std::getenv("LPCVM_TRACE_FN"))
                    std::cerr << "[ret-enter] " << f.fn->name << " bp=" << f.bp
                              << " argNum=" << f.argNum << " stack=" << stack_.size()
                              << " frames=" << frames_.size() << " op=" << in.op << "\n";
                if (in.op == 72) {
                    if (!stack_.empty()) returnValue_ = pop().value;
                    else returnValue_ = make_int(0);
                } else {
                    size_t expectedVoid = static_cast<size_t>(f.bp) + 1;
                    if (std::getenv("LPCVM_TRACE") && f.fn && f.fn->name == "save_to_text")
                        std::cerr << "[ret] " << f.fn->name << " bp=" << f.bp << " stack=" << stack_.size()
                                  << " expVoid=" << expectedVoid << " top="
                                  << (stack_.empty() ? "empty" : to_string(stack_.back().value, false)) << "\n";
                    if (stack_.size() == expectedVoid) {
                        returnValue_ = UndefinedTag{};   // void return value (type 1)
                    } else if (stack_.size() == expectedVoid + 1 && !stack_.empty()) {
                        returnValue_ = pop().value;
                    } else {
                        // Stack not in canonical shape; take the top if any.
                        if (!stack_.empty()) returnValue_ = pop().value;
                        else returnValue_ = UndefinedTag{};
                    }
                }
                if (std::getenv("LPCVM_TRACE") && f.fn && f.fn->name == "save_to_text")
                    std::cerr << "[ret] -> " << to_string(returnValue_, false) << "\n";
                returned_ = true;
                // Context frame (fn == nullptr): this is a global init code
                // segment run by runInheritedInits.  Do NOT pop it (the caller
                // pops it) and do NOT resume a caller stream - end the loop.
                if (f.fn == nullptr) return;
                // ---- explicit stack machine: return to the caller ----
                // Clean the callee's argument/local slots off the stack
                // (mirrors the cleanup executeFunctionOnStack used to do
                // after execute() returned; vm_returnFromFun restores sp to
                // bp - argNum + 1).  Use the LIVE frame values: resort_stack
                // moves the frame base and updates argNum.
                int liveBp = f.bp;
                int usedArgs = f.argNum;
                int newSize = liveBp - usedArgs + 1 - (f.zeroArgPlaceholder ? 1 : 0);
                if (newSize < 0) newSize = 0;
                // Grab the caller context before popping the frame.
                const std::vector<OriginalInsn>* retCode = f.retCode;
                const std::vector<uint8_t>* retMeta = f.retMeta;
                size_t retIp = f.retIp;
                // Cross-object inline call: restore the caller's obj vars and
                // function-index table (constitution).
                if (f.savedObjVars) objVars_ = f.savedObjVars;
                if (f.savedFuncLookup) funcLookup_ = f.savedFuncLookup;
                frames_.pop_back();
                stack_.resize(static_cast<size_t>(newSize));
                if (retCode == nullptr) {
                    // Top-level frame (pushed by executeFunctionOnStack): the
                    // whole call is done; the result stays in returnValue_.
                    return;
                }
                code_ = retCode;
                meta_ = retMeta;
                ip_ = retIp;
                if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_FN") &&
                    f.fn && f.fn->name == std::getenv("LPCVM_TRACE_FN"))
                    std::cerr << "[ret-t] " << f.fn->name << " bp=" << f.bp << " argNum=" << f.argNum
                              << " retCode=" << (retCode ? "set" : "null")
                              << " retCodePtr=" << (retCode ? std::to_string(reinterpret_cast<uintptr_t>(retCode)) : "-")
                              << " curCodePtr=" << (code_ ? std::to_string(reinterpret_cast<uintptr_t>(code_)) : "-")
                              << " retIp=" << retIp << " newStack=" << newSize
                              << " framesAfter=" << frames_.size() << "\n";
                pushValue(returnValue_);   // caller sees the return value on top
                break;
            }
            case 59: // push_number(S)
                pushValue(make_int(static_cast<int16_t>(in.a)));
                break;
            case 60: // push_number(L)
                pushValue(make_int(fromMeta ? m0 : metaOperand(*meta_, in.a, in.b)));
                break;
            case 63: // push_real_from_word (float bits in the first metadata word)
                pushValue(make_float(*reinterpret_cast<const float*>(&m0)));
                break;
            case 64: // push_shared_string
                {
                    const std::vector<std::string>* tbl =
                        frames_.empty() || !frames_.back().strings ? &obj_.strings : frames_.back().strings;
                    if (in.a < tbl->size()) pushValue(make_string((*tbl)[in.a]));
                    else pushValue(make_int(0));
                }
                break;
            case 66: // push_undefined (original type 0 "undefined")
                pushValue(Value{});
                break;
            case 65: // push_this_object
                pushValue(make_object_owned(thisObjectName_, &obj_));
                break;
            case 52: // push_arg_num
                pushValue(make_int(frames_.empty() ? 0 : frames_.back().argNum));
                break;
            case 53: // push_global_var
                pushValue(sharedGlobal(in.a));
                break;
            case 61: { // push_obj_var
                int vi = static_cast<int>(in.a) + currentVarOffset();
                bool isQed = !frames_.empty() && frames_.back().fn &&
                             frames_.back().fn->name == "query_entire_dbase";
                if (std::getenv("LPCVM_TRACE_OPERMAP") && objName_.find("filed.o") != std::string::npos &&
                    !frames_.empty() && frames_.back().fn) {
                    const std::string& fnn = frames_.back().fn->name;
                    if (fnn == "do_operate" || fnn == "msg_operate_done" || fnn == "operate_done" ||
                        fnn == "msg_read_content" || fnn == "operate_done_ex") {
                        bool isMap = vi >= 0 && static_cast<size_t>(vi) < objVars_->size() &&
                                     (*objVars_)[vi].index() == 6;
                        std::cerr << "[opermap] fn=" << fnn << " push_ovar idx=" << vi
                                  << " vars=" << objVars_->size()
                                  << " type=" << ((vi >= 0 && static_cast<size_t>(vi) < objVars_->size()) ? (*objVars_)[vi].index() : -1)
                                  << " size=" << (isMap ? std::get<std::shared_ptr<MappingValue>>((*objVars_)[vi])->size() : -1)
                                  << " vm=" << this << "\n";
                    }
                }
                if (std::getenv("LPCVM_DBG_OBJVAR") &&
                    ((objName_.find("/zone/") != std::string::npos && vi < 2) || isQed)) {
                    std::cerr << "[zvar] obj=" << objName_ << " fn="
                              << (frames_.empty() || !frames_.back().fn ? "<top>" : frames_.back().fn->name)
                              << " varOff=" << currentVarOffset()
                              << " idx=" << vi << " vars=" << objVars_->size()
                              << " type=" << ((vi >= 0 && static_cast<size_t>(vi) < objVars_->size()) ? (*objVars_)[vi].index() : -1)
                              << "\n";
                }
                if (static_cast<size_t>(vi) < objVars_->size()) pushValue((*objVars_)[vi]);
                else pushValue(make_int(0));
                break;
            }
            case 54: // push_global_vptr
                push(makeGlobalRef(in.a));
                break;
            case 62: // push_obj_vptr
                push(makeObjRef(static_cast<int>(in.a) + currentVarOffset()));
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
            case 78: { // set_obj_var
                int vi = static_cast<int>(in.a) + currentVarOffset();
                Value sv = pop().value;
                if (std::getenv("LPCVM_DBG_OBJVAR") && objName_.find("/zone/") != std::string::npos &&
                    sv.index() == 6) {
                    std::cerr << "[setvar-map] obj=" << objName_ << " fn="
                              << (frames_.empty() || !frames_.back().fn ? "<top>" : frames_.back().fn->name)
                              << " idx=" << vi << " vars=" << objVars_->size() << "\n";
                }
                if (static_cast<size_t>(vi) < objVars_->size()) (*objVars_)[vi] = sv;
                break;
            }
            case 77: // set_local_var
                if (!stack_.empty()) {
                    StackItem& slot = stack_[static_cast<size_t>(slotOf(static_cast<int16_t>(in.a)))];
                    Value sv = pop().value;
                    if (slot.value.index() == 11) {
                        auto r = std::get<std::shared_ptr<CrossVmRef>>(slot.value);
                        OriginalVm* vm = static_cast<OriginalVm*>(r->vm);
                        if (vm && r->objIndex >= 0 && static_cast<size_t>(r->objIndex) < vm->objVars().size())
                            vm->objVars()[static_cast<size_t>(r->objIndex)] = sv;
                    } else {
                        slot.value = sv;
                    }
                }
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
                // vm_createArray (0x80918c8): the compiler pushes array
                // elements in REVERSE source order (vm_f_wait_event's caller
                // pushes (value, key) pairs the same way), so element 0 is on
                // top of the stack.  vm_createArray copies stack[vm_sp+i]
                // straight into arr[i] WITHOUT reversing - popping in order
                // yields the source order.  (Verified against filed.o's
                // do_operate: `result = ({ -2, cookie })` compiles to
                // push cookie; push -2; create_array 2, and the array must be
                // {-2, cookie} - operate_done reads set_result[1]==cookie.)
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.push_back(pop().value);
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                pushValue(arr);
                break;
            }
            case 23: { // create_function_value (bound count = par8 & 0x3F)
                // bit7 (0x80) selects the GLOBAL program's function table, just
                // like call_in_user_fun; the index must resolve against the
                // master program, not this object's merged table.
                const OriginalFunction* fn = nullptr;
                const OriginalObject* fnOwner = nullptr;
                if (in.b & 0x80) {
                    fn = findMasterFunction(in.a);
                    fnOwner = objectRegistry().masterObj;
                    if (!fn) {
                        // Some global closure indices are builtin efuns
                        // (e.g. (: create_thread, ... :) in ccs_clientd /
                        // tts_clientd create()), exactly like the call path's
                        // efun fallback below.  The original resolves them via
                        // vm_pGlobalProgram->pFunctions[index] which includes
                        // the efun slots, so a closure over create_thread
                        // (efun 268) must resolve here too - otherwise the
                        // closure collapses to int 0 and the daemon thread is
                        // never spawned.
                        const auto& names = efunNameTable();
                        if (in.a >= 0 && static_cast<size_t>(in.a) < names.size() &&
                            !names[in.a].empty()) {
                            auto cl = std::make_shared<ClosureValue>();
                            cl->functionName = names[in.a];
                            cl->funcIndex = in.a;
                            cl->func = nullptr;   // efun: resolved by name at call time
                            // owner = nullptr so invocation reaches the efun
                            // fallback (call_fun_ptr's final else) instead of
                            // the cross-object owner-resolve path.
                            cl->owner = nullptr;
                            int boundCount = in.b & 0x3F;
                            std::vector<Value> bound;
                            for (int i = 0; i < boundCount && !stack_.empty(); ++i)
                                bound.push_back(pop().value);
                            cl->boundArgs = std::move(bound);
                            if (std::getenv("LPCVM_TRACE"))
                                std::cerr << "[cfv] efun-closure " << cl->functionName
                                          << " idx=" << in.a << " bind=" << boundCount << "\n";
                            pushValue(cl);
                            break;
                        }
                    }
                } else {
                    // The closure's function index is LOCAL to the CURRENT
                    // function's program.  For an inline cross-object frame
                    // (e.g. a cfg handler's main running on the serverd VM),
                    // resolve against the OWNING object's table first - this
                    // VM's merged table may map the same index to a DIFFERENT
                    // function (serverd's empty_entries at index 3 vs the
                    // handler's write_etc), which dispatched the wrong closure.
                    const OriginalFunction* cur = frames_.empty() ? nullptr : frames_.back().fn;
                    if (cur) {
                        std::lock_guard<std::recursive_mutex> rl(registryMutex());
                        for (auto& kv : objectRegistry().vms) {
                            if (kv.second.get() != this && kv.second->ownsFunction(cur)) {
                                const OriginalFunction* own = kv.second->functionByIndex(in.a);
                                if (own && own->index >= 0) {
                                    fn = own;
                                    fnOwner = &kv.second->object();
                                    break;
                                }
                            }
                        }
                    }
                    if (!fn) {
                        auto it = funcsByIndex_.find(in.a);
                        if (it != funcsByIndex_.end()) fn = it->second;
                    }
                }
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[cfv] index=" << in.a << " global=" << ((in.b & 0x80) != 0)
                              << " found=" << (fn != nullptr)
                              << (fn ? " name=" + fn->name : "") << "\n";
                if (fn) {
                    std::vector<Value> bound;
                    int boundCount = in.b & 0x3F;
                    for (int i = 0; i < boundCount && !stack_.empty(); ++i) bound.push_back(pop().value);
                    if (std::getenv("LPCVM_TRACE") && fn->name == "process_command" &&
                        (in.a == 32 || in.a == 153)) {
                        std::string dbg;
                        for (size_t k = 0; k < bound.size(); ++k)
                            dbg += (k ? " | " : "") + to_string(bound[k], false).substr(0, 40);
                        std::cerr << "[cfv-create] " << fn->name << " idx=" << in.a
                                  << " bind=" << boundCount << " cur="
                                  << (frames_.empty() || !frames_.back().fn
                                          ? "<global>" : frames_.back().fn->name)
                                  << " vals=[" << dbg << "]\n";
                    }
                    auto cl = std::make_shared<ClosureValue>();
                    cl->functionName = fn->name;
                    cl->funcIndex = in.a;   // global/merged index of the exact function
                    cl->func = fn;          // exact function: invoke directly
                    cl->boundArgs = bound;
                    cl->owner = fnOwner ? fnOwner : &obj_;
                    pushValue(cl);
                } else {
                    pushValue(make_int(0));
                }
                break;
            }
            case 24: { // create_mapping
                // The compiler pushes (value, key) pairs (see dbd's
                // load_db_config: push Host, push "host", ...).  We pop them
                // reversed and pair them back as (key, value).
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.push_back(pop().value);
                std::reverse(items.begin(), items.end());
                auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(items.size() / 2));
                for (size_t i = 0; i + 1 < items.size(); i += 2)
                    mp->insert(items[i + 1], items[i]);
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
                if (std::getenv("LPCVM_DBG_OBJVAR") &&
                    objName_.find("import/room") != std::string::npos)
                    std::cerr << "[lsh-import] obj=" << objName_ << " aType=" << a.index()
                              << " bType=" << b.index()
                              << " aSize=" << (a.index() == 6 ? std::get<std::shared_ptr<MappingValue>>(a)->size() : -1)
                              << " bSize=" << (b.index() == 6 ? std::get<std::shared_ptr<MappingValue>>(b)->size() : -1) << "\n";
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
            case 79: { // sizeof: LPC semantics (vm_sizeof 0x808f840)
                //   string -> byte length; buffer -> length;
                //   array/mapping -> element count; int/real/void -> 0
                Value v = resolveCrossVmRef(pop().value);
                switch (v.index()) {
                case 4:
                    pushValue(make_int(static_cast<int64_t>(std::get<std::string>(v).size())));
                    break;
                case 5:
                    pushValue(make_int(static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(v)->items.size())));
                    break;
                case 6:
                    pushValue(make_int(static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(v)->size())));
                    break;
                case 10:
                    pushValue(make_int(static_cast<int64_t>(std::get<std::shared_ptr<BufferValue>>(v)->data.size())));
                    break;
                default:
                    pushValue(make_int(0));
                    break;
                }
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
                // in.a bit0 = "index from the end" (vm_queryMember a1):
                //   mks[<1] compiles to push 1 + query_member a=1 -> len-1.
                int mflag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value key = pop().value;
                StackItem c = pop();
                Value container = c.isRef ? deref(c) : c.value;
                if (mflag & 1) {
                    size_t len = 0;
                    if (container.index() == 4) len = std::get<std::string>(container).size();
                    else if (container.index() == 5) len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
                    else if (container.index() == 10) len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
                    key = make_int(static_cast<int64_t>(len) - toInt(key));
                }
                pushValue(queryMemberValue(container, key));
                break;
            }
            case 68: { // query_member_ptr
                // Same from-end flag as query_member (vm_queryMemberPtr).
                int mflag = fromMeta ? m0 : static_cast<int16_t>(in.a);
                Value key = pop().value;
                StackItem c = pop();
                Value container = c.isRef ? deref(c) : c.value;
                if (mflag & 1) {
                    size_t len = 0;
                    if (container.index() == 4) len = std::get<std::string>(container).size();
                    else if (container.index() == 5) len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
                    else if (container.index() == 10) len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
                    key = make_int(static_cast<int64_t>(len) - toInt(key));
                }
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
                if (std::getenv("LPCVM_TRACE") && objName_.find("cmd_log_important") != std::string::npos)
                    std::cerr << "[efun-pre] efun=" << efun << " argc=" << argc
                              << " stack=" << stack_.size() << " frames=" << frames_.size()
                              << " meta=" << (meta_ ? meta_->size() : 0)
                              << " t=" << std::this_thread::get_id() << "\n";
                const auto& names = efunNameTable();
                if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                    names[efun] == "write_buffer") {
                    // vm_f_write_buffer (0x80dd90c): write_buffer(&buf, off,
                    // int|buffer|string) writes a 4-byte big-endian int or raw
                    // bytes into the REFERENCED buffer at off and returns the
                    // length written (move_v37.c / basic.o notice_others_appear
                    // pad movement packets this way).  BufferValue is shared,
                    // so mutating buf->data updates the caller's buffer in
                    // place - no writeThrough needed.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    std::reverse(items.begin(), items.end());
                    int64_t written = 0;
                    if (items.size() >= 3 && items[0].isRef) {
                        Value bv = deref(items[0]);
                        if (bv.index() == 10) {
                            Value offv = items[1].isRef ? deref(items[1]) : items[1].value;
                            Value dv = items[2].isRef ? deref(items[2]) : items[2].value;
                            std::string data;
                            if (dv.index() == 2) {
                                uint32_t n = static_cast<uint32_t>(static_cast<int32_t>(std::get<int64_t>(dv)));
                                data += static_cast<char>((n >> 24) & 0xFF);
                                data += static_cast<char>((n >> 16) & 0xFF);
                                data += static_cast<char>((n >> 8) & 0xFF);
                                data += static_cast<char>(n & 0xFF);
                            } else if (dv.index() == 10) {
                                data = std::get<std::shared_ptr<BufferValue>>(dv)->data;
                            } else if (dv.index() == 4) {
                                data = std::get<std::string>(dv);
                            }
                            int64_t off = toInt(offv);
                            if (off >= 0 && !data.empty()) {
                                auto buf = std::get<std::shared_ptr<BufferValue>>(bv);
                                if (off + static_cast<int64_t>(data.size()) >
                                    static_cast<int64_t>(buf->data.size()))
                                    buf->data.resize(static_cast<size_t>(off + data.size()), '\0');
                                buf->data.replace(static_cast<size_t>(off), data.size(), data);
                                written = static_cast<int64_t>(data.size());
                            }
                        }
                    }
                    pushValue(make_int(written));
                    break;
                }
                if (efun >= 0 && static_cast<size_t>(efun) < names.size() && names[efun] == "sscanf") {
                    // sscanf(input, fmt, &a, &b, ...): the output arguments are
                    // references on the stack.  Parse, write each conversion
                    // back through its reference, push the match count.
                    if (argc >= 2) {
                        auto strOf = [&](const StackItem& r) -> std::string {
                            Value v = r.isRef ? deref(r) : r.value;
                            if (v.index() == 4) return std::get<std::string>(v);
                            return to_string(v, false);
                        };
                        // Pop all args (top of stack = last argument pushed =
                        // args[0]).  args[0] = input, args[1] = format, the
                        // rest are reference outputs.
                        std::vector<StackItem> refs;
                        for (int i = 0; i < argc && !stack_.empty(); ++i) refs.push_back(pop());
                        std::string input = strOf(refs[0]);
                        std::string fmtStr = strOf(refs[1]);
                        std::vector<Value> outs;
                        // The original tracks the remaining output pointers;
                        // a trailing-input match only counts when a ref exists.
                        int maxOutputs = static_cast<int>(refs.size()) - 2;
                        int n = lpcSscanf(input, fmtStr, outs, maxOutputs);
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[sscanf] input=[" << input.substr(0, 30) << "] fmt=[" << fmtStr
                                      << "] n=" << n << " outs=" << outs.size() << "\n";
                        for (size_t i = 0; i + 2 < refs.size() && i < outs.size(); ++i)
                            writeThrough(refs[i + 2], outs[i]);
                        pushValue(make_int(n));
                        break;
                    }
                    if (names[efun] == "recv") {
                        // vm_f_recv (0x80e80c8): recv(fd, &buf, len) writes the
                        // received bytes back through the reference and returns
                        // the byte count.  Cooperative: when no data is
                        // available yet the process parks on the socket
                        // (kind 5) and the scheduler resumes it once the fd
                        // becomes readable.
                        std::vector<StackItem> items;
                        for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                        std::reverse(items.begin(), items.end());
                        if (items.size() >= 3) {
                            Value fdv = items[0].isRef ? deref(items[0]) : items[0].value;
                            Value lenv = items[2].isRef ? deref(items[2]) : items[2].value;
                            int fd = static_cast<int>(toInt(fdv));
                            int len = static_cast<int>(toInt(lenv));
                            if (std::getenv("LPCVM_TRACE"))
                                std::cerr << "[recv] fd=" << fd << " len=" << len << "\n";
                            bool ok = false;
                            std::string data = xdbcRecvAsync(fd, len, ok);
                            if (ok || !xdbcSocketExists(fd)) {
                                if (ok && !data.empty()) writeThrough(items[1], make_string(data));
                                if (std::getenv("LPCVM_TRACE"))
                                    std::cerr << "[recv] fd=" << fd << " got=" << data.size() << "\n";
                                pushValue(make_int(static_cast<int64_t>(data.size())));
                            } else {
                                // No data yet: park until readable.
                                ResumeCond cond;
                                cond.kind = 5;
                                cond.fd = fd;
                                cond.wantRead = true;
                                StackItem bufRef = items[1];
                                auto hook = [this, fd, len, bufRef]() mutable {
                                    bool rok = false;
                                    std::string d = xdbcRecvAsync(fd, len, rok);
                                    if (rok && !d.empty()) writeThrough(bufRef, make_string(d));
                                    resumeValue_ = make_int(static_cast<int64_t>(d.size()));
                                };
                                parkAndSuspend(cond, std::move(hook), make_int(0));
                                break;   // unreachable (SuspendSignal thrown)
                            }
                        } else {
                            pushValue(make_int(-1));
                        }
                        break;
                    }
                    if (names[efun] == "select") {
                        // vm_f_select: select(&read, &write, &exc, ms) filters
                        // the fd arrays in place and returns the ready count.
                        std::vector<StackItem> items;
                        for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                        std::reverse(items.begin(), items.end());
                        int timeoutMs = 0;
                        if (items.size() >= 4) {
                            Value tv = items[3].isRef ? deref(items[3]) : items[3].value;
                            timeoutMs = static_cast<int>(toInt(tv));
                        }
                        int readyCount = 0;
                        if (!items.empty()) {
                            Value arrVal = items[0].isRef ? deref(items[0]) : items[0].value;
                            if (arrVal.index() == 5) {
                                auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
                                std::vector<int> fds;
                                for (const auto& f : arr->items) fds.push_back(static_cast<int>(toInt(f)));
                                int nready = xdbcSelect(fds, timeoutMs);
                                auto out = std::make_shared<ArrayValue>();
                                for (int f : fds) out->items.push_back(make_int(f));
                                writeThrough(items[0], out);
                                readyCount = nready;
                            }
                        }
                        pushValue(make_int(readyCount));
                        break;
                    }
                }
                else if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                         names[efun] == "receive_from_queue") {
                    // vm_f_receive_from_queue (0x80f29f8):
                    //   receive_from_queue(qid, &msg, timeout)
                    // Blocks the process until an element arrives, writes it
                    // through the reference, and returns 0.  Returns -402 on
                    // timeout / empty-with-no-timeout and -101 if the queue
                    // does not exist.  The VM pushes args in reverse (param0
                    // on top), so pop() yields arg0 first.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    if (std::getenv("LPCVM_TRACE")) {
                        std::string dbg;
                        for (auto& it : items) {
                            Value v = it.isRef ? deref(it) : it.value;
                            if (!dbg.empty()) dbg += ",";
                            dbg += (it.isRef ? "&" : "") + to_string(v, false);
                        }
                        std::cerr << "[queue] receive argc=" << argc << " items=[" << dbg << "]\n";
                    }
                    if (items.size() >= 2) {
                        Value qv = items[0].isRef ? deref(items[0]) : items[0].value;
                        int64_t qid = queueResolveId(qv);
                        int timeoutMs = -1;
                        if (items.size() >= 3) {
                            Value tv = items[2].isRef ? deref(items[2]) : items[2].value;
                            double secs = tv.index() == 3 ? std::get<double>(tv)
                                                          : static_cast<double>(toInt(tv));
                            timeoutMs = secs < 0 ? -1 : static_cast<int>(secs * 1000.0);
                        }
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[queue] receive qid=" << qid
                                      << " timeoutMs=" << timeoutMs << "\n";
                        // Cooperative scheduling: try a non-blocking receive
                        // first; if the queue is empty, PARK this VM on the
                        // queue (send_to_queue wakes it) instead of blocking
                        // the OS thread.
                        bool ok = false;
                        Value msg = queueReceive(qid, 0, ok);
                        if (ok) {
                            writeThrough(items[1], msg);
                            pushValue(make_int(0));
                        } else {
                            // Park: wake up when send_to_queue pushes to this
                            // queue.  The resume hook writes the message back
                            // through the reference; resume value is 0.
                            ResumeCond cond;
                            cond.kind = 2;
                            cond.qkey = "q:" + std::to_string(qid);
                            auto hook = [this, item = items[1], qid]() mutable {
                                bool rok = false;
                                Value m = queueReceive(qid, 0, rok);
                                if (rok) writeThrough(item, m);
                            };
                            parkAndSuspend(cond, std::move(hook), make_int(0));
                        }
                    } else {
                        pushValue(make_int(-101));
                    }
                    break;
                }
                else if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                         names[efun] == "send_to_queue") {
                    // vm_f_send_to_queue (0x80f2d94): send_to_queue(qid, value,
                    // timeout) enqueues and wakes a receiver; 0 on success,
                    // -101 unknown queue.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    if (items.size() >= 2) {
                        Value qv = items[0].isRef ? deref(items[0]) : items[0].value;
                        Value v = items[1].isRef ? deref(items[1]) : items[1].value;
                        int64_t qid = queueResolveId(qv);
                        bool ok = queueSend(qid, v);
                        {
                            std::string cf = frames_.empty() || !frames_.back().fn
                                                 ? "<global>" : frames_.back().fn->name;
                            std::string qvdesc = (qv.index() == 2)
                                                     ? std::to_string(std::get<int64_t>(qv))
                                                     : ("t" + std::to_string(qv.index()));
                            std::cerr << "[q-send] qid=" << qid << " ok=" << ok
                                      << " obj=" << objName_ << " fn=" << cf
                                      << " this=" << thisObjectName_ << " qv=" << qvdesc << "\n";
                        }
                        if (!ok && std::getenv("LPCVM_TRACE")) {
                            // Detailed dump (trace only): what the LPC global
                            // resolved to + registered queues.
                            std::string cf = frames_.empty() || !frames_.back().fn
                                                 ? "<global>" : frames_.back().fn->name;
                            std::cerr << "[q-fail] qv=" << to_string(qv, false)
                                      << " qid=" << qid << " obj=" << objName_
                                      << " fn=" << cf << " reg={" << lpc::queueNameDump()
                                      << "} this=" << thisObjectName_ << "\n";
                        }
                        // Wake any process parked on this queue (cooperative
                        // scheduler): the message was already enqueued, the
                        // resumed receiver's hook dequeues it.
                        scheduler().wakeQueue("q:" + std::to_string(qid));
                        pushValue(make_int(ok ? 0 : -101));
                    } else {
                        pushValue(make_int(-101));
                    }
                    break;
                }
                else if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                         names[efun] == "sleep") {
                    // vm_f_sleep (0x80ec9e4): sleep(secs) parks the process
                    // on a timer; the scheduler resumes it when the timer
                    // fires.  Cooperative: never blocks the OS thread.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    double secs = 0;
                    if (!items.empty()) {
                        Value tv = items[0].isRef ? deref(items[0]) : items[0].value;
                        secs = tv.index() == 3 ? std::get<double>(tv)
                                               : static_cast<double>(toInt(tv));
                    }
                    if (secs <= 0.0) {
                        pushValue(make_int(1));
                    } else {
                        // Park on a timer for `secs` seconds.
                        ResumeCond cond;
                        cond.kind = 1;
                        using namespace std::chrono;
                        int64_t nowMs = duration_cast<milliseconds>(
                            steady_clock::now().time_since_epoch()).count();
                        cond.wakeAtMs = nowMs + static_cast<int64_t>(secs * 1000.0);
                        parkAndSuspend(cond, nullptr, make_int(1));
                    }
                    break;
                }
                else if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                         names[efun] == "wait_event") {
                    // vm_f_wait_event (0x80f097c): wait_event(name, mask, ref,
                    // timeout).  The original parks the current PROCESS on the
                    // event's pend list and resumes it from raise_event.  In
                    // our multithreaded port the waiter and the raiser can run
                    // on different OS threads (filed's do_operate blocks on the
                    // worker thread; the dba reply is dispatched by the recv
                    // thread into the SAME filed VM), so the block MUST release
                    // this VM's execution lock - otherwise the raiser could
                    // never enter the VM to raise the event (deadlock).  We
                    // detach the VM state, block on the real event, then
                    // re-acquire the lock and restore.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    std::string ename;
                    Value eref;
                    if (!items.empty()) {
                        eref = items[0].isRef ? deref(items[0]) : items[0].value;
                        if (eref.index() == 4) ename = std::get<std::string>(eref);
                        else if (eref.index() == 2) ename = "oid:" + std::to_string(std::get<int64_t>(eref));
                    }
                    uint32_t mask = 0;
                    if (items.size() >= 2) {
                        Value mv = items[1].isRef ? deref(items[1]) : items[1].value;
                        mask = static_cast<uint32_t>(toInt(mv));
                    }
                    double secs = -1.0;
                    if (items.size() >= 4) {
                        Value tv = items[3].isRef ? deref(items[3]) : items[3].value;
                        if (tv.index() == 2) secs = static_cast<double>(std::get<int64_t>(tv));
                        else if (tv.index() == 3) secs = std::get<double>(tv);
                    }
                    if (ename.rfind("oid:", 0) == 0 && std::getenv("LPCVM_SVC_TRACE")) {
                        std::string wf = frames_.empty() || !frames_.back().fn
                                              ? "<global>" : frames_.back().fn->name;
                        std::cerr << "[wait-oid] " << ename << " obj=" << objName_
                                  << " fn=" << wf << " frames=" << frames_.size()
                                  << " stack=" << stack_.size() << "\n";
                    }
                    if (std::getenv("LPCVM_EVENT_TRACE"))
                        std::cerr << "[wait_event] " << ename << " mask=0x" << std::hex
                                  << mask << std::dec << " secs=" << secs
                                  << " vm=" << this << " obj=" << objName_ << "\n";
                    // Cooperative scheduling: probe with timeout 0 first.  If
                    // the event is already raised, return immediately; else
                    // PARK this VM on the event (raise_event wakes it).  A
                    // finite timeout also parks with a wake timer (stage C).
                    int64_t probe = lpc::eventWait(eref, mask, 0.0);
                    if (probe == 0) {
                        if (std::getenv("LPCVM_EVENT_TRACE"))
                            std::cerr << "[wait_event] immediate " << ename << "\n";
                        pushValue(make_int(0));
                    } else if (secs < 0 || secs > 0.0001) {
                        ResumeCond cond;
                        cond.kind = 3;
                        cond.ename = ename;
                        // A finite timeout ALSO parks with a wake timer (stage
                        // C): raise_event before the park must not leave the
                        // waiter asleep forever.  The timer fires at
                        // now + secs and resumes with the timeout result.
                        if (secs >= 0) {
                            using namespace std::chrono;
                            int64_t nowMs = duration_cast<milliseconds>(
                                steady_clock::now().time_since_epoch()).count();
                            cond.wakeAtMs = nowMs + static_cast<int64_t>(secs * 1000.0);
                        }
                        // The resume hook consumes the raised bits via a
                        // blocking-free wait on the already-raised event.
                        auto hook = [this, eref, mask]() {
                            bool rok = false;
                            int64_t rc = lpc::eventWait(eref, mask, 0.0);
                            (void)rc;
                        };
                        parkAndSuspend(cond, std::move(hook), make_int(0));
                    } else {
                        // zero timeout, not raised -> timeout
                        pushValue(make_int(-501));
                    }
                    break;
                }
                else if (efun >= 0 && static_cast<size_t>(efun) < names.size() &&
                         names[efun] == "raise_event") {
                    // vm_f_raise_event (0x80f07b8): raise_event(name, mask)
                    // ORs mask into the event and wakes blocked waiters.  Not
                    // blocking, so no lock release needed.
                    std::vector<StackItem> items;
                    for (int i = 0; i < argc && !stack_.empty(); ++i) items.push_back(pop());
                    std::string ename;
                    Value eref;
                    if (!items.empty()) {
                        eref = items[0].isRef ? deref(items[0]) : items[0].value;
                        if (eref.index() == 4) ename = std::get<std::string>(eref);
                        else if (eref.index() == 2) ename = "oid:" + std::to_string(std::get<int64_t>(eref));
                    }
                    uint32_t mask = 0;
                    if (items.size() >= 2) {
                        Value mv = items[1].isRef ? deref(items[1]) : items[1].value;
                        mask = static_cast<uint32_t>(toInt(mv));
                    }
                    int64_t rc = lpc::eventRaise(eref, mask);
                    if (std::getenv("LPCVM_EVENT_TRACE"))
                        std::cerr << "[raise_event] " << ename << " mask=0x" << std::hex
                                  << mask << std::dec << " -> " << rc
                                  << " vm=" << this << " obj=" << objName_ << "\n";
                    // Wake any process parked on this event (cooperative
                    // scheduler).  The resumed waiter's hook consumes the
                    // raised bits.
                    if (rc == 0) scheduler().wakeEvent(ename);
                    pushValue(make_int(rc));
                    break;
                }
                if (efunInlineStarted_) {
                    // A leaked flag (a previous inline start whose callee
                    // returned without another efun call) must not swallow
                    // THIS efun: consume it and dispatch normally.
                    efunInlineStarted_ = false;
                    pushValue(callEfun(efun, argc));
                } else {
                    // call_other_arg sets the flag DURING its dispatch (when it
                    // starts the callee inline).  The callee's return op pushes
                    // the result, so the placeholder call_other_arg returned
                    // must NOT be pushed - otherwise the flag check above (which
                    // runs BEFORE the call) would leak the flag into the
                    // callee's FIRST efun and swallow it (e.g. read_content's
                    // mysql_bind_parameters, dropping the WHERE clause).
                    Value r = callEfun(efun, argc);
                    if (efunInlineStarted_) {
                        efunInlineStarted_ = false;   // callee return pushes the result
                    } else {
                        pushValue(r);
                    }
                }
                if (std::getenv("LPCVM_TRACE") && !frames_.empty() && frames_.back().fn &&
                    frames_.back().fn->name == "save_to_text" && efun == 102)
                    std::cerr << "[efun-dbg] save_string after: stack=" << stack_.size()
                              << " top=" << to_string(top().value, false) << " idx=" << top().value.index() << "\n";
                break;
            }
            case 7: { // call_in_user_fun
                int funIndex = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int flags = fromMeta ? m1 : in.b;
                // vm_callInUserFun (0x8094080): bit 7 of the flags selects the
                // GLOBAL program's function table; otherwise the index is
                // LOCAL to the current function's constitution and must be
                // offset by consititution->baseFunOffset.  Inherited
                // functions (relay_agent in feature/common/agent.o etc.) call
                // their own program's functions by local index, so the
                // accumulated baseFunOffset of the CURRENT frame applies.
                int off = 0;
                if ((flags & 0x80) == 0) {
                    off = frames_.empty() ? 0 : frames_.back().funOffset;
                    if (off != 0) funIndex += off;
                }
                if (std::getenv("LPCVM_TRACE") && !frames_.empty() && frames_.back().fn &&
                    frames_.back().fn->name == "save_to_text") {
                    auto it = funcsByIndex_.find(funIndex);
                    std::cerr << "[cfu-dbg] save_to_text call a=" << (fromMeta ? m0 : static_cast<int16_t>(in.a))
                              << " funIndex=" << funIndex << " off=" << off
                              << " -> " << (it != funcsByIndex_.end() ? it->second->name : "NOT FOUND")
                              << " flags=0x" << std::hex << flags << std::dec << "\n";
                }
                if ((std::getenv("LPCVM_EVENT_TRACE")) &&
                    objName_.find("filed.o") != std::string::npos &&
                    (funIndex >= 108 && funIndex <= 115)) {
                    std::string nm;
                    auto it = funcsByIndex_.find(funIndex);
                    if (it != funcsByIndex_.end()) nm = it->second->name;
                    std::cerr << "[cfu-filed] idx=" << funIndex << " name=" << nm
                              << " flags=0x" << std::hex << flags << std::dec
                              << " obj=" << objName_ << " vm=" << this << "\n";
                }
                Value r7 = make_int(0);
                bool started = callUserFunction(funIndex, flags, r7);
                // Inline start: the callee's return op will push its own
                // result; do NOT push anything here.
                if (!started) pushValue(r7);
                break;
            }
            case 8: { // call_in_other_fun_ex
                Value pathVal = pop().value;
                Value funVal = pop().value;
                int argn = fromMeta ? m1 : in.b;
                if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_FN") &&
                    frames_.back().fn && frames_.back().fn->name == std::getenv("LPCVM_TRACE_FN"))
                    std::cerr << "[case8-t] cur=" << frames_.back().fn->name
                              << " pathIdx=" << pathVal.index()
                              << " path=" << (pathVal.index() == 4 ? std::get<std::string>(pathVal).substr(0, 60) : "?")
                              << " funIdx=" << funVal.index()
                              << " fun=" << (funVal.index() == 4 ? std::get<std::string>(funVal) : "?")
                              << " argn=" << argn << " stack=" << stack_.size() << "\n";
                if (std::getenv("LPCVM_TRACE") && !frames_.empty() && frames_.back().fn &&
                    funVal.index() == 4 && std::get<std::string>(funVal) == "save_to_text")
                    std::cerr << "[case8-dbg] save_to_text cur=" << frames_.back().fn->name
                              << " stack=" << stack_.size() << " argn=" << argn
                              << " frames=" << frames_.size() << "\n";
                if (std::getenv("LPCVM_TRACE")) {
                    std::string cur = frames_.empty() || !frames_.back().fn ? "<global>" : frames_.back().fn->name;
                    std::string fv = funVal.index() == 4 ? std::get<std::string>(funVal) : "";
                    if (fv == "lite_query_temp")
                        std::cerr << "[case8-lqt] pathIdx=" << static_cast<int>(pathVal.index())
                                  << " path='" << to_string(pathVal, false).substr(0, 60)
                                  << "' cur=" << cur << " argn=" << argn << "\n";
                }
                // NOTE: the arguments are NOT popped yet.  Same-VM calls
                // (this_object()/self paths) run the callee INLINE on the
                // current stack (executeFunctionOnStack, like call_in_user_fun)
                // so reference parameters (&path, &name, &branch in
                // get_save_path) stay as live refs to the caller's slots and
                // assignments inside the callee write back to the caller.
                // Only genuinely cross-VM / cross-object calls pop the args.
                std::string funName = funVal.index() == 4 ? std::get<std::string>(funVal) : "";
                int funIndex = -1;
                if (funVal.index() == 2) funIndex = static_cast<int>(std::get<int64_t>(funVal));
                if (funName.empty() && funIndex >= 0) {
                    auto fit = funcsByIndex_.find(funIndex);
                    if (fit != funcsByIndex_.end()) funName = fit->second->name;
                }
                Value result = make_int(0);
                bool found = false;
                bool argsConsumed = false;   // true once popArgs() or the inline
                                             // executeFunctionOnStack has taken
                                             // the argument slots off the stack
                // True when resolveSelf() started the callee INLINE (frame
                // pushed, code_/meta_/ip_ switched).  The callee's return op
                // will then push its own result, so the case-8 epilogue must
                // NOT pushValue(result) - that would double the value.
                bool inlineStarted = false;
                // The target object path, if the call names one.  The original
                // driver resolves the function in that object; only when the
                // path is absent/self do we fall back to the current object
                // (which covers inherited function calls).
                auto popArgs = [&]() {
                    std::vector<Value> out;
                    for (int i = 0; i < argn && !stack_.empty(); ++i) {
                        StackItem it = pop();
                        // Reference arguments (&var in call_other, e.g.
                        // importd->create_data(file, &_form_index, &_form_value)
                        // from form2's attach_form_file): the ref item's .value
                        // is nil, so flattening it to undefined would hand the
                        // callee a nil mapping/array and the caller's object
                        // variable would never receive the callee's writes.
                        // Deref to the LIVE value instead - mappings/arrays are
                        // shared_ptr-backed, so the callee's in-place mutation
                        // (attrib_list[section] = ...) still lands in the
                        // caller's variable.  (Reassignment of the whole ref,
                        // e.g. attrib_index = keys(...), cannot propagate
                        // across the VM boundary and is a known limitation.)
                        if (it.isRef) {
                            Value dv = deref(it);
                            if (std::getenv("LPCVM_TRACE") && funName == "create_data")
                                std::cerr << "[popref] create_data arg[" << i << "] refKind="
                                          << (it.slotIndex >= 0 ? "slot" : it.globalIndex >= 0 ? "global"
                                             : it.objIndex >= 0 ? "obj" : "member")
                                          << " idx=" << it.objIndex << " derefType=" << dv.index() << "\n";
                            // Object-variable references must stay WRITABLE
                            // across the VM boundary: pass a CrossVmRef so the
                            // callee's assignments (e.g. importd.create_data's
                            // attrib_index += (...)) land in THIS VM's object
                            // variable.  Passing the deref'd value would make a
                            // reassigned array invisible to the caller.
                            if (it.objIndex >= 0) {
                                auto r = std::make_shared<CrossVmRef>();
                                r->vm = this;
                                r->objIndex = it.objIndex;
                                out.push_back(make_crossvm_ref(std::move(r)));
                            } else {
                                out.push_back(dv);
                            }
                        } else {
                            out.push_back(it.value);
                        }
                    }
                    argsConsumed = true;
                    return out;
                };
                auto resolveSelf = [&]() {
                    const OriginalFunction* selfFn = nullptr;
                    if (funIndex >= 0) {
                        // direct inherited/own function index
                        auto fit = funcsByIndex_.find(funIndex);
                        if (fit != funcsByIndex_.end()) selfFn = fit->second;
                    }
                    if (!selfFn) selfFn = findMergedFunction(funName);
                    if (selfFn && selfFn->index >= 0) {
                        auto fit = funcsByIndex_.find(selfFn->index);
                        if (fit != funcsByIndex_.end()) {
                            const OriginalFunction& fn = *fit->second;
                            // Pull the passed args off the stack (StackItems, so
                            // reference parameters keep their slot pointers),
                            // pad missing declared defaults BELOW them, then push
                            // back so param0 ends on top - startInlineFunction
                            // expects [paramN-1 ... param0(top)] exactly like
                            // executeFunctionFromOutsideLocked builds it.
                            std::vector<StackItem> real;
                            for (int i = 0; i < argn && !stack_.empty(); ++i) real.push_back(pop());
                            // real[0] = param0 (popped from top first).
                            int declared = fn.argCount;
                            int pad = (declared > argn) ? declared - argn : 0;
                            for (int i = 0; i < pad; ++i) pushValue(Value{});
                            for (int i = static_cast<int>(real.size()) - 1; i >= 0; --i)
                                push(real[static_cast<size_t>(i)]);
                            if (declared == 0 && argn == 0) {
                                // executeFunctionFromOutsideLocked pushes one
                                // placeholder slot for 0-arg calls so bp >= 0;
                                // mirror that here.
                                pushValue(Value{});
                                inlineStarted = startInlineFunction(fn, true, 0);
                            } else {
                                inlineStarted = startInlineFunction(fn, false, argn);
                            }
                            found = true;
                            argsConsumed = true;   // frame args popped with the frame
                        }
                    }
                };
                if (funName == "process_command" || funName == "cmd_internal_auth" ||
                    funName == "relay_agent" || funName == "main" || funName == "send_message") {
                    std::string pv = pathVal.index() == 4 ? std::get<std::string>(pathVal)
                        : (pathVal.index() == 7 ? std::get<std::shared_ptr<ObjectValue>>(pathVal)->programName : "?");
                    std::cerr << "[case8] " << objName_ << " -> " << pv << "::" << funName
                              << " argc=" << argn << "\n";
                }
                if (pathVal.index() == 4) {
                    std::string path = std::get<std::string>(pathVal);
                    if (std::getenv("LPCVM_TRACE_C8")) {
                        std::string cur = frames_.empty() || !frames_.back().fn ? "<top>" : frames_.back().fn->name;
                        std::cerr << "[c8] vm=" << this << " obj=" << objName_ << " fn=" << cur
                                  << " pathIdx=" << pathVal.index() << " funIdx=" << funVal.index()
                                  << " path='" << path << "' funName='" << funName << "'"
                                  << " funIndex=" << funIndex << " argn=" << argn << "\n";
                    }
                    if (std::getenv("LPCVM_TRACE") && funName == "wait_config")
                        std::cerr << "[case8-wc] path=" << path << " fun=" << funName
                                  << " argn=" << argn << " pathIdx=" << pathVal.index()
                                  << " funIdx=" << funVal.index() << "\n";
                    if (path.empty() || path == "<this>" || path == ".") {
                        resolveSelf();
                    } else {
                        // Cross-object call: the original driver switches the
                        // process's constitution to the target object and runs
                        // the callee INLINE on the SAME process stack, so a
                        // blocking efun inside it parks the whole process and
                        // resume continues through the callee back to the
                        // caller.  Running it on a separate registry VM breaks
                        // that (the parked state lives in a VM that has no
                        // process; SuspendSignal escapes).  Mirror the inline
                        // semantics: resolve the target object + function and
                        // execute on THIS VM's stack with objVars switched.
                        auto args = popArgs();
                        std::shared_ptr<OriginalVm> targetVm;
                        const OriginalFunction* targetFn = nullptr;
                        {
                            std::string rel = path;
                            if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
                            if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
                            auto& reg1 = objectRegistry();
                            std::lock_guard<std::recursive_mutex> rl1(registryMutex());
                            for (auto& kv : reg1.vms) {
                                if (hasSuffix(kv.first, rel)) { targetVm = kv.second; break; }
                            }
                        }
                        if (targetVm) {
                            if (funName.empty()) targetFn = targetVm->functionByIndex(funIndex);
                            else targetFn = targetVm->findFunctionAny(funName);
                        }
                        if (std::getenv("LPCVM_TRACE") && funName == "wait_config")
                            std::cerr << "[case8-wc2] targetVm=" << (targetVm ? targetVm->objName() : "NULL")
                                      << " targetFn=" << (targetFn ? targetFn->name : "NULL")
                                      << " fns=" << (targetVm ? targetVm->object().functions.size() : -1) << "\n";
                        if (std::getenv("LPCVM_TRACE_C8"))
                            std::cerr << "[c8-res] targetVm=" << (targetVm ? targetVm->objName() : "NULL")
                                      << " targetFn=" << (targetFn ? targetFn->name : "NULL")
                                      << " fnCode=" << (targetFn ? targetFn->code.size() : -1)
                                      << " busy=" << (targetVm ? lpc::scheduler().isVmParked(targetVm.get()) : -1)
                                      << "\n";
                        if (targetFn && targetFn->index >= 0 && !targetFn->code.empty() &&
                            targetVm->objName().find("filed.o") != std::string::npos &&
                            objName_.find('#') == std::string::npos) {
                            // Cross-object inline for filed.o service calls (a
                            // blocking efun parks the whole caller process).  The
                            // serviced registry-VM path is single-flight: two
                            // callers parking inside the SAME busy object (e.g.
                            // filed.o read_content from a threadd job and from
                            // master's winter_vacationd create) collide on one
                            // parked process - one waiter's ev_filed park/key
                            // overwrites the other's, its operate_done never
                            // fires, and the construct hangs master's load_all
                            // forever.  When the target is IDLE the serviced
                            // path is kept: it resolves the callee in the
                            // TARGET's table by name, which index-based call
                            // sites depend on (gs's load_dist_start_time
                            // filed.read_content compiled to a local index that
                            // must NOT be re-resolved against the master's
                            // merged table).
                            const OriginalFunction& fn = *targetFn;
                            int declared = fn.argCount;
                            int pad = (declared > static_cast<int>(args.size())) ? declared - static_cast<int>(args.size()) : 0;
                            for (int i = 0; i < pad; ++i) pushValue(Value{});
                            for (int i = static_cast<int>(args.size()) - 1; i >= 0; --i)
                                pushValue(args[static_cast<size_t>(i)]);
                            if (declared == 0 && args.empty()) {
                                pushValue(Value{});
                                inlineStarted = startInlineFunction(fn, true, 0,
                                                                    targetVm->objVarsShared(),
                                                                    &targetVm->object().strings,
                                                                    std::getenv("LPCVM_NO_CONST")
                                                                        ? nullptr
                                                                        : &targetVm->functionTable());
                            } else {
                                inlineStarted = startInlineFunction(fn, false, static_cast<int>(args.size()),
                                                                    targetVm->objVarsShared(),
                                                                    &targetVm->object().strings,
                                                                    std::getenv("LPCVM_NO_CONST")
                                                                        ? nullptr
                                                                        : &targetVm->functionTable());
                            }
                            if (!inlineStarted) {
                                // Stack too shallow: pop the pushed args back and
                                // fall back to the registry-VM path.
                                for (int i = 0; i < pad; ++i) pop();
                                for (size_t i = 0; i < args.size(); ++i) pop();
                                if (funName.empty()) {
                                    result = runDetached([&] {
                                        return tryCallOtherObjectByIndex(searchRoot_, path, funIndex, args, thisObjectName_);
                                    });
                                } else {
                                    result = runDetached([&] {
                                        return tryCallOtherObject(searchRoot_, path, funName, args, thisObjectName_);
                                    });
                                }
                                found = true;
                            } else {
                                found = true;
                                argsConsumed = true;
                            }
                        } else {
                            // No target / no function: fall back to the old
                            // registry-VM path (returns the efun's value).
                            if (funName.empty()) {
                                result = runDetached([&] {
                                    return tryCallOtherObjectByIndex(searchRoot_, path, funIndex, args, thisObjectName_);
                                });
                            } else {
                                result = runDetached([&] {
                                    return tryCallOtherObject(searchRoot_, path, funName, args, thisObjectName_);
                                });
                            }
                            found = true;
                        }
                    }
                } else if (pathVal.index() == 7) {
                    // Object value: resolve through the clone/registry key.
                    auto ov = std::get<std::shared_ptr<ObjectValue>>(pathVal);
                    std::string path = ov->programName;
                    if (path == "<this>" || path == "." || path.empty()) {
                        // object(<this>) captured at creation time: resolve
                        // through the OWNER's registry VM, not the current
                        // (callee) VM.  handler.c's create receives
                        // (["module": object(<this>)]) from load_object and
                        // does call_other(module, "register_entry", ...); the
                        // module object is the loader (master), so it must map
                        // to the master's registered VM.
                        if (ov->owner) {
                            auto& reg0 = objectRegistry();
                            std::shared_ptr<OriginalVm> ownerVm;
                            {
                                std::lock_guard<std::recursive_mutex> rl0(registryMutex());
                                for (auto& kv : reg0.vms) {
                                    if (&kv.second->object() == ov->owner) {
                                        ownerVm = kv.second;
                                        break;
                                    }
                                }
                            }
                            if (ownerVm) {
                                if (std::getenv("LPCVM_TRACE"))
                                    std::cerr << "[obj-this] owner-resolve -> "
                                              << ownerVm->object().sourceName
                                              << (ownerVm.get() == this ? " (same VM)" : "") << "\n";
                                if (ownerVm.get() == this) {
                                    // object(<this>) owned by THIS VM (e.g.
                                    // save.c's this_object()->get_save_path(&path,
                                    // &name, &branch)): run the callee inline on
                                    // the current stack so reference arguments
                                    // keep pointing at the caller's slots.
                                    resolveSelf();
                                } else if (funName.empty()) {
                                    auto args = popArgs();
                                    result = runDetached([&] { return ownerVm->callFunctionByIndex(funIndex); });
                                    found = true;
                                } else {
                                    auto args = popArgs();
                                    ownerVm->setPreviousObjectName(thisObjectName_);
                                    result = runDetached([&] { return ownerVm->callFunctionByName(funName, args); });
                                    found = true;
                                }
                                // NOTE: no break here!  A break would exit the
                                // case-8 switch and skip the pushValue(result)
                                // below, dropping the callee's return value from
                                // the stack (e.g. save_to_text's "([])" string
                                // vanished, so save.c's `data = save_to_text()`
                                // read garbage and share_datad's restore loop
                                // retried forever).  Fall through to the common
                                // pushValue at the end of case 8 instead.
                            } else {
                                resolveSelf();
                            }
                        }
                    } else {
                        auto args = popArgs();
                        if (funName.empty()) {
                            result = runDetached([&] {
                                return tryCallOtherObjectByIndex(searchRoot_, path, funIndex, args);
                            });
                            found = true;
                        } else {
                            result = runDetached([&] {
                                return tryCallOtherObject(searchRoot_, path, funName, args);
                            });
                            found = true;
                        }
                    }
                }
                if (!found && pathVal.index() == 4 && !funName.empty()) {
                    auto args = popArgs();
                    result = runDetached([&] {
                        return tryCallOtherObject(searchRoot_, std::get<std::string>(pathVal), funName, args,
                                                  thisObjectName_);
                    });
                }
                // If nothing consumed the argument slots (no function found at
                // all), drop them so they don't leak into the caller's stack.
                if (!argsConsumed) {
                    for (int i = 0; i < argn && !stack_.empty(); ++i) pop();
                }
                // An inline start means the callee's return_from_fun will push
                // its own result; do NOT pushValue(result) in that case.
                if (!inlineStarted) pushValue(result);
                if (std::getenv("LPCVM_TRACE") && funVal.index() == 4 &&
                    std::get<std::string>(funVal) == "get_registered_name")
                    std::cerr << "[grn] result=[" << to_string(result, false).substr(0, 60)
                              << "] cur=" << (frames_.empty() || !frames_.back().fn
                                                  ? "<top>" : frames_.back().fn->name) << "\n";
                if (std::getenv("LPCVM_TRACE") && funVal.index() == 4 &&
                    std::get<std::string>(funVal) == "save_to_text")
                    std::cerr << "[case8-ret] save_to_text result=" << to_string(result, false)
                              << " idx=" << result.index() << " stack=" << stack_.size()
                              << " cur=" << (frames_.empty() || !frames_.back().fn
                                                 ? "<top>" : frames_.back().fn->name) << "\n";
                break;
            }
            case 5: { // call_fun_ptr
                int mayNull = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int total = fromMeta ? m1 : in.b;
                StackItem funItem = pop();
                std::vector<Value> args;
                for (int i = 1; i < total && !stack_.empty(); ++i) args.push_back(pop().value);
                Value result = make_int(0);
                // True when a same-owner/global closure was started INLINE
                // (its return op will push the result; we must not).
                bool inlineStarted = false;
                // The master's post_init() loops `_post_init_funcs` and calls
                // each (*f)() closure SYNCHRONOUSLY.  In the original driver a
                // daemon init's blocking wait (read_etc/do_operate -> dba
                // round-trip) merely parks that process and the cooperative
                // scheduler moves on to the next init, so post_init() returns
                // after starting ALL daemons.  Our real threads must not block
                // post_init on one daemon's round-trip (ccs_questiond's
                // auto_recover_read leaves the post_init VM stuck forever and
                // ccs_serverd's init - which starts the 8110 listener - never
                // runs).  Detach the init closures instead: post_init returns
                // promptly, each init completes its waits on its own thread.
                bool inPostInit = false;
                if (!frames_.empty() && frames_.back().fn &&
                    frames_.back().fn->name == "post_init")
                    inPostInit = true;
                if (funItem.value.index() == 8) {
                    auto cl = std::get<std::shared_ptr<ClosureValue>>(funItem.value);
                    if (std::getenv("LPCVM_EVENT_TRACE"))
                        std::cerr << "[cfptr] " << cl->functionName << " obj=" << objName_
                                  << " bound=" << cl->boundArgs.size() << " inPostInit=" << inPostInit
                                  << " vm=" << this << "\n";
                    if ((std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) &&
                        (cl->functionName == "init" || cl->functionName == "do_init" ||
                         cl->functionName == "listen_daemon" || cl->functionName == "accept_daemon" ||
                         cl->functionName == "update_config_daemon" || cl->functionName == "daemon"))
                        std::cerr << "[cfptr] " << cl->functionName << " obj=" << objName_
                                  << " bound=" << cl->boundArgs.size() << " vm=" << this
                                  << " inPostInit=" << inPostInit << "\n";
                    if (cl->functionName == "do_when_new_connection" ||
                        cl->functionName == "register_auth" || cl->functionName == "register_ban" ||
                        cl->functionName == "auth_connection") {
                        std::cerr << "[auth] call_fun_ptr " << cl->functionName
                                  << " obj=" << objName_ << " bound=" << cl->boundArgs.size()
                                  << " args=" << args.size() << "\n";
                    }
                    if ((std::getenv("LPCVM_EVENT_TRACE")) &&
                        cl->functionName == "operate_done") {
                        std::string dbg;
                        for (auto& b : cl->boundArgs) {
                            if (!dbg.empty()) dbg += ", ";
                            dbg += to_string(b, false).substr(0, 30);
                        }
                        std::cerr << "[cfptr] operate_done bound=[" << dbg << "] args=[";
                        dbg.clear();
                        for (auto& a : args) {
                            if (!dbg.empty()) dbg += ", ";
                            dbg += to_string(a, false).substr(0, 30);
                        }
                        std::cerr << dbg << "] argc=" << args.size() << " vm=" << this
                                  << " obj=" << objName_ << "\n";
                    }
                    std::vector<Value> all = cl->boundArgs;
                    all.insert(all.end(), args.begin(), args.end());
                    const OriginalFunction* fn = nullptr;
                    const OriginalObject* master = objectRegistry().masterObj;
                    bool sameOwner = (cl->owner == nullptr) || (cl->owner == &obj_) || (cl->owner == master);
                    // Prefer the function captured at closure-creation time:
                    // create_function_value(n) binds the EXACT merged function
                    // at index n.  Re-resolving by name later can pick a
                    // DIFFERENT (overriding) copy in the current VM - e.g.
                    // handler_module's process_command (merged 32) vs
                    // accountd's own process_command (153) - which changed the
                    // dispatch target and caused an infinite re-dispatch loop.
                    if (cl->func && sameOwner) {
                        // global function (owner = master) or same-object:
                        // run inline on THIS VM's stack/obj vars.
                        fn = cl->func;
                    } else if (cl->funcIndex >= 0 && !cl->func) {
                        // legacy closure without a func pointer
                        auto fit = funcsByIndex_.find(cl->funcIndex);
                        if (fit != funcsByIndex_.end()) {
                            fn = fit->second;
                        } else if (const OriginalFunction* gfn = findMasterFunction(cl->funcIndex)) {
                            fn = gfn;
                        }
                    }
                    if (std::getenv("LPCVM_TRACE") && cl->funcIndex >= 0)
                        std::cerr << "[cfv] closure by-index " << cl->functionName
                                  << " idx=" << cl->funcIndex
                                  << " -> " << (fn ? fn->name : "?") << "\n";
                    if (fn) {
                        if (inPostInit) {
                            // post_init: run the init INLINE (synchronously) in
                            // the original driver's post_init loop order - the
                            // master's post_init() calls each do_init closure
                            // one after another on the single thread.  A
                            // blocking efun inside the init parks the VM
                            // (SuspendSignal) and the scheduler resumes it; it
                            // never spawns a second OS thread.
                            if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE"))
                                std::cerr << "[postinit-sync] " << cl->functionName << " on " << obj_.sourceName << "\n";
                            bool ph = pushArgsForFunction(*fn, all);
                            inlineStarted = startInlineFunction(*fn, ph, static_cast<int>(all.size()));
                        } else {
                            if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) {
                                std::string adbg;
                                for (auto& a : all) {
                                    if (!adbg.empty()) adbg += ", ";
                                    adbg += to_string(a, false).substr(0, 25);
                                }
                                std::cerr << "[cfptr-call] " << cl->functionName << " n=" << all.size()
                                          << " all=[" << adbg << "]\n";
                            }
                            // Same-owner / global closure: run INLINE on this
                            // VM's stack via the explicit stack machine (frame
                            // push + stream switch); the callee's return op
                            // pushes the result itself.
                            bool ph = pushArgsForFunction(*fn, all);
                            inlineStarted = startInlineFunction(*fn, ph, static_cast<int>(all.size()));
                        }
                        if ((std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) &&
                            (cl->functionName == "init" || cl->functionName == "do_init"))
                            std::cerr << "[cfptr] DONE " << cl->functionName << " obj=" << objName_
                                      << " vm=" << this << "\n";
                    } else if (cl->owner && cl->owner != &obj_) {
                        // The closure was created by another object (e.g. the
                        // master's post_init loop calling a daemon's do_init):
                        // run it on the OWNER's VM with the owner's object vars.
                        auto& reg = objectRegistry();
                        std::shared_ptr<OriginalVm> ownerVm;
                        {
                            std::lock_guard<std::recursive_mutex> rl(registryMutex());
                            for (auto& kv : reg.vms) {
                                if (&kv.second->object() == cl->owner) {
                                    if (std::getenv("LPCVM_TRACE"))
                                        std::cerr << "[cfv] owner-resolve " << cl->functionName
                                                  << " in " << kv.first << "\n";
                                    ownerVm = kv.second;
                                    break;
                                }
                            }
                        }
                        if (ownerVm) {
                            if ((std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) &&
                                (cl->functionName == "init" || cl->functionName == "do_init"))
                                std::cerr << "[cfptr] owner-init " << cl->functionName
                                          << " on " << ownerVm->object().sourceName
                                          << " vm=" << this << "\n";
                            if (std::getenv("LPCVM_TRACE"))
                                std::cerr << "[cfv] calling " << cl->functionName << " on " << ownerVm->object().sourceName
                                          << " args=" << all.size() << "\n";
                            // Use the captured function pointer when available so
                            // an overloaded name (process_command) resolves to the
                            // exact function the closure was bound to.
                            if (inPostInit) {
                                // post_init: run the init INLINE (synchronously)
                                // on the owner's VM, matching the original
                                // driver's sequential post_init loop.  No second
                                // OS thread - blocking efuns park the process.
                                if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE"))
                                    std::cerr << "[postinit-sync] " << cl->functionName
                                              << " on " << ownerVm->object().sourceName << "\n";
                                ownerVm->setPreviousObjectName(thisObjectName_);
                                try {
                                    if (cl->func) result = ownerVm->callFunctionPointer(cl->func, all);
                                    else result = ownerVm->callFunctionByName(cl->functionName, all);
                                } catch (const VmError& e) {
                                    // The LPC post_init loop wraps each
                                    // (*f)() in try/catch and continues; an
                                    // error from one daemon init (e.g. a task
                                    // template register that fails validation)
                                    // must not kill the master's post_init.
                                    if (std::getenv("LPCVM_TRACE"))
                                        std::cerr << "[postinit-err] " << cl->functionName
                                                  << " on " << ownerVm->object().sourceName
                                                  << ": " << e.what() << "\n";
                                    result = make_int(0);
                                } catch (const SuspendSignal&) {
                                    throw;   // park: the scheduler resumes the callee
                                } catch (const TimeSliceSignal&) {
                                    throw;   // yield: the scheduler resumes the callee
                                } catch (...) {
                                    result = make_int(0);
                                }
                            } else if (cl->func)
                                result = runDetached([&] { return ownerVm->callFunctionPointer(cl->func, all); });
                            else
                                result = runDetached([&] { return ownerVm->callFunctionByName(cl->functionName, all); });
                            if ((std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE")) &&
                                (cl->functionName == "init" || cl->functionName == "do_init"))
                                std::cerr << "[cfptr] owner-init DONE " << cl->functionName
                                          << " on " << ownerVm->object().sourceName << "\n";
                        } else if (inPostInit) {
                            // post_init closure whose owner VM is gone (or the
                            // closure's owner is not in the registry): the LPC
                            // post_init loop's try/catch would swallow an error
                            // from a daemon init and continue with the next
                            // closure.  Mirror that here so one failing init
                            // (e.g. a task template register that errors) cannot
                            // kill the master's post_init loop and stall the
                            // whole startup.
                            if (std::getenv("LPCVM_TRACE"))
                                std::cerr << "[postinit-skip] " << cl->functionName
                                          << " owner VM missing, continue\n";
                        }
                    } else {
                        // The closure may reference a builtin efun (e.g. the
                        // mysql_connect closure created by find_function).
                        const auto& names = efunNameTable();
                        for (size_t i = 0; i < names.size(); ++i) {
                            if (names[i] == cl->functionName) {
                                result = callEfunWithArgs(static_cast<int>(i), all);
                                break;
                            }
                        }
                    }
                } else if (funItem.value.index() == 4) {
                    const std::string& fnName = std::get<std::string>(funItem.value);
                    const OriginalFunction* fn = findFunctionAny(fnName);
                    if (fn) {
                        bool ph = pushArgsForFunction(*fn, args);
                        inlineStarted = startInlineFunction(*fn, ph, static_cast<int>(args.size()));
                    }
                }
                if (funItem.value.index() == 0 || funItem.value.index() == 1) {
                    // null function value
                }
                if (!inlineStarted) pushValue(result);
                break;
            }
            case 33: { // if: jump when isZero != bit0; pop when bit1 set
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int flags = fromMeta ? m1 : in.b;
                bool zero = isZero(top().value);
                if (zero != ((flags & 1) != 0)) ip_ += offset;
                if (flags & 2) pop();
                break;
            }
            case 31: // goto
                ip_ += static_cast<int16_t>(in.a);
                break;
            case 27: { // enter_function
                const Frame& f = frames_.back();
                bool traceEnter = std::getenv("LPCVM_TRACE") && f.fn &&
                    (f.fn->name == "load_file_dependence" || f.fn->name == "load_comm_desc");
                if (traceEnter)
                    std::cerr << "[enter] " << f.fn->name << " bp=" << f.bp << " localCount=" << f.localCount
                              << " stackBefore=" << stack_.size() << "\n";
                for (int i = 0; i < f.localCount; ++i) pushValue(Value{});   // uninit locals = type 0 "undefined"
                if (traceEnter)
                    std::cerr << "[enter] " << f.fn->name << " stackAfter=" << stack_.size() << "\n";
                break;
            }
            case 15: { // check_args_num
                // vm_checkArgNum (0x809521c): the operand is the function's
                // MAX argument count (declared + defaults).  When fewer
                // arguments were actually passed the compiler falls through
                // to the inline default-value code (which pushes the missing
                // defaults, then resort_stack reorders them); when all were
                // passed it skips that block.  handler.c's create and other
                // functions rely on this: create declares 1 arg but has a
                // default create_mapping second argument (maxArgNo=2), so a
                // 1-arg call MUST run the default block that sets up the
                // command-registration mapping.
                int skipAll = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int passed = frames_.empty() ? 0 : frames_.back().argNum;
                int declared = frames_.empty() || !frames_.back().fn ? 0 : frames_.back().fn->argCount;
                int maxArgNo = skipAll;
                if (maxArgNo < declared) maxArgNo = declared;
                if (std::getenv("LPCVM_TRACE") && frames_.back().fn &&
                    frames_.back().fn->name == "read_etc_file")
                    std::cerr << "[checkargs] read_etc_file passed=" << passed
                              << " declared=" << declared << " skipAll=" << skipAll
                              << " stack=" << stack_.size() << " bp=" << frames_.back().bp << "\n";
                // If every declared argument was passed, skip the default-code block.
                if (passed >= maxArgNo) ip_ += skipAll;
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
                    // Parameter layout is [paramN ... param1, param0(top)]: the
                    // compiler pushes the default-value block in FORWARD param
                    // order (param1's default first, paramN's default last),
                    // and the original vm_resortStack assigns the FIRST-pushed
                    // default to the FIRST default param slot (param argNum).
                    // With our up-growing stack that means the defaults must be
                    // re-pushed in REVERSE order so param(argNum) ends up
                    // closest to the real args.  (get_all_catalog's
                    // condition=""/timeout=1800 were landing swapped, corrupting
                    // the cmd_get_catalog_ex wire body.)
                    for (auto it = defaults.rbegin(); it != defaults.rend(); ++it)
                        stack_.push_back(*it);
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
                    std::cerr << "[loop] init fn=" << (frames_.empty() || !frames_.back().fn
                                                           ? "<global>" : frames_.back().fn->name)
                              << " varNo=" << varNo << " slot=" << slot
                              << " typeV=" << number << " second.type=" << second.index()
                              << " second=" << to_string(second, false)
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
                    std::cerr << "[loop] init ctrl slot out of range: " << slot << " size=" << stack_.size()
                              << " fn=" << (frames_.empty() || !frames_.back().fn
                                                ? "<global>" : frames_.back().fn->name)
                              << " bp=" << (frames_.empty() ? -999 : frames_.back().bp) << "\n";
                break;
            }
            case 18: { // cond_jmp_for_loop
                int breakOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(ip_, slotOf(varNo), breakOffset, 0);
                break;
            }
            case 80: { // step_jmp_for_loop
                int continueOffset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int varNo = fromMeta ? m1 : static_cast<int8_t>(in.b);
                loopStep(ip_, slotOf(varNo), 0, continueOffset);
                break;
            }
            case 82: // to_reserve_stack_value (placeholder slot; type irrelevant)
                pushValue(Value{});
                break;
            case 83: // to_unreserve_stack_value
                if (!stack_.empty()) pop();
                break;
            case 14: // caught_error
                // (catch support disabled; errors propagate to the caller)
                break;
            case 26: // end_thread_catch
                break;
            case 44: // new_thread_catch
                break;
            case 16: // check_is_ref_type
            case 17: // check_is_ref_type_i
            case 19: // convert_to_ptr
            case 74: // rev_value
            case 84: // trace
                break;
            case 12: { // case_default
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                if (!stack_.empty()) pop();
                ip_ += offset;
                break;
            }
            case 10: { // case_check_number
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int number = fromMeta ? m1 : 0;
                Value v = top().value;
                bool hit = (v.index() == 2 && std::get<int64_t>(v) == number);
                if (hit) {
                    pop();
                    ip_ += offset;
                }
                break;
            }
            case 11: { // case_check_string
                int offset = fromMeta ? m0 : static_cast<int16_t>(in.a);
                int strIndex = fromMeta ? m1 : 0;
                Value v = top().value;
                const std::vector<std::string>* tbl =
                    frames_.empty() || !frames_.back().strings ? &obj_.strings : frames_.back().strings;
                if (v.index() == 4 && strIndex >= 0 &&
                    static_cast<size_t>(strIndex) < tbl->size() &&
                    std::get<std::string>(v) == (*tbl)[static_cast<size_t>(strIndex)]) {
                    pop();
                    ip_ += offset;
                }
                break;
            }
            default:
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[vm] unhandled op " << in.op << " " << ai.name
                              << " at ip=" << (ip_ - 1) << "\n";
                break;
            }
            } catch (const TimeSliceSignal&) {
                // Master time-slice yield: execution state stays in the
                // members; unwind runLoop untouched (the scheduler requeues
                // the master at the tail of ready_ and resumes it later).
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[slice-yield] vm=" << this << " obj=" << objName_
                              << " ip=" << ip_ << " frames=" << frames_.size() << "\n";
                throw;
            } catch (const SuspendSignal& sig) {
                // A blocking efun parked this process.  Execution state stays
                // in the members; unwind runLoop untouched (the scheduler
                // re-enters via resumeProcess).
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[loop-suspend] vm=" << this << " obj=" << objName_
                              << " sigVm=" << (sig.vm ? reinterpret_cast<void*>(sig.vm) : nullptr)
                              << " frames=" << frames_.size() << "\n";
                throw;
            } catch (const VmError& e) {
                // Unwind to the innermost new_thread_catch, if any.
                if (catches_.empty()) throw;
                CatchRecord rec = catches_.back();
                // The catch must belong to the current frame; otherwise let the
                // error propagate up to the frame that owns the catch.
                if (rec.frameCount != frames_.size()) throw;
                catches_.pop_back();
                stack_.resize(rec.stackSize);
                pushValue(make_string(std::string(e.what())));
                ip_ = rec.ipTarget;
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[catch] error \"" << e.what() << "\" resumed at ip=" << rec.ipTarget << "\n";
            } catch (const std::exception& e) {
                if (!catches_.empty()) {
                    CatchRecord rec = catches_.back();
                    if (rec.frameCount != frames_.size()) throw;
                    catches_.pop_back();
                    stack_.resize(rec.stackSize);
                    pushValue(make_string(std::string(e.what())));
                    ip_ = rec.ipTarget;
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[catch] std error \"" << e.what() << "\" resumed at ip=" << rec.ipTarget << "\n";
                } else {
                    throw;
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Loop step (mirrors vm_cmpAndJmpInLoop)
    // ------------------------------------------------------------------
    void loopStep(size_t& ip, int ctrlSlot, int breakOffset, int continueOffset) {
        if (ctrlSlot < 0 || static_cast<size_t>(ctrlSlot) >= stack_.size()) {
            if (std::getenv("LPCVM_TRACE")) {
                std::string fn = frames_.empty() || !frames_.back().fn ? "<global>" : frames_.back().fn->name;
                std::cerr << "[loop] ctrl slot out of range: " << ctrlSlot << " size=" << stack_.size()
                          << " fn=" << fn << " bp=" << (frames_.empty() ? -1 : frames_.back().bp)
                          << " localCount=" << (frames_.empty() ? -1 : frames_.back().localCount) << "\n";
            }
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
                if (std::getenv("LPCVM_TRACE") && iter == 0)
                    std::cerr << "[loop] 901 setVar slot=" << c->slotOfVar << " val="
                              << to_string(c->arr->items[iter], false).substr(0, 20) << "\n";
                ip += continueOffset;
            } else {
                ip += breakOffset;
            }
        } else if (c->type == 902) {
            if (!c->map) return;
            // foreach over a mapping iterates in vm_mappingIndices order
            // (bucket high -> low, chain head first).
            const auto entries = c->map->items();
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

    Value queryMemberValue(const Value& rawContainer, const Value& key) {
        const Value& container = resolveCrossVmRef(rawContainer);
        if (std::getenv("LPCVM_TRACE") && std::getenv("LPCVM_TRACE_MEMBER")) {
            std::string msg = "[member] key=" + to_string(key, false) + " cont=" + to_string(container, false) + "\n";
            std::cerr.write(msg.data(), static_cast<std::streamsize>(msg.size()));
        }
        if (container.index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(container);
            const Value* hit = mp->find(key);
            if (hit) return *hit;
            return Value{};   // miss -> original type 0 "undefined"
        }
        if (container.index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(container);
            int64_t i = toInt(key);
            if (i >= 0 && i < static_cast<int64_t>(arr->items.size())) return arr->items[static_cast<size_t>(i)];
            return Value{};   // OOB -> original type 0 "undefined"
        }
        if (container.index() == 4) {
            // LPC string indexing returns the ASCII code (int), e.g.
            // path[strlen-1] != 47 in load_all_path/load_all.
            const std::string& s = std::get<std::string>(container);
            int64_t i = toInt(key);
            if (i >= 0 && i < static_cast<int64_t>(s.size()))
                return make_int(static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(i)])));
            return Value{};   // OOB -> original type 0 "undefined"
        }
        return Value{};
    }

    Value queryRangeValue(const Value& container, const Value& from, const Value& to,
                          int flag, bool asPtr) {
        int64_t a = toInt(from);
        int64_t b = toInt(to);
        size_t len = 0;
        if (container.index() == 5) len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
        else if (container.index() == 4) len = std::get<std::string>(container).size();
        else if (container.index() == 10)
            len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
        if (flag & 1) { if (b <= 0) b = 1; b = static_cast<int64_t>(len) - b; }
        if (flag & 2) { if (a <= 0) a = 1; a = static_cast<int64_t>(len) - a; }
        if (a < 0) a = 0;
        if (static_cast<int64_t>(len) <= b) b = static_cast<int64_t>(len) - 1;
        if (a > b) {
            // Empty range keeps the container's type (picd's chinese_list is a
            // buffer; an empty buffer slice must stay a buffer, not become an
            // array - the old code turned it into ({}) which stringified to
            // "({})" and made get_random_chinese() return a constant text).
            if (container.index() == 4) return make_string("");
            if (container.index() == 10) return make_buffer("");
            return make_array();
        }
        if (container.index() == 4) {
            const std::string& s = std::get<std::string>(container);
            return make_string(s.substr(static_cast<size_t>(a), static_cast<size_t>(b - a + 1)));
        }
        if (container.index() == 10) {
            const std::string& s = std::get<std::shared_ptr<BufferValue>>(container)->data;
            return make_buffer(s.substr(static_cast<size_t>(a), static_cast<size_t>(b - a + 1)));
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


// Scheduler member definitions (OriginalVm is complete here).
void Scheduler::spawn(std::shared_ptr<OriginalVm> vm, const std::string& entry,
                      std::vector<Value> args) {
    auto p = std::make_shared<Process>();
    p->vm = std::move(vm);
    p->homeVm = p->vm;
    p->entry = entry;
    p->args = std::move(args);
    p->tid = allocTid();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_.push_back(p);
    }
    cv_.notify_one();
}

void Scheduler::spawnConstruction(std::shared_ptr<OriginalVm> vm, std::vector<Value> args,
                                  std::string doneEvent) {
    auto p = std::make_shared<Process>();
    p->vm = std::move(vm);
    p->homeVm = p->vm;
    p->entry = "__init__";
    p->args = std::move(args);
    p->doneEvent = std::move(doneEvent);
    p->tid = allocTid();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        // FRONT of the queue: the original driver runs the construction
        // INLINE on the loading process (create() is part of load_object's own
        // execution), so processes spawned by EARLIER creates (e.g. mmd's
        // MMD.service daemon) must not run between this load_object and its
        // create().  A tail insert would let them preempt the constructor.
        ready_.push_front(p);
    }
    cv_.notify_one();
}

void Scheduler::spawnDelayed(std::shared_ptr<OriginalVm> vm, const std::string& entry,
                             std::vector<Value> args, int64_t atMs,
                             std::shared_ptr<TimerEntry> timerEntry) {
    auto p = std::make_shared<Process>();
    p->vm = std::move(vm);
    p->homeVm = p->vm;
    p->entry = entry;
    p->args = std::move(args);
    p->timerEntry = std::move(timerEntry);
    p->tid = allocTid();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        timers_.emplace(atMs, TimerEntry2{p, nullptr});
    }
    cv_.notify_one();
}

void Scheduler::wake(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        // The resume hook (run inside resumeProcess BEFORE the value is
        // pushed) may update parkedResumeValue (e.g. connect's result check);
        // re-read it there rather than snapshotting here.
        if (std::getenv("LPCVM_TRACE")) {
            ResumeCond c;
            if (p->parkedVm) c = p->parkedVm->resumeCond();
            std::cerr << "[wake] " << p->entry << " cond.kind=" << c.kind
                      << " key=" << (c.qkey.empty() ? c.ename : c.qkey)
                      << " evWoken=" << p->eventWoken << " timed=" << p->timedEvent << "\n";
        }
        ready_.push_back(std::move(p));
    }
    cv_.notify_one();
}
void Scheduler::wakeFront(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_.push_front(std::move(p));
    }
    cv_.notify_one();
}

void Scheduler::requeue(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_.push_back(std::move(p));
    }
    cv_.notify_one();
}

void Scheduler::parkCurrent(std::shared_ptr<Process> p) {
    std::lock_guard<std::mutex> lk(mtx_);
    // The parked VM owns the ResumeCond (the VM that threw SuspendSignal).
    const ResumeCond& c = p->parkedVm->resumeCond();
    // Fresh park: clear stale wake bookkeeping from any earlier park.
    p->eventWoken = false;
    p->timedEvent = false;
    // A process can re-park on a new condition while the wake timer of a
    // PREVIOUS park is still pending in timers_.  Cancel that timer so it
    // cannot spuriously resume the new park (the oid:32 timed wait's timer
    // firing after share_datad's init re-parked on ev_filed0 resumed the
    // wrong park and made do_operate's wait_event return success early).
    if (p->parkTimerCancelled) *p->parkTimerCancelled = true;
    p->parkTimerCancelled.reset();
    std::shared_ptr<std::atomic<bool>> newToken;
    if (c.kind == 1 || (c.kind == 3 && c.wakeAtMs > 0)) {
        newToken = std::make_shared<std::atomic<bool>>(false);
        p->parkTimerCancelled = newToken;
    }
    switch (c.kind) {
    case 1:   // timer
        timers_.emplace(c.wakeAtMs, TimerEntry2{p, newToken});
        break;
    case 2:   // queue
        queueWaiters_[c.qkey].push_back(p);
        break;
    case 3:   // event
        eventWaiters_[c.ename].push_back(p);
        // A finite-timeout wait_event (wakeAtMs set) also parks a wake timer:
        // if the event never gets raised again (e.g. it was raised BEFORE this
        // park), the timer resumes the waiter with the timeout result (-501).
        if (c.wakeAtMs > 0) {
            p->timedEvent = true;
            timers_.emplace(c.wakeAtMs, TimerEntry2{p, newToken});
        }
        break;
    case 5:   // socket (connect/recv)
        sockWaiters_[c.fd].push_back(p);
        break;
    default:  // kind 0/4: no wakeup registered; park forever (external
              // explicit wake via wake() or process list)
        parked_.push_back(p);
        break;
    }
}

void Scheduler::wakeQueue(const std::string& qkey) {
    // The original driver's queue pend list wakes ONE waiter per push
    // (vm_f_send_to_queue -> vm_resumeFromObject on the pend head); that
    // waiter's resume hook (vm_acquiredValueFromQueue) dequeues the value.
    // Waking ALL waiters made every daemon poll the same queue: one got the
    // message, the other 19 dequeued empty -> "Bad message" spam.  Wake the
    // head waiter only; the rest stay parked for the next push.
    std::shared_ptr<Process> hit;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = queueWaiters_.find(qkey);
        if (it != queueWaiters_.end() && !it->second.empty()) {
            hit = std::move(it->second.front());
            it->second.pop_front();
            if (it->second.empty()) queueWaiters_.erase(it);
        }
    }
    if (hit) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[wq] " << qkey << " wake1\n";
        wake(std::move(hit));
    }
}

void Scheduler::wakeEvent(const std::string& ename) {
    std::deque<std::shared_ptr<Process>> hits;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = eventWaiters_.find(ename);
        if (it != eventWaiters_.end()) {
            hits = std::move(it->second);
            eventWaiters_.erase(it);
        }
    }
    for (auto& p : hits) {
        // A timed wait_event parks in BOTH maps; if the timeout timer already
        // woke it, drop this stale event wakeup.
        if (p->eventWoken) continue;
        p->eventWoken = true;
        wake(std::move(p));
    }
}

bool Scheduler::isVmParked(const OriginalVm* vm) {
    if (!vm) return false;
    std::lock_guard<std::mutex> lk(mtx_);
    auto on = [&](const std::shared_ptr<Process>& p) {
        return p && p->parkedVm && p->parkedVm.get() == vm;
    };
    for (auto& p : parked_)
        if (on(p)) return true;
    for (auto& kv : queueWaiters_)
        for (auto& p : kv.second)
            if (on(p)) return true;
    for (auto& kv : eventWaiters_)
        for (auto& p : kv.second)
            if (on(p)) return true;
    for (auto& kv : sockWaiters_)
        for (auto& p : kv.second)
            if (on(p)) return true;
    for (auto& te : timers_)
        if (on(te.second.p)) return true;
    return false;
}


// Run an LPC function call on the scheduler thread.  Direct (nested) when
// already on it; otherwise spawn a scheduler process carrying a result
// promise and block on it.  This keeps ALL VM execution on the scheduler
// thread (stage F: network threads never touch VM state concurrently).
Value Scheduler::execOnMain(const std::shared_ptr<OriginalVm>& vm, const std::string& funName,
                            const std::vector<Value>& args) {
    if (isSchedulerThread()) return vm->callFunctionByName(funName, args);
    auto p = std::make_shared<Process>();
    p->vm = vm;
    p->homeVm = vm;
    p->entry = funName;
    p->args = args;
    p->resultPromise = std::make_shared<std::promise<Value>>();
    p->tid = allocTid();
    auto fut = p->resultPromise->get_future();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_.push_back(p);
    }
    cv_.notify_one();
    return fut.get();   // blocks until the whole call chain completes
}

// Construction-completion wake: the object's load_object caller resumes at the
// FRONT of the ready queue (the original driver continues the loading process
// inline, so its "Load: ... OK" print comes before any processes spawned
// during create() get to run).
void Scheduler::wakeEventFront(const std::string& ename) {
    std::deque<std::shared_ptr<Process>> hits;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = eventWaiters_.find(ename);
        if (it != eventWaiters_.end()) {
            hits = std::move(it->second);
            eventWaiters_.erase(it);
        }
    }
    for (auto& p : hits) wakeFront(std::move(p));
}

void Scheduler::wakeSocket(int fd, bool readable, bool writable) {
    std::deque<std::shared_ptr<Process>> hits;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = sockWaiters_.find(fd);
        if (it != sockWaiters_.end()) {
            // Only wake waiters whose condition is satisfied; keep the rest.
            std::deque<std::shared_ptr<Process>> keep;
            for (auto& p : it->second) {
                const ResumeCond& c = p->parkedVm->resumeCond();
                if ((c.wantRead && readable) || (c.wantWrite && writable))
                    hits.push_back(std::move(p));
                else
                    keep.push_back(std::move(p));
            }
            if (keep.empty()) sockWaiters_.erase(it);
            else it->second = std::move(keep);
        }
    }
    for (auto& p : hits) wake(std::move(p));
}

void Scheduler::run() {
    // Efun watchdog: report a single efun call running >3s (C++ spin inside
    // the efun implementation - the LPC-level [vm-stuck] printer never fires
    // because no instruction boundary is crossed).
    static std::once_flag efunWatchFlag;
    std::call_once(efunWatchFlag, []() {
        if (!std::getenv("LPCVM_STUCK_TRACE")) return;
        std::thread([]() {
            int64_t lastReported = 0;
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                int64_t start;
                {
                    std::lock_guard<std::mutex> lk(g_curEfunMx);
                    start = g_curEfunStartMs.load();
                }
                if (!start) continue;
                using namespace std::chrono;
                int64_t now = duration_cast<milliseconds>(
                    steady_clock::now().time_since_epoch()).count();
                if (now - start < 3000) continue;
                if (lastReported == start) continue;   // already reported this hang
                lastReported = start;
                std::lock_guard<std::mutex> lk(g_curEfunMx);
                std::cerr << "[efun-hang] efun=" << g_curEfunName
                          << " obj=" << g_curEfunObj << " fn=" << g_curEfunFn
                          << " durMs=" << (now - start) << "\n";
            }
        }).detach();
    });
    schedThreadId_ = std::this_thread::get_id();
    while (true) {
        // Poll parked sockets (kind-5 waits: connect/recv) REGARDLESS of
        // whether the ready queue is busy.  The original vm_mainLoop scans
        // sockets every iteration; if some daemon keeps the ready queue fed
        // (e.g. ccs's CONFIG_D retry loop), a connect/recv completion would
        // NEVER be noticed - the connecting process stays parked forever and
        // the service never starts.  Throttle to at most one select() per
        // 20ms, and use a zero timeout so a busy ready queue is not slowed
        // down by a blocking select (the idle wait below sleeps between
        // polls).
        if (!sockWaiters_.empty() && (nowMs() - lastPollMs_) >= 20) {
            lastPollMs_ = nowMs();
            std::vector<int> fds;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                for (auto& kv : sockWaiters_) fds.push_back(kv.first);
            }
            std::vector<int> rd = fds, wr = fds;
            int nrd = lpc::xdbcPoll(fds, rd, wr, 0);
            if (std::getenv("LPCVM_TRACE") && nrd != 0)
                std::cerr << "[sock-poll] n=" << nrd << "\n";
            for (int fd : fds) {
                bool r = std::find(rd.begin(), rd.end(), fd) != rd.end();
                bool w = std::find(wr.begin(), wr.end(), fd) != wr.end();
                if (r || w) wakeSocket(fd, r, w);
            }
        }
        std::shared_ptr<Process> p;
        int64_t now = nowMs();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            // Move due timers to ready.
            while (!timers_.empty() && timers_.begin()->first <= now) {
                auto it = timers_.begin();
                auto te = it->second;
                timers_.erase(it);
                auto tp = te.p;
                // A re-park cancelled this wake timer (the process is now
                // waiting on a DIFFERENT condition); drop it - it must not
                // resume the new park.
                if (te.cancelled && *te.cancelled) continue;
                // Timed wait_event: the event already woke it -> drop the
                // stale timer; otherwise this IS the timeout -> resume with
                // the timeout result (-501, matching the zero-timeout branch).
                if (tp->eventWoken) continue;
                tp->eventWoken = true;
                if (tp->timedEvent && tp->vm)
                    tp->vm->parkedResumeValue() = make_int(-501);
                ready_.push_back(std::move(tp));
            }
            if (!ready_.empty()) {
                p = std::move(ready_.front());
                ready_.pop_front();
            }
        }
        if (p) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-pop] " << p->entry << " first=" << (p->firstRun ? 1 : 0)
                          << " vm=" << p->vm.get() << "\n";
            runProcess(p);
            if (p->done) {
                // The process finished (normal return or error): release its
                // tid so a later spawn can reuse it.  Tid is the basis of
                // read_etc's wait/raise event (ev_readetcd<tid/32> bit
                // tid%32); a process that PARKED keeps its tid (it is still
                // alive and may resume), but a finished process must free it -
                // otherwise the global counter wraps after 128 spawns and two
                // live processes share an id, so configd's read_etc waits get
                // woken by replies meant for the other process and time out.
                releaseTid(p->tid);
            }
            if (std::getenv("LPCVM_TRACE") && !p->done)
                std::cerr << "[sched-pop] " << p->entry << " NOT DONE after runProcess\n";
            continue;
        }
        {
            std::unique_lock<std::mutex> lk(mtx_);
            if (shutdown_ && ready_.empty() && timers_.empty()) return;
            if (std::getenv("LPCVM_TRACE") && (step_ % 10000) == 0)
                std::cerr << "[sched-idle] ready=" << ready_.size()
                          << " timers=" << timers_.size()
                          << " qwait=" << queueWaiters_.size()
                          << " evwait=" << eventWaiters_.size()
                          << " sockwait=" << sockWaiters_.size() << "\n";
            // Idle-waiter dump (diagnostic): when the ready queue is empty and
            // nothing can run, print every waiter's condition every 5s so a
            // deadlock/forever-wait is attributable (which event/queue/fd the
            // master or a daemon is stuck on).
            {
                static int64_t lastDump = 0;
                int64_t nd = nowMs();
                if (nd - lastDump >= 5000) {
                    lastDump = nd;
                    for (auto& kv : eventWaiters_)
                        for (auto& w : kv.second)
                            std::cerr << "[idle-wait] event=" << kv.first
                                      << " entry=" << w->entry << "\n";
                    for (auto& kv : queueWaiters_)
                        for (auto& w : kv.second)
                            std::cerr << "[idle-wait] queue=" << kv.first
                                      << " entry=" << w->entry << "\n";
                    for (auto& kv : sockWaiters_)
                        for (auto& w : kv.second)
                            std::cerr << "[idle-wait] fd=" << kv.first
                                      << " entry=" << w->entry << "\n";
                    for (auto& w : parked_)
                        std::cerr << "[idle-wait] parked entry=" << w->entry << "\n";
                }
            }
            ++step_;
            // Idle wait: sleep until the next timer, but never longer than the
            // socket-poll interval when sockets are parked (the poll at the top
            // of this loop notices connect/recv completion).
            if (!timers_.empty()) {
                int64_t w = timers_.begin()->first - nowMs();
                if (w < 0) w = 0;
                if (!sockWaiters_.empty() && w > 20) w = 20;
                cv_.wait_for(lk, std::chrono::milliseconds(w));
            } else if (!sockWaiters_.empty()) {
                cv_.wait_for(lk, std::chrono::milliseconds(20));
            } else {
                cv_.wait(lk);
            }
        }
    }
}

void Scheduler::requestShutdown() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        shutdown_ = true;
    }
    cv_.notify_all();
}

void Scheduler::parkForever(std::shared_ptr<Process> p) {
    std::lock_guard<std::mutex> lk(mtx_);
    parked_.push_back(std::move(p));
}

int64_t Scheduler::nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void Scheduler::runProcess(std::shared_ptr<Process> p) {
    Value callResult = make_int(0);
    currentProcess_ = p;   // get_thread_id() returns this process's tid
    try {
        if (p->timerEntry && p->timerEntry->cancelled && *p->timerEntry->cancelled) {
            // delete_timer() cancelled this callback before it fired.
            p->done = true;
            if (p->resultPromise) p->resultPromise->set_value(callResult);
            return;
        }
        if (std::getenv("LPCVM_TRACE")) {
            std::cerr << "[sched-run] entry=" << p->entry << " first=" << (p->firstRun ? 1 : 0) << "\n";
            std::cerr << "  obj=[" << p->vm->object().sourceName << "]" << "\n";
            std::cerr << "  objName=[" << p->vm->objName() << "]" << "\n";
            std::cerr << "  fns=" << std::to_string(p->vm->object().functions.size()) << "\n";
            std::cerr << "  code=" << (p->vm->code() ? std::to_string(p->vm->code()->size()) : "-1")
                      << " ip=" << std::to_string(p->vm->ip()) << "\n";
            std::cerr.flush();
        }
        if (p->firstRun) {
            p->firstRun = false;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-call] " << p->entry << " args=" << p->args.size() << "\n";
            if (p->entry == "__master__") {
                // The driver's master process: run the global init code and
                // create() with time slices (see runAsMaster).
                p->vm->runAsMaster();
            } else if (p->entry == "__init__") {
                // Object-construction process spawned by load_object: run the
                // object's global init code + create() (see runConstruction).
                p->vm->runConstruction(p->args, p->doneEvent);
            } else {
                callResult = p->vm->callFunctionByName(p->entry, p->args);
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-call-done] " << p->entry << "\n";
            p->done = true;
        } else if (p->resumeRunLoop) {
            // Cross-VM time-slice resume: continue the OWNER VM's frame chain
            // (a post_init closure's relinquish).  relinquish already pushed
            // its result, so resumeRunLoop() continues the stream without
            // pushing another value.  When the closure's top-level call
            // returns, hand its result to the caller VM (the master's
            // post_init loop) via resumeCaller().
            p->resumeRunLoop = false;
            auto rvm = p->parkedVm ? p->parkedVm : p->vm;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-xvm-resume] " << p->entry << " vm=" << rvm.get() << "\n";
            rvm->resumeRunLoop();
            callResult = rvm->lastReturnValue();
            if (p->callerVm && p->callerVm != rvm) {
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[sched-caller] " << p->entry << " resume caller vm="
                              << p->callerVm.get() << " with callee result\n";
                p->callerVm->resumeCaller(rvm->lastReturnValue());
                p->callerVm.reset();
            }
            if (p->entry == "__init__")
                p->homeVm->runConstruction(p->args, p->doneEvent);
            else if (p->entry == "__master__" && !p->homeVm->masterInitDone_)
                p->homeVm->runAsMaster();
            p->done = true;
        } else if (p->sliceResume) {
            // Time-slice yield resume: continue the master's startup phases
            // (or the construction's phases for "__init__" processes).
            p->sliceResume = false;
            if (p->entry == "__master__")
                p->vm->runAsMaster();
            else if (p->entry == "__init__")
                p->vm->runConstruction(p->args, p->doneEvent);
            else
                p->vm->runLoop();   // daemon yielded (rare): continue its stream
            p->done = true;
        } else {
            // Resume the VM whose execution state was parked (usually p->vm,
            // but cross-object blocking runs on the target's registry VM).
            auto rvm = p->parkedVm ? p->parkedVm : p->vm;
            rvm->resumeProcess(std::move(p->resumeValue));
            callResult = rvm->lastReturnValue();
            // Cross-object blocking call finished on the target's registry VM:
            // continue the CALLER's suspended frame chain on its own VM with
            // the callee's result (the caller's [do_init, start_service] etc.
            // were detached by runDetached while the callee ran on rvm).
            // NESTED layers (master post_init -> daemon do_init on its owner
            // VM -> configd read_etc): each runDetached layer restored its own
            // VM's frames into callers_ (innermost first).  Resume them
            // innermost-first so do_init's remaining code (init_all_npc etc.)
            // runs after read_etc returns, then resume the process VM.
            Value res = rvm->lastReturnValue();
            for (auto it = p->callers_.rbegin(); it != p->callers_.rend(); ++it) {
                auto cv = *it;
                if (cv == rvm) continue;
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[sched-caller-chain] " << p->entry
                              << " resume " << cv->objName() << "\n";
                cv->resumeCaller(res);
                res = cv->lastReturnValue();
            }
            p->callers_.clear();
            if (p->callerVm && p->callerVm != rvm) {
                if (std::getenv("LPCVM_TRACE"))
                    std::cerr << "[sched-caller] " << p->entry << " resume caller vm="
                              << p->callerVm.get() << " with callee result\n";
                p->callerVm->resumeCaller(res);
                p->callerVm.reset();
            }
            // The parked call finished; a construction process must advance
            // its phase machine (raise the done event), and the master must
            // continue its startup phases after load_all finished.
            if (p->entry == "__init__")
                p->homeVm->runConstruction(p->args, p->doneEvent);
            else if (p->entry == "__master__" && !p->homeVm->masterInitDone_)
                p->homeVm->runAsMaster();
            p->done = true;
        }
        // A set_timer callback finished (or was cancelled): remove its entry
        // from the timer list (the original driver unregisters the timer when
        // its callback fires).
        if (p->timerEntry) {
            timerUnregister(p->timerEntry);
            p->timerEntry.reset();
        }
        // An external-thread call (execOnMain) is blocked on this promise:
        // release it with the call's result exactly once.
        if (p->resultPromise) p->resultPromise->set_value(callResult);
    } catch (const TimeSliceSignal& sig) {
        if (sig.vm && sig.vm != p->vm.get()) {
            // CROSS-VM time-slice yield: the master's post_init() loop ran a
            // daemon init/do_init closure INLINE on the owner's registry VM
            // (call_fun_ptr inPostInit branch).  The yield parks the OWNER
            // VM's frame chain (its load_all continues where it left off),
            // and remembers the CALLER VM (the master) so the post_init loop
            // advances after the closure completes.  Restarting the master
            // (sliceResume) instead would drop the owner VM's frames and skip
            // the entire load - the original single-thread scheduler never
            // does that: relinquish inside a closure just yields the CPU and
            // resumes the same closure later.
            p->callerVm = p->vm;
            std::shared_ptr<OriginalVm> holder;
            {
                auto& reg = objectRegistry();
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                for (auto& kv : reg.vms)
                    if (kv.second.get() == sig.vm) { holder = kv.second; break; }
            }
            p->parkedVm = holder ? std::move(holder) : p->vm;
            p->resumeRunLoop = true;
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-slice-xvm] " << p->entry
                          << " park owner vm=" << p->parkedVm.get()
                          << " caller=" << p->callerVm.get() << "\n";
            requeue(p);
        } else {
            // Master (or daemon) process yielded its own time slice: VM
            // execution state stays in the members; requeue at the tail of
            // ready_ so other processes (daemons created during load_all) get
            // a turn, then the master resumes via runAsMaster() (phase
            // continuation).
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched-slice] " << p->entry << " requeue\n";
            p->sliceResume = true;
            requeue(p);
        }
    } catch (const SuspendSignal& sig) {
        // A blocking efun parked a VM.  Cross-object call_other runs blocking
        // efuns on the TARGET's registry VM (sig.vm), so cond_ lives there;
        // the process waits on that cond and resumes THAT VM (its stack holds
        // the parked call).  p->parkedVm records it for resume.
        if (sig.vm && sig.vm != p->vm.get()) {
            // Cross-VM blocking: the process's own VM (whose frame chain was
            // detached by runDetached) must be resumed after the callee
            // returns; remember it as the caller VM.  p->vm is never
            // overwritten by the park, so re-parks keep detecting cross-VM
            // correctly.
            p->callerVm = p->vm;
            std::shared_ptr<OriginalVm> holder;
            {
                auto& reg = objectRegistry();
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                for (auto& kv : reg.vms)
                    if (kv.second.get() == sig.vm) { holder = kv.second; break; }
            }
            if (holder) p->parkedVm = std::move(holder);
            else p->parkedVm = p->vm;
        } else {
            p->parkedVm = p->vm;
        }
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[sched-park] " << p->entry << " kind=" << sig.cond.kind
                      << " key=" << (sig.cond.qkey.empty() ? sig.cond.ename : sig.cond.qkey)
                      << " vm=" << p->vm.get() << " parkedVm=" << p->parkedVm.get() << "\n";
        // Register the wakeup under the PARKED VM's cond (it owns cond_).
        parkCurrent(p);
    } catch (const std::exception& e) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[sched] process " << p->entry << " error: " << e.what() << "\n";
        // A construction process that failed must still wake its parked
        // load_object caller (its resume hook reads the registry and yields
        // make_int(0) when the object never got initialized).
        if (p->entry == "__init__" && !p->doneEvent.empty()) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched] __init__ failed, waking " << p->doneEvent << "\n";
            scheduler().wakeEventFront(p->doneEvent);
        }
        if (p->resultPromise) p->resultPromise->set_value(make_int(0));
        p->done = true;
    } catch (...) {
        if (std::getenv("LPCVM_TRACE"))
            std::cerr << "[sched] process " << p->entry << " error: unknown\n";
        if (p->entry == "__init__" && !p->doneEvent.empty()) {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[sched] __init__ failed, waking " << p->doneEvent << "\n";
            scheduler().wakeEventFront(p->doneEvent);
        }
        if (p->resultPromise) p->resultPromise->set_value(make_int(0));
        p->done = true;
    }
}

// The single driver-level scheduler instance (anonymous-namespace type; the
// lpc::schedulerRun() entry below calls this).
Scheduler& scheduler() {
    static Scheduler s;
    return s;
}

// ---- efun watchdog state definitions ----
namespace {
std::string g_curEfunName, g_curEfunObj, g_curEfunFn;
std::mutex g_curEfunMx;
std::atomic<int64_t> g_curEfunStartMs{0};
std::mutex g_efunProfMx;
std::unordered_map<std::string, EfunProfEntry> g_efunProf;
} // namespace

std::atomic<int64_t> Scheduler::nextTid_{1};

ObjectRegistry& objectRegistry() {
    static ObjectRegistry reg;
    return reg;
}

// Guards the registry maps (vms/objects/initialized...).  Registry entries are
// inserted while objects load (possibly from several OS threads) and scanned by
// cross-object call resolution; a recursive mutex keeps those consistent.
std::recursive_mutex& registryMutex() {
    static std::recursive_mutex m;
    return m;
}

std::vector<PendingThread>& pendingThreads() {
    static std::vector<PendingThread> threads;
    return threads;
}

std::mutex& pendingThreadsMutex() {
    static std::mutex m;
    return m;
}

// True if the object already ran its global initializer + create() (either the
// main master VM or a loaded object).  Threads for such objects must not
// re-run the initializer: that would restart the whole driver (broadcast,
// post_init, thread registration) for every spawned thread.
bool isObjectInitialized(const OriginalObject* obj) {
    if (!obj) return false;
    auto& reg = objectRegistry();
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    auto it = reg.initializedObj.find(obj);
    if (it != reg.initializedObj.end()) return true;
    for (auto& kv : reg.initialized) {
        auto it2 = reg.objects.find(kv.first);
        if (it2 != reg.objects.end() && it2->second.get() == obj) return true;
    }
    return false;
}

// True if obj is the driver's global program (the master, start_aaa).
bool isMasterObject(const OriginalObject* obj) {
    if (!obj) return false;
    auto& reg = objectRegistry();
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    if (reg.masterObj == obj) return true;
    for (auto& kv : reg.vms) {
        if (kv.first.find("<main>") != std::string::npos && &kv.second->object() == obj)
            return true;
    }
    return false;
}

std::vector<PendingThread>& postInitFuncs() {
    static std::vector<PendingThread> funcs;
    return funcs;
}

Value handleBroadcastEfun(const std::string& searchRoot, const std::vector<Value>&) {
    static bool inBroadcast = false;
    if (inBroadcast) return make_int(1);
    inBroadcast = true;
    setBootLoading(true);
    struct Section { std::string title; std::string dir; };
    std::string op = vfsObjPrefix();
    std::string daemonsDir = "/" + op + "/daemons";
    std::string cloneDir = "/" + op + "/clone";
    std::string cmdsDir = "/" + op + "/cmds";
    std::vector<Section> sections = {
        {"Load all global feature.", "/feature"},
        {"Load all global inherits.", "/inherit"},
        {"Load all global objects.", "/clone"},
        {"Load all global cmds.", "/cmds"},
        {"", "/daemons"},
        {"", daemonsDir},
        {"", cloneDir},
        {"", cmdsDir},
    };
    auto& reg = objectRegistry();
    for (auto& sec : sections) {
        std::string title = sec.title;
        if (!title.empty()) std::cout << title << "\n";
        fs::path dir = fs::path(searchRoot) / (sec.dir.c_str() + 1);
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
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                if (reg.vms.find(full) != reg.vms.end()) continue;
            }
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[load] " << full << "\n";
            try {
                auto objPtr = std::make_shared<OriginalObject>(loadOriginalObject(full));
                auto vmPtr = std::make_shared<OriginalVm>(*objPtr, searchRoot);
                {
                    std::lock_guard<std::recursive_mutex> rl(registryMutex());
                    reg.objects[full] = objPtr;
                    reg.vms[full] = vmPtr;
                }
                vmPtr->runInit();
                {
                    std::lock_guard<std::recursive_mutex> rl(registryMutex());
                    reg.initialized[full] = true;
                }
            } catch (const std::exception& e) {
                std::cout << "Load: " << virt << "... FAILED: " << e.what() << "\n";
            }
        }
    }
    runPostInitFunctions(searchRoot);
    setBootLoading(false);
    inBroadcast = false;
    return make_int(1);
}

bool hasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Value tryCallOtherObjectByIndex(const std::string& searchRoot, const std::string& objectPath,
                                int funIndex, const std::vector<Value>& args,
                                const std::string& callerThisName) {
    // call_in_other_fun_ex with a function-INDEX funVal on an explicit object:
    // resolve the index in the target object's merged function table.
    if (searchRoot.empty() || objectPath.empty() || funIndex < 0) return make_int(0);
    std::string rel = objectPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::vector<std::string> candidates;
    candidates.push_back(searchRoot + "/" + rel);
    candidates.push_back("pak:" + rel);
    std::string opPrefix = vfsObjPrefix() + "/";
    if (rel.compare(0, opPrefix.size(), opPrefix) == 0) {
        candidates.push_back(searchRoot + "/" + rel.substr(opPrefix.size()));
        candidates.push_back("pak:" + rel.substr(opPrefix.size()));
    }
    auto& reg = objectRegistry();
    auto lookup = [&](const std::string& c) -> std::shared_ptr<OriginalVm> {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        auto it = reg.vms.find(c);
        return it == reg.vms.end() ? nullptr : it->second;
    };
    for (const auto& c : candidates) {
        auto vm = lookup(c);
        if (vm) {
            std::string name = vm->functionNameByIndex(funIndex);
            if (!name.empty()) {
                if (!callerThisName.empty()) vm->setPreviousObjectName(callerThisName);
                return vm->callFunctionByName(name, args);
            }
            return make_int(0);
        }
    }
    std::shared_ptr<OriginalVm> hit;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        for (auto& kv : reg.vms)
            if (hasSuffix(kv.first, rel)) { hit = kv.second; break; }
    }
    if (hit) {
        std::string name = hit->functionNameByIndex(funIndex);
        if (!name.empty()) {
            if (!callerThisName.empty()) hit->setPreviousObjectName(callerThisName);
            return hit->callFunctionByName(name, args);
        }
    }
    return make_int(0);
}

Value tryCallOtherObject(const std::string& searchRoot, const std::string& objectPath,
                         const std::string& funName, const std::vector<Value>& args,
                         const std::string& callerThisName) {
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[call_other] " << objectPath << " -> " << funName
                  << " args=" << args.size() << "\n";
    if (std::getenv("LPCVM_TRACE") && funName == "msg_read_content")
        std::cerr << "[co-mrc] path=" << objectPath << " root='" << searchRoot << "'\n";
    // Clone instances are registered under "clone:<rel>#<seq>"; resolve them
    // by exact key first (vm_callInOtherFun with an object value).  This must
    // run BEFORE the searchRoot check: clone keys carry no search root.
    if (objectPath.compare(0, 6, "clone:") == 0) {
        auto& reg0 = objectRegistry();
        std::shared_ptr<OriginalVm> hit;
        {
            std::lock_guard<std::recursive_mutex> rl0(registryMutex());
            auto it0 = reg0.vms.find(objectPath);
            if (it0 != reg0.vms.end()) hit = it0->second;
        }
        if (std::getenv("LPCVM_TRACE") && funName == "lite_query_temp")
            std::cerr << "[lqt] tryCallOtherObject clone path=" << objectPath
                      << " hit=" << (hit ? "YES" : "NO") << "\n";
        if (hit) return hit->callFunctionByName(funName, args);
        return make_int(0);
    }
    if (searchRoot.empty() || objectPath.empty()) return make_int(0);
    std::string rel = objectPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::vector<std::string> candidates;
    candidates.push_back(searchRoot + "/" + rel);
    candidates.push_back("pak:" + rel);
    std::string opPrefix = vfsObjPrefix() + "/";
    if (rel.compare(0, opPrefix.size(), opPrefix) == 0) {
        candidates.push_back(searchRoot + "/" + rel.substr(opPrefix.size()));
        candidates.push_back("pak:" + rel.substr(opPrefix.size()));
    }
    auto& reg = objectRegistry();
    for (const auto& c : candidates) {
        std::shared_ptr<OriginalVm> existing;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            auto it = reg.vms.find(c);
            if (it != reg.vms.end()) existing = it->second;
        }
        if (existing) {
            if (std::getenv("LPCVM_TRACE") && funName == "get_my_info")
                std::cerr << "[call_other] -> " << c << " (exact)\n";
            if (std::getenv("LPCVM_TRACE") && funName == "msg_read_content")
                std::cerr << "[co-mrc] exact-hit " << c << "\n";
            // previous_object() in the callee sees the caller: the VM that
            // executed the call_other (the current VM's this_object identity).
            if (!callerThisName.empty()) existing->setPreviousObjectName(callerThisName);
            return existing->callFunctionByName(funName, args);
        }
    }
    // Suffix match (covers registry keys stored with a different root, e.g.
    // absolute pak paths) - mirrors findObjectInRegistry's resolution.
    {
        std::shared_ptr<OriginalVm> hit;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                if (hasSuffix(kv.first, rel)) {
                    if (std::getenv("LPCVM_TRACE") && funName == "get_my_info")
                        std::cerr << "[call_other] -> " << kv.first << " (suffix)\n";
                    hit = kv.second;
                    break;
                }
            }
        }
        if (std::getenv("LPCVM_TRACE") && funName == "process_command" &&
            rel.find("connectiond") != std::string::npos)
            std::cerr << "[call_other-pc] rel=" << rel << " hit=" << (hit ? "YES" : "NO")
                      << " root='" << searchRoot << "'\n";
        if (hit) {
            if (!callerThisName.empty()) hit->setPreviousObjectName(callerThisName);
            return hit->callFunctionByName(funName, args);
        }
    }
    // The original driver does NOT auto-load objects on call_other: an
    // unloaded target raises "Object is not found (when call in function)."
    // (vm_callInOtherFun).  Only objects loaded by load_object/safe_load_object
    // are callable.
    return make_int(0);
}

void runPendingOriginalThreads(const std::string& searchRoot) {
    std::vector<PendingThread> snapshot;
    {
        std::lock_guard<std::mutex> lk(pendingThreadsMutex());
        snapshot.swap(pendingThreads());
    }
    if (std::getenv("LPCVM_TRACE")) std::cerr << "[orig] pending threads=" << snapshot.size() << "\n";
    // Run each thread concurrently so a long-running loop (e.g. a service loop)
    // does not block the other threads.
    std::vector<std::thread> workers;
    for (auto& th : snapshot) {
        PendingThread copy = th;   // capture by value; the vector is cleared below
        workers.emplace_back([copy, searchRoot]() {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[thread] run " << copy.fun << " args=" << copy.args.size() << "\n";
            // A telnet daemon thread serves one client; tell the builtins layer
            // which client it is so input_string()/printf route to the socket.
            bool isTelnet = copy.fun == "telnet_daemon" || copy.fun == "buildin_telnet_daemon";
            if (isTelnet && !copy.args.empty() && copy.args[0].index() == 2) {
                setThreadTelnetClient(static_cast<int>(std::get<int64_t>(copy.args[0])));
            }
            try {
                OriginalVm vm(*copy.obj, searchRoot);
                // Like the original driver, a thread shares the object context
                // of its creator: we never re-run the initializer here (that
                // would re-run create() and re-register threads/timers).  The
                // thread VM gets the initialized instance variables instead.
                {
                    auto& reg = objectRegistry();
                    std::lock_guard<std::recursive_mutex> rl(registryMutex());
                    for (auto& kv : reg.vms) {
                        if (&kv.second->object() == copy.obj) {
                            auto& src = kv.second->objVars();
                            auto& dst = vm.objVars();
                            if (dst.size() < src.size()) dst.resize(src.size());
                            for (size_t i = 0; i < src.size(); ++i) dst[i] = src[i];
                            break;
                        }
                    }
                }
                vm.callFunctionByName(copy.fun, copy.args);
            } catch (const std::exception& e) {
                std::cerr << "[thread] " << copy.fun << " error: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[thread] " << copy.fun << " error: unknown\n";
            }
            setThreadTelnetClient(-1);
        });
    }
    for (auto& w : workers) w.detach();
}

void runPostInitFunctions(const std::string& searchRoot) {
    auto& funcs = postInitFuncs();
    if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE"))
        std::cerr << "[orig] post_init funcs=" << funcs.size() << "\n";
    // The original driver runs post_init closures SEQUENTIALLY on the single
    // cooperative scheduler: each init runs to completion OR parks on a
    // blocking efun (wait_config -> wait_event) and the scheduler resumes it
    // later, then the next init starts.  Our old synchronous loop let a
    // SuspendSignal escape post_init (uncaught C++ exception -> 0xe06d7363 on
    // dba: dbaserverd do_init waits for ev_config that only update_config_daemon
    // - a scheduler process - can raise).  Run each closure as a scheduler
    // process instead: the same sequential semantics, but blocking efuns park
    // and resume through the scheduler.
    for (auto& fn : funcs) {
        if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE"))
            std::cerr << "[orig] post_init -> " << fn.obj->sourceName << "::" << fn.fun
                      << " args=" << fn.args.size() << "\n";
        // Share the registered VM's initialized instance variables (the
        // original post_init runs each init in the object's OWN context).
        std::shared_ptr<OriginalVm> ownerVm;
        {
            auto& reg = objectRegistry();
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                if (&kv.second->object() == fn.obj) { ownerVm = kv.second; break; }
            }
        }
        if (!ownerVm) {
            // Object not registered: fall back to a fresh VM over a copy.
            auto objPtr = std::make_shared<OriginalObject>(*fn.obj);
            ownerVm = std::make_shared<OriginalVm>(objPtr, searchRoot);
        }
        if (fn.closure && !fn.closure->func) {
            // The registered closure wraps a builtin efun (create_thread in
            // ccs_clientd/tts_clientd create()).  Spawning "create_thread" as
            // an LPC entry would not resolve; run the efun with the closure's
            // bound args on the owner VM instead.  The efun itself (create_thread)
            // spawns the daemon as a scheduler process, so this stays on the
            // cooperative scheduler.
            if (std::getenv("LPCVM_TRACE") || std::getenv("LPCVM_EVENT_TRACE"))
                std::cerr << "[orig] post_init efun-closure -> " << fn.fun
                          << " args=" << fn.args.size() << "\n";
            ownerVm->callEfunPublic(fn.closure->funcIndex, fn.args);
            continue;
        }
        scheduler().spawn(ownerVm, fn.fun, fn.args);
    }
    funcs.clear();
}

// Register the object's VM in the registry (load the .o, create the VM,
// insert into objects/vms) WITHOUT running its initializer.  load_object's
// cooperative path uses this + a construction scheduler process; the
// synchronous loadObjectIntoRegistry uses it + runInit.  Returns the registry
// key ("" on failure).
std::string registerObjectVM(const std::string& virtualPath, const std::string& searchRoot) {
    if (searchRoot.empty() || virtualPath.empty()) return "";
    std::string rel = virtualPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::vector<std::string> relCandidates;
    relCandidates.push_back(rel);
    std::string opPrefix = vfsObjPrefix() + "/";
    if (rel.compare(0, opPrefix.size(), opPrefix) == 0)
        relCandidates.push_back(rel.substr(opPrefix.size()));
    for (const auto& rc : relCandidates) {
        auto pakObj = loadOriginalObjectByRelPath(rc);
        if (!pakObj) continue;
        std::string key = "pak:" + rc;
        auto& reg = objectRegistry();
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            if (reg.vms.find(key) != reg.vms.end()) return key;
        }
        auto vmPtr = std::make_shared<OriginalVm>(*pakObj, searchRoot);
        vmPtr->setThisObjectName(key);
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            reg.objects[key] = pakObj;
            reg.vms[key] = vmPtr;
        }
        return key;
    }
    for (const auto& rc : relCandidates) {
        std::string c = searchRoot + "/" + rc;
        std::error_code ec;
        if (!std::filesystem::exists(c, ec)) continue;
        auto& reg = objectRegistry();
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            if (reg.vms.find(c) != reg.vms.end()) return c;
        }
        auto objPtr = std::make_shared<OriginalObject>(loadOriginalObject(c));
        auto vmPtr = std::make_shared<OriginalVm>(*objPtr, searchRoot);
        vmPtr->setThisObjectName(c);
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            reg.objects[c] = objPtr;
            reg.vms[c] = vmPtr;
        }
        return c;
    }
    return "";
}

} // namespace

// Current LPC process's tid (the original driver's PCB->tid).  Called by the
// get_thread_id() efun.  On the single scheduler thread every process would
// otherwise share one thread_local id; this returns the per-process id.
int64_t schedulerCurrentTid() {
    return scheduler().currentProcessTid();
}

// ---------------------------------------------------------------------------
// Driver-level cooperative scheduler entry (stage B).  Runs the single-thread
// event loop that drives daemon processes, wake timers and queue/event
// wakeups.  Replaces main()'s idle sleep loop; returns only when no work
// remains (all processes finished / parked forever).
// ---------------------------------------------------------------------------
void schedulerRun() {
    scheduler().run();
}

int runOriginalObject(const OriginalObject& obj, std::ostream& out, const std::string& searchRoot) {
    OriginalVm vm(obj, searchRoot);
    // The master's this_object() must resolve to a REAL registry key so
    // object(<this>) values created by the master (e.g. load_object's
    // (["module": object(<this>)]) context) can be called back later - the
    // cmd_*.o create() does call_other(module, "register_entry", ...) with
    // that captured object.  "<this>" alone cannot be resolved outside the
    // creating VM.
    std::string mainKey = searchRoot.empty() ? "<main>" : searchRoot + "/<main>";
    vm.setThisObjectName(mainKey);
    // Register the main object (the driver's "global program") so cross-object
    // function lookups can resolve its functions.  The same OriginalObject is
    // used by the running VM, so thread closures created from it share state.
    std::shared_ptr<OriginalVm> mainVm;
    {
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        if (reg.vms.find(mainKey) == reg.vms.end()) {
            auto objPtr = std::make_shared<OriginalObject>(obj);
            reg.objects[mainKey] = objPtr;
            mainVm = std::make_shared<OriginalVm>(*objPtr, searchRoot);
            mainVm->setThisObjectName(mainKey);
            reg.vms[mainKey] = mainVm;
        } else {
            mainVm = reg.vms[mainKey];
        }
    }
    {
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        // Set BEFORE the master runs: the master's own init code calls global
        // functions (call_in_user_fun bit7), which resolve against this object.
        reg.masterObj = &obj;
    }
    {
        auto& m = sharedMasterFunctions();
        if (m.empty()) {
            // One shared copy of the master's string table for ALL its
            // function copies (per-function copies of start_gs.o's 2978
            // strings x 1308 functions burned ~500MB at boot).
            auto table = std::make_shared<const std::vector<std::string>>(obj.strings);
            for (const auto& fn : obj.functions) {
                if (fn.index < 0 || fn.code.empty()) continue;
                auto cp = std::make_shared<OriginalFunction>(fn);
                cp->strings = table;
                cp->varOffset = 0;
                cp->funOffset = 0;
                m[fn.index] = cp;
            }
        }
    }
    // STAGE B: the master is a SCHEDULER process, exactly like the original
    // driver (vm_mainLoop runs from the very start; the master's startup code
    // - load_all, post_init - shares the CPU with the daemon processes it
    // creates via runTicks time slices).  runAsMaster() runs the global init
    // code and create(), yielding (TimeSliceSignal) every slice so daemon
    // processes spawned during load run interleaved - their
    // "THREAD_D.service(...) daemon start..." prints appear while objects are
    // still loading, matching the reference aaa.log.  post_init closures are
    // queued behind the master.
    mainVm->setMasterProcess(true);
    scheduler().spawn(mainVm, "__master__", {});
    // post_init closures are queued by runAsMaster() AFTER the master's
    // startup finished (register_post_init is intercepted so the LPC
    // _post_init_funcs array stays empty; the closures collected during
    // load_all/daemon-create are spawned as scheduler processes then).
    {
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        reg.initialized[mainKey] = true;
        reg.initializedObj[&obj] = true;
    }
    out << "Original .o VM finished.\n";
    return 0;
}

int runOriginalObjectInit(const OriginalObject& obj, const std::string& searchRoot) {
    OriginalVm vm(obj, searchRoot);
    vm.runInit();
    return 0;
}

int loadObjectIntoRegistry(const std::string& virtualPath, const std::string& searchRoot,
                           const std::vector<Value>& createArgs) {
    if (searchRoot.empty() || virtualPath.empty()) return 0;
    // Register the object's VM (load the .o, create the VM, insert into the
    // registry) WITHOUT running its initializer; the caller then runs
    // runInit/runConstruction.  Mirrors the original vm_f_load_object ->
    // vm_loadObject split: loading and constructing are separate steps.
    auto regKey = [&]() -> std::string {
        std::string rel = virtualPath;
        if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
        if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
        // The original driver resolves compiled objects through the binary
        // directory (the mounted pak) first, then the real filesystem.
        std::vector<std::string> relCandidates;
        relCandidates.push_back(rel);
        std::string opPrefix = vfsObjPrefix() + "/";
        if (rel.compare(0, opPrefix.size(), opPrefix) == 0)
            relCandidates.push_back(rel.substr(opPrefix.size()));
        for (const auto& rc : relCandidates) {
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[loadobj] stage1 rc=" << rc << " t=" << std::this_thread::get_id() << "\n";
            auto pakObj = loadOriginalObjectByRelPath(rc);
            if (!pakObj) continue;
            std::string key = "pak:" + rc;
            auto& reg = objectRegistry();
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                if (reg.vms.find(key) != reg.vms.end()) return key;
            }
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[loadobj] constructing VM key=" << key << " t=" << std::this_thread::get_id() << "\n";
            auto vmPtr = std::make_shared<OriginalVm>(*pakObj, searchRoot);
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                reg.objects[key] = pakObj;
                reg.vms[key] = vmPtr;
            }
            return key;
        }
        for (const auto& rc : relCandidates) {
            std::string c = searchRoot + "/" + rc;
            std::error_code ec;
            if (!std::filesystem::exists(c, ec)) continue;
            auto& reg = objectRegistry();
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                if (reg.vms.find(c) != reg.vms.end()) return c;
            }
            auto objPtr = std::make_shared<OriginalObject>(loadOriginalObject(c));
            auto vmPtr = std::make_shared<OriginalVm>(*objPtr, searchRoot);
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                reg.objects[c] = objPtr;
                reg.vms[c] = vmPtr;
            }
            return c;
        }
        return "";
    }();
    if (regKey.empty()) return 0;
    {
        auto& reg = objectRegistry();
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        if (reg.initialized.count(regKey) && reg.initialized[regKey]) return 1;
    }
    try {
        auto& reg = objectRegistry();
        std::shared_ptr<OriginalVm> vm;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            vm = reg.vms[regKey];
        }
        if (std::getenv("LPCVM_LOCK_TRACE"))
            std::cerr << "[loadobj] runInit key=" << regKey << " t=" << std::this_thread::get_id() << "\n";
        vm->runInit(createArgs);
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            reg.initialized[regKey] = true;
        }
        if (std::getenv("LPCVM_LOCK_TRACE"))
            std::cerr << "[loadobj] done key=" << regKey << " t=" << std::this_thread::get_id() << "\n";
        return 1;
    } catch (...) {
        return 0;
    }
}

Value findObjectInRegistry(const std::string& virtualPath, const std::string& searchRoot) {
    (void)searchRoot;
    std::string rel = virtualPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    auto& reg = objectRegistry();
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    for (auto& kv : reg.vms) {
        std::string key = kv.first;
        // registry keys are real paths like "linux/aaa - bak/aaa/daemons/x.o";
        // compare by suffix so "/aaa/daemons/x.c" matches.
        if (hasSuffix(key, rel)) return make_object(virtualPath);
    }
    return make_int(0);
}

Value cloneObjectInRegistry(const std::string& virtualPath, const std::string& searchRoot,
                            const std::vector<Value>& createArgs) {
    // vm_f_clone_object (0x80f8eac): look up the loaded template, clone it
    // (fresh instance variables), run its create() initializer and return the
    // new object.  Clones are registered under "clone:<rel>#<seq>" so later
    // call_other through the object value resolves to the clone.
    if (virtualPath.empty()) return make_int(0);
    // Phase profiler (LPCVM_CLONE_PROF=1): split each clone's cost into
    // lookup/vm-construction / runInit / NPC-extras and report averages + the
    // registry sizes every 2000 clones, to localize the per-clone CPU.
    static const bool cloneProfOn = (std::getenv("LPCVM_CLONE_PROF") != nullptr);
    static int64_t cpCalls = 0, cpLookup = 0, cpInit = 0, cpNpc = 0, cpTail = 0;
    auto cpNow = [] {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    };
    int64_t cpT = cloneProfOn ? cpNow() : 0;
    auto cpMark = [&](const char* what) {
        if (!cloneProfOn) return;
        int64_t n = cpNow();
        int64_t d = n - cpT; cpT = n;
        if (what == std::string("lookup")) cpLookup += d;
        else if (what == std::string("init")) cpInit += d;
        else if (what == std::string("npc")) cpNpc += d;
        else cpTail += d;
    };
    std::string rel = virtualPath;
    if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (hasSuffix(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    auto& reg = objectRegistry();
    std::shared_ptr<OriginalVm> tmplVm;
    bool relIsClone = rel.find('#') != std::string::npos;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        if (relIsClone) {
            // Cloning an EXISTING clone instance (place_char clones the basic
            // object: clone_object(find_basic_object_by_import_name(...)) with
            // a "clone:...basic.o#410" object value): the template is that
            // instance itself, so its my_import_name/name/type carry over.
            for (auto& kv : reg.vms) {
                std::string key = kv.first;
                size_t hash = key.find('#');
                if (hash != std::string::npos) key.resize(hash);  // strip #seq
                if (hasSuffix(key, rel)) { tmplVm = kv.second; break; }
            }
            if (!tmplVm) {
                // exact clone-key match (shouldn't normally be needed)
                for (auto& kv : reg.vms)
                    if (hasSuffix(kv.first, rel)) { tmplVm = kv.second; break; }
            }
        } else {
            for (auto& kv : reg.vms) {
                std::string key = kv.first;
                // template keys are real paths or "pak:<rel>"; strip "#" clones.
                size_t hash = key.find('#');
                if (hash != std::string::npos) key.resize(hash);
                if (hasSuffix(key, rel)) { tmplVm = kv.second; break; }
            }
        }
    }
    if (!tmplVm) {
        // Not loaded yet: load it first (safe_load_object semantics), then
        // re-look it up.
        if (!loadObjectIntoRegistry(virtualPath, searchRoot)) return make_int(0);
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        for (auto& kv : reg.vms) {
            std::string key = kv.first;
            size_t hash = key.find('#');
            if (hash != std::string::npos) key.resize(hash);
            if (hasSuffix(key, rel)) { tmplVm = kv.second; break; }
        }
    }
    if (!tmplVm) return make_int(0);
    // Clone: fresh OriginalObject copy (program+metadata shared) but a NEW
    // OriginalVm whose instance-variable storage is independent.
    static std::atomic<int64_t> cloneSeq{0};
    int64_t seq = cloneSeq.fetch_add(1, std::memory_order_relaxed);
    // The clone key must be the PROGRAM path (no "#<seq>" of the source clone):
    // "clone:gs/clone/npc/basic.o#410" -> "clone:gs/clone/npc/basic.o#<new>".
    std::string relBase = rel;
    size_t hashPos = relBase.find('#');
    if (hashPos != std::string::npos) relBase.resize(hashPos);
    std::string cloneKey = "clone:" + relBase + "#" + std::to_string(seq);
    std::shared_ptr<const OriginalObject> objPtr;
    std::shared_ptr<OriginalVm> vmPtr;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        // find the template's OriginalObject to copy program/objVars layout
        std::shared_ptr<OriginalObject> tmplObj;
        std::string relProg = rel;
        size_t rp = relProg.find('#');
        if (rp != std::string::npos) relProg.resize(rp);
        for (auto& kv : reg.objects) {
            std::string key = kv.first;
            size_t hash = key.find('#');
            if (hash != std::string::npos) key.resize(hash);
            if (hasSuffix(key, relProg)) { tmplObj = kv.second; break; }
        }
        if (!tmplObj) {
            // fall back to the VM's object
            objPtr = tmplVm->sharedObject();
        } else {
            // SHARE the template's program (functions/code/merged table are
            // read-only after construction): deep-copying it per clone and
            // re-merging the inherit chain cost ~1.5GB for the thousands of
            // NPC clones.  Instance variables live in the VM's objVars_,
            // which the clone constructor allocates fresh.
            objPtr = tmplObj;
        }
        vmPtr = std::make_shared<OriginalVm>(objPtr, tmplVm.get());
        vmPtr->setThisObjectName(cloneKey);
        vmPtr->setTemplateVm(tmplVm);   // object_template() returns this
        reg.objects[cloneKey] = tmplObj;   // share the read-only program
        reg.vms[cloneKey] = vmPtr;
    }
    cpMark("lookup");
    try {
        // vm_f_construct_object2 runs the full construction: the global
        // initializer code (initOrderList constitutions -> initCodeSegment)
        // THEN create() - clone instances must have their instance variables
        // (tmp_dbase etc.) initialized exactly like the original.
        // createArgs (e.g. clone_object(path, (["account": account])) in
        // account_login) become create()'s arguments.
        vmPtr->runInit(createArgs);
        cpMark("init");
        // Inherit the template's import identity.  The original driver's
        // clone runs every constitution's create(); basic_object.c's create()
        // sets my_import_name from object_template() so a clone's
        // get_name()/query() fall through to its basic object's dbase.  Our
        // multi-create chain may skip it, leaving a place_char'd NPC clone
        // (守护天神 etc.) with an empty my_import_name and get_name()
        // undefined -> get_registered_name() undefined -> guardd can't find
        // 守护天神@天墉城.  Re-run the template's import on the clone (it is
        // idempotent: re-sets my_import_name, file_id and the npc.list
        // attributes including name).
        if (tmplVm && rel.find("npc/basic") != std::string::npos) {
            Value tmplImp = tmplVm->callFunctionByName("get_import_name", {});
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[clone-imp] " << cloneKey << " tmpl=" << tmplVm->objName()
                          << " import=[" << to_string(tmplImp, false).substr(0, 40) << "]\n";
            if (tmplImp.index() == 4 && !std::get<std::string>(tmplImp).empty()) {
                vmPtr->setPreviousObjectName(tmplVm->objName());
                vmPtr->callFunctionByName("import", {tmplImp});
            }
            // The clone's create chain may not include id.c's create()
            // (_id = allocate_id(this_object())) when the inherited id.c
            // create collided in the merged function table.  Without an id
            // the NPC's put_in_map stores 0 and get_from_map / guardd's
            // _find_all_npc can never resolve it.  Mirror id.c's create here:
            // if get_id() is not a positive int, allocate one and store it in
            // the _id instance variable (get_id's varOffset).
            Value curId = vmPtr->callFunctionByName("get_id", {});
            bool needId = !(curId.index() == 2 && std::get<int64_t>(curId) > 0);
            if (needId) {
                if (const OriginalFunction* getFn = vmPtr->findFunctionAny("get_id")) {
                    Value self = make_object_owned(cloneKey, objPtr.get());
                    Value idv = call_builtin(nullptr, "allocate_id", {self});
                    if (idv.index() == 2 && std::get<int64_t>(idv) > 0) {
                        int slot = getFn->varOffset + 0;   // _id is id.c var 0
                        if (slot >= 0 && slot < (int)vmPtr->objVars().size()) {
                            vmPtr->objVars()[static_cast<size_t>(slot)] = idv;
                            if (std::getenv("LPCVM_TRACE"))
                                std::cerr << "[clone-id] " << cloneKey << " id="
                                          << std::get<int64_t>(idv) << " slot=" << slot << "\n";
                        }
                    }
                }
            }
        }
        cpMark("npc");
    } catch (const std::exception& e) {
        std::cerr << "[clone] " << cloneKey << " create error: " << e.what() << "\n";
    } catch (...) {
    }
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        reg.initialized[cloneKey] = true;
    }
    if (cloneProfOn) {
        cpTail += cpNow() - cpT;
        ++cpCalls;
        if ((cpCalls % 2000) == 0) {
            int64_t tot = cpLookup + cpInit + cpNpc + cpTail;
            std::cerr << "[clone-prof] calls=" << cpCalls << " avg=" << (cpCalls ? tot / cpCalls : 0)
                      << "us (lookup=" << (cpCalls ? cpLookup / cpCalls : 0)
                      << " init=" << (cpCalls ? cpInit / cpCalls : 0)
                      << " npc=" << (cpCalls ? cpNpc / cpCalls : 0)
                      << " tail=" << (cpCalls ? cpTail / cpCalls : 0)
                      << ") reg.vms=" << reg.vms.size() << " objects=" << reg.objects.size() << "\n";
            cpLookup = cpInit = cpNpc = cpTail = 0;
        }
    }
    auto ov = std::make_shared<ObjectValue>();
    ov->programName = cloneKey;
    return ov;
}

Value callOriginalObjectFunction(const std::string& oPath, const std::string& funName,
                                 const std::vector<Value>& args, const std::string& searchRoot) {
    return tryCallOtherObject(searchRoot, oPath, funName, args);
}

std::string masterSearchRoot() {
    auto& reg = objectRegistry();
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    for (auto& kv : reg.vms)
        if (kv.first.find("<main>") != std::string::npos) {
            std::string key = kv.first;
            size_t p = key.rfind("/<main>");
            if (p != std::string::npos) return key.substr(0, p);
            return "";
        }
    return "";
}

bool masterDefineLookup(const std::string& name, std::string& expansion) {
    auto& reg = objectRegistry();
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    for (auto& kv : reg.vms) {
        if (kv.first.find("<main>") != std::string::npos) {
            const auto& defs = kv.second->object().defines;
            // The .o define table pads names to a fixed width with trailing
            // spaces ("LV_NONE " for LV_NONE); compare trimmed.
            for (const auto& d : defs) {
                size_t e = d.first.find_last_not_of(" \t\r\n");
                std::string dn = (e == std::string::npos) ? "" : d.first.substr(0, e + 1);
                if (dn == name) { expansion = d.second; return true; }
            }
            return false;
        }
    }
    return false;
}

Value applyClosureOnOwner(const Value& closure, const std::vector<Value>& args) {
    if (closure.index() != 8) return make_int(0);
    auto cl = std::get<std::shared_ptr<ClosureValue>>(closure);
    std::vector<Value> all = cl->boundArgs;
    all.insert(all.end(), args.begin(), args.end());
    auto& reg = objectRegistry();
    std::shared_ptr<OriginalVm> ownerVm;
    if (cl->owner) {
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                if (&kv.second->object() == cl->owner) {
                    if (std::getenv("LPCVM_TRACE"))
                        std::cerr << "[applyClosure] owner-resolve " << cl->functionName
                                  << " in " << kv.first << "\n";
                    ownerVm = kv.second;
                    break;
                }
            }
        }
        if (!ownerVm && std::getenv("LPCVM_TRACE"))
            std::cerr << "[applyClosure] owner not in registry: " << cl->functionName
                      << " owner=" << (cl->owner ? cl->owner->sourceName : "?") << "\n";
    } else if (std::getenv("LPCVM_TRACE")) {
        std::cerr << "[applyClosure] no owner for " << cl->functionName << "\n";
    }
    // Call OUTSIDE the registry lock: the owner VM's execution may itself
    // touch the registry (clone_object, call_other, ...) and taking the
    // registry lock while holding another VM's exec lock deadlocks against a
    // thread doing the reverse.  External threads (packet dispatch) must run
    // on the scheduler thread (stage F: single-threaded VM execution).
    if (ownerVm)
        return scheduler().execOnMain(ownerVm, cl->functionName, all);
    return callMasterProgramFunction(cl->functionName, all, masterSearchRoot());
}

Value callMasterProgramFunction(const std::string& funName, const std::vector<Value>& args,
                                const std::string& searchRoot) {
    auto& reg = objectRegistry();
    std::shared_ptr<OriginalVm> target;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        if (searchRoot.empty()) {
            // Auto-locate the master VM under any registered root.
            for (auto& kv : reg.vms) {
                if (kv.first.find("<main>") != std::string::npos) {
                    target = kv.second;
                    break;
                }
            }
        } else {
            std::string key = searchRoot + "/<main>";
            auto it = reg.vms.find(key);
            if (it != reg.vms.end()) target = it->second;
        }
    }
    if (!target) return make_int(0);
    // Route through the scheduler: VM execution must be single-threaded.
    // Nested calls from a running process (scheduler thread) run directly;
    // calls from network/telnet/mysql threads spawn a scheduler process and
    // block on its result (stage F).
    return scheduler().execOnMain(target, funName, args);
}

void drainPendingOriginalThreads(const std::string& searchRoot) {
    // Snapshot the queue under the lock, then run outside it so the spawned
    // threads (which may themselves create_thread) can enqueue freely.
    std::vector<PendingThread> snapshot;
    {
        std::lock_guard<std::mutex> lk(pendingThreadsMutex());
        snapshot.swap(pendingThreads());
    }
    if (snapshot.empty()) return;
    if (std::getenv("LPCVM_TRACE"))
        std::cerr << "[orig] runtime pending threads=" << snapshot.size() << "\n";
    std::vector<std::thread> workers;
    for (auto& th : snapshot) {
        PendingThread copy = th;
        workers.emplace_back([copy, searchRoot]() {
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[thread] run " << copy.fun << " args=" << copy.args.size() << "\n";
            bool isTelnet = copy.fun == "telnet_daemon" || copy.fun == "buildin_telnet_daemon";
            if (isTelnet && !copy.args.empty() && copy.args[0].index() == 2) {
                setThreadTelnetClient(static_cast<int>(std::get<int64_t>(copy.args[0])));
            }
            try {
                OriginalVm vm(*copy.obj, searchRoot);
                // Never re-run the initializer (would restart the driver);
                // copy the initialized instance variables instead.
                {
                    auto& reg = objectRegistry();
                    std::lock_guard<std::recursive_mutex> rl(registryMutex());
                    for (auto& kv : reg.vms) {
                        if (&kv.second->object() == copy.obj) {
                            auto& src = kv.second->objVars();
                            auto& dst = vm.objVars();
                            if (dst.size() < src.size()) dst.resize(src.size());
                            for (size_t i = 0; i < src.size(); ++i) dst[i] = src[i];
                            break;
                        }
                    }
                }
                vm.callFunctionByName(copy.fun, copy.args);
            } catch (const std::exception& e) {
                std::cerr << "[thread] " << copy.fun << " error: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "[thread] " << copy.fun << " error: unknown\n";
            }
            setThreadTelnetClient(-1);
        });
    }
    for (auto& w : workers) w.detach();
}

} // namespace lpc
