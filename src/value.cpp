#include "value.h"
#include "original_vm.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>

namespace lpc {

// Out-of-line definition of SharedString's empty-text anchor (str() of a
// null-block / empty SharedString returns this).
const std::string SharedString::kEmpty;

// ---------------------------------------------------------------------------
// Original vm_mapping.c support: whashstr/whashstrc with the driver's T table
// (dumped from magic_Linux32x at 0x83cc3e0).
// ---------------------------------------------------------------------------
namespace {

const unsigned char kHashT[256] = {
    0x01, 0x57, 0x31, 0x0c, 0xb0, 0xb2, 0x66, 0xa6, 0x79, 0xc1, 0x06, 0x54, 0xf9, 0xe6, 0x2c, 0xa3,
    0x0e, 0xc5, 0xd5, 0xb5, 0xa1, 0x55, 0xda, 0x50, 0x40, 0xef, 0x18, 0xe2, 0xec, 0x8e, 0x26, 0xc8,
    0x6e, 0xb1, 0x68, 0x67, 0x8d, 0xfd, 0xff, 0x32, 0x4d, 0x65, 0x51, 0x12, 0x2d, 0x60, 0x1f, 0xde,
    0x19, 0x6b, 0xbe, 0x46, 0x56, 0xed, 0xf0, 0x22, 0x48, 0xf2, 0x14, 0xd6, 0xf4, 0xe3, 0x95, 0xeb,
    0x61, 0xea, 0x39, 0x16, 0x3c, 0xfa, 0x52, 0xaf, 0xd0, 0x05, 0x7f, 0xc7, 0x6f, 0x3e, 0x87, 0xf8,
    0xae, 0xa9, 0xd3, 0x3a, 0x42, 0x9a, 0x6a, 0xc3, 0xf5, 0xab, 0x11, 0xbb, 0xb6, 0xb3, 0x00, 0xf3,
    0x84, 0x38, 0x94, 0x4b, 0x80, 0x85, 0x9e, 0x64, 0x82, 0x7e, 0x5b, 0x0d, 0x99, 0xf6, 0xd8, 0xdb,
    0x77, 0x44, 0xdf, 0x4e, 0x53, 0x58, 0xc9, 0x63, 0x7a, 0x0b, 0x5c, 0x20, 0x88, 0x72, 0x34, 0x0a,
    0x8a, 0x1e, 0x30, 0xb7, 0x9c, 0x23, 0x3d, 0x1a, 0x8f, 0x4a, 0xfb, 0x5e, 0x81, 0xa2, 0x3f, 0x98,
    0xaa, 0x07, 0x73, 0xa7, 0xf1, 0xce, 0x03, 0x96, 0x37, 0x3b, 0x97, 0xdc, 0x5a, 0x35, 0x17, 0x83,
    0x7d, 0xad, 0x0f, 0xee, 0x4f, 0x5f, 0x59, 0x10, 0x69, 0x89, 0xe1, 0xe0, 0xd9, 0xa0, 0x25, 0x7b,
    0x76, 0x49, 0x02, 0x9d, 0x2e, 0x74, 0x09, 0x91, 0x86, 0xe4, 0xcf, 0xd4, 0xca, 0xd7, 0x45, 0xe5,
    0x1b, 0xbc, 0x43, 0x7c, 0xa8, 0xfc, 0x2a, 0x04, 0x1d, 0x6c, 0x15, 0xf7, 0x13, 0xcd, 0x27, 0xcb,
    0xe9, 0x28, 0xba, 0x93, 0xc6, 0xc0, 0x9b, 0x21, 0xa4, 0xbf, 0x62, 0xcc, 0xa5, 0xb4, 0x75, 0x4c,
    0x8c, 0x24, 0xd2, 0xac, 0x29, 0x36, 0x9f, 0x08, 0xb9, 0xe8, 0x71, 0xc4, 0xe7, 0x2f, 0x92, 0x78,
    0x33, 0x41, 0x1c, 0x90, 0xfe, 0xdd, 0x5d, 0xbd, 0xc2, 0x8b, 0x70, 0x2b, 0x47, 0x6d, 0xb8, 0xd1,
};

// vm_whashstr (0x8089dc8): case sensitive string hash, 16 bit result.
uint16_t whashstr(const char* str, int maxchar) {
    if (!str[0] || maxchar <= 0) return 0;
    unsigned h1 = kHashT[static_cast<unsigned char>(str[0])];
    unsigned h2 = 0;
    for (int i = 1; i <= maxchar - 1 && str[i]; ++i) {
        unsigned c = static_cast<unsigned char>(str[i]);
        h1 = kHashT[h1 ^ c];
        h2 = kHashT[c ^ h2];
    }
    return static_cast<uint16_t>((h1 << 8) | h2);
}

// vm_whashstrc (0x8089e40): case insensitive variant (chars OR 0x20).
uint16_t whashstrc(const char* str, int maxchar) {
    if (!str[0] || maxchar <= 0) return 0;
    unsigned h1 = kHashT[static_cast<unsigned char>(str[0]) | 0x20];
    unsigned h2 = 0;
    for (int i = 1; i <= maxchar - 1 && str[i]; ++i) {
        unsigned c = static_cast<unsigned char>(str[i]) | 0x20;
        h1 = kHashT[h1 ^ c];
        h2 = kHashT[c ^ h2];
    }
    return static_cast<uint16_t>((h1 << 8) | h2);
}

bool striequal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca | 0x20);
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb | 0x20);
        if (ca != cb) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Fast numeric -> text.  The previous to_string() number paths built a
// std::ostringstream per conversion, and EIP sampling showed ~85% of CPU
// during gs's basic_objectd import inside the stream/locale machinery
// (basic_ostringstream/_ltod3_default/_ultod3_default/...).  These printers
// emit byte-identical text to the old `os << int64` / `os << double`
// (C locale, default precision 6 = %g semantics) without any stream object.
// ---------------------------------------------------------------------------
inline void appendInt64(std::string& out, int64_t v) {
    char buf[24];
    char* p = buf + sizeof(buf);
    uint64_t u = v < 0 ? 0ULL - static_cast<uint64_t>(v) : static_cast<uint64_t>(v);
    do {
        *--p = static_cast<char>('0' + (u % 10));
        u /= 10;
    } while (u);
    if (v < 0) *--p = '-';
    out.append(p, static_cast<size_t>(buf + sizeof(buf) - p));
}

inline std::string formatInt64(int64_t v) {
    std::string r;
    appendInt64(r, v);
    return r;
}

// Real text: integral reals render as integers (the old ostringstream branch
// printed `(int64)d` when d == (int64)d); everything else renders like
// ostream << double with the default precision (6), i.e. printf %.6g.
inline void appendReal64(std::string& out, double d) {
    if (d == static_cast<int64_t>(d)) {
        appendInt64(out, static_cast<int64_t>(d));
        return;
    }
    char buf[40];
    int n = std::snprintf(buf, sizeof(buf), "%.6g", d);
    if (n > 0) out.append(buf, static_cast<size_t>(n));
}

// Byte-parity self check (LPCVM_FMT_CHECK=1): exercises the fast printers
// against the ostringstream reference over an integer corpus plus a sweep of
// the float32 space, then reports any mismatch once to stderr.
bool fmtSelfCheckOnce() {
    static const bool on = (std::getenv("LPCVM_FMT_CHECK") != nullptr);
    static bool done = [] {
        if (!on) return false;
        int mism = 0, tested = 0;
        auto cmpI = [&](int64_t v) {
            ++tested;
            std::ostringstream ref; ref << v;
            if (formatInt64(v) != ref.str()) {
                if (mism < 5) std::cerr << "[fmt-check] int mismatch " << v << " fast="
                                        << formatInt64(v) << " ref=" << ref.str() << "\n";
                ++mism;
            }
        };
        cmpI(0); cmpI(1); cmpI(-1); cmpI(9); cmpI(10); cmpI(-10);
        cmpI(std::numeric_limits<int64_t>::min()); cmpI(std::numeric_limits<int64_t>::max());
        cmpI(123456789012345678LL); cmpI(-123456789012345678LL); cmpI(1000000);
        auto cmpD = [&](double d) {
            ++tested;
            std::ostringstream ref;
            if (d == static_cast<int64_t>(d)) ref << static_cast<int64_t>(d);
            else ref << d;
            std::string fast;
            appendReal64(fast, d);
            if (fast != ref.str()) {
                if (mism < 8) std::cerr << "[fmt-check] real mismatch d=" << d
                                        << " fast=" << fast << " ref=" << ref.str() << "\n";
                ++mism;
            }
        };
        cmpD(0.0); cmpD(-0.0); cmpD(0.5); cmpD(-0.5); cmpD(0.1); cmpD(33.333332f);
        cmpD(1.0 / 3.0); cmpD(1e6); cmpD(1e-6); cmpD(1e20); cmpD(1e-20); cmpD(123.456);
        // float32-space sweep (uniform stride + band edges)
        for (uint64_t b = 0; b < 0x100000000ULL; b += 65536u) {
            float f;
            uint32_t bits = static_cast<uint32_t>(b);
            std::memcpy(&f, &bits, 4);
            cmpD(static_cast<double>(f));
        }
        std::cerr << "[fmt-check] tested=" << tested << " mismatches=" << mism << "\n";
        return true;
    }();
    return done;
}

} // namespace

uint32_t MappingValue_hashKey(const Value& key, bool caseSensitive);

namespace {
// Live container census: incremented by ArrayValue/MappingValue ctors and
// decremented by their dtors (they are always used via shared_ptr, so a dtor
// runs exactly once when the last owner drops).
struct HeapStats {
    std::atomic<long> arrLive{0};
    std::atomic<long> mapLive{0};
    std::atomic<long> arrCum{0};
    std::atomic<long> mapCum{0};
};
HeapStats& heapStats() { static HeapStats s; return s; }
} // namespace

ArrayValue::ArrayValue() { heapStats().arrLive++; heapStats().arrCum++; }
ArrayValue::~ArrayValue() { heapStats().arrLive--; }
MappingValue::~MappingValue() { heapStats().mapLive--; }

void liveHeapSnapshot(long& arrLive, long& mapLive, long& arrCum, long& mapCum) {
    arrLive = heapStats().arrLive.load();
    mapLive = heapStats().mapLive.load();
    arrCum = heapStats().arrCum.load();
    mapCum = heapStats().mapCum.load();
}

MappingValue::MappingValue() {
    table.assign(8, -1);
    heapStats().mapLive++;
    heapStats().mapCum++;
}
MappingValue::MappingValue(uint32_t sizeHint) {
    // vm_safeAllocateMapping: size <= 8 -> 8 buckets; otherwise round the
    // size up to a power of two and use that many buckets.
    uint32_t n = sizeHint >= 0x300001 ? 0x300000 : sizeHint;
    if (n > 8) {
        uint32_t v = n;
        v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
        tableSize = v;
        table.assign(v + 1, -1);
    } else {
        tableSize = 7;
        table.assign(8, -1);
    }
    unfilled = static_cast<int32_t>(80 * (tableSize + 1) / 100);
    heapStats().mapLive++;
    heapStats().mapCum++;
}

// Hash of a key exactly like vm_safeFindForInsert:
//   string -> whashstr / whashstrc, int -> number >> 4,
//   float -> low 32 bits of the double >> 4, everything else -> identity >> 4.
uint32_t MappingValue_hashKey(const Value& key, bool caseSensitive) {
    switch (key.index()) {
    case 0: return 0;
    case 2: {
        int32_t n = static_cast<int32_t>(std::get<int64_t>(key));
        return static_cast<uint32_t>(n >> 4);
    }
    case 3: {
        double d = std::get<double>(key);
        int32_t low;
        std::memcpy(&low, &d, 4);
        return static_cast<uint32_t>(low >> 4);
    }
    case 4: {
        const std::string& s = std::get<SharedString>(key);
        return caseSensitive ? whashstr(s.c_str(), 32) : whashstrc(s.c_str(), 32);
    }
    default: {
        // identity hash on the shared_ptr payload (or the variant index for
        // tag-only types)
        uintptr_t p = 0;
        switch (key.index()) {
        case 5: p = reinterpret_cast<uintptr_t>(std::get<std::shared_ptr<ArrayValue>>(key).get()); break;
        case 6: p = reinterpret_cast<uintptr_t>(std::get<std::shared_ptr<MappingValue>>(key).get()); break;
        case 7: p = reinterpret_cast<uintptr_t>(std::get<std::shared_ptr<ObjectValue>>(key).get()); break;
        case 8: p = reinterpret_cast<uintptr_t>(std::get<std::shared_ptr<ClosureValue>>(key).get()); break;
        case 9: p = reinterpret_cast<uintptr_t>(std::get<std::shared_ptr<LoopCtrl>>(key).get()); break;
        default: p = key.index(); break;
        }
        return static_cast<uint32_t>(p >> 4);
    }
    }
}

namespace {
bool mapKeyEqual(const Value& a, const Value& b, bool caseSensitive) {
    if (a.index() != b.index()) return false;
    switch (a.index()) {
    case 0: case 1: return true;
    case 2: return std::get<int64_t>(a) == std::get<int64_t>(b);
    case 3: return std::get<double>(a) == std::get<double>(b);
    case 4:
        if (caseSensitive) return std::get<SharedString>(a) == std::get<SharedString>(b);
        return striequal(std::get<SharedString>(a), std::get<SharedString>(b));
    case 5: return std::get<std::shared_ptr<ArrayValue>>(a).get() == std::get<std::shared_ptr<ArrayValue>>(b).get();
    case 6: return std::get<std::shared_ptr<MappingValue>>(a).get() == std::get<std::shared_ptr<MappingValue>>(b).get();
    case 7: return std::get<std::shared_ptr<ObjectValue>>(a).get() == std::get<std::shared_ptr<ObjectValue>>(b).get();
    case 8: return std::get<std::shared_ptr<ClosureValue>>(a).get() == std::get<std::shared_ptr<ClosureValue>>(b).get();
    case 9: return std::get<std::shared_ptr<LoopCtrl>>(a).get() == std::get<std::shared_ptr<LoopCtrl>>(b).get();
    default: return false;
    }
}
} // namespace

const Value* MappingValue::find(const Value& key) const {
    uint32_t h = MappingValue_hashKey(key, caseSensitive);
    for (int32_t ni = table[h & tableSize]; ni != -1; ni = nodes[ni].next)
        if (mapKeyEqual(nodes[ni].key, key, caseSensitive)) return &nodes[ni].value;
    return nullptr;
}

Value* MappingValue::find(const Value& key) {
    return const_cast<Value*>(static_cast<const MappingValue*>(this)->find(key));
}

void MappingValue::grow() {
    // vm_growMapping (0x809c894): double the bucket count, reset unfilled to
    // 80% of the OLD count, then split old buckets high -> low, moving nodes
    // whose hash has the new bit set to the new bucket (head insertion).
    uint32_t oldCount = tableSize + 1;
    uint32_t newCount = oldCount * 2;
    if (newCount > 0x400000) {
        unfilled = static_cast<int32_t>(tableSize);   // original error path
        return;
    }
    table.resize(newCount, -1);
    unfilled = static_cast<int32_t>(80 * oldCount / 100);
    tableSize = newCount - 1;
    for (int32_t b = static_cast<int32_t>(oldCount) - 1; b >= 0; --b) {
        int32_t head = table[b];
        if (head == -1) continue;
        int32_t* link = &table[b];
        int32_t* pnew = &table[b + oldCount];
        int32_t ni = head;
        while (ni != -1) {
            Node& nd = nodes[ni];
            if (MappingValue_hashKey(nd.key, caseSensitive) & oldCount) {
                // unlink from the old chain
                int32_t next = nd.next;
                *link = next;
                // head-insert into the new bucket
                if (*pnew == -1) --unfilled;
                nd.next = *pnew;
                *pnew = ni;
                ni = next;
            } else {
                link = &nd.next;
                ni = nd.next;
            }
        }
        if (table[b] == -1) ++unfilled;
    }
}

Value& MappingValue::insert(const Value& key, const Value& value) {
    uint32_t h = MappingValue_hashKey(key, caseSensitive);
    uint32_t bucket = h & tableSize;
    // existing key -> replace the value in place (no reorder)
    for (int32_t ni = table[bucket]; ni != -1; ni = nodes[ni].next) {
        if (mapKeyEqual(nodes[ni].key, key, caseSensitive)) {
            nodes[ni].value = value;
            return nodes[ni].value;
        }
    }
    // vm_safeFindForInsert: inserting into an EMPTY bucket touches unfilled;
    // when it was 1 the table grows first (the new bucket is then recomputed
    // from the hash, which equals bucket | new bit when set).
    if (table[bucket] == -1) {
        bool doGrow = (unfilled == 1);
        --unfilled;
        if (doGrow) {
            grow();
            bucket = h & tableSize;
        }
    }
    // head insertion
    Node nd;
    nd.key = key;
    nd.value = value;
    nd.next = table[bucket];
    nodes.push_back(std::move(nd));
    table[bucket] = static_cast<int32_t>(nodes.size() - 1);
    ++cnt;
    return nodes.back().value;
}

void MappingValue::erase(const Value& key) {
    // vm_mappingDelete (0x809cf84)
    uint32_t h = MappingValue_hashKey(key, caseSensitive);
    uint32_t bucket = h & tableSize;
    int32_t* link = &table[bucket];
    for (int32_t ni = table[bucket]; ni != -1; ) {
        Node& nd = nodes[ni];
        if (mapKeyEqual(nd.key, key, caseSensitive)) {
            bool wasTail = (nd.next == -1);
            *link = nd.next;
            if (wasTail && table[bucket] == -1) ++unfilled;
            --cnt;
            return;
        }
        link = &nd.next;
        ni = nd.next;
    }
}

void MappingValue::setCaseCheck(bool cs, int obsorb) {
    // vm_setCaseCheck (0x809ca60)
    bool old = caseSensitive;
    caseSensitive = cs;
    if (old == cs) return;
    std::vector<int32_t> nt(table.size(), -1);
    // walk OLD buckets low -> high, re-insert each node at the TAIL of its
    // new bucket; with obsorb set, later duplicates are dropped.
    for (size_t b = 0; b < table.size(); ++b) {
        for (int32_t ni = table[b]; ni != -1; ) {
            Node& nd = nodes[ni];
            int32_t next = nd.next;
            uint32_t h = MappingValue_hashKey(nd.key, cs);
            int32_t slot = h & tableSize;
            // walk the new chain
            int32_t* tailLink = &nt[slot];
            int32_t existing = nt[slot];
            bool dup = false;
            while (existing != -1) {
                if (obsorb && mapKeyEqual(nodes[existing].key, nd.key, cs)) { dup = true; break; }
                tailLink = &nodes[existing].next;
                existing = nodes[existing].next;
            }
            if (dup) {
                // absorbed: drop this node
                nd.key = Value{};
                nd.value = Value{};
                --cnt;
            } else {
                nd.next = -1;
                *tailLink = ni;
            }
            ni = next;
        }
        table[b] = -1;
    }
    table = nt;
    // unfilled = max(0, 80% of buckets - non-empty buckets)
    int32_t v = static_cast<int32_t>(80 * (tableSize + 1) / 100);
    for (size_t b = 0; b < table.size() && v > 0; ++b)
        if (table[b] != -1) --v;
    unfilled = v;
}

std::vector<std::pair<Value, Value>> MappingValue::items() const {
    // vm_mappingIndices (0x809e688): buckets from tableSize DOWN TO 0,
    // each chain head first.
    std::vector<std::pair<Value, Value>> out;
    out.reserve(cnt);
    for (int32_t b = static_cast<int32_t>(tableSize); b >= 0; --b)
        for (int32_t ni = table[b]; ni != -1; ni = nodes[ni].next)
            out.emplace_back(nodes[ni].key, nodes[ni].value);
    return out;
}

Value make_int(int64_t v) { return v; }
// Original VM reals are IEEE single precision (probed live on the original:
// 100.0/3.0 prints 33.333332, and 0.1+0.2==0.3 is true because every op
// rounds through float32).  Round every real at creation so stored values and
// the comparisons/prints on top of them match the original exactly.
Value make_float(double v) { return static_cast<double>(static_cast<float>(v)); }
// String values now live in a reference-counted shared slot (SharedString):
// the incoming buffer is moved into one shared block, so every later copy of
// the Value (push/assign/param/local read) is a pointer + refcount bump.
Value make_string(std::string s) { return SharedString(std::move(s)); }
Value make_buffer(std::string data) {
    auto b = std::make_shared<BufferValue>();
    b->data = std::move(data);
    return b;
}
Value make_array() { return std::make_shared<ArrayValue>(); }
Value make_mapping() { return std::make_shared<MappingValue>(); }
Value make_object(std::string programName) {
    auto o = std::make_shared<ObjectValue>();
    o->programName = std::move(programName);
    return o;
}
Value make_object_owned(std::string programName, const struct OriginalObject* owner) {
    auto o = std::make_shared<ObjectValue>();
    o->programName = std::move(programName);
    o->owner = owner;
    return o;
}
Value make_crossvm_ref(std::shared_ptr<CrossVmRef> r) { return std::move(r); }
Value make_closure(std::string name, std::vector<Value> args) {
    auto c = std::make_shared<ClosureValue>();
    c->functionName = std::move(name);
    c->boundArgs = std::move(args);
    return c;
}

std::string value_type_name(const Value& v) {
    switch (v.index()) {
    case 0: return "nil";
    case 1: return "undefined";
    case 2: return "int";
    case 3: return "float";
    case 4: return "string";
    case 5: return "array";
    case 6: return "mapping";
    case 7: return "object";
    case 8: return "function";
    case 9: return "loop";
    case 10: return "buffer";
    default: return "unknown";
    }
}

std::string to_string(const Value& v, bool quote_strings) {
    switch (v.index()) {
    // Original type 0 = "undefined" (missing keys/uninit vars) renders as
    // "(undefined)" (vm_typeValueAt); type 1 = "void" (void fn result)
    // renders as "(void)".  Probed live: %O and (string) of a missing key
    // both give "(undefined)".
    case 0: return "(undefined)";
    case 1: return "(void)";
    case 2: {
        // int -> text without ostringstream (was ~85% of gs import CPU)
        (void)fmtSelfCheckOnce();
        return formatInt64(std::get<int64_t>(v));
    }
    case 3: {
        double d = std::get<double>(v);
        (void)fmtSelfCheckOnce();
        std::string out;
        appendReal64(out, d);
        return out;
    }
    case 4: {
        const std::string& s = std::get<SharedString>(v);
        if (!quote_strings) return s;
        std::string out = "\"";
        for (char c : s) {
            if (c == '\\' || c == '"') out += '\\';
            out += c;
        }
        out += '"';
        return out;
    }
    case 5: {
        auto& arr = *std::get<std::shared_ptr<ArrayValue>>(v);
        std::string out = "({";
        for (size_t i = 0; i < arr.items.size(); ++i) {
            if (i) out += ", ";
            out += to_string(arr.items[i]);
        }
        out += "})";
        return out;
    }
    case 6: {
        auto& mp = *std::get<std::shared_ptr<MappingValue>>(v);
        std::string out = "([";
        auto items = mp.items();
        for (size_t i = 0; i < items.size(); ++i) {
            if (i) out += ", ";
            out += to_string(items[i].first);
            out += ": ";
            out += to_string(items[i].second);
        }
        out += "])";
        return out;
    }
    case 7: {
        return std::string("object(") + std::get<std::shared_ptr<ObjectValue>>(v)->programName + ")";
    }
    case 8: {
        auto c = std::get<std::shared_ptr<ClosureValue>>(v);
        std::string out = "<function " + c->functionName + "(";
        for (size_t i = 0; i < c->boundArgs.size(); ++i) {
            if (i) out += ", ";
            out += to_string(c->boundArgs[i]);
        }
        out += ")>";
        return out;
    }
    case 10: {
        const std::string& s = std::get<std::shared_ptr<BufferValue>>(v)->data;
        static const char* hex = "0123456789ABCDEF";
        std::string out = ":";
        for (unsigned char c : s) { out += hex[c >> 4]; out += hex[c & 0xF]; }
        out += ":";
        return out;
    }
    case 11: return "<crossvm-ref>";
    default: return "<?>";
    }
}

void append_text(std::string& out, const Value& v) {
    if (v.index() == 4) {
        const std::string& s = std::get<SharedString>(v).str();
        out.append(s.data(), s.size());
        return;
    }
    out += to_string(v, false);
}

bool is_truthy(const Value& v) {
    switch (v.index()) {
    case 0: return false;
    case 1: return false;
    case 2: return std::get<int64_t>(v) != 0;
    case 3: return std::get<double>(v) != 0.0;
    case 4: return true;  // strings are ALWAYS truthy in the original driver
                          // (probed on the live original: if("") is truthy;
                          // vm_isZero only zeroes number-0/real-0/undefined).
    // Arrays, mappings and buffers are ALWAYS truthy in the original driver
    // (vm_isZero 0x808cae6: only number-0 / real-0 / null-string / undefined
    // are zero; everything else falls to the default -> not zero).  Treating
    // an empty array as false broke dba's cmd_read_content ("if (!arr)" with
    // a ({}) result silently skipped the 17409 reply, hanging gs's filed
    // do_operate forever).
    case 5: return true;
    case 6: return true;
    case 7: return true;
    case 8: return true;
    case 10: return true;
    default: return false;
    }
}

bool value_equal(const Value& a, const Value& b) {
    if (a.index() != b.index()) {
        // LPC usually converts int/float
        if (a.index() == 2 && b.index() == 3) {
            return std::get<int64_t>(a) == static_cast<int64_t>(std::get<double>(b));
        }
        if (a.index() == 3 && b.index() == 2) {
            return static_cast<int64_t>(std::get<double>(a)) == std::get<int64_t>(b);
        }
        return false;
    }
    switch (a.index()) {
    case 0: return true;
    case 1: return true;
    case 2: return std::get<int64_t>(a) == std::get<int64_t>(b);
    case 3: return std::get<double>(a) == std::get<double>(b);
    case 4: return std::get<SharedString>(a) == std::get<SharedString>(b);
    case 5: {
        auto pa = std::get<std::shared_ptr<ArrayValue>>(a);
        auto pb = std::get<std::shared_ptr<ArrayValue>>(b);
        // Same array instance (aliased variable) - equality is by identity
        // before contents.  LPC code frequently compares a value against
        // itself or against a copy that aliases the same container; without
        // this the full O(n) element walk ran on every such == (op_eq_value
        // was ~93% of gs boot CPU during NPC restore).
        if (pa == pb) return true;
        auto& aa = *pa;
        auto& bb = *pb;
        if (aa.items.size() != bb.items.size()) return false;
        for (size_t i = 0; i < aa.items.size(); ++i)
            if (!value_equal(aa.items[i], bb.items[i])) return false;
        return true;
    }
    case 6: {
        auto pa = std::get<std::shared_ptr<MappingValue>>(a);
        auto pb = std::get<std::shared_ptr<MappingValue>>(b);
        if (pa == pb) return true;
        auto& aa = *pa;
        auto& bb = *pb;
        if (aa.size() != bb.size()) return false;
        // forEachActive walks the live chains with NO allocation; items()
        // built a whole key/value vector copy per comparison.
        bool eq = true;
        aa.forEachActive([&](const Value& k, const Value& v) {
            if (!eq) return;
            const Value* hit = bb.find(k);
            if (!hit || !value_equal(v, *hit)) eq = false;
        });
        return eq;
    }
    case 7: {
        auto pa = std::get<std::shared_ptr<ObjectValue>>(a);
        auto pb = std::get<std::shared_ptr<ObjectValue>>(b);
        if (pa == pb) return true;
        if (pa->programName == pb->programName) return true;
        // Same object addressed differently: LPC code holds the room/daemon by
        // its program path ("/gs/zone/x/x.c") while object()/env_object()
        // return the registered key ("pak:gs/zone/x/x.o").  Only pay for the
        // registry lookup when one side is a plain source path - comparing two
        // different clones must stay a string compare.
        auto plainSourcePath = [](const std::string& s) {
            return s.size() > 2 && s.compare(s.size() - 2, 2, ".c") == 0;
        };
        if (plainSourcePath(pa->programName) || plainSourcePath(pb->programName))
            return canonicalObjectPath(pa->programName) == canonicalObjectPath(pb->programName);
        return false;
    }
    case 8: {
        auto pa = std::get<std::shared_ptr<ClosureValue>>(a);
        auto pb = std::get<std::shared_ptr<ClosureValue>>(b);
        if (pa == pb) return true;
        auto& ca = *pa;
        auto& cb = *pb;
        if (ca.functionName != cb.functionName || ca.boundArgs.size() != cb.boundArgs.size())
            return false;
        for (size_t i = 0; i < ca.boundArgs.size(); ++i)
            if (!value_equal(ca.boundArgs[i], cb.boundArgs[i])) return false;
        return true;
    }
    case 10: {
        auto pa = std::get<std::shared_ptr<BufferValue>>(a);
        auto pb = std::get<std::shared_ptr<BufferValue>>(b);
        if (pa == pb) return true;
        return pa->data == pb->data;
    }
    default: return false;
    }
}

bool value_less(const Value& a, const Value& b) {
    if (a.index() == 2 && b.index() == 3) return std::get<int64_t>(a) < std::get<double>(b);
    if (a.index() == 3 && b.index() == 2) return std::get<double>(a) < std::get<int64_t>(b);
    if (a.index() != b.index()) return a.index() < b.index();
    switch (a.index()) {
    case 2: return std::get<int64_t>(a) < std::get<int64_t>(b);
    case 3: return std::get<double>(a) < std::get<double>(b);
    case 4: return std::get<SharedString>(a) < std::get<SharedString>(b);
    default: return false;
    }
}

} // namespace lpc
