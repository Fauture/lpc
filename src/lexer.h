#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace lpc {

enum class Tok {
    End, Ident, Number, Float, String,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, Question, Arrow, Scope,
    Plus, Minus, Star, Slash, Percent,
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign,
    Eq, Ne, Lt, Gt, Le, Ge,
    AndAnd, OrOr, Not, BitAnd, BitOr, BitXor, BitNot, Shl, Shr,
    Dot, Ellipsis,
    KwIf, KwElse, KwWhile, KwDo, KwFor, KwLoop, KwIn, KwSwitch, KwCase, KwDefault,
    KwBreak, KwContinue, KwReturn, KwCatch, KwTry, KwInherit,
    KwUndefined, KwNil, KwNew, KwTrue, KwFalse,
    KwVoid, KwInt, KwFloat, KwString, KwMixed, KwArray, KwMapping, KwObject, KwBuffer,
    KwPrivate, KwPublic, KwOverride, KwStatic, KwNomask, KwVarargs,
    LambdaBegin, LambdaEnd // (: :)
};

struct Token {
    Tok kind;
    std::string text;
    double number = 0;
    int line = 0;
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
    std::unordered_map<std::string, Token> defines_;

    char peek(size_t off = 0) const;
    char get();
    bool match(char c);
    void skipSpaceAndComments();
    Token make(Tok k, std::string t) const;
};

} // namespace lpc
