#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "value.h"

namespace lpc {

enum class Flow {
    Normal,
    Return,
    Break,
    Continue
};

struct Environment {
    std::unordered_map<std::string, Value> vars;
    Environment* parent = nullptr;
};

class Interpreter {
public:
    explicit Interpreter(const Program& program);

    void runGlobal();
    Value callFunction(const std::string& name, const std::vector<Value>& args);
    Value eval(ExprPtr expr, Environment& env);
    Flow exec(StmtPtr stmt, Environment& env);

    // Builtins are registered in builtins.cpp
    Value callBuiltin(const std::string& name, const std::vector<Value>& args);

    static std::string lastError;

private:
    const Program& program_;
    Environment globalEnv_;
    std::unordered_map<std::string, const FunctionDef*> functions_;
    Value returnValue_;
    Flow flow_ = Flow::Normal;
    Value thisObject_;
    std::vector<Value> currentArgs_;

    Flow execBlock(const std::vector<StmtPtr>& stmts, Environment& env);
    Value evalIdent(const std::string& name, Environment& env);
    void assignIdent(const std::string& name, const Value& v, Environment& env);
    Value evalBinary(const std::string& op, const Value& a, const Value& b, int line);
    Value evalUnary(const std::string& op, const Value& v, int line);
    Value evalIndex(ExprPtr lhs, ExprPtr rhs, Environment& env);
    void assignIndex(ExprPtr lhs, ExprPtr rhs, const Value& v, Environment& env);
    Value evalCall(ExprPtr target, const std::vector<ExprPtr>& args, Environment& env);
    Value evalCallOther(ExprPtr target, const std::string& fn, const std::vector<ExprPtr>& args, Environment& env);
    Value evalLambda(const Expr& e, Environment& env);
    Value makeDefaultObject(const std::string& path);
};

} // namespace lpc
