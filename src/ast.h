#pragma once
#include <memory>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct Expr {
    enum class Kind {
        Int, Float, String, Ident, Array, Mapping, Unary, Binary, Assign,
        Call, Index, Member, CallOther, Lambda, ThisObject, New, Cast, Catch, Range
    };
    Kind kind;
    int line = 0;
    // literal / ident
    int64_t intValue = 0;
    double floatValue = 0;
    std::string text;
    // compound
    std::vector<ExprPtr> args;
    std::vector<std::pair<ExprPtr, ExprPtr>> mapEntries;
    std::string op;
    ExprPtr lhs;
    ExprPtr rhs;
    ExprPtr cond;
    // function call target / call-other
    ExprPtr target;
    // lambda
    std::vector<ExprPtr> lambdaArgs;
};

struct Stmt {
    enum class Kind {
        Expr, Block, VarDecl, If, While, DoWhile, For, Loop, Switch, Return, Break, Continue, TryCatch
    };
    Kind kind;
    int line = 0;
    ExprPtr expr;
    std::vector<StmtPtr> block;
    // var decl
    std::string varType;
    std::vector<std::pair<std::string, ExprPtr>> vars;
    // if/while/for/loop
    ExprPtr cond;
    StmtPtr thenStmt;
    StmtPtr elseStmt;
    StmtPtr initStmt;
    StmtPtr incStmt;
    std::string loopVar;
    // switch
    std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> cases;
    std::vector<StmtPtr> defaultCase;
    // return
    bool hasValue = false;
};

struct Param {
    std::string type;
    std::string name;
    ExprPtr defaultValue;
};

struct FunctionDef {
    std::string returnType = "mixed";
    std::string name;
    std::vector<Param> params;
    std::vector<StmtPtr> body;
    std::vector<std::string> modifiers;
    bool isForward = false;
};

struct Program {
    std::vector<std::string> inherits;
    std::vector<std::pair<std::string, std::string>> globalVars; // name, type
    std::vector<StmtPtr> globalBody;
    std::vector<FunctionDef> functions;
};

} // namespace lpc
