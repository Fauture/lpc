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
struct MappingValue {
    std::vector<std::pair<Value, Value>> entries;
};
struct ObjectValue {
    std::string programName;
    std::unordered_map<std::string, Value> variables;
};
struct ClosureValue {
    std::string functionName;
    std::vector<Value> boundArgs;
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
Value make_closure(std::string name, std::vector<Value> args = {});

std::string value_type_name(const Value& v);
std::string to_string(const Value& v, bool quote_strings = true);
bool is_truthy(const Value& v);
bool value_equal(const Value& a, const Value& b);
bool value_less(const Value& a, const Value& b);

} // namespace lpc
