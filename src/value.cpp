#include "value.h"

#include <cmath>
#include <sstream>

namespace lpc {

Value make_int(int64_t v) { return v; }
Value make_float(double v) { return v; }
Value make_string(std::string s) { return std::move(s); }
Value make_array() { return std::make_shared<ArrayValue>(); }
Value make_mapping() { return std::make_shared<MappingValue>(); }
Value make_object(std::string programName) {
    auto o = std::make_shared<ObjectValue>();
    o->programName = std::move(programName);
    return o;
}
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
    default: return "unknown";
    }
}

std::string to_string(const Value& v, bool quote_strings) {
    switch (v.index()) {
    case 0: return "0";
    case 1: return "undefined";
    case 2: {
        std::ostringstream os;
        os << std::get<int64_t>(v);
        return os.str();
    }
    case 3: {
        std::ostringstream os;
        double d = std::get<double>(v);
        if (d == static_cast<int64_t>(d)) {
            os << static_cast<int64_t>(d);
        } else {
            os << d;
        }
        return os.str();
    }
    case 4: {
        const std::string& s = std::get<std::string>(v);
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
        for (size_t i = 0; i < mp.entries.size(); ++i) {
            if (i) out += ", ";
            out += to_string(mp.entries[i].first);
            out += ": ";
            out += to_string(mp.entries[i].second);
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
    default: return "<?>";
    }
}

bool is_truthy(const Value& v) {
    switch (v.index()) {
    case 0: return false;
    case 1: return false;
    case 2: return std::get<int64_t>(v) != 0;
    case 3: return std::get<double>(v) != 0.0;
    case 4: return !std::get<std::string>(v).empty();
    case 5: return !std::get<std::shared_ptr<ArrayValue>>(v)->items.empty();
    case 6: return !std::get<std::shared_ptr<MappingValue>>(v)->entries.empty();
    case 7: return true;
    case 8: return true;
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
    case 4: return std::get<std::string>(a) == std::get<std::string>(b);
    case 5: {
        auto& aa = *std::get<std::shared_ptr<ArrayValue>>(a);
        auto& bb = *std::get<std::shared_ptr<ArrayValue>>(b);
        if (aa.items.size() != bb.items.size()) return false;
        for (size_t i = 0; i < aa.items.size(); ++i)
            if (!value_equal(aa.items[i], bb.items[i])) return false;
        return true;
    }
    case 6: {
        auto& aa = *std::get<std::shared_ptr<MappingValue>>(a);
        auto& bb = *std::get<std::shared_ptr<MappingValue>>(b);
        if (aa.entries.size() != bb.entries.size()) return false;
        for (auto& e : aa.entries) {
            bool found = false;
            for (auto& f : bb.entries) {
                if (value_equal(e.first, f.first) && value_equal(e.second, f.second)) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    case 7: return std::get<std::shared_ptr<ObjectValue>>(a)->programName ==
                    std::get<std::shared_ptr<ObjectValue>>(b)->programName;
    case 8: {
        auto& ca = *std::get<std::shared_ptr<ClosureValue>>(a);
        auto& cb = *std::get<std::shared_ptr<ClosureValue>>(b);
        if (ca.functionName != cb.functionName || ca.boundArgs.size() != cb.boundArgs.size())
            return false;
        for (size_t i = 0; i < ca.boundArgs.size(); ++i)
            if (!value_equal(ca.boundArgs[i], cb.boundArgs[i])) return false;
        return true;
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
    case 4: return std::get<std::string>(a) < std::get<std::string>(b);
    default: return false;
    }
}

} // namespace lpc
