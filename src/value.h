#pragma once
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
struct Program;

struct UndefinedTag {};

using Value = std::variant<
    std::monostate,          // nil / 0
    UndefinedTag,            // undefined
    int64_t,                 // integer
    double,                  // float
    std::string,             // string
    std::shared_ptr<ArrayValue>,
    std::shared_ptr<MappingValue>,
    std::shared_ptr<ObjectValue>,
    std::shared_ptr<ClosureValue>,
    std::shared_ptr<LoopCtrl>>;

struct ArrayValue {
    std::vector<Value> items;
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

    MappingValue() { table.assign(8, -1); }
    explicit MappingValue(uint32_t sizeHint);

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

Value make_int(int64_t v);
Value make_float(double v);
Value make_string(std::string s);
Value make_array();
Value make_mapping();
Value make_object(std::string programName);
Value make_object_owned(std::string programName, const struct OriginalObject* owner);
Value make_closure(std::string name, std::vector<Value> args = {});

std::string value_type_name(const Value& v);
std::string to_string(const Value& v, bool quote_strings = true);
bool is_truthy(const Value& v);
bool value_equal(const Value& a, const Value& b);
bool value_less(const Value& a, const Value& b);

} // namespace lpc
