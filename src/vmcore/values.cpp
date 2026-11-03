// ===========================================================================
// vmcore/values.cpp — 值语义实现 (搬运自原实现, 语义一字不改)
// ===========================================================================
#include "values.h"
#include "trace.h"
#include "original_vm.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

namespace lpc {
namespace vmcore {

int64_t toInt(const Value& v) {
    switch (v.index()) {
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: {
        const std::string& s = std::get<SharedString>(v);
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
    case 4: return std::strtod(std::get<SharedString>(v).c_str(), nullptr);
    default: return 0.0;
    }
}

bool isZero(const Value& v) {
    switch (v.index()) {
    case 0:
    case 1: return true;
    case 2: return std::get<int64_t>(v) == 0;
    case 3: return std::get<double>(v) == 0.0;
    case 7: {
        // Reference invalidation (original driver semantics): an object value
        // whose alive flag was cleared at real free IS 0 — master's iid map
        // entries and every other stored reference stop being truthy, so
        // `if (global_95[iid])` skips instead of reporting a duplicate iid.
        auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
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

// ---------------------------------------------------------------------------
// Original-VM numeric semantics (probed live): 32-bit int wrap, float32 mixing,
// left-operand type keeps, truncate-toward-zero back to int.
// ---------------------------------------------------------------------------
static int64_t i32(int64_t v) { return static_cast<int64_t>(static_cast<int32_t>(v)); }
static double f32(double v) { return static_cast<double>(static_cast<float>(v)); }
static int64_t truncWrap(double v) { return i32(static_cast<int64_t>(v)); }

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
        auto r = std::make_shared<MappingValue>(
            static_cast<uint32_t>(std::get<std::shared_ptr<MappingValue>>(a)->size()));
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
    if (a.index() == 4 || b.index() == 4) {
        std::string r;
        r.reserve((a.index() == 4 ? std::get<SharedString>(a).str().size() : 0) +
                  (b.index() == 4 ? std::get<SharedString>(b).str().size() : 0) + 8);
        append_text(r, a);
        append_text(r, b);
        return make_string(std::move(r));
    }
    if (a.index() == 3) return make_float(f32(toReal(a)) + f32(toReal(b)));
    if (b.index() == 3) return make_int(truncWrap(f32(toReal(a)) + f32(toReal(b))));
    return make_int(i32(toInt(a) + toInt(b)));
}

Value subValues(const Value& a, const Value& b) {
    // array - array: remove b's elements from a (vm_subValue / vm_subArray)
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
    // vm_lshValue (0x808df54): mapping << mapping MERGES IN PLACE on the left
    // (vm_concatMapping) — LPC relies on the aliasing everywhere
    // (register_daemon_data / itemd / partywar `m << ([...])`).
    if (a.index() == 6 && b.index() == 6) {
        auto left = std::get<std::shared_ptr<MappingValue>>(a);
        auto right = std::get<std::shared_ptr<MappingValue>>(b);
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
    return make_int(i32(static_cast<uint32_t>(toInt(a)) << (static_cast<uint32_t>(toInt(b)) & 31)));
}

Value rshValues(const Value& a, const Value& b) {
    return make_int(static_cast<int32_t>(toInt(a)) >> (static_cast<uint32_t>(toInt(b)) & 31));
}

Value andValues(const Value& a, const Value& b) {
    return make_int(static_cast<int32_t>(toInt(a)) & static_cast<int32_t>(toInt(b)));
}
Value orValues(const Value& a, const Value& b) {
    return make_int(static_cast<int32_t>(toInt(a)) | static_cast<int32_t>(toInt(b)));
}
Value xorValues(const Value& a, const Value& b) {
    return make_int(static_cast<int32_t>(toInt(a)) ^ static_cast<int32_t>(toInt(b)));
}

Value lpcOrValues(const Value& a, const Value& b) {
    if (a.index() == 5 && b.index() == 5) {
        auto out = std::make_shared<ArrayValue>();
        const auto& ai = std::get<std::shared_ptr<ArrayValue>>(a)->items;
        const auto& bi = std::get<std::shared_ptr<ArrayValue>>(b)->items;
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
    return orValues(a, b);
}

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
        const auto& x = std::get<SharedString>(a);
        const auto& y = std::get<SharedString>(b);
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
        int64_t x = std::get<int64_t>(a), y = std::get<int64_t>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    case 3: {
        double x = std::get<double>(a), y = std::get<double>(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    case 4: {
        const std::string x = std::get<SharedString>(a).str();
        const std::string y = std::get<SharedString>(b).str();
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
        const std::string& x = std::get<std::shared_ptr<BufferValue>>(a)->data;
        const std::string& y = std::get<std::shared_ptr<BufferValue>>(b)->data;
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
            case 5: return std::get<5>(v).get();
            case 6: return std::get<6>(v).get();
            case 7: return std::get<7>(v).get();
            case 8: return std::get<8>(v).get();
            case 9: return std::get<9>(v).get();
            case 10: return std::get<10>(v).get();
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
    switch (targetType) {
    case 2:
        if (v.index() == 3) return make_int(static_cast<int64_t>(std::get<double>(v)));
        if (v.index() == 4) {
            // (int)string -> strtol; non-numeric string of >=4 bytes -> big-endian
            // first 4 bytes (buffers are byte strings in this VM).
            const std::string& s = std::get<SharedString>(v);
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
            const std::string& s = std::get<std::shared_ptr<BufferValue>>(v)->data;
            uint64_t be = 0;
            for (unsigned char c : s) be = (be << 8) | c;
            return make_int(static_cast<int64_t>(be));
        }
        if (v.index() == 1 || v.index() == 0) return make_int(0);
        return v;
    case 3:
        if (v.index() == 2) return make_float(static_cast<double>(std::get<int64_t>(v)));
        if (v.index() == 4) return make_float(std::strtod(std::get<SharedString>(v).c_str(), nullptr));
        if (v.index() == 1 || v.index() == 0) return make_float(0.0);
        return v;
    case 4:
        if (v.index() == 4) return v;
        if (v.index() == 10) {
            // (string)buffer: raw bytes, stripping a leading UTF-8 BOM.
            std::string s = std::get<std::shared_ptr<BufferValue>>(v)->data;
            if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
                static_cast<unsigned char>(s[1]) == 0xBB &&
                static_cast<unsigned char>(s[2]) == 0xBF)
                s.erase(0, 3);
            return make_string(s);
        }
        if (v.index() == 0) return make_string("(undefined)");
        if (v.index() == 1) return make_string("(void)");
        return make_string(to_string(v, false));
    case 5:
        // (buffer) casts ALWAYS allocate a real buffer:
        //   int -> 4-byte big-endian; string -> same bytes; nil -> empty.
        if (v.index() == 2) {
            uint32_t n = static_cast<uint32_t>(static_cast<int32_t>(std::get<int64_t>(v)));
            std::string s;
            s += static_cast<char>((n >> 24) & 0xFF);
            s += static_cast<char>((n >> 16) & 0xFF);
            s += static_cast<char>((n >> 8) & 0xFF);
            s += static_cast<char>(n & 0xFF);
            return make_buffer(s);
        }
        if (v.index() == 4) return make_buffer(std::get<SharedString>(v));
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

Value queryMemberValue(const Value& container, const Value& key) {
    if (container.index() == 6) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(container);
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
                          << " intkey=" << std::get<int64_t>(key) << "\n";
            }
        }
        return Value{};   // miss -> original type 0 "undefined"
    }
    if (container.index() == 5) {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(container);
        if (!arr || reinterpret_cast<uintptr_t>(arr.get()) < 0x10000)
            return Value{};
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(arr->items.size()))
            return arr->items[static_cast<size_t>(i)];
        return Value{};
    }
    if (container.index() == 4) {
        // string indexing returns the ASCII code (int).
        const std::string& s = std::get<SharedString>(container);
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(s.size()))
            return make_int(static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(i)])));
        return Value{};
    }
    if (container.index() == 10) {
        // buffer indexing yields the BYTE as an int (vm_queryMember type-5
        // branch) — admincommandd::get_args relies on it.
        const std::string& s = std::get<std::shared_ptr<BufferValue>>(container)->data;
        int64_t i = toInt(key);
        if (i >= 0 && i < static_cast<int64_t>(s.size()))
            return make_int(static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(i)])));
        return Value{};
    }
    return Value{};
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
        len = std::get<std::shared_ptr<ArrayValue>>(container)->items.size();
    else if (container.index() == 4)
        len = std::get<SharedString>(container).size();
    else if (container.index() == 10)
        len = std::get<std::shared_ptr<BufferValue>>(container)->data.size();
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
        const std::string& s = std::get<SharedString>(container);
        const size_t b0 = static_cast<size_t>(a);
        const size_t b1 = static_cast<size_t>(b + 1);
        std::string out = s.substr(b0, b1 > b0 ? b1 - b0 : 0);
        return make_string(out);
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

} // namespace vmcore
} // namespace lpc
