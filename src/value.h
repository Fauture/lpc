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
    std::shared_ptr<ClosureValue>>;

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
