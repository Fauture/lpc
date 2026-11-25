// ===========================================================================
// vmcore/values.cpp — 值语义实现 (搬运自原实现, 语义一字不改)
// ===========================================================================
#include "values.h"
#include "trace.h"
#include "vmcore.h"
#include "original_vm.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

namespace lpc {
namespace vmcore {

int64_t toInt(const Value& v) {
    switch (v.index()) {
    case 2: return lpc::vget<int64_t>(v);
    case 3: return static_cast<int64_t>(lpc::vget<double>(v));
    case 4: {
        const std::string& s = lpc::vget<SharedString>(v);
        return s.empty() ? 0 : std::strtoll(s.c_str(), nullptr, 10);
    }
    case 5: return static_cast<int64_t>(lpc::vget<lpc::Ref<ArrayValue>>(v)->items.size());
    case 6: return static_cast<int64_t>(lpc::vget<lpc::Ref<MappingValue>>(v)->size());
    default: return 0;
    }
}

double toReal(const Value& v) {
    switch (v.index()) {
    case 2: return static_cast<double>(lpc::vget<int64_t>(v));
    case 3: return lpc::vget<double>(v);
    case 4: return std::strtod(lpc::vget<SharedString>(v).c_str(), nullptr);
    default: return 0.0;
    }
}

bool isZero(const Value& v) {
    switch (v.index()) {
    case 0:
    case 1: return true;
    case 2: return lpc::vget<int64_t>(v) == 0;
    case 3: return lpc::vget<double>(v) == 0.0;
    case 7: {
        // Reference invalidation (original driver semantics): an object value
        // whose alive flag was cleared at real free IS 0 — master's iid map
        // entries and every other stored reference stop being truthy, so
        // `if (global_95[iid])` skips instead of reporting a duplicate iid.
        auto ov = lpc::vget<lpc::Ref<ObjectValue>>(v);
        if (!ov) return true;
        return ov->alive && !ov->alive->load(std::memory_order_relaxed);
    }
    default: return false;
    }
}

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

// 原版 vm_valueType (0x808a13c) 的类型名 —— vm_error 文案里的 %s 就是它。
// 注意原版编号: 5=buffer 6=array 7=mapping 9=function 10=object,
// 而 Value::index() 的是另一种编号, 这里按 index 映射回原版字符串。
const char* typeNameOf(const Value& v) {
    switch (v.index()) {
    case 0: return "undefined";
    case 1: return "void";
    case 2: return "integer";
    case 3: return "float";
    case 4: return "string";
    case 5: return "array";
    case 6: return "mapping";
    case 7: return "object";
    case 8: return "function";
    case 9: return "loop_ctrl";
    case 10: return "buffer";
    default: return "pointer";
    }
}

// 原版算术/位运算的文案都是三段式:
//   "<op>, type invalid (got %s) of left value."   左值类型不在白名单
//   "<op>, right isn't a number when left is."     左值已定, 右值与左值不配
// 白名单由各运算符按 IDA 反编译结果逐条写死, 这里不提供"半截"拼装 helper。

// ---------------------------------------------------------------------------
// Original-VM numeric semantics (probed live): 32-bit int wrap, float32 mixing,
// left-operand type keeps, truncate-toward-zero back to int.
// ---------------------------------------------------------------------------
static int64_t i32(int64_t v) { return static_cast<int64_t>(static_cast<int32_t>(v)); }
static double f32(double v) { return static_cast<double>(static_cast<float>(v)); }
static int64_t truncWrap(double v) { return i32(static_cast<int64_t>(v)); }

// vm_addValue 0x808e3b8 左值白名单: int / real / string / buffer / array /
// mapping —— undefined/void/object/function 一律
// "Bad +, type invalid (got %s) of left value."
Value addValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:   // int
        if (b.index() == 2) return make_int(i32(toInt(a) + toInt(b)));
        if (b.index() == 3) return make_int(truncWrap(f32(toReal(a)) + f32(toReal(b))));
        vmError("Bad +, right isn't a number when left is.\n");
    case 3:   // real
        if (b.index() == 2) return make_float(f32(toReal(a)) + f32(toReal(b)));
        if (b.index() == 3) return make_float(f32(toReal(a)) + f32(toReal(b)));
        vmError("Bad +, right isn't a real when left is.\n");
    case 4: { // string
        if (b.index() == 0)
            vmError("Bad +, string can not be add with undefined type.\n");
        if (b.index() == 1) vmError("Bad +, string can not be add with void.\n");
        if (b.index() != 2 && b.index() != 3 && b.index() != 4)
            vmError("Bad +, right can not be add with a string.\n");
        std::string r;
        r.reserve(lpc::vget<SharedString>(a).str().size() + 16);
        append_text(r, a);
        append_text(r, b);
        return make_string(std::move(r));
    }
    case 10: { // buffer
        if (b.index() == 1)
            vmError("Bad +, buffer can not be add with void.\n");
        if (b.index() == 0)
            vmError("Bad +, buffer can not be add with undefined type.\n");
        if (b.index() != 4 && b.index() != 10)
            vmError("Bad +, right can not be add with a buffer.\n");
        auto cat = [](const Value& v) -> std::string {
            if (v.index() == 10) return lpc::vget<lpc::Ref<BufferValue>>(v)->data;
            return lpc::vget<SharedString>(v).str();
        };
        return make_buffer(lpc::vget<lpc::Ref<BufferValue>>(a)->data + cat(b));
    }
    case 5: { // array
        if (b.index() != 5)
            vmError("Bad +, right can not be add with an array.\n");
        auto r = lpc::Ref<ArrayValue>(new ArrayValue());
        const auto& aa = lpc::vget<lpc::Ref<ArrayValue>>(a)->items;
        const auto& bb = lpc::vget<lpc::Ref<ArrayValue>>(b)->items;
        r->items.reserve(aa.size() + bb.size());
        r->items.insert(r->items.end(), aa.begin(), aa.end());
        r->items.insert(r->items.end(), bb.begin(), bb.end());
        return r;
    }
    case 6: { // mapping
        if (b.index() != 6)
            vmError("Bad +, right can not be add with a mapping.\n");
        auto r = lpc::Ref<MappingValue>(new MappingValue(
            static_cast<uint32_t>(lpc::vget<lpc::Ref<MappingValue>>(a)->size())));
        for (const auto& kv : lpc::vget<lpc::Ref<MappingValue>>(a)->items())
            r->insert(kv.first, kv.second);
        for (const auto& kv : lpc::vget<lpc::Ref<MappingValue>>(b)->items())
            r->insert(kv.first, kv.second);
        return r;
    }
    default:
        vmError(std::string("Bad +, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_subValue 0x808e654 左值白名单: int / real / array / mapping。
Value subValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 3:   // real
        if (b.index() == 2) return make_float(f32(toReal(a) - f32(toReal(b))));
        if (b.index() == 3) return make_float(f32(toReal(a)) - f32(toReal(b)));
        vmError("Bad -, right isn't a number when left is.\n");
    case 2:   // int
        if (b.index() == 2) return make_int(i32(toInt(a) - toInt(b)));
        if (b.index() == 3)
            return make_int(truncWrap(f32(toReal(a)) - f32(toReal(b))));
        vmError("Bad -, right isn't a number when left is.\n");
    case 5: { // array - array: remove b's elements from a (vm_subArray)
        if (b.index() != 5)
            vmError("Bad -, right can not be sub with an array.\n");
        auto r = lpc::Ref<ArrayValue>(new ArrayValue());
        const auto& bb = lpc::vget<lpc::Ref<ArrayValue>>(b)->items;
        for (const auto& item : lpc::vget<lpc::Ref<ArrayValue>>(a)->items) {
            bool found = false;
            for (const auto& x : bb)
                if (value_equal(item, x)) { found = true; break; }
            if (!found) r->items.push_back(item);
        }
        return r;
    }
    case 6: { // mapping - mapping/array: remove keys present in b
        if (b.index() != 6 && b.index() != 5)
            vmError("Bad -, right can not be sub with a mapping or an array.\n");
        auto r = lpc::Ref<MappingValue>(new MappingValue());
        auto am = lpc::vget<lpc::Ref<MappingValue>>(a);
        for (const auto& kv : am->items()) {
            bool found = false;
            if (b.index() == 6) {
                found = lpc::vget<lpc::Ref<MappingValue>>(b)->find(kv.first) != nullptr;
            } else {
                for (const auto& x : lpc::vget<lpc::Ref<ArrayValue>>(b)->items)
                    if (value_equal(kv.first, x)) { found = true; break; }
            }
            if (!found) r->insert(kv.first, kv.second);
        }
        return r;
    }
    default:
        vmError(std::string("Bad -, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_mulValue 0x808e2f8 左值白名单: int / real。
Value mulValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:
        if (b.index() == 2) return make_int(i32(toInt(a) * toInt(b)));
        if (b.index() == 3)
            return make_int(truncWrap(f32(toReal(a)) * f32(toReal(b))));
        vmError("Bad *, right isn't a number when left is.\n");
    case 3:
        if (b.index() == 2) return make_float(f32(toReal(a)) * f32(toReal(b)));
        if (b.index() == 3) return make_float(f32(toReal(a)) * f32(toReal(b)));
        vmError("Bad *, right isn't a number when left is.\n");
    default:
        vmError(std::string("Bad *, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_divValue 0x808e1a4 左值白名单: int / real; 除零直接报错。
Value divValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:
        if (b.index() == 2) {
            const int64_t d = toInt(b);
            if (d == 0) vmError("divide by zero.\n");
            return make_int(i32(toInt(a) / d));
        }
        if (b.index() == 3) {
            const double d = f32(toReal(b));
            if (d == 0.0) vmError("divide by zero.\n");
            return make_int(truncWrap(f32(toReal(a)) / d));
        }
        vmError("Bad /, right isn't a number when left is.\n");
    case 3:
        if (b.index() == 2) {
            const int64_t d = toInt(b);
            if (d == 0) vmError("divide by zero.\n");
            return make_float(f32(toReal(a)) / f32(static_cast<double>(d)));
        }
        if (b.index() == 3) {
            const double d = f32(toReal(b));
            if (d == 0.0) vmError("divide by zero.\n");
            return make_float(f32(toReal(a)) / f32(d));
        }
        vmError("Bad /, right isn't a number when left is.\n");
    default:
        vmError(std::string("Bad /, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_modValue 0x808e7bc 左值白名单: int / real。
//   左 int 右 int → int（模 0 报 "Module by zero."）；右 real → fmod, 结果 FLOAT
//   左 real        → fmod, 结果 float
// 右值非数时原版用的文案是 "Bad *, right isn't a real when left is."（原文如此）。
Value modValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:
        if (b.index() == 2) {
            const int64_t d = toInt(b);
            if (d == 0) vmError("Module by zero.\n");
            return make_int(i32(toInt(a) % d));
        }
        if (b.index() == 3) {
            const double d = f32(toReal(b));
            if (d == 0.0) return make_float(0.0);
            return make_float(f32(std::fmod(f32(toReal(a)), d)));
        }
        vmError("Bad *, right isn't a real when left is.\n");
    case 3:
        if (b.index() == 2 || b.index() == 3) {
            const double d = f32(toReal(b));
            if (d == 0.0) return make_float(0.0);
            return make_float(f32(std::fmod(f32(toReal(a)), d)));
        }
        vmError("Bad *, right isn't a real when left is.\n");
    default:
        vmError(std::string("Bad %, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_lshValue 0x808df54 左值白名单: int / mapping（mapping<<mapping 原地合并）。
Value lshValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 6: {   // mapping << mapping MERGES IN PLACE on the left (vm_concatMapping)
        if (b.index() != 6)
            vmError("Bad <<, right isn't a mapping when left is.\n");
        auto left = lpc::vget<lpc::Ref<MappingValue>>(a);
        auto right = lpc::vget<lpc::Ref<MappingValue>>(b);
        // [lshdbg] import 合并取证: room import 的 dbase<<room_info 若左值
        // 是副本（非 dbase 引用）合并即丢失 (monster_list undefined 实锤)。
        static const std::atomic<bool>& ldFlag =
            lpc::dynProbeFlag("LPCVM_LSHDBG");
        if (ldFlag.load(std::memory_order_relaxed)) {
            const auto lk = left->items();
            const auto rk = right->items();
            if (rk.size() >= 4) {   // room_info 这类多字段合并才打
                std::cerr << "[lshdbg] merge left=" << (const void*)left.get()
                          << " left_sz=" << lk.size()
                          << " right_sz=" << rk.size() << " keys=";
                int n = 0;
                for (const auto& kv : rk) {
                    if (n++ >= 10) break;
                    std::cerr << to_string(kv.first, false).substr(0, 20)
                              << ",";
                }
                // monster_list 的实际值 (暗雷取证: 合并时的真实内容)
                for (const auto& kv : rk) {
                    if (to_string(kv.first, false) == "monster_list") {
                        std::cerr << " ML="
                                  << to_string(kv.second, false).substr(0, 120);
                        break;
                    }
                }
                std::cerr << "\n";
            }
        }
        for (const auto& kv : right->items()) left->insert(kv.first, kv.second);
        if (ldFlag.load(std::memory_order_relaxed) && right->size() >= 4)
            std::cerr << "[lshdbg] merged left=" << (const void*)left.get()
                      << " post_sz=" << left->size() << "\n";
        return left;   // same shared_ptr, mutated in place
    }
    case 2: {
        if (b.index() != 2)
            vmError("Bad <<, right isn't a number when left is.\n");
        const uint32_t sh = static_cast<uint32_t>(toInt(b));
        if (sh > 0x1F) return make_int(0);   // 原版: info>0x1F 直接 0
        return make_int(i32(static_cast<uint32_t>(toInt(a)) << sh));
    }
    default:
        vmError(std::string("Bad <<, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_rshValue 0x808e020 左值白名单: int / mapping。
//   int >> int 是 **逻辑右移** (原版 unsigned >>)，移位 >0x1F 结果为 0。
Value rshValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 6: {
        if (b.index() != 6)
            vmError("Bad >>, right isn't a mapping when left is.\n");
        auto left = lpc::vget<lpc::Ref<MappingValue>>(a);
        auto right = lpc::vget<lpc::Ref<MappingValue>>(b);
        for (const auto& kv : right->items()) left->insert(kv.first, kv.second);
        return left;
    }
    case 2: {
        if (b.index() != 2)
            vmError("Bad >>, right isn't a number when left is.\n");
        const uint32_t sh = static_cast<uint32_t>(toInt(b));
        if (sh > 0x1F) return make_int(0);
        return make_int(static_cast<int32_t>(
            static_cast<uint32_t>(toInt(a)) >> sh));
    }
    default:
        vmError(std::string("Bad >>, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_andValue 0x808ea10 左值白名单: int / array（array&array = 交集）。
Value andValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:
        if (b.index() != 2)
            vmError("Bad &, right isn't a number when left is.\n");
        return make_int(static_cast<int32_t>(toInt(a)) &
                        static_cast<int32_t>(toInt(b)));
    case 5: {
        if (b.index() != 5)
            vmError("Bad |, right is't an array when left is.\n");
        auto out = lpc::Ref<ArrayValue>(new ArrayValue());
        const auto& bi = lpc::vget<lpc::Ref<ArrayValue>>(b)->items;
        for (const auto& x : lpc::vget<lpc::Ref<ArrayValue>>(a)->items) {
            bool hit = false;
            for (const auto& y : bi)
                if (value_equal(x, y)) { hit = true; break; }
            if (!hit) continue;
            bool dup = false;
            for (const auto& y : out->items)
                if (value_equal(x, y)) { dup = true; break; }
            if (!dup) out->items.push_back(x);
        }
        return out;
    }
    default:
        vmError(std::string("Bad &, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_orValue 0x808eabc 左值白名单: int / array（array|array = 并集去重）。
Value orValues(const Value& a, const Value& b) {
    switch (a.index()) {
    case 2:
        if (b.index() != 2)
            vmError("Bad |, right isn't a number when left is.\n");
        return make_int(static_cast<int32_t>(toInt(a)) |
                        static_cast<int32_t>(toInt(b)));
    case 5: {
        if (b.index() != 5)
            vmError("Bad |, right is't an array when left is.\n");
        auto out = lpc::Ref<ArrayValue>(new ArrayValue());
        const auto& ai = lpc::vget<lpc::Ref<ArrayValue>>(a)->items;
        const auto& bi = lpc::vget<lpc::Ref<ArrayValue>>(b)->items;
        out->items.reserve(ai.size() + bi.size());
        for (const auto& x : ai) out->items.push_back(x);
        for (const auto& x : bi) {
            bool dup = false;
            for (const auto& y : out->items)
                if (value_equal(x, y)) { dup = true; break; }
            if (!dup) out->items.push_back(x);
        }
        return out;
    }
    default:
        vmError(std::string("Bad |, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    }
}

// vm_xorValue 0x808e134 左值白名单: 仅 int。
Value xorValues(const Value& a, const Value& b) {
    if (a.index() != 2)
        vmError(std::string("Bad ^, type invalid (got ") + typeNameOf(a) +
                ") of left value.\n");
    if (b.index() != 2)
        vmError("Bad ^, right isn't a number when left is.\n");
    return make_int(static_cast<int32_t>(toInt(a)) ^
                    static_cast<int32_t>(toInt(b)));
}

// 逻辑/算术共用的 op | 语义: 原版只有 vm_orValue 一份。
Value lpcOrValues(const Value& a, const Value& b) { return orValues(a, b); }

// vm_negValue 0x808e8bc: 单目取负, 只接受 int / real。
//   注意原版文案是 "of right value" 且不带 (got %s)。
Value negValue(const Value& a) {
    switch (a.index()) {
    case 2: return make_int(i32(-toInt(a)));
    case 3: return make_float(-f32(toReal(a)));
    default:
        vmError("Bad -, type invalid of right value.\n");
    }
}

int compareValues(const Value& a, const Value& b) {
    if (a.index() == 3 || b.index() == 3) {
        double x = toReal(a), y = toReal(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    if (a.index() == 4 && b.index() == 4) {
        const auto& x = lpc::vget<SharedString>(a);
        const auto& y = lpc::vget<SharedString>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    int64_t x = toInt(a), y = toInt(b);
    return x < y ? -1 : (x > y ? 1 : 0);
}

int compareValuesOriginal(const Value& a, const Value& b) {
    auto rank = [](const Value& v) -> int {
        switch (v.index()) {
        case 0: case 1: return static_cast<int>(v.index());
        case 2: case 3: case 4: return static_cast<int>(v.index());
        case 10: return 5;              // buffer
        case 5: return 6;               // array
        case 6: return 7;               // mapping
        case 7: return 8;               // object
        case 8: return 9;               // closure
        default: return 10;
        }
    };
    const int ra = rank(a), rb = rank(b);
    if (ra != rb) return ra < rb ? -1 : 1;
    switch (ra) {
    case 0: case 1:
        return 0;
    case 2: {
        int64_t x = lpc::vget<int64_t>(a), y = lpc::vget<int64_t>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    case 3: {
        double x = lpc::vget<double>(a), y = lpc::vget<double>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    case 4: {
        const std::string x = lpc::vget<SharedString>(a).str();
        const std::string y = lpc::vget<SharedString>(b).str();
        if (!x.empty() && !y.empty() &&
            std::isdigit(static_cast<unsigned char>(x[0])) &&
            std::isdigit(static_cast<unsigned char>(y[0]))) {
            long long nx = std::strtoll(x.c_str(), nullptr, 10);
            long long ny = std::strtoll(y.c_str(), nullptr, 10);
            if (nx < ny) return -1;
            if (nx > ny) return 1;
        }
        int c = x.compare(y);
        return c < 0 ? -1 : (c > 0 ? 1 : 0);
    }
    case 5: {
        const std::string& x = lpc::vget<lpc::Ref<BufferValue>>(a)->data;
        const std::string& y = lpc::vget<lpc::Ref<BufferValue>>(b)->data;
        size_t n = x.size() < y.size() ? x.size() : y.size();
        if (n) {
            int c = std::memcmp(x.data(), y.data(), n);
            if (c) return c < 0 ? -1 : 1;
        }
        return x.size() < y.size() ? -1 : (x.size() > y.size() ? 1 : 0);
    }
    default: {
        auto ptr = [](const Value& v) -> const void* {
            switch (v.index()) {
            case 5: return lpc::vgetI<5>(v).get();
            case 6: return lpc::vgetI<6>(v).get();
            case 7: return lpc::vgetI<7>(v).get();
            case 8: return lpc::vgetI<8>(v).get();
            case 9: return lpc::vgetI<9>(v).get();
            case 10: return lpc::vgetI<10>(v).get();
            default: return nullptr;
            }
        };
        const void* x = ptr(a);
        const void* y = ptr(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    }
}

Value castValue(Value v, int targetType) {
    // 原版 vm_castType 0x809006c: 每个目标类型只接受白名单源类型, 其余一律
    // vm_error("Can not cast the type to <t>.") —— 旧实现用 `return v` /
    // to_string 兜底静默放行, 于是 (mapping)0 变成"空 mapping", 后续
    // map["k"] 取到 undefined 并被 (string) 渲染成 "(undefined)" 写进库
    // (server_status.report_time 事故)。以下按 IDA 白名单逐 case 对齐。
    // 源类型编号 (我们的 Value index): 0=nil 1=undef 2=int 3=real 4=string
    //   5=array 6=mapping 7=object 8=closure 9=loop 10=buffer 11=xref
    //   原版: 5=buffer 6=array 7=mapping —— 映射见各 case 注释。
    switch (targetType) {
    case 2:
        if (v.index() == 3) return make_int(static_cast<int64_t>(lpc::vget<double>(v)));
        if (v.index() == 4) {
            // (int)string -> strtol; non-numeric string of >=4 bytes -> big-endian
            // first 4 bytes (buffers are byte strings in this VM).
            const std::string& s = lpc::vget<SharedString>(v);
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
            // (int)buffer: every byte big-endian (v22 = byte | (v22 << 8)).
            const std::string& s = lpc::vget<lpc::Ref<BufferValue>>(v)->data;
            uint64_t be = 0;
            for (unsigned char c : s) be = (be << 8) | c;
            return make_int(static_cast<int64_t>(be));
        }
        if (v.index() == 2) return v;
        if (v.index() == 1 || v.index() == 0) return make_int(0);
        vmError("Can not cast the type to integer.\n");
    case 3:
        if (v.index() == 2) return make_float(static_cast<double>(lpc::vget<int64_t>(v)));
        if (v.index() == 4) return make_float(std::strtod(lpc::vget<SharedString>(v).c_str(), nullptr));
        if (v.index() == 3) return v;
        if (v.index() == 1 || v.index() == 0) return make_float(0.0);
        // 原版 real 白名单只有 undefined/void/int/real/string (buffer 也报错)。
        vmError("Can not cast the type to real.\n");
    case 4:
        if (v.index() == 4) return v;
        if (v.index() == 10) {
            // (string)buffer: raw bytes, stripping a leading UTF-8 BOM.
            std::string s = lpc::vget<lpc::Ref<BufferValue>>(v)->data;
            if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
                static_cast<unsigned char>(s[1]) == 0xBB &&
                static_cast<unsigned char>(s[2]) == 0xBF)
                s.erase(0, 3);
            return make_string(s);
        }
        if (v.index() == 0) return make_string("(undefined)");
        if (v.index() == 1) return make_string("(void)");
        if (v.index() == 2 || v.index() == 3)
            return make_string(to_string(v, false));   // 数字 → 文本 (vm_typeValue)
        // 原版: array/mapping/object/closure 等 → "Can not cast the type to string."
        vmError("Can not cast the type to string.\n");
    case 5:
        // (buffer) casts ALWAYS allocate a real buffer:
        //   int -> 4-byte big-endian; string -> same bytes; nil -> empty.
        if (v.index() == 2) {
            uint32_t n = static_cast<uint32_t>(static_cast<int32_t>(lpc::vget<int64_t>(v)));
            std::string s;
            s += static_cast<char>((n >> 24) & 0xFF);
            s += static_cast<char>((n >> 16) & 0xFF);
            s += static_cast<char>((n >> 8) & 0xFF);
            s += static_cast<char>(n & 0xFF);
            return make_buffer(s);
        }
        if (v.index() == 4) return make_buffer(lpc::vget<SharedString>(v));
        if (v.index() == 1 || v.index() == 0) return make_buffer("");
        if (v.index() == 10) return v;
        // 原版 buffer 白名单: undefined/void/int/string/buffer (real 亦报错)。
        vmError("Can not cast the type to buffer.\n");
    case 6:
        if (v.index() == 1 || v.index() == 0) return make_array();
        if (v.index() == 5) return v;
        vmError("Can not cast the type to array.\n");
    case 7:
        if (v.index() == 1 || v.index() == 0) return make_mapping();
        if (v.index() == 6) return v;
        vmError("Can not cast the type to mapping.\n");
    default:
        vmError("Can not cast to such type.\n");
    }
}

Value queryMemberValue(const Value& container, const Value& key) {
    if (container.index() == 6) {
        auto mp = lpc::vget<lpc::Ref<MappingValue>>(container);
        // A NULL (or implausible) mapping payload reads as a miss instead of
        // crashing (tmp_dbase[path] before materialisation).
        if (!mp || reinterpret_cast<uintptr_t>(mp.get()) < 0x10000)
            return Value{};
        const Value* hit = mp->find(key);
        if (hit) return *hit;
        {
            if (vmcore::traceOn() && key.index() == 2) {
                std::cerr << "[mapmiss] map=" << mp.get()
                          << " size=" << mp->size()
                          << " intkey=" << lpc::vget<int64_t>(key) << "\n";
            }
        }
        return Value{};   // miss -> original type 0 "undefined"
    }
    if (container.index() == 5) {
        auto arr = lpc::vget<lpc::Ref<ArrayValue>>(container);
        if (!arr || reinterpret_cast<uintptr_t>(arr.get()) < 0x10000)
            return Value{};
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(arr->items.size()))
            return arr->items[static_cast<size_t>(i)];
        // 原版 vm_queryMember 0x808fc25: "Index of the array out of range."
        // (临时附带 idx/size, 定位越界现场后改回原文案)
        vmError("Index of the array out of range. (idx=" + std::to_string(i) +
                " size=" + std::to_string(arr->items.size()) + ")\n");
    }
    if (container.index() == 4) {
        // string indexing returns the ASCII code (int).
        const std::string& s = lpc::vget<SharedString>(container);
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(s.size()))
            return make_int(static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(i)])));
        // 原版 0x808fb28: "Index out of range."
        vmError("Index out of range.\n");
    }
    if (container.index() == 10) {
        // buffer indexing yields the BYTE as an int (vm_queryMember type-5
        // branch) — admincommandd::get_args relies on it.
        const std::string& s = lpc::vget<lpc::Ref<BufferValue>>(container)->data;
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(s.size()))
            return make_int(static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(i)])));
        vmError("Index out of range.\n");
    }
    // 原版 0x808fabd: 容器不是 array/string/buffer/mapping 时
    // "Bad type(%s) to index." —— 旧实现返回 undefined, 被 (string) 渲染成
    // "(undefined)" 后一路扩散 (server_status.report_time 事故的同源分支)。
    vmError(std::string("Bad type(") + typeNameOf(container) + ") to index.\n");
}

// GBK character helpers (string ranges are BYTE based; kept for parity).
static size_t gbkCharLenAt(const std::string& s, size_t bytePos) {
    if (bytePos >= s.size()) return 0;
    if (static_cast<unsigned char>(s[bytePos]) < 0x80) return 1;
    return (bytePos + 1 < s.size()) ? 2 : 1;
}
static size_t gbkCharCount(const std::string& s) {
    size_t n = 0, i = 0;
    while (i < s.size()) { ++n; i += gbkCharLenAt(s, i); }
    return n;
}
static size_t gbkByteOffsetOfChar(const std::string& s, int64_t charIdx) {
    if (charIdx <= 0) return 0;
    size_t i = 0;
    int64_t c = 0;
    while (i < s.size() && c < charIdx) { i += gbkCharLenAt(s, i); ++c; }
    return i;
}

Value queryRangeValue(const Value& container, const Value& from, const Value& to,
                      int flag, bool asPtr) {
    int64_t a = toInt(from);
    int64_t b = toInt(to);
    size_t len = 0;
    // String ranges are BYTE based, exactly like the original
    // (vm_queryRange 0x808fd04 does byte pointer arithmetic over pString->len).
    if (container.index() == 5)
        len = lpc::vget<lpc::Ref<ArrayValue>>(container)->items.size();
    else if (container.index() == 4)
        len = lpc::vget<SharedString>(container).size();
    else if (container.index() == 10)
        len = lpc::vget<lpc::Ref<BufferValue>>(container)->data.size();
    if (flag & 1) { if (b <= 0) b = 1; b = static_cast<int64_t>(len) - b; }
    if (flag & 2) { if (a <= 0) a = 1; a = static_cast<int64_t>(len) - a; }
    if (a < 0) a = 0;
    if (static_cast<int64_t>(len) <= b) b = static_cast<int64_t>(len) - 1;
    if (a > b) {
        // Empty range keeps the container's type (buffer slice stays buffer).
        if (container.index() == 4) return make_string("");
        if (container.index() == 10) return make_buffer("");
        return make_array();
    }
    if (container.index() == 4) {
        const std::string& s = lpc::vget<SharedString>(container);
        const size_t b0 = static_cast<size_t>(a);
        const size_t b1 = static_cast<size_t>(b + 1);
        std::string out = s.substr(b0, b1 > b0 ? b1 - b0 : 0);
        return make_string(out);
    }
    if (container.index() == 10) {
        const std::string& s = lpc::vget<lpc::Ref<BufferValue>>(container)->data;
        return make_buffer(s.substr(static_cast<size_t>(a), static_cast<size_t>(b - a + 1)));
    }
    if (container.index() == 5) {
        const auto& items = lpc::vget<lpc::Ref<ArrayValue>>(container)->items;
        auto arr = lpc::Ref<ArrayValue>(new ArrayValue());
        for (int64_t i = a; i <= b; ++i) arr->items.push_back(items[static_cast<size_t>(i)]);
        return arr;
    }
    return make_array();
}

} // namespace vmcore
} // namespace lpc
