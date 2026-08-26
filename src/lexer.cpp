#include "lexer.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace lpc {

Lexer::Lexer(std::string source) : src_(std::move(source)) {}

char Lexer::peek(size_t off) const {
    if (pos_ + off >= src_.size()) return '\0';
    return src_[pos_ + off];
}

char Lexer::get() {
    if (pos_ >= src_.size()) return '\0';
    char c = src_[pos_++];
    if (c == '\n') ++line_;
    return c;
}

bool Lexer::match(char c) {
    if (peek() == c) {
        ++pos_;
        return true;
    }
    return false;
}

Token Lexer::make(Tok k, std::string t) const {
    Token tok;
    tok.kind = k;
    tok.text = std::move(t);
    tok.line = line_;
    return tok;
}

void Lexer::skipSpaceAndComments() {
    while (pos_ < src_.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            get();
        } else if (c == '/' && peek(1) == '/') {
            while (pos_ < src_.size() && peek() != '\n') get();
        } else if (c == '/' && peek(1) == '*') {
            get(); get();
            while (pos_ < src_.size() && !(peek() == '*' && peek(1) == '/')) get();
            if (pos_ < src_.size()) { get(); get(); }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipSpaceAndComments();
        if (pos_ >= src_.size()) {
            tokens.push_back(make(Tok::End, "<eof>"));
            break;
        }
        char c = peek();
        if (c == '#') {
            get();
            if (peek() == '#') {
                get();
                std::string num;
                if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
                    num += get(); num += get();
                    while (std::isxdigit(static_cast<unsigned char>(peek()))) num += get();
                    Token t = make(Tok::Number, num);
                    t.number = static_cast<double>(std::strtoll(num.c_str(), nullptr, 16));
                    tokens.push_back(t);
                } else {
                    while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
                    Token t = make(Tok::Number, num);
                    t.number = std::strtod(num.c_str(), nullptr);
                    tokens.push_back(t);
                }
            } else {
                std::string word;
                while (std::isalpha(static_cast<unsigned char>(peek()))) word += get();
                if (word == "define") {
                    while (peek() == ' ' || peek() == '\t') get();
                    std::string name;
                    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') name += get();
                    while (peek() == ' ' || peek() == '\t') get();
                    // Read one macro value token until end of line.
                    Token val = make(Tok::Number, "0");
                    if (peek() == '#' && peek(1) == '#') {
                        get(); get();
                        std::string num;
                        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
                            num += get(); num += get();
                            while (std::isxdigit(static_cast<unsigned char>(peek()))) num += get();
                            val = make(Tok::Number, num);
                            val.number = static_cast<double>(std::strtoll(num.c_str(), nullptr, 16));
                        } else {
                            while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
                            val = make(Tok::Number, num);
                            val.number = std::strtod(num.c_str(), nullptr);
                        }
                    } else if (peek() == '"') {
                        get();
                        std::string s;
                        while (peek() && peek() != '"') s += get();
                        if (peek() == '"') get();
                        val = make(Tok::String, s);
                    } else if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
                        std::string num;
                        num += get(); num += get();
                        while (std::isxdigit(static_cast<unsigned char>(peek()))) num += get();
                        val = make(Tok::Number, num);
                        val.number = static_cast<double>(std::strtoll(num.c_str(), nullptr, 16));
                    } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
                        std::string num;
                        while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
                        val = make(Tok::Number, num);
                        val.number = std::strtod(num.c_str(), nullptr);
                    } else {
                        std::string ident;
                        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') ident += get();
                        val = make(Tok::Ident, ident);
                    }
                    defines_[name] = val;
                }
                // skip rest of preprocessor line
                while (pos_ < src_.size() && peek() != '\n') get();
            }
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && std::isdigit(static_cast<unsigned char>(peek(1))))) {
            bool isFloat = false;
            std::string num;
            if (c == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
                num += get(); num += get();
                while (std::isxdigit(static_cast<unsigned char>(peek()))) num += get();
                tokens.push_back(make(Tok::Number, num));
                continue;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
            if (peek() == '.' && peek(1) != '.') {
                isFloat = true;
                num += get();
                while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
            }
            if (peek() == 'e' || peek() == 'E') {
                isFloat = true;
                num += get();
                if (peek() == '+' || peek() == '-') num += get();
                while (std::isdigit(static_cast<unsigned char>(peek()))) num += get();
            }
            Token t = make(isFloat ? Tok::Float : Tok::Number, num);
            t.number = std::strtod(num.c_str(), nullptr);
            tokens.push_back(t);
            continue;
        }
        if (c == '"') {
            get();
            std::string s;
            while (pos_ < src_.size() && peek() != '"') {
                char ch = get();
                if (ch == '\\') {
                    char e = get();
                    switch (e) {
                    case 'n': s += '\n'; break;
                    case 't': s += '\t'; break;
                    case 'r': s += '\r'; break;
                    case '\\': s += '\\'; break;
                    case '"': s += '"'; break;
                    case '\'': s += '\''; break;
                    case '0': s += '\0'; break;
                    default: s += e; break;
                    }
                } else {
                    s += ch;
                }
            }
            if (pos_ < src_.size()) get();
            tokens.push_back(make(Tok::String, s));
            continue;
        }
        auto idStart = [](char x) { return std::isalpha(static_cast<unsigned char>(x)) || x == '_'; };
        auto idPart = [](char x) { return std::isalnum(static_cast<unsigned char>(x)) || x == '_'; };
        if (idStart(c)) {
            std::string id;
            while (idPart(peek())) id += get();
            static const struct { const char* name; Tok tok; } kw[] = {
                {"if", Tok::KwIf}, {"else", Tok::KwElse}, {"while", Tok::KwWhile}, {"do", Tok::KwDo},
                {"for", Tok::KwFor}, {"loop", Tok::KwLoop}, {"in", Tok::KwIn},
                {"switch", Tok::KwSwitch}, {"case", Tok::KwCase}, {"default", Tok::KwDefault},
                {"break", Tok::KwBreak}, {"continue", Tok::KwContinue}, {"return", Tok::KwReturn},
                {"catch", Tok::KwCatch}, {"try", Tok::KwTry}, {"inherit", Tok::KwInherit}, {"undefined", Tok::KwUndefined},
                {"nil", Tok::KwNil}, {"new", Tok::KwNew}, {"true", Tok::KwTrue}, {"false", Tok::KwFalse},
                {"void", Tok::KwVoid}, {"int", Tok::KwInt}, {"float", Tok::KwFloat},
                {"string", Tok::KwString}, {"mixed", Tok::KwMixed}, {"array", Tok::KwArray},
                {"mapping", Tok::KwMapping}, {"object", Tok::KwObject}, {"buffer", Tok::KwBuffer}, {"function", Tok::KwFunction},
                {"private", Tok::KwPrivate}, {"public", Tok::KwPublic}, {"override", Tok::KwOverride},
                {"static", Tok::KwStatic}, {"nomask", Tok::KwNomask}, {"varargs", Tok::KwVarargs},
            };
            auto dit = defines_.find(id);
            if (dit != defines_.end()) {
                Token t = dit->second;
                t.line = line_;
                tokens.push_back(t);
                continue;
            }
            Tok kind = Tok::Ident;
            for (auto& k : kw) {
                if (id == k.name) { kind = k.tok; break; }
            }
            tokens.push_back(make(kind, id));
            continue;
        }
        get();
        switch (c) {
        case '(':
            if (peek() == ':') { get(); tokens.push_back(make(Tok::LambdaBegin, "(: ")); }
            else tokens.push_back(make(Tok::LParen, "("));
            break;
        case ')':
            if (peek() == ':') { get(); tokens.push_back(make(Tok::LambdaEnd, ":)")); }
            else tokens.push_back(make(Tok::RParen, ")"));
            break;
        case '{': tokens.push_back(make(Tok::LBrace, "{")); break;
        case '}': tokens.push_back(make(Tok::RBrace, "}")); break;
        case '[': tokens.push_back(make(Tok::LBracket, "[")); break;
        case ']': tokens.push_back(make(Tok::RBracket, "]")); break;
        case ',': tokens.push_back(make(Tok::Comma, ",")); break;
        case ';': tokens.push_back(make(Tok::Semicolon, ";")); break;
        case ':':
            if (match(':')) tokens.push_back(make(Tok::Scope, "::"));
            else if (match(')')) tokens.push_back(make(Tok::LambdaEnd, ":)"));
            else tokens.push_back(make(Tok::Colon, ":"));
            break;
        case '?': tokens.push_back(make(Tok::Question, "?")); break;
        case '+':
            if (match('+')) tokens.push_back(make(Tok::PlusPlus, "++"));
            else if (match('=')) tokens.push_back(make(Tok::PlusAssign, "+="));
            else tokens.push_back(make(Tok::Plus, "+"));
            break;
        case '-':
            if (match('-')) tokens.push_back(make(Tok::MinusMinus, "--"));
            else if (match('=')) tokens.push_back(make(Tok::MinusAssign, "-="));
            else if (match('>')) tokens.push_back(make(Tok::Arrow, "->"));
            else tokens.push_back(make(Tok::Minus, "-"));
            break;
        case '*':
            if (match('=')) tokens.push_back(make(Tok::StarAssign, "*="));
            else tokens.push_back(make(Tok::Star, "*"));
            break;
        case '/':
            if (match('=')) tokens.push_back(make(Tok::SlashAssign, "/="));
            else tokens.push_back(make(Tok::Slash, "/"));
            break;
        case '%': tokens.push_back(make(Tok::Percent, "%")); break;
        case '=':
            if (match('=')) tokens.push_back(make(Tok::Eq, "=="));
            else tokens.push_back(make(Tok::Assign, "="));
            break;
        case '!':
            if (match('=')) tokens.push_back(make(Tok::Ne, "!="));
            else tokens.push_back(make(Tok::Not, "!"));
            break;
        case '<':
            if (match('<')) {
                if (match('=')) tokens.push_back(make(Tok::Assign, "<<="));
                else tokens.push_back(make(Tok::Shl, "<<"));
            } else if (match('=')) tokens.push_back(make(Tok::Le, "<="));
            else tokens.push_back(make(Tok::Lt, "<"));
            break;
        case '>':
            if (match('>')) {
                if (match('=')) tokens.push_back(make(Tok::Assign, ">>="));
                else tokens.push_back(make(Tok::Shr, ">>"));
            } else if (match('=')) tokens.push_back(make(Tok::Ge, ">="));
            else tokens.push_back(make(Tok::Gt, ">"));
            break;
        case '&':
            if (match('&')) tokens.push_back(make(Tok::AndAnd, "&&"));
            else if (match('=')) tokens.push_back(make(Tok::Assign, "&="));
            else tokens.push_back(make(Tok::BitAnd, "&"));
            break;
        case '|':
            if (match('|')) tokens.push_back(make(Tok::OrOr, "||"));
            else if (match('=')) tokens.push_back(make(Tok::Assign, "|="));
            else tokens.push_back(make(Tok::BitOr, "|"));
            break;
        case '^':
            if (match('=')) tokens.push_back(make(Tok::Assign, "^="));
            else tokens.push_back(make(Tok::BitXor, "^"));
            break;
        case '~': tokens.push_back(make(Tok::BitNot, "~")); break;
        case '.':
            if (match('.')) {
                if (match('<')) tokens.push_back(make(Tok::EllipsisLt, "..<"));
                else tokens.push_back(make(Tok::Ellipsis, ".."));
            } else {
                tokens.push_back(make(Tok::Dot, "."));
            }
            break;
        case '$':
            if (match('<')) tokens.push_back(make(Tok::DollarLess, "$<"));
            else throw std::runtime_error("unexpected character '$'");
            break;
        default:
            throw std::runtime_error(std::string("unexpected character '") + c + "'");
        }
    }
    return tokens;
}

} // namespace lpc
