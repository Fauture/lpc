#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace lpc {

struct ArrayValue;
struct MappingValue;
struct ObjectValue;
struct ClosureValue;
struct LoopCtrl;
struct BufferValue;
struct Program;

// ---------------------------------------------------------------------------
// SharedString: the string slot of a Value.
//
// The original driver stores strings as reference-counted shared objects
// (push/assign/copy of a string value only moves a pointer and bumps a
// count).  std::string's copy semantics deep-copied the whole buffer on every
// push/assignment/parameter/local read, which showed up as heavy malloc +
// memcpy + free churn wherever Values holding text were duplicated (mapping
// inserts, array pushes, stack traffic, save/serialize).
//
// SharedString reproduces the shared model: the text lives in a heap Block
// whose refcount is shared by every copy of the SharedString.  Copying a
// SharedString is one atomic increment; destroying one is one decrement.
// Strings are IMMUTABLE - there is no in-place mutation on this type.  Code
// that needs an independent, writable buffer copies explicitly (e.g. through
// the std::string materialization below, .substr(), ...).
//
// The Block stores a real std::string, which gives read sites a zero-copy
// window onto the text: str()/the implicit conversion to const std::string&
// bind straight to the block's string, so existing code that reads a string
// Value through a `const std::string&` keeps working without byte copies.
// Each freshly-created string costs one block allocation (small strings ride
// the std::string SSO inside the block).  The refcount is atomic because
// Values cross scheduler/telnet threads (a producer on one thread may still
// hold a copy while another thread destroys its own).
// ---------------------------------------------------------------------------
class SharedString {
public:
    struct Block {
        std::atomic<int32_t> refs;
        std::string s;
        explicit Block(std::string&& v) : refs(1), s(std::move(v)) {}
    };

    SharedString() noexcept : b_(nullptr) {}
    SharedString(const char* s) : b_(nullptr) {
        if (s && s[0]) b_ = new Block(std::string(s));
    }
    SharedString(const std::string& s) : b_(nullptr) {
        if (!s.empty()) b_ = new Block(std::string(s));
    }
    SharedString(std::string&& s) : b_(nullptr) {
        if (!s.empty()) b_ = new Block(std::move(s));
    }
    SharedString(const SharedString& o) noexcept : b_(o.b_) { retain(); }
    SharedString(SharedString&& o) noexcept : b_(o.b_) { o.b_ = nullptr; }
    SharedString& operator=(const SharedString& o) noexcept {
        if (this != &o) {
            SharedString t(o);
            release();
            b_ = t.b_;
            t.b_ = nullptr;
        }
        return *this;
    }
    SharedString& operator=(SharedString&& o) noexcept {
        if (this != &o) {
            release();
            b_ = o.b_;
            o.b_ = nullptr;
        }
        return *this;
    }
    ~SharedString() { release(); }

    // True when this slot does not hold a text payload.  (An empty string is
    // represented by a null block, exactly like an empty std::string slot.)
    bool empty() const noexcept { return b_ == nullptr || b_->s.empty(); }
    size_t size() const noexcept { return b_ ? b_->s.size() : 0; }
    const char* c_str() const noexcept { return b_ ? b_->s.c_str() : kEmpty.c_str(); }
    const char* data() const noexcept { return b_ ? b_->s.data() : kEmpty.data(); }
    char operator[](size_t i) const noexcept { return str()[i]; }

    // Read-only view of the text.  Binds to the block's std::string with no
    // copy; do not keep the reference past this SharedString's last copy
    // being destroyed (same lifetime rule as today's variant slot).
    const std::string& str() const noexcept { return b_ ? b_->s : kEmpty; }

    // Implicit read interop: lets a SharedString flow into code that expects
    // const std::string& (function parameters, bindings, std::string member
    // calls) without copying bytes.  Copying INTO an owned std::string still
    // happens for std::string by-value targets, which is the explicit-copy
    // boundary for code that wants an independent buffer.
    operator const std::string&() const noexcept { return str(); }

    using const_iterator = std::string::const_iterator;
    const_iterator begin() const noexcept { return str().begin(); }
    const_iterator end() const noexcept { return str().end(); }

private:
    static const std::string kEmpty;

    void retain() noexcept {
        if (b_) b_->refs.fetch_add(1, std::memory_order_relaxed);
    }
    void release() noexcept {
        if (b_ && b_->refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete b_;
    }
    Block* b_;
};

// std::string's comparison operators are templates and cannot deduce through
// SharedString, so provide our own, comparing through the underlying text
// (str() binds the block's std::string with no copy).
inline bool operator==(const SharedString& a, const SharedString& b) noexcept { return a.str() == b.str(); }
inline bool operator!=(const SharedString& a, const SharedString& b) noexcept { return a.str() != b.str(); }
inline bool operator<(const SharedString& a, const SharedString& b) noexcept { return a.str() < b.str(); }
inline bool operator<=(const SharedString& a, const SharedString& b) noexcept { return a.str() <= b.str(); }
inline bool operator>(const SharedString& a, const SharedString& b) noexcept { return a.str() > b.str(); }
inline bool operator>=(const SharedString& a, const SharedString& b) noexcept { return a.str() >= b.str(); }
inline bool operator==(const SharedString& a, const char* b) noexcept { return a.str() == b; }
inline bool operator!=(const SharedString& a, const char* b) noexcept { return a.str() != b; }
inline bool operator<(const SharedString& a, const char* b) noexcept { return a.str() < b; }
inline bool operator<=(const SharedString& a, const char* b) noexcept { return a.str() <= b; }
inline bool operator>(const SharedString& a, const char* b) noexcept { return a.str() > b; }
inline bool operator>=(const SharedString& a, const char* b) noexcept { return a.str() >= b; }
inline bool operator==(const char* a, const SharedString& b) noexcept { return a == b.str(); }
inline bool operator!=(const char* a, const SharedString& b) noexcept { return a != b.str(); }
inline bool operator<(const char* a, const SharedString& b) noexcept { return a < b.str(); }
inline bool operator<=(const char* a, const SharedString& b) noexcept { return a <= b.str(); }
inline bool operator>(const char* a, const SharedString& b) noexcept { return a > b.str(); }
inline bool operator>=(const char* a, const SharedString& b) noexcept { return a >= b.str(); }
inline bool operator==(const SharedString& a, const std::string& b) noexcept { return a.str() == b; }
inline bool operator!=(const SharedString& a, const std::string& b) noexcept { return a.str() != b; }
inline bool operator<(const SharedString& a, const std::string& b) noexcept { return a.str() < b; }
inline bool operator<=(const SharedString& a, const std::string& b) noexcept { return a.str() <= b; }
inline bool operator>(const SharedString& a, const std::string& b) noexcept { return a.str() > b; }
inline bool operator>=(const SharedString& a, const std::string& b) noexcept { return a.str() >= b; }
inline bool operator==(const std::string& a, const SharedString& b) noexcept { return a == b.str(); }
inline bool operator!=(const std::string& a, const SharedString& b) noexcept { return a != b.str(); }
inline bool operator<(const std::string& a, const SharedString& b) noexcept { return a < b.str(); }
inline bool operator<=(const std::string& a, const SharedString& b) noexcept { return a <= b.str(); }
inline bool operator>(const std::string& a, const SharedString& b) noexcept { return a > b.str(); }
inline bool operator>=(const std::string& a, const SharedString& b) noexcept { return a >= b.str(); }

struct UndefinedTag {};

// A cross-VM object-variable reference passed as a call_other argument.
// The original driver's reference parameters (&var) point straight at the
// CALLER's instance variable; a callee's assignment writes through it.  Our
// registry-VM path passes deref'd VALUES, which works for shared_ptr-backed
// mappings (in-place mutation) but not for arrays REASSIGNED with `=`/`+=`
// (a fresh array is built and the caller's variable never changes) - see
// importd.create_data(file, &attrib_index, &attrib_list): attrib_list (a
// mapping) survives, attrib_index (an array) never gets written back, so
// import/npc.o's import() sees an empty attrib_index and raw NPCs (守护天神
// etc.) never receive their npc.list attributes.  Wrapping such arguments in
// a CrossVmRef makes every callee-side read/write forward to the caller's
// object variable (the slot keeps the CrossVmRef, so repeated `+=` all land
// in the caller's variable).
struct CrossVmRef {
    void* vm = nullptr;        // OriginalVm* of the caller (opaque to value.h)
    int objIndex = -1;         // index into the caller's objVars (or -1)
    int slotIndex = -1;        // index into the caller's stack (local slot; or -1)
};

using Value = std::variant<
    std::monostate,          // nil / 0
    UndefinedTag,            // undefined
    int64_t,                 // integer
    double,                  // float
    SharedString,            // string (reference-counted shared text)
    std::shared_ptr<ArrayValue>,
    std::shared_ptr<MappingValue>,
    std::shared_ptr<ObjectValue>,
    std::shared_ptr<ClosureValue>,
    std::shared_ptr<LoopCtrl>,
    std::shared_ptr<BufferValue>,
    std::shared_ptr<CrossVmRef>>;   // cross-VM object-variable reference

struct ArrayValue {
    std::vector<Value> items;
    ArrayValue();
    ~ArrayValue();
};

// Faithful port of the original driver's vm_mapping.c (gameshell):
//   - bucketed hash table; string keys hashed with vm_whashstr (T table),
//     int keys with (number >> 4), everything else by identity,
//   - new nodes are inserted at the HEAD of their bucket chain,
//   - an "unfilled" counter tracks (80% of buckets - empty-bucket inserts)
//     and triggers vm_growMapping when it hits 1,
//   - keys()/values()/foreach iterate buckets from tableSize DOWN TO 0 and
//     each chain head-first, which is what produces the original load order.
struct MappingValue {
    struct Node {
        Value key;
        Value value;
        int32_t next = -1;   // next node in the same bucket chain
    };
    std::vector<Node> nodes;        // node storage (indices are stable)
    std::vector<int32_t> table;     // bucket -> head node index (-1 = empty)
    uint32_t tableSize = 7;         // bucket count - 1 (always 2^k - 1)
    int32_t unfilled = 6;
    uint32_t cnt = 0;
    bool caseSensitive = true;      // attrib & 1

    MappingValue();
    explicit MappingValue(uint32_t sizeHint);
    ~MappingValue();

    size_t size() const { return cnt; }
    bool empty() const { return cnt == 0; }

    // Find the value stored for key (nullptr when absent).
    Value* find(const Value& key);
    const Value* find(const Value& key) const;

    // vm_safeFindForInsert: add or replace; reproduces original bucket order.
    Value& insert(const Value& key, const Value& value);

    // vm_mappingDelete.
    void erase(const Value& key);

    // vm_growMapping (internal).
    void grow();

    // vm_setCaseCheck: toggle case sensitivity and rehash (low->high buckets,
    // tail insertion; obsorb drops later duplicates when set).
    void setCaseCheck(bool cs, int obsorb);

    // (key, value) pairs in vm_mappingIndices order (bucket high->low).
    std::vector<std::pair<Value, Value>> items() const;
};
struct ObjectValue {
    std::string programName;
    // The program that created this object value (set by push_this_object).
    // When programName is "<this>" the value is an object-identity reference
    // captured at creation time; callers resolve it through this owner's
    // registry VM (handler.c's create calls call_other(module, ...) with a
    // captured (["module": object(<this>)])).
    const struct OriginalObject* owner = nullptr;
    std::unordered_map<std::string, Value> variables;
};
struct ClosureValue {
    std::string functionName;
    // Merged function-table index captured at closure-creation time.  A
    // closure created from create_function_value(n) must invoke THAT function
    // (e.g. handler_module's inherited process_command at merged index 32),
    // not whatever the current VM's findFunctionAny(name) happens to resolve
    // later (which would pick the OVERRIDING copy - accountd's own
    // process_command - and loop forever).  -1 = resolve by name only.
    int funcIndex = -1;
    // Exact function captured at closure-creation time.  call_fun_ptr invokes
    // THIS function directly, so a closure made by object A (merged index n)
    // still runs A's function when invoked from object B's VM (B's own index n
    // is a different function).  Null when only funcIndex/name are known.
    const struct OriginalFunction* func = nullptr;
    std::vector<Value> boundArgs;
    // Object that owns this closure (the program whose bytecode created it).
    // When the closure is invoked from another VM (e.g. the master's post_init
    // loop calling a daemon's do_init), call_fun_ptr resolves the function in
    // this owner instead of the current VM.  Null for efun closures.
    const struct OriginalObject* owner = nullptr;
};
// Loop control block created by init_for_loop (original VM value type 20).
struct LoopCtrl {
    int type = 0;                     // 1 / -1 (int), 901 (array), 902 (mapping)
    int slotOfVar = -1;               // stack index of the loop variable
    int64_t current = 0;
    int64_t to = 0;
    int done = 0;
    std::shared_ptr<ArrayValue> arr;
    std::shared_ptr<MappingValue> map;
    size_t iter = 0;                  // array iterator
    size_t mIdx = 0;                  // mapping iterator
    int mElementIndex = 0;
};
// Binary buffer (the original driver's Vm_Buffer_T).  Kept distinct from a
// text string so send_to_client/pkt_pad_buffer serialize it as raw bytes
// rather than as a length-prefixed string.
struct BufferValue {
    std::string data;
};

Value make_int(int64_t v);
Value make_float(double v);
Value make_string(std::string s);
Value make_buffer(std::string data);
Value make_array();
Value make_mapping();
Value make_object(std::string programName);
Value make_object_owned(std::string programName, const struct OriginalObject* owner);
Value make_crossvm_ref(std::shared_ptr<CrossVmRef> r);
Value make_closure(std::string name, std::vector<Value> args = {});

std::string value_type_name(const Value& v);
std::string to_string(const Value& v, bool quote_strings = true);
// Append v's unquoted textual form (exactly what to_string(v,false) yields) to
// out WITHOUT deep-copying a string Value first (its shared text is appended
// straight from the block).  Used by string concatenation points so a + b
// builds the result in one buffer instead of copying each operand twice.
void append_text(std::string& out, const Value& v);
bool is_truthy(const Value& v);
bool value_equal(const Value& a, const Value& b);
bool value_less(const Value& a, const Value& b);

// Live container census (ArrayValue/MappingValue alive + cumulative creates).
// Updated from their ctors/dtors; cheap atomics, used by the LPCVM_MEM2 heap
// diagnostics to attribute memory growth during boot plateaus.
void liveHeapSnapshot(long& arrLive, long& mapLive, long& arrCum, long& mapCum);

} // namespace lpc
