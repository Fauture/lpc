#pragma once
#include <vector>

#include "ast.h"
#include "lexer.h"

namespace lpc {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program parseProgram();
    // Parse a single LPC expression (used by the telnet shell's execute_data
    // to evaluate command strings like "obj->func()" or "1 + 2").
    ExprPtr parseShellExpression();

private:
    std::vector<Token> toks_;
    size_t pos_ = 0;

    const Token& cur() const;
    const Token& peek(size_t off = 1) const;
    bool at(Tok k) const;
    bool atType() const;
    bool atModifier() const;
    Token advance();
    bool match(Tok k);
    Token expect(Tok k, const std::string& what);
    std::string parseType();
    std::vector<std::string> parseModifiers();

    ExprPtr parseExpression();
    ExprPtr parseAssignment();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseBitOr();
    ExprPtr parseBitXor();
    ExprPtr parseBitAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseShift();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePostfix(ExprPtr lhs);
    ExprPtr parsePrimary();
    ExprPtr parseArrayLiteral();
    ExprPtr parseMappingLiteral();
    ExprPtr parseLambda();
    ExprPtr parseCallOther(ExprPtr lhs, const Token& arrow);

    StmtPtr parseStatement();
    StmtPtr parseBlock();
    StmtPtr parseVarDeclStatement();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseDoWhile();
    StmtPtr parseFor();
    StmtPtr parseForeach();
    StmtPtr parseLoop();
    StmtPtr parseSwitch();
    StmtPtr parseReturn();
    StmtPtr parseTryCatch();

    std::vector<ExprPtr> parseArgs();
    bool isTypeToken(Tok k) const;
};

} // namespace lpc
