#include "interpreter.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "builtins.h"
#include "original_vm.h"

namespace lpc {

std::string Interpreter::lastError;

namespace {

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& msg) : std::runtime_error(msg) {}
};

class ThreadYield : public std::exception {
public:
    const char* what() const noexcept override { return "thread yield"; }
};

int64_t as_int(const Value& v) {
    switch (v.index()) {
    case 0: return 0;
    case 1: return 0;
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: return std::get<std::string>(v).empty() ? 0 : 1;
    default: return 0;
    }
}

double as_float(const Value& v) {
    switch (v.index()) {
    case 0: return 0.0;
    case 1: return 0.0;
    case 2: return static_cast<double>(std::get<int64_t>(v));
    case 3: return std::get<double>(v);
    case 4: return std::stod(std::get<std::string>(v));
    default: return 0.0;
    }
}

std::string as_string(const Value& v) {
    return to_string(v, false);
}

Value make_value_from_string(const std::string& type, const Value& v) {
    if (type == "int") return make_int(as_int(v));
    if (type == "float") return make_float(as_float(v));
    if (type == "string") return make_string(as_string(v));
    return v;
}

} // namespace

Interpreter::Interpreter(const Program& program) : program_(program) {
    for (const auto& fn : program_.functions) {
        functions_[fn.name] = &fn;
    }
    for (const auto& gv : program_.globalVars) {
        if (gv.first == "COMPILE_ACTION") globalEnv_.vars[gv.first] = make_string("Load");
        else globalEnv_.vars[gv.first] = make_int(0);
    }
    thisObject_ = make_object("<anonymous>");
}

void Interpreter::runGlobal() {
    Environment env;
    env.parent = &globalEnv_;
    try {
        execBlock(program_.globalBody, env);
    } catch (const RuntimeError& e) {
        lastError = e.what();
        throw;
    }
}

Value Interpreter::callBuiltin(const std::string& name, const std::vector<Value>& args) {
    if (name == "create_thread") {
        if (args.size() >= 2 && args[1].index() == 8) {
            auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
            threads_.emplace_back(cl->functionName, cl->boundArgs);
            return make_int(static_cast<int64_t>(threads_.size()));
        }
        return make_int(0);
    }
    if (name == "sleep" && inThread_) {
        throw ThreadYield();
    }
    return call_builtin(this, name, args);
}

void Interpreter::runThreads(int maxTicks) {
    for (int tick = 0; tick < maxTicks; ++tick) {
        bool any = false;
        for (auto& th : threads_) {
            inThread_ = true;
            stepCounter_ = 0;
            try {
                callFunction(th.first, th.second);
            } catch (const ThreadYield&) {
                any = true;
            } catch (...) {
                // ignore thread errors
            }
            inThread_ = false;
        }
        if (!any) break;
    }
}

Value Interpreter::callFunction(const std::string& name, const std::vector<Value>& args) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return callBuiltin(name, args);
    }
    const FunctionDef* fn = it->second;
    if (fn->isForward) return make_int(0);
    Environment env;
    env.parent = &globalEnv_;
    for (size_t i = 0; i < fn->params.size(); ++i) {
        Value v = i < args.size() ? args[i] : (fn->params[i].defaultValue ? eval(fn->params[i].defaultValue, env) : make_int(0));
        env.vars[fn->params[i].name] = v;
    }
    Flow oldFlow = flow_;
    Value oldReturn = returnValue_;
    auto oldArgs = std::move(currentArgs_);
    currentArgs_ = args;
    flow_ = Flow::Normal;
    returnValue_ = make_int(0);
    try {
        execBlock(fn->body, env);
    } catch (...) {
        flow_ = oldFlow;
        returnValue_ = oldReturn;
        currentArgs_ = std::move(oldArgs);
        throw;
    }
    Value result = returnValue_;
    flow_ = oldFlow;
    returnValue_ = oldReturn;
    currentArgs_ = std::move(oldArgs);
    return result;
}

Flow Interpreter::execBlock(const std::vector<StmtPtr>& stmts, Environment& env) {
    for (const auto& s : stmts) {
        Flow f = exec(s, env);
        if (f != Flow::Normal) return f;
    }
    return Flow::Normal;
}

Flow Interpreter::exec(StmtPtr stmt, Environment& env) {
    switch (stmt->kind) {
    case Stmt::Kind::Expr:
        eval(stmt->expr, env);
        return Flow::Normal;
    case Stmt::Kind::Block: {
        Environment child;
        child.parent = &env;
        return execBlock(stmt->block, child);
    }
    case Stmt::Kind::VarDecl: {
        for (auto& v : stmt->vars) {
            Value val = v.second ? eval(v.second, env) : make_int(0);
            env.vars[v.first] = val;
        }
        return Flow::Normal;
    }
    case Stmt::Kind::If: {
        if (is_truthy(eval(stmt->cond, env))) return exec(stmt->thenStmt, env);
        if (stmt->elseStmt) return exec(stmt->elseStmt, env);
        return Flow::Normal;
    }
    case Stmt::Kind::DoWhile: {
        do {
            if (++stepCounter_ > 50000) throw ThreadYield();
            Flow f = exec(stmt->thenStmt, env);
            if (f == Flow::Return) return f;
            if (f == Flow::Break) break;
        } while (is_truthy(eval(stmt->cond, env)));
        return Flow::Normal;
    }
    case Stmt::Kind::While: {
        while (is_truthy(eval(stmt->cond, env))) {
            if (++stepCounter_ > 50000) throw ThreadYield();
            Flow f = exec(stmt->thenStmt, env);
            if (f == Flow::Return) return f;
            if (f == Flow::Break) break;
        }
        return Flow::Normal;
    }
    case Stmt::Kind::For: {
        Environment loopEnv;
        loopEnv.parent = &env;
        if (stmt->initStmt) exec(stmt->initStmt, loopEnv);
        while (!stmt->cond || is_truthy(eval(stmt->cond, loopEnv))) {
            if (++stepCounter_ > 50000) throw ThreadYield();
            Flow f = exec(stmt->thenStmt, loopEnv);
            if (f == Flow::Return) return f;
            if (f == Flow::Break) break;
            if (stmt->incStmt) exec(stmt->incStmt, loopEnv);
        }
        return Flow::Normal;
    }
    case Stmt::Kind::Loop: {
        Value container = eval(stmt->cond, env);
        if (container.index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(container);
            for (auto& item : arr->items) {
                if (++stepCounter_ > 50000) throw ThreadYield();
                env.vars[stmt->loopVar] = item;
                Flow f = exec(stmt->thenStmt, env);
                if (f == Flow::Return) return f;
                if (f == Flow::Break) break;
            }
        } else if (container.index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(container);
            for (auto& kv : mp->items()) {
                if (++stepCounter_ > 50000) throw ThreadYield();
                env.vars[stmt->loopVar] = kv.first;
                Flow f = exec(stmt->thenStmt, env);
                if (f == Flow::Return) return f;
                if (f == Flow::Break) break;
            }
        }
        return Flow::Normal;
    }
    case Stmt::Kind::Switch: {
        Value val = eval(stmt->cond, env);
        bool matched = false;
        for (auto& c : stmt->cases) {
            if (c.first && value_equal(val, eval(c.first, env))) {
                matched = true;
                Flow f = execBlock(c.second, env);
                if (f == Flow::Return) return f;
                if (f == Flow::Break) return Flow::Normal;
                // fall through to subsequent cases in LPC style
            } else if (!c.first && !matched) {
                // null case used as implicit marker (not used by parser now)
            }
        }
        if (!stmt->defaultCase.empty()) {
            Flow f = execBlock(stmt->defaultCase, env);
            if (f == Flow::Return) return f;
        }
        return Flow::Normal;
    }
    case Stmt::Kind::TryCatch:
        try {
            Flow f = exec(stmt->thenStmt, env);
            return f;
        } catch (const std::exception&) {
            if (stmt->elseStmt) return exec(stmt->elseStmt, env);
            throw;
        }
    case Stmt::Kind::Return:
        returnValue_ = stmt->hasValue ? eval(stmt->expr, env) : make_int(0);
        return Flow::Return;
    case Stmt::Kind::Break:
        return Flow::Break;
    case Stmt::Kind::Continue:
        return Flow::Continue;
    }
    return Flow::Normal;
}

Value Interpreter::eval(ExprPtr expr, Environment& env) {
    switch (expr->kind) {
    case Expr::Kind::Int:
        return make_int(expr->intValue);
    case Expr::Kind::Float:
        return make_float(expr->floatValue);
    case Expr::Kind::String:
        return make_string(expr->text);
    case Expr::Kind::Ident:
        return evalIdent(expr->text, env);
    case Expr::Kind::Array: {
        auto arr = std::make_shared<ArrayValue>();
        for (auto& e : expr->args) arr->items.push_back(eval(e, env));
        return arr;
    }
    case Expr::Kind::Mapping: {
        auto mp = std::make_shared<MappingValue>(static_cast<uint32_t>(expr->mapEntries.size()));
        for (auto& kv : expr->mapEntries) mp->insert(eval(kv.first, env), eval(kv.second, env));
        return mp;
    }
    case Expr::Kind::Unary:
        return evalUnary(expr->op, eval(expr->rhs, env), expr->line);
    case Expr::Kind::Binary:
        if (expr->op == "?:") {
            return is_truthy(eval(expr->lhs, env)) ? eval(expr->rhs, env) : eval(expr->cond, env);
        }
        if (expr->op == "&&") {
            // Short-circuit: the RHS must NOT be evaluated when the LHS is
            // falsy (original compiler emits if+goto; side effects in the RHS
            // must not run).  a && b -> 1 when both truthy else 0.
            if (!is_truthy(eval(expr->lhs, env))) return make_int(0);
            return make_int(is_truthy(eval(expr->rhs, env)) ? 1 : 0);
        }
        if (expr->op == "||") {
            // Short-circuit: the RHS must NOT be evaluated when the LHS is
            // truthy.  a || b -> 1 when either truthy else 0.
            if (is_truthy(eval(expr->lhs, env))) return make_int(1);
            return make_int(is_truthy(eval(expr->rhs, env)) ? 1 : 0);
        }
        return evalBinary(expr->op, eval(expr->lhs, env), eval(expr->rhs, env), expr->line);
    case Expr::Kind::Assign: {
        Value r = eval(expr->rhs, env);
        Value l = eval(expr->lhs, env);
        if (expr->op == "=") {
            if (expr->lhs->kind == Expr::Kind::Ident) {
                assignIdent(expr->lhs->text, r, env);
            } else if (expr->lhs->kind == Expr::Kind::Index) {
                assignIndex(expr->lhs->lhs, expr->lhs->rhs, r, env);
            } else {
                throw RuntimeError("bad assignment target");
            }
            return r;
        }
        Value result = make_int(0);
        if (expr->op == "+=") result = evalBinary("+", l, r, expr->line);
        else if (expr->op == "-=") result = evalBinary("-", l, r, expr->line);
        else if (expr->op == "*=") result = evalBinary("*", l, r, expr->line);
        else if (expr->op == "/=") result = evalBinary("/", l, r, expr->line);
        else if (expr->op == "^=") result = evalBinary("^", l, r, expr->line);
        else if (expr->op == "&=") result = evalBinary("&", l, r, expr->line);
        else if (expr->op == "|=") result = evalBinary("|", l, r, expr->line);
        else if (expr->op == "<<=") result = evalBinary("<<", l, r, expr->line);
        else if (expr->op == ">>=") result = evalBinary(">>", l, r, expr->line);
        if (expr->lhs->kind == Expr::Kind::Ident) assignIdent(expr->lhs->text, result, env);
        else if (expr->lhs->kind == Expr::Kind::Index) assignIndex(expr->lhs->lhs, expr->lhs->rhs, result, env);
        return result;
    }
    case Expr::Kind::Call:
        return evalCall(expr->target, expr->args, env);
    case Expr::Kind::Index:
        return evalIndex(expr->lhs, expr->rhs, env);
    case Expr::Kind::Member: {
        Value obj = eval(expr->lhs, env);
        if (obj.index() == 7) {
            auto& vars = std::get<std::shared_ptr<ObjectValue>>(obj)->variables;
            auto it = vars.find(expr->text);
            if (it != vars.end()) return it->second;
            return make_int(0);
        }
        return make_int(0);
    }
    case Expr::Kind::CallOther:
        return evalCallOther(expr->lhs, expr->text, expr->args, env);
    case Expr::Kind::Lambda:
        return evalLambda(*expr, env);
    case Expr::Kind::ThisObject:
        return thisObject_;
    case Expr::Kind::New:
        return makeDefaultObject(expr->text);
    case Expr::Kind::Cast:
        return make_value_from_string(expr->text, eval(expr->rhs, env));
    case Expr::Kind::Catch:
        try {
            return eval(expr->rhs, env);
        } catch (const std::exception& e) {
            return make_string(e.what());
        }
    }
    return make_int(0);
}

Value Interpreter::evalIdent(const std::string& name, Environment& env) {
    // Original: "undefined"/"nil" are NOT keywords - an undeclared identifier
    // is reported as "Undefined variable(x), treat as undefined value" which
    // is type 0 "undefined" (save "U", is_undefined=1).  Probed live:
    //   save_string(undefined) = "U", save_string(nil) = "U".
    if (name == "undefined") return Value{};
    if (name == "nil") return Value{};
    if (name == "true") return make_int(1);
    if (name == "false") return make_int(0);
    if (name == "this_object") {
        // Handled as a function call in evalCall; bare ident returns the object.
        return thisObject_;
    }
    if (name == "args") {
        auto arr = std::make_shared<ArrayValue>();
        arr->items = currentArgs_;
        return arr;
    }
    for (Environment* e = &env; e; e = e->parent) {
        auto it = e->vars.find(name);
        if (it != e->vars.end()) return it->second;
    }
    auto it = globalEnv_.vars.find(name);
    if (it != globalEnv_.vars.end()) return it->second;
    return make_int(0);
}

void Interpreter::assignIdent(const std::string& name, const Value& v, Environment& env) {
    for (Environment* e = &env; e; e = e->parent) {
        auto it = e->vars.find(name);
        if (it != e->vars.end()) {
            it->second = v;
            return;
        }
    }
    globalEnv_.vars[name] = v;
}

Value Interpreter::evalBinary(const std::string& op, const Value& a, const Value& b, int line) {
    if (op == "&&") return make_int(is_truthy(a) && is_truthy(b) ? 1 : 0);
    if (op == "||") return make_int(is_truthy(a) || is_truthy(b) ? 1 : 0);
    if (op == "+") {
        if (a.index() == 5 && b.index() == 5) {
            // LPC array concatenation: a + b
            auto arr = std::make_shared<ArrayValue>();
            arr->items = std::get<std::shared_ptr<ArrayValue>>(a)->items;
            const auto& bv = std::get<std::shared_ptr<ArrayValue>>(b)->items;
            arr->items.insert(arr->items.end(), bv.begin(), bv.end());
            return arr;
        }
        if (a.index() == 4 || b.index() == 4) return make_string(as_string(a) + as_string(b));
        // original VM: mixed int/real keeps the LEFT operand's type
        if (a.index() == 3) return make_float(as_float(a) + as_float(b));
        if (b.index() == 3) return make_int(static_cast<int64_t>(as_float(a) + as_float(b)));
        return make_int(as_int(a) + as_int(b));
    }
    if (op == "-") {
        if (a.index() == 3) return make_float(as_float(a) - as_float(b));
        if (b.index() == 3) return make_int(static_cast<int64_t>(as_float(a) - as_float(b)));
        return make_int(as_int(a) - as_int(b));
    }
    if (op == "*") {
        if (a.index() == 3) return make_float(as_float(a) * as_float(b));
        if (b.index() == 3) return make_int(static_cast<int64_t>(as_float(a) * as_float(b)));
        return make_int(as_int(a) * as_int(b));
    }
    if (op == "/") {
        if (a.index() == 3) return make_float(as_float(a) / as_float(b));
        if (b.index() == 3) return make_int(static_cast<int64_t>(as_float(a) / as_float(b)));
        return make_int(as_int(a) / as_int(b));
    }
    if (op == "%") return make_int(as_int(a) % as_int(b));
    if (op == "==") return make_int(value_equal(a, b) ? 1 : 0);
    if (op == "!=") return make_int(value_equal(a, b) ? 0 : 1);
    if (op == "<") return make_int(value_less(a, b) ? 1 : 0);
    if (op == ">") return make_int(value_less(b, a) ? 1 : 0);
    if (op == "<=") return make_int((value_less(a, b) || value_equal(a, b)) ? 1 : 0);
    if (op == ">=") return make_int((value_less(b, a) || value_equal(a, b)) ? 1 : 0);
    if (op == "<<") return make_int(as_int(a) << as_int(b));
    if (op == ">>") return make_int(as_int(a) >> as_int(b));
    if (op == "&") return make_int(as_int(a) & as_int(b));
    if (op == "|") return make_int(as_int(a) | as_int(b));
    if (op == "^") return make_int(as_int(a) ^ as_int(b));
    throw RuntimeError("unsupported binary operator " + op + " at line " + std::to_string(line));
}

Value Interpreter::evalUnary(const std::string& op, const Value& v, int line) {
    if (op == "!") return make_int(is_truthy(v) ? 0 : 1);
    if (op == "-") return v.index() == 3 ? make_float(-as_float(v)) : make_int(-as_int(v));
    if (op == "~") return make_int(~as_int(v));
    if (op == "*" || op == "&") return v; // function-pointer dereference / reference marker
    throw RuntimeError("unsupported unary operator " + op + " at line " + std::to_string(line));
}

Value Interpreter::evalIndex(ExprPtr lhs, ExprPtr rhs, Environment& env) {
    Value c = eval(lhs, env);
    auto resolveIndex = [&](ExprPtr e, int64_t len) -> int64_t {
        if (e->kind == Expr::Kind::Unary && e->op == "<") return len - as_int(eval(e->rhs, env));
        return as_int(eval(e, env));
    };
    if (rhs->kind == Expr::Kind::Range) {
        int64_t len = 0;
        if (c.index() == 5) len = static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(c)->items.size());
        else if (c.index() == 4) len = static_cast<int64_t>(std::get<std::string>(c).size());
        int64_t start = resolveIndex(rhs->lhs, len);
        int64_t end = resolveIndex(rhs->rhs, len);
        if (c.index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(c);
            int64_t len = static_cast<int64_t>(arr->items.size());
            if (start < 0) start += len;
            if (end < 0) end += len;
            if (rhs->text == "..<") end = len - end;
            if (start < 0) start = 0;
            if (end >= len) end = len - 1;
            auto out = std::make_shared<ArrayValue>();
            if (start <= end) {
                for (int64_t i = start; i <= end; ++i) out->items.push_back(arr->items[static_cast<size_t>(i)]);
            }
            return out;
        }
        if (c.index() == 4) {
            const std::string& s = std::get<std::string>(c);
            int64_t len = static_cast<int64_t>(s.size());
            if (start < 0) start += len;
            if (end < 0) end += len;
            if (rhs->text == "..<") end = len - end;
            if (start < 0) start = 0;
            if (end >= len) end = len - 1;
            if (start > end) return make_string("");
            return make_string(s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start + 1)));
        }
        return Value{};   // bad range -> undefined (type 0)
    }
    if (c.index() == 5) {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(c);
        int64_t len = static_cast<int64_t>(arr->items.size());
        int64_t i = resolveIndex(rhs, len);
        if (i < 0) i += len;
        if (i < 0 || i >= len) return Value{};   // OOB -> undefined (type 0)
        return arr->items[static_cast<size_t>(i)];
    }
    if (c.index() == 6) {
        Value idx = eval(rhs, env);
        auto mp = std::get<std::shared_ptr<MappingValue>>(c);
        const Value* hit = mp->find(idx);
        if (hit) return *hit;
        return Value{};   // miss -> undefined (type 0)
    }
    if (c.index() == 4) {
        const std::string& s = std::get<std::string>(c);
        int64_t len = static_cast<int64_t>(s.size());
        int64_t i = resolveIndex(rhs, len);
        if (i < 0) i += len;
        if (i < 0 || i >= len) return Value{};   // OOB -> undefined (type 0)
        return make_string(std::string(1, s[static_cast<size_t>(i)]));
    }
    return Value{};
}

void Interpreter::assignIndex(ExprPtr lhs, ExprPtr rhs, const Value& v, Environment& env) {
    Value c = eval(lhs, env);
    Value idx = eval(rhs, env);
    if (c.index() == 5) {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(c);
        int64_t i = as_int(idx);
        if (i < 0) i += static_cast<int64_t>(arr->items.size());
        if (i >= 0 && i < static_cast<int64_t>(arr->items.size())) arr->items[static_cast<size_t>(i)] = v;
        return;
    }
    if (c.index() == 6) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(c);
        mp->insert(idx, v);
        return;
    }
    throw RuntimeError("cannot index-assign this value");
}

Value Interpreter::evalCall(ExprPtr target, const std::vector<ExprPtr>& args, Environment& env) {
    std::vector<Value> argValues;
    for (auto& a : args) argValues.push_back(eval(a, env));
    if (target->kind == Expr::Kind::Ident) {
        const std::string& name = target->text;
        if (name == "this_object") return thisObject_;
        if (functions_.find(name) != functions_.end()) return callFunction(name, argValues);
        return callBuiltin(name, argValues);
    }
    Value callee = eval(target, env);
    if (callee.index() == 8) {
        auto cl = std::get<std::shared_ptr<ClosureValue>>(callee);
        std::vector<Value> all = cl->boundArgs;
        all.insert(all.end(), argValues.begin(), argValues.end());
        return callFunction(cl->functionName, all);
    }
    if (callee.index() == 7) {
        // Calling an object directly is not common; return 0.
        return make_int(0);
    }
    throw RuntimeError("attempt to call a non-function value");
}

Value Interpreter::evalCallOther(ExprPtr target, const std::string& fn, const std::vector<ExprPtr>& args, Environment& env) {
    Value obj = eval(target, env);
    std::vector<Value> argValues;
    for (auto& a : args) argValues.push_back(eval(a, env));
    if (obj.index() == 7) {
        auto o = std::get<std::shared_ptr<ObjectValue>>(obj);
        auto it = o->variables.find(fn);
        if (it != o->variables.end() && it->second.index() == 8) {
            auto cl = std::get<std::shared_ptr<ClosureValue>>(it->second);
            return callFunction(cl->functionName, argValues);
        }
        // If the object is one of our in-memory objects we might still have the function in the current program.
        if (functions_.find(fn) != functions_.end()) return callFunction(fn, argValues);
        return make_int(0);
    }
    // String call_other: load the target object from the extracted pak and call
    // the requested function through the original bytecode VM.
    if (obj.index() == 4) {
        if (functions_.find(fn) != functions_.end()) return callFunction(fn, argValues);
        return callOriginalObjectFunction(std::get<std::string>(obj), fn, argValues, "out/lib_aaa32");
    }
    return make_int(0);
}

Value Interpreter::evalLambda(const Expr& e, Environment& env) {
    std::vector<Value> bound;
    for (auto& a : e.lambdaArgs) bound.push_back(eval(a, env));
    return make_closure(e.text, bound);
}

Value Interpreter::makeDefaultObject(const std::string& path) {
    auto o = std::make_shared<ObjectValue>();
    o->programName = path;
    return o;
}

} // namespace lpc
