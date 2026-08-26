#include "original_vm.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

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

int32_t readI32(const std::vector<uint8_t>& meta, uint32_t off) {
    if (off + 4 > meta.size()) throw std::runtime_error("original_vm: bad metadata reference");
    return static_cast<int32_t>(meta[off]) |
           (static_cast<int32_t>(meta[off + 1]) << 8) |
           (static_cast<int32_t>(meta[off + 2]) << 16) |
           (static_cast<int32_t>(meta[off + 3]) << 24);
}

class OriginalVm {
public:
    explicit OriginalVm(const OriginalObject& obj) : obj_(obj) {
        globals_.resize(obj.globalVarCount);
        for (auto& g : globals_) g = make_int(0);
        for (const auto& fn : obj.functions) {
            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
        }
    }

    int run() {
        try {
            execute(obj_.globalCode, obj_.globalMeta);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(e.what()) + " at ip=" + std::to_string(lastIp_) +
                                     " op=" + std::to_string(lastOp_));
        }
        return 0;
    }

private:
    const OriginalObject& obj_;
    std::vector<StackItem> stack_;
    std::vector<Value> globals_;
    std::map<int, const OriginalFunction*> funcsByIndex_;
    int lastOp_ = -1;
    size_t lastIp_ = 0;

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

    Value callEfunStub(int /*efunIndex*/, int argCount) {
        // We do not have the original efun name table wired up yet; return 0.
        for (int i = 0; i < argCount; ++i) pop();
        return make_int(0);
    }

    Value callUserFunction(int index, int /*flags*/) {
        auto it = funcsByIndex_.find(index);
        if (it == funcsByIndex_.end()) return make_int(0);
        const OriginalFunction& fn = *it->second;
        // Pop arguments (the VM pushes them before the call).
        std::vector<Value> args;
        for (int i = 0; i < fn.argCount && !stack_.empty(); ++i) args.insert(args.begin(), pop().value);
        // Execute the function body on a separate stack? For global initialization
        // we simply return 0 after discarding the body. This is enough to let the
        // global initializer continue without crashing.
        (void)fn;
        return make_int(0);
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
                StackItem it;
                it.kind = ItemKind::LocalRef;
                it.index = in.a;
                it.value = make_int(0);
                push(it);
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
                } else if (ref.kind == ItemKind::LocalRef) {
                    // local refs are not backed by a real frame in this partial VM
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
            case 8: // call_in_other_fun_ex
                pop();
                push(makeValue(make_int(0)));
                break;
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
                pop();
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

} // namespace

int runOriginalObject(const OriginalObject& obj, std::ostream& out) {
    OriginalVm vm(obj);
    vm.run();
    out << "Original .o VM finished (partial opcode support).\n";
    return 0;
}

} // namespace lpc
