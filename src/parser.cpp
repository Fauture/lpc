#include "parser.h"

#include <stdexcept>

namespace lpc {

Parser::Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

const Token& Parser::cur() const { return toks_[pos_]; }
const Token& Parser::peek(size_t off) const {
    size_t i = pos_ + off;
    if (i >= toks_.size()) return toks_.back();
    return toks_[i];
}
bool Parser::at(Tok k) const { return cur().kind == k; }
bool Parser::atType() const {
    switch (cur().kind) {
    case Tok::KwVoid: case Tok::KwInt: case Tok::KwFloat: case Tok::KwString:
    case Tok::KwMixed: case Tok::KwArray: case Tok::KwMapping: case Tok::KwObject:
    case Tok::KwBuffer: case Tok::KwFunction:
        return true;
    default: return false;
    }
}
bool Parser::atModifier() const {
    switch (cur().kind) {
    case Tok::KwPrivate: case Tok::KwPublic: case Tok::KwOverride:
    case Tok::KwStatic: case Tok::KwNomask: case Tok::KwVarargs:
        return true;
    default: return false;
    }
}
Token Parser::advance() {
    if (pos_ < toks_.size()) return toks_[pos_++];
    return toks_.back();
}
bool Parser::match(Tok k) {
    if (at(k)) { advance(); return true; }
    return false;
}
Token Parser::expect(Tok k, const std::string& what) {
    if (!at(k)) throw std::runtime_error("expected " + what + " at line " + std::to_string(cur().line));
    return advance();
}
std::string Parser::parseType() {
    if (!atType()) throw std::runtime_error("expected type at line " + std::to_string(cur().line));
    return advance().text;
}
std::vector<std::string> Parser::parseModifiers() {
    std::vector<std::string> mods;
    while (atModifier()) mods.push_back(advance().text);
    return mods;
}
bool Parser::isTypeToken(Tok k) const {
    switch (k) {
    case Tok::KwVoid: case Tok::KwInt: case Tok::KwFloat: case Tok::KwString:
    case Tok::KwMixed: case Tok::KwArray: case Tok::KwMapping: case Tok::KwObject:
    case Tok::KwBuffer: case Tok::KwFunction:
        return true;
    default: return false;
    }
}

Program Parser::parseProgram() {
    Program prog;
    while (!at(Tok::End)) {
        if (at(Tok::KwInherit)) {
            advance();
            if (cur().kind == Tok::String) prog.inherits.push_back(advance().text);
            else if (cur().kind == Tok::Ident) prog.inherits.push_back(advance().text);
            expect(Tok::Semicolon, "';' after inherit");
            continue;
        }
        // Function/global declaration.
        if (atType() || atModifier()) {
            std::vector<std::string> mods = parseModifiers();
            std::string type = parseType();
            if (cur().kind == Tok::Ident && peek().kind == Tok::LParen) {
                FunctionDef fn;
                fn.modifiers = std::move(mods);
                fn.returnType = std::move(type);
                fn.name = advance().text;
                expect(Tok::LParen, "'('");
                if (!at(Tok::RParen)) {
                    do {
                        Param p;
                        if (atType()) p.type = parseType();
                        else p.type = "mixed";
                        match(Tok::BitAnd); // reference parameter marker
                        p.name = expect(Tok::Ident, "parameter name").text;
                        if (match(Tok::Assign)) p.defaultValue = parseAssignment();
                        fn.params.push_back(std::move(p));
                    } while (match(Tok::Comma));
                }
                expect(Tok::RParen, "')'");
                if (match(Tok::Semicolon)) {
                    fn.isForward = true;
                } else {
                    expect(Tok::LBrace, "'{'");
                    while (!at(Tok::RBrace) && !at(Tok::End)) fn.body.push_back(parseStatement());
                    expect(Tok::RBrace, "'}'");
                }
                prog.functions.push_back(std::move(fn));
                continue;
            }
            // Global variable declaration: type name [, name]* ;
            while (true) {
                std::string name = expect(Tok::Ident, "global variable name").text;
                prog.globalVars.emplace_back(name, type);
                if (!match(Tok::Comma)) break;
                if (atType()) type = parseType();
            }
            expect(Tok::Semicolon, "';' after global variable");
            continue;
        }
        // Global initializer / statement.
        if (at(Tok::KwIf) || at(Tok::KwWhile) || at(Tok::KwFor) || at(Tok::KwLoop) ||
            at(Tok::KwSwitch) || at(Tok::KwReturn) || at(Tok::LBrace) || at(Tok::KwTry) || at(Tok::KwDo)) {
            prog.globalBody.push_back(parseStatement());
            continue;
        }
        prog.globalBody.push_back(std::make_shared<Stmt>(Stmt{Stmt::Kind::Expr, cur().line, parseExpression(), {}, "", {}, nullptr, nullptr, nullptr, nullptr, nullptr, "", {}, {}, false}));
        expect(Tok::Semicolon, "';' after global expression");
    }
    return prog;
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
    ExprPtr lhs = parseOr();
    if (at(Tok::Question)) {
        advance();
        ExprPtr trueExpr = parseAssignment();
        expect(Tok::Colon, "':' in ternary expression");
        ExprPtr falseExpr = parseAssignment();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary;
        e->line = lhs->line;
        e->op = "?:";
        e->lhs = lhs;
        e->rhs = trueExpr;
        e->cond = falseExpr;
        return e;
    }
    if (at(Tok::Assign) || at(Tok::PlusAssign) || at(Tok::MinusAssign) ||
        at(Tok::StarAssign) || at(Tok::SlashAssign)) {
        std::string op = advance().text;
        ExprPtr rhs = parseAssignment();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Assign;
        e->line = lhs->line;
        e->op = op;
        e->lhs = lhs;
        e->rhs = rhs;
        return e;
    }
    return lhs;
}

ExprPtr Parser::parseOr() {
    ExprPtr lhs = parseAnd();
    while (at(Tok::OrOr)) {
        std::string op = advance().text;
        ExprPtr rhs = parseAnd();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseAnd() {
    ExprPtr lhs = parseBitOr();
    while (at(Tok::AndAnd)) {
        std::string op = advance().text;
        ExprPtr rhs = parseBitOr();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseBitOr() {
    ExprPtr lhs = parseBitXor();
    while (at(Tok::BitOr)) {
        std::string op = advance().text;
        ExprPtr rhs = parseBitXor();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseBitXor() {
    ExprPtr lhs = parseBitAnd();
    while (at(Tok::BitXor)) {
        std::string op = advance().text;
        ExprPtr rhs = parseBitAnd();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseBitAnd() {
    ExprPtr lhs = parseEquality();
    while (at(Tok::BitAnd)) {
        std::string op = advance().text;
        ExprPtr rhs = parseEquality();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseEquality() {
    ExprPtr lhs = parseRelational();
    while (at(Tok::Eq) || at(Tok::Ne)) {
        std::string op = advance().text;
        ExprPtr rhs = parseRelational();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseRelational() {
    ExprPtr lhs = parseShift();
    while (at(Tok::Lt) || at(Tok::Gt) || at(Tok::Le) || at(Tok::Ge)) {
        std::string op = advance().text;
        ExprPtr rhs = parseShift();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseShift() {
    ExprPtr lhs = parseAdditive();
    while (at(Tok::Shl) || at(Tok::Shr)) {
        std::string op = advance().text;
        ExprPtr rhs = parseAdditive();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseAdditive() {
    ExprPtr lhs = parseMultiplicative();
    while (at(Tok::Plus) || at(Tok::Minus)) {
        std::string op = advance().text;
        ExprPtr rhs = parseMultiplicative();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseMultiplicative() {
    ExprPtr lhs = parseUnary();
    while (at(Tok::Star) || at(Tok::Slash) || at(Tok::Percent)) {
        std::string op = advance().text;
        ExprPtr rhs = parseUnary();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; e->line = lhs->line;
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parseUnary() {
    if (at(Tok::PlusPlus) || at(Tok::MinusMinus)) {
        std::string op = advance().text;
        ExprPtr target = parseUnary();
        auto one = std::make_shared<Expr>();
        one->kind = Expr::Kind::Int;
        one->line = target->line;
        one->intValue = 1;
        auto bin = std::make_shared<Expr>();
        bin->kind = Expr::Kind::Binary;
        bin->line = target->line;
        bin->op = (op == "++") ? "+" : "-";
        bin->lhs = target;
        bin->rhs = one;
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Assign;
        e->line = target->line;
        e->op = "=";
        e->lhs = target;
        e->rhs = bin;
        return e;
    }
    if (at(Tok::Not) || at(Tok::Minus) || at(Tok::BitNot) || at(Tok::Star) || at(Tok::BitAnd)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Unary;
        e->line = cur().line;
        e->op = advance().text;
        e->rhs = parseUnary();
        return e;
    }
    if (at(Tok::LParen) && isTypeToken(peek().kind) && peek(2).kind == Tok::RParen) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Cast;
        e->line = cur().line;
        advance(); // (
        e->text = parseType();
        expect(Tok::RParen, "')'");
        e->rhs = parseUnary();
        return e;
    }
    return parsePostfix(parsePrimary());
}

ExprPtr Parser::parsePostfix(ExprPtr lhs) {
    while (true) {
        if (at(Tok::LParen)) {
            std::vector<ExprPtr> args = parseArgs();
            auto e = std::make_shared<Expr>();
            e->kind = Expr::Kind::Call;
            e->line = lhs->line;
            e->target = lhs;
            e->args = std::move(args);
            lhs = e;
        } else if (at(Tok::LBracket)) {
            advance();
            ExprPtr idx;
            if (at(Tok::Lt)) {
                // LPC <n> means "nth from the end" inside an index.
                advance();
                auto neg = std::make_shared<Expr>();
                neg->kind = Expr::Kind::Unary;
                neg->line = cur().line;
                neg->op = "<";
                neg->rhs = parseExpression();
                idx = neg;
            } else {
                idx = parseExpression();
            }
            if (at(Tok::Ellipsis) || at(Tok::EllipsisLt)) {
                std::string op = advance().text;
                ExprPtr end = parseExpression();
                auto range = std::make_shared<Expr>();
                range->kind = Expr::Kind::Range;
                range->line = idx->line;
                range->text = op;
                range->lhs = idx;
                range->rhs = end;
                idx = range;
            }
            expect(Tok::RBracket, "']'");
            auto e = std::make_shared<Expr>();
            e->kind = Expr::Kind::Index;
            e->line = lhs->line;
            e->lhs = lhs;
            e->rhs = idx;
            lhs = e;
        } else if (at(Tok::PlusPlus) || at(Tok::MinusMinus)) {
            std::string op = advance().text;
            auto one = std::make_shared<Expr>();
            one->kind = Expr::Kind::Int;
            one->line = lhs->line;
            one->intValue = 1;
            auto bin = std::make_shared<Expr>();
            bin->kind = Expr::Kind::Binary;
            bin->line = lhs->line;
            bin->op = (op == "++") ? "+" : "-";
            bin->lhs = lhs;
            bin->rhs = one;
            auto e = std::make_shared<Expr>();
            e->kind = Expr::Kind::Assign;
            e->line = lhs->line;
            e->op = "=";
            e->lhs = lhs;
            e->rhs = bin;
            lhs = e;
        } else if (at(Tok::Arrow)) {
            Token arrow = advance();
            lhs = parseCallOther(lhs, arrow);
        } else if (at(Tok::Dot)) {
            advance();
            auto e = std::make_shared<Expr>();
            e->kind = Expr::Kind::Member;
            e->line = lhs->line;
            e->lhs = lhs;
            e->text = expect(Tok::Ident, "member name").text;
            lhs = e;
        } else if (at(Tok::Scope)) {
            // inherited call `obj::func(...)` - keep simple: skip `::` and treat as member/call
            advance();
            std::string name = expect(Tok::Ident, "function name after ::").text;
            if (at(Tok::LParen)) {
                auto args = parseArgs();
                auto e = std::make_shared<Expr>();
                e->kind = Expr::Kind::CallOther;
                e->line = lhs->line;
                e->lhs = lhs;
                e->text = name;
                e->args = std::move(args);
                lhs = e;
            } else {
                auto e = std::make_shared<Expr>();
                e->kind = Expr::Kind::Member;
                e->line = lhs->line;
                e->lhs = lhs;
                e->text = name;
                lhs = e;
            }
        } else {
            break;
        }
    }
    return lhs;
}

ExprPtr Parser::parseCallOther(ExprPtr lhs, const Token&) {
    // After `->`, expect identifier (function name). The complete call may already
    // be parsed as a postfix call if we return a Call expr with lhs = original.
    std::string name = expect(Tok::Ident, "function name after '->'").text;
    auto call = std::make_shared<Expr>();
    call->kind = Expr::Kind::CallOther;
    call->line = lhs->line;
    call->lhs = lhs;
    call->text = name;
    if (at(Tok::LParen)) call->args = parseArgs();
    return call;
}

ExprPtr Parser::parsePrimary() {
    if (at(Tok::Number)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Int;
        e->line = cur().line;
        e->intValue = static_cast<int64_t>(cur().number);
        if (!e->intValue && cur().text != "0") {
            // hex literal
            e->intValue = std::stoll(cur().text, nullptr, 0);
        }
        advance();
        return e;
    }
    if (at(Tok::Float)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Float;
        e->line = cur().line;
        e->floatValue = cur().number;
        advance();
        return e;
    }
    if (at(Tok::String)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::String;
        e->line = cur().line;
        e->text = advance().text;
        return e;
    }
    if (at(Tok::Ident)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Ident;
        e->line = cur().line;
        e->text = advance().text;
        return e;
    }
    if (at(Tok::DollarLess)) {
        advance();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Ident;
        e->line = cur().line;
        e->text = "args";
        return e;
    }
    if (at(Tok::KwUndefined) || at(Tok::KwNil) || at(Tok::KwTrue) || at(Tok::KwFalse)) {
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Ident;
        e->line = cur().line;
        e->text = advance().text;
        return e;
    }
    if (at(Tok::KwNew)) {
        advance();
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::New;
        e->line = cur().line;
        e->text = expect(Tok::String, "path string after new").text;
        return e;
    }
    // Array literal ({ ... })
    if (at(Tok::LParen) && peek().kind == Tok::LBrace) {
        advance(); advance();
        return parseArrayLiteral();
    }
    // Mapping literal ([ ... ])
    if (at(Tok::LParen) && peek().kind == Tok::LBracket) {
        advance(); advance();
        return parseMappingLiteral();
    }
    if (at(Tok::LParen)) {
        advance();
        ExprPtr inner = parseExpression();
        expect(Tok::RParen, "')'");
        return parsePostfix(inner);
    }
    if (at(Tok::LambdaBegin)) return parseLambda();
    if (at(Tok::KwCatch)) {
        advance();
        expect(Tok::LParen, "'(' after catch");
        ExprPtr inner = parseExpression();
        expect(Tok::RParen, "')'");
        auto e = std::make_shared<Expr>();
        e->kind = Expr::Kind::Catch;
        e->line = cur().line;
        e->rhs = inner;
        return e;
    }
    throw std::runtime_error("unexpected token '" + cur().text + "' at line " + std::to_string(cur().line));
}

ExprPtr Parser::parseArrayLiteral() {
    auto e = std::make_shared<Expr>();
    e->kind = Expr::Kind::Array;
    e->line = cur().line;
    while (!at(Tok::RBrace)) {
        if (at(Tok::End)) throw std::runtime_error("unterminated array literal");
        e->args.push_back(parseExpression());
        if (!match(Tok::Comma)) break;
    }
    expect(Tok::RBrace, "'}'");
    expect(Tok::RParen, "')'");
    return e;
}

ExprPtr Parser::parseMappingLiteral() {
    auto e = std::make_shared<Expr>();
    e->kind = Expr::Kind::Mapping;
    e->line = cur().line;
    while (!at(Tok::RBracket)) {
        if (at(Tok::End)) throw std::runtime_error("unterminated mapping literal");
        ExprPtr key = parseExpression();
        expect(Tok::Colon, "':' in mapping literal");
        ExprPtr val = parseExpression();
        e->mapEntries.emplace_back(key, val);
        if (!match(Tok::Comma)) break;
    }
    expect(Tok::RBracket, "']'");
    expect(Tok::RParen, "')'");
    return e;
}

ExprPtr Parser::parseLambda() {
    auto e = std::make_shared<Expr>();
    e->kind = Expr::Kind::Lambda;
    e->line = cur().line;
    expect(Tok::LambdaBegin, "'(:'");
    if (at(Tok::Ident)) e->text = advance().text;
    else if (at(Tok::String)) e->text = advance().text;
    while (match(Tok::Comma)) {
        e->lambdaArgs.push_back(parseAssignment());
    }
    expect(Tok::LambdaEnd, ":)'");
    return e;
}

std::vector<ExprPtr> Parser::parseArgs() {
    expect(Tok::LParen, "'('");
    std::vector<ExprPtr> args;
    if (!at(Tok::RParen)) {
        do {
            args.push_back(parseAssignment());
        } while (match(Tok::Comma));
    }
    expect(Tok::RParen, "')'");
    return args;
}

StmtPtr Parser::parseStatement() {
    if (at(Tok::LBrace)) return parseBlock();
    if (atType()) return parseVarDeclStatement();
    if (at(Tok::KwIf)) return parseIf();
    if (at(Tok::KwWhile)) return parseWhile();
    if (at(Tok::KwDo)) return parseDoWhile();
    if (at(Tok::KwFor)) return parseFor();
    if (at(Tok::KwLoop)) return parseLoop();
    if (at(Tok::KwSwitch)) return parseSwitch();
    if (at(Tok::KwReturn)) return parseReturn();
    if (at(Tok::KwTry)) return parseTryCatch();
    if (at(Tok::KwBreak)) {
        advance();
        expect(Tok::Semicolon, "';' after break");
        auto s = std::make_shared<Stmt>();
        s->kind = Stmt::Kind::Break;
        s->line = cur().line;
        return s;
    }
    if (at(Tok::KwContinue)) {
        advance();
        expect(Tok::Semicolon, "';' after continue");
        auto s = std::make_shared<Stmt>();
        s->kind = Stmt::Kind::Continue;
        s->line = cur().line;
        return s;
    }
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::Expr;
    s->line = cur().line;
    s->expr = parseExpression();
    expect(Tok::Semicolon, "';' after expression");
    return s;
}

StmtPtr Parser::parseBlock() {
    expect(Tok::LBrace, "'{'");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::Block;
    s->line = cur().line;
    while (!at(Tok::RBrace) && !at(Tok::End)) s->block.push_back(parseStatement());
    expect(Tok::RBrace, "'}'");
    return s;
}

StmtPtr Parser::parseVarDeclStatement() {
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::VarDecl;
    s->line = cur().line;
    s->varType = parseType();
    while (true) {
        std::string name = expect(Tok::Ident, "variable name").text;
        ExprPtr init;
        if (match(Tok::Assign)) init = parseAssignment();
        s->vars.emplace_back(name, init);
        if (!match(Tok::Comma)) break;
        if (atType()) s->varType = parseType();
    }
    expect(Tok::Semicolon, "';' after variable declaration");
    return s;
}

StmtPtr Parser::parseIf() {
    expect(Tok::KwIf, "'if'");
    expect(Tok::LParen, "'('");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::If;
    s->line = cur().line;
    s->cond = parseExpression();
    expect(Tok::RParen, "')'");
    s->thenStmt = parseStatement();
    if (match(Tok::KwElse)) s->elseStmt = parseStatement();
    return s;
}

StmtPtr Parser::parseWhile() {
    expect(Tok::KwWhile, "'while'");
    expect(Tok::LParen, "'('");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::While;
    s->line = cur().line;
    s->cond = parseExpression();
    expect(Tok::RParen, "')'");
    s->thenStmt = parseStatement();
    return s;
}

StmtPtr Parser::parseDoWhile() {
    expect(Tok::KwDo, "'do'");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::DoWhile;
    s->line = cur().line;
    s->thenStmt = parseStatement();
    expect(Tok::KwWhile, "'while' after do body");
    expect(Tok::LParen, "'('");
    s->cond = parseExpression();
    expect(Tok::RParen, "')'");
    expect(Tok::Semicolon, "';' after do-while");
    return s;
}

StmtPtr Parser::parseFor() {
    expect(Tok::KwFor, "'for'");
    expect(Tok::LParen, "'('");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::For;
    s->line = cur().line;
    if (!at(Tok::Semicolon)) {
        if (atType()) {
            s->initStmt = parseVarDeclStatement();
        } else {
            auto es = std::make_shared<Stmt>();
            es->kind = Stmt::Kind::Expr;
            es->expr = parseExpression();
            expect(Tok::Semicolon, "';' after for-init");
            s->initStmt = es;
        }
    } else {
        advance();
    }
    if (!at(Tok::Semicolon)) s->cond = parseExpression();
    expect(Tok::Semicolon, "';' after for-cond");
    if (!at(Tok::RParen)) {
        auto es = std::make_shared<Stmt>();
        es->kind = Stmt::Kind::Expr;
        es->expr = parseExpression();
        s->incStmt = es;
    }
    expect(Tok::RParen, "')'");
    s->thenStmt = parseStatement();
    return s;
}

StmtPtr Parser::parseLoop() {
    expect(Tok::KwLoop, "'loop'");
    expect(Tok::LParen, "'('");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::Loop;
    s->line = cur().line;
    s->loopVar = expect(Tok::Ident, "loop variable").text;
    expect(Tok::KwIn, "'in'");
    s->cond = parseExpression();
    expect(Tok::RParen, "')'");
    s->thenStmt = parseStatement();
    return s;
}

StmtPtr Parser::parseSwitch() {
    expect(Tok::KwSwitch, "'switch'");
    expect(Tok::LParen, "'('");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::Switch;
    s->line = cur().line;
    s->cond = parseExpression();
    expect(Tok::RParen, "')'");
    expect(Tok::LBrace, "'{'");
    const size_t npos = static_cast<size_t>(-1);
    size_t curCase = npos;
    bool inDefault = false;
    while (!at(Tok::RBrace) && !at(Tok::End)) {
        if (at(Tok::KwCase)) {
            advance();
            ExprPtr val = parseExpression();
            expect(Tok::Colon, "':' after case");
            s->cases.emplace_back(val, std::vector<StmtPtr>{});
            curCase = s->cases.size() - 1;
            inDefault = false;
        } else if (at(Tok::KwDefault)) {
            advance();
            expect(Tok::Colon, "':' after default");
            s->defaultCase.clear();
            curCase = npos;
            inDefault = true;
        } else {
            StmtPtr st = parseStatement();
            if (inDefault) s->defaultCase.push_back(st);
            else if (curCase != npos) s->cases[curCase].second.push_back(st);
            // Statements before the first case are ignored (LPC switch grammar).
        }
    }
    expect(Tok::RBrace, "'}'");
    return s;
}

StmtPtr Parser::parseReturn() {
    expect(Tok::KwReturn, "'return'");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::Return;
    s->line = cur().line;
    if (!at(Tok::Semicolon)) {
        s->expr = parseExpression();
        s->hasValue = true;
    }
    expect(Tok::Semicolon, "';' after return");
    return s;
}

StmtPtr Parser::parseTryCatch() {
    expect(Tok::KwTry, "'try'");
    auto s = std::make_shared<Stmt>();
    s->kind = Stmt::Kind::TryCatch;
    s->line = cur().line;
    s->thenStmt = parseStatement();
    if (match(Tok::KwCatch)) {
        s->elseStmt = parseStatement();
    }
    return s;
}

ExprPtr Parser::parseShellExpression() {
    return parseExpression();
}

} // namespace lpc
