#include "original_vm.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "builtins.h"
#include "efun_table.h"
#include "value.h"

namespace lpc {

namespace {

enum class ItemKind { Value, GlobalRef, LocalRef };

struct StackItem {
    ItemKind kind = ItemKind::Value;
    Value value;
    int index = 0;
};

StackItem makeValue(Value v) {
    StackItem it;
    it.kind = ItemKind::Value;
    it.value = std::move(v);
    return it;
}

Value tryCallOtherObject(const std::string& searchRoot, const std::string& objectPath,
                         const std::string& funName, const std::vector<Value>& args);

int32_t readI32(const std::vector<uint8_t>& meta, uint32_t off) {
    if (off + 4 > meta.size()) throw std::runtime_error("original_vm: bad metadata reference");
    return static_cast<int32_t>(meta[off]) |
           (static_cast<int32_t>(meta[off + 1]) << 8) |
           (static_cast<int32_t>(meta[off + 2]) << 16) |
           (static_cast<int32_t>(meta[off + 3]) << 24);
}

class OriginalVm {
public:
    explicit OriginalVm(const OriginalObject& obj, std::string searchRoot = "") : obj_(obj), searchRoot_(std::move(searchRoot)) {
        globals_.resize(obj.globalVarCount);
        for (auto& g : globals_) g = make_int(0);
        for (const auto& fn : obj.functions) {
            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
        }
    }

    Value callFunctionByName(const std::string& name, const std::vector<Value>& args) {
        for (const auto& fn : obj_.functions) {
            if (fn.name == name && fn.index >= 0) {
                auto it = funcsByIndex_.find(fn.index);
                if (it != funcsByIndex_.end()) return executeFunction(*it->second, args);
            }
        }
        return make_int(0);
    }

    int run() {
        try {
            execute(obj_.globalCode, obj_.globalMeta);
            // LPC object loading normally calls create() after the global initializer.
            callCreate();
            // Simple main loop: run heart_beat ticks on a timer until the tick limit.
            runMainLoop(10, std::chrono::milliseconds(50));
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_));
        }
        return 0;
    }

    void callCreate() {
        for (const auto& fn : obj_.functions) {
            if (fn.name == "create" && fn.index >= 0) {
                auto it = funcsByIndex_.find(fn.index);
                if (it != funcsByIndex_.end()) {
                    std::vector<Value> args;
                    for (int i = 0; i < fn.argCount; ++i) args.push_back(make_int(0));
                    executeFunction(*it->second, args);
                }
                break;
            }
        }
    }

    void callHeartBeat(int ticks) {
        const OriginalFunction* hb = nullptr;
        for (const auto& fn : obj_.functions) {
            if (fn.name == "heart_beat" && fn.index >= 0) {
                auto it = funcsByIndex_.find(fn.index);
                if (it != funcsByIndex_.end()) { hb = it->second; break; }
            }
        }
        if (!hb) return;
        std::vector<Value> args;
        for (int i = 0; i < hb->argCount; ++i) args.push_back(make_int(0));
        for (int i = 0; i < ticks; ++i) {
            executeFunction(*hb, args);
        }
    }

private:
    const OriginalObject& obj_;
    std::string searchRoot_;
    std::vector<StackItem> stack_;
    std::vector<Value> globals_;
    std::vector<Value> locals_;
    std::map<int, const OriginalFunction*> funcsByIndex_;
    int lastOp_ = -1;
    size_t lastIp_ = 0;
    Value returnValue_ = make_int(0);

    void push(const StackItem& it) { stack_.push_back(it); }
    StackItem pop() {
        if (stack_.empty()) throw std::runtime_error("original_vm: stack underflow");
        StackItem it = std::move(stack_.back());
        stack_.pop_back();
        return it;
    }

    int32_t metaOperand(const std::vector<uint8_t>& meta, uint16_t a, uint8_t b) const {
        return readI32(meta, static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 16));
    }

    Value callEfunStub(int efunIndex, int argCount) {
        std::vector<Value> args;
        for (int i = 0; i < argCount && !stack_.empty(); ++i) args.insert(args.begin(), pop().value);
        const auto& names = efunNameTable();
        if (efunIndex >= 0 && static_cast<size_t>(efunIndex) < names.size() && !names[efunIndex].empty()) {
            return call_builtin(nullptr, names[efunIndex], args);
        }
        return make_int(0);
    }

    Value callUserFunction(int index, int /*flags*/) {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return make_int(0);
        const OriginalFunction& fn = *it->second;
        // Pop arguments (the VM pushes them before the call).
        std::vector<Value> args;
        for (int i = 0; i < fn.argCount && !stack_.empty(); ++i) args.insert(args.begin(), pop().value);
        return executeFunction(fn, args);
    }

    Value executeFunction(const OriginalFunction& fn, const std::vector<Value>& args) {
        std::vector<Value> savedLocals = std::move(locals_);
        Value savedReturn = returnValue_;
        locals_.clear();
        locals_.reserve(static_cast<size_t>(fn.argCount + fn.localCount));
        for (const auto& a : args) locals_.push_back(a);
        for (int i = 0; i < fn.localCount; ++i) locals_.push_back(make_int(0));
        returnValue_ = make_int(0);
        size_t savedStackSize = stack_.size();
        try {
            execute(fn.code, fn.metaData);
        } catch (...) {
            locals_ = std::move(savedLocals);
            returnValue_ = savedReturn;
            throw;
        }
        Value result = returnValue_;
        if (stack_.size() > savedStackSize) stack_.resize(savedStackSize);
        locals_ = std::move(savedLocals);
        returnValue_ = savedReturn;
        return result;
    }

    void execute(const std::vector<OriginalInsn>& code, const std::vector<uint8_t>& meta) {
        size_t ip = 0;
        while (ip < code.size()) {
            const OriginalInsn& in = code[ip++];
            lastOp_ = in.op;
            lastIp_ = ip - 1;
            switch (in.op) {
            case 46: // null_action
                break;
            case 29: // finish_action
                return;
            case 72: // return_from_fake_fun
            case 73: // return_from_fun
                if (!stack_.empty()) returnValue_ = pop().value;
                else returnValue_ = make_int(0);
                return;
            case 59: // push_number(S)
                push(makeValue(make_int(static_cast<int16_t>(in.a))));
                break;
            case 60: // push_number(L)
                push(makeValue(make_int(metaOperand(meta, in.a, in.b))));
                break;
            case 64: // push_shared_string
                if (in.a < obj_.strings.size()) push(makeValue(make_string(obj_.strings[in.a])));
                else push(makeValue(make_int(0)));
                break;
            case 66: // push_undefined
                push(makeValue(UndefinedTag{}));
                break;
            case 65: // push_this_object
                push(makeValue(make_object("<this>")));
                break;
            case 53: // push_global_var
                if (in.a < globals_.size()) push(makeValue(globals_[in.a]));
                else push(makeValue(make_int(0)));
                break;
            case 54: { // push_global_vptr
                StackItem it;
                it.kind = ItemKind::GlobalRef;
                it.index = in.a;
                push(it);
                break;
            }
            case 55: { // push_local_value
                Value v = in.a < locals_.size() ? locals_[in.a] : make_int(0);
                push(makeValue(v));
                break;
            }
            case 57: { // push_local_vptr
                StackItem it;
                it.kind = ItemKind::LocalRef;
                it.index = in.a;
                push(it);
                break;
            }
            case 3: { // assign_value: pop ref, keep value on stack
                StackItem ref = pop();
                if (ref.kind == ItemKind::GlobalRef && ref.index >= 0 && static_cast<size_t>(ref.index) < globals_.size()) {
                    if (!stack_.empty()) globals_[ref.index] = stack_.back().value;
                } else if (ref.kind == ItemKind::LocalRef && ref.index >= 0 && static_cast<size_t>(ref.index) < locals_.size()) {
                    if (!stack_.empty()) locals_[ref.index] = stack_.back().value;
                }
                break;
            }
            case 4: { // assign_value_reverse: pop value, keep ref
                StackItem val = pop();
                if (!stack_.empty() && stack_.back().kind == ItemKind::GlobalRef) {
                    StackItem& ref = stack_.back();
                    if (ref.index >= 0 && static_cast<size_t>(ref.index) < globals_.size()) globals_[ref.index] = val.value;
                }
                break;
            }
            case 51: // pop_stack
                pop();
                break;
            case 22: { // create_array
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.insert(items.begin(), pop().value);
                auto arr = std::make_shared<ArrayValue>();
                arr->items = std::move(items);
                push(makeValue(arr));
                break;
            }
            case 24: { // create_mapping
                uint32_t count = in.a;
                std::vector<Value> items;
                for (uint32_t i = 0; i < count && !stack_.empty(); ++i) items.insert(items.begin(), pop().value);
                auto mp = std::make_shared<MappingValue>();
                for (size_t i = 0; i + 1 < items.size(); i += 2) {
                    mp->entries.emplace_back(items[i], items[i + 1]);
                }
                push(makeValue(mp));
                break;
            }
            case 1: { // add
                Value b = pop().value;
                Value a = pop().value;
                if (a.index() == 4 || b.index() == 4) push(makeValue(make_string(to_string(a, false) + to_string(b, false))));
                else if (a.index() == 3 || b.index() == 3) push(makeValue(make_float(to_float(a) + to_float(b))));
                else push(makeValue(make_int(to_int(a) + to_int(b))));
                break;
            }
            case 81: { // sub_value
                Value b = pop().value;
                Value a = pop().value;
                push(makeValue(make_int(to_int(a) - to_int(b))));
                break;
            }
            case 25: { // div_value
                Value b = pop().value;
                Value a = pop().value;
                int64_t bb = to_int(b);
                push(makeValue(make_int(bb == 0 ? 0 : to_int(a) / bb)));
                break;
            }
            case 41: { // mul_value
                Value b = pop().value;
                Value a = pop().value;
                push(makeValue(make_int(to_int(a) * to_int(b))));
                break;
            }
            case 28: // eq_value
                push(makeValue(make_int(value_equal(pop().value, pop().value) ? 1 : 0)));
                break;
            case 42: // ne_value
                push(makeValue(make_int(value_equal(pop().value, pop().value) ? 0 : 1)));
                break;
            case 45: // not_value
                push(makeValue(make_int(is_truthy(pop().value) ? 0 : 1)));
                break;
            case 6: { // call_in_efun
                int efun = in.a;
                int argc = in.b;
                Value r = callEfunStub(efun, argc);
                push(makeValue(r));
                break;
            }
            case 7: { // call_in_user_fun
                int funIndex = in.a;
                int flags = in.b;
                Value r = callUserFunction(funIndex, flags);
                push(makeValue(r));
                break;
            }
            case 5: // call_fun_ptr
                pop();
                push(makeValue(make_int(0)));
                break;
            case 8: { // call_in_other_fun_ex
                // Stack: [args...] [function name] [object path]
                Value pathVal = pop().value;
                Value funVal = pop().value;
                int argn = in.b;
                std::vector<Value> args;
                for (int i = 0; i < argn && !stack_.empty(); ++i) args.insert(args.begin(), pop().value);
                std::string funName = funVal.index() == 4 ? std::get<std::string>(funVal) : "";
                Value result = make_int(0);
                bool found = false;
                for (const auto& fn : obj_.functions) {
                    if (fn.name == funName && fn.index >= 0) {
                        auto fit = funcsByIndex_.find(fn.index);
                        if (fit != funcsByIndex_.end()) {
                            result = executeFunction(*fit->second, args);
                            found = true;
                        }
                        break;
                    }
                }
                if (!found && pathVal.index() == 4) {
                    result = tryCallOtherObject(searchRoot_, std::get<std::string>(pathVal), funName, args);
                }
                push(makeValue(result));
                break;
            }
            case 33: { // if
                Value cond = pop().value;
                if (!is_truthy(cond)) ip += in.a;
                break;
            }
            case 31: // goto
                ip += static_cast<int16_t>(in.a);
                break;
            case 67: // query_member
            case 68: // query_member_ptr
            case 69: // query_range
            case 70: // query_range_ptr
                pop();
                push(makeValue(make_int(0)));
                break;
            case 76: // set_global_var
                pop();
                if (in.a < globals_.size()) globals_[in.a] = pop().value;
                break;
            case 77: // set_local_var
                if (!stack_.empty()) {
                    Value v = pop().value;
                    if (in.a < locals_.size()) locals_[in.a] = v;
                }
                break;
            case 34: // inc_value
                if (!stack_.empty()) {
                    Value v = pop().value;
                    push(makeValue(make_int(to_int(v) + 1)));
                }
                break;
            case 15: // check_args_num
            case 27: // enter_function
            case 50: // pop_n_stack
            case 52: // push_arg_num
            case 71: // resort_stack
            case 79: // sizeof
            case 80: // step_jmp_for_loop
            case 82: // to_reserve_stack_value
            case 83: // to_unreserve_stack_value
                break;
            default:
                // Unknown opcodes are ignored for now so large global initializers can
                // still make progress. This is intentionally a partial implementation.
                break;
            }
        }
    }

    static int64_t to_int(const Value& v) {
        switch (v.index()) {
        case 0: return 0;
        case 1: return 0;
        case 2: return std::get<int64_t>(v);
        case 3: return static_cast<int64_t>(std::get<double>(v));
        case 4: return std::get<std::string>(v).empty() ? 0 : 1;
        default: return 0;
        }
    }
    static double to_float(const Value& v) {
        switch (v.index()) {
        case 0: return 0.0;
        case 1: return 0.0;
        case 2: return static_cast<double>(std::get<int64_t>(v));
        case 3: return std::get<double>(v);
        case 4: return std::stod(std::get<std::string>(v));
        default: return 0.0;
        }
    }
};

struct ObjectRegistry {
    std::unordered_map<std::string, std::shared_ptr<OriginalObject>> objects;
    std::unordered_map<std::string, std::shared_ptr<OriginalVm>> vms;
};

ObjectRegistry& objectRegistry() {
    static ObjectRegistry reg;
    return reg;
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
    // Some pak roots omit the leading "aaa/" directory.
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
            // try next candidate
        }
    }
    return make_int(0);
}

} // namespace

int runOriginalObject(const OriginalObject& obj, std::ostream& out, const std::string& searchRoot) {
    OriginalVm vm(obj, searchRoot);
    vm.run();
    out << "Original .o VM finished (partial opcode support).\n";
    return 0;
}

} // namespace lpc
