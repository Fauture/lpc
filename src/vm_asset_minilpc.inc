// grammar: no if/while/for - only loop(var in arr), return, declarations,
// assignment (= += &= ++), arithmetic (+ - * % & << >> < >), arrays ({...}),
// mappings ([k:v]), closures (: fn, args :), and the efuns sprintf/strlen/
// sizeof/sort/values/keys/toupper/tolower/allocate_mapping/count_total.
// ---------------------------------------------------------------------------
namespace minilpc {

struct Node;
using NodeP = std::shared_ptr<Node>;
struct Stmt;
using StmtP = std::shared_ptr<Stmt>;
struct Prog;

struct Node {
    enum Kind { NUM, STR, VAR, BIN, UN, CALL, ARR, MAPLIT, CLO, SELF, INDEX } k = NUM;
    int64_t num = 0;
    std::string str;              // STR text / VAR or CALL target name / BIN op
    std::vector<NodeP> kids;      // operands / args / elements
    std::string assignOp;         // VAR/INDEX assignment: "=", "+=", "&="
    NodeP rhs;                    // assignment right side
    int incDec = 0;               // VAR: +1 "++" / -1 "--"
    bool post = false;            // VAR: postfix form
    bool arrow = false;           // CALL: kids[0] is the base object (obj->fn)
};

struct Stmt {
    enum Kind { EXPRS, RET, DECL, LOOPIN, LOOPUPTO, BLOCK } k = EXPRS;
    NodeP expr;                   // EXPRS / RET / DECL init / LOOPIN iterable
    NodeP start;                  // LOOPUPTO: var = start
    std::string name;             // DECL var / LOOPIN var / LOOPUPTO var
    std::string declType;         // DECL: "mixed"/"int"/...
    std::vector<StmtP> stmts;     // BLOCK / LOOPIN / LOOPUPTO body
};

struct MiniFn {
    std::vector<std::pair<std::string, NodeP>> params;  // name -> default (may be null)
    StmtP body;
};

struct Prog {
    std::map<std::string, MiniFn> fns;
    std::map<std::string, Value> globals;
    std::vector<std::pair<std::string, NodeP>> globalInits;
};

// ---- lexer -------------------------------------------------------------
struct Tok {
    enum K { ID, NUM, STR, PUN, END } k = END;
    std::string s;
    int64_t n = 0;
};
struct Lexer {
    std::string src;
    size_t p = 0;
    std::vector<Tok> out;
    explicit Lexer(std::string t) : src(std::move(t)) {}
    void run() {
        while (p < src.size()) {
            char c = src[p];
            if (isspace(static_cast<unsigned char>(c))) { ++p; continue; }
            if (c == '/' && p + 1 < src.size() && src[p + 1] == '/') { while (p < src.size() && src[p] != '\n') ++p; continue; }
            if (c == '/' && p + 1 < src.size() && src[p + 1] == '*') { p += 2; while (p + 1 < src.size() && !(src[p] == '*' && src[p + 1] == '/')) ++p; p += 2; continue; }
            if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
                size_t q = p;
                while (q < src.size() && (isalnum(static_cast<unsigned char>(src[q])) || src[q] == '_')) ++q;
                out.push_back({Tok::ID, src.substr(p, q - p), 0});
                p = q;
                continue;
            }
            if (isdigit(static_cast<unsigned char>(c)) ||
                (c == '.' && p + 1 < src.size() && isdigit(static_cast<unsigned char>(src[p + 1])))) {
                int64_t v = 0;
                if (c == '0' && p + 1 < src.size() && (src[p + 1] == 'x' || src[p + 1] == 'X')) {
                    p += 2;
                    while (p < src.size() && isxdigit(static_cast<unsigned char>(src[p]))) {
                        char h = src[p++];
                        v = v * 16 + (h <= '9' ? h - '0' : (tolower(h) - 'a' + 10));
                    }
                } else {
                    while (p < src.size() && isdigit(static_cast<unsigned char>(src[p]))) v = v * 10 + (src[p++] - '0');
                }
                out.push_back({Tok::NUM, "", v});
                continue;
            }
            if (c == '"' || c == '\'') {
                char q = c;
                ++p;
                std::string s;
                while (p < src.size() && src[p] != q) {
                    if (src[p] == '\\' && p + 1 < src.size()) { ++p; s += src[p++]; }
                    else s += src[p++];
                }
                ++p;  // closing quote
                out.push_back({Tok::STR, s, 0});
                continue;
            }
            // punctuation (longest first)
            static const char* puns3[] = {"<<=", ">>=", nullptr};
            static const char* puns2[] = {"(:", ":)", "->", "++", "--", "+=", "-=", "*=", "&=", "|=", "<<", ">>", "<=", ">=", "==", "!=", nullptr};
            bool matched = false;
            for (int i = 0; puns3[i]; ++i) {
                if (src.compare(p, 3, puns3[i]) == 0) { out.push_back({Tok::PUN, puns3[i], 0}); p += 3; matched = true; break; }
            }
            if (!matched)
                for (int i = 0; puns2[i]; ++i) {
                    if (src.compare(p, 2, puns2[i]) == 0) { out.push_back({Tok::PUN, puns2[i], 0}); p += 2; matched = true; break; }
                }
            if (!matched) { out.push_back({Tok::PUN, std::string(1, c), 0}); ++p; }
        }
        out.push_back({Tok::END, "", 0});
    }
};

// ---- parser ------------------------------------------------------------
struct Parser {
    std::vector<Tok> t;
    size_t p = 0;
    explicit Parser(std::vector<Tok> toks) : t(std::move(toks)) {}
    const Tok& cur() const { return t[p]; }
    bool isPun(const char* s) const { return cur().k == Tok::PUN && cur().s == s; }
    bool eat(const char* s) {
        if (isPun(s)) { ++p; return true; }
        return false;
    }
    bool eatId(const char* s) {
        if (cur().k == Tok::ID && cur().s == s) { ++p; return true; }
        return false;
    }
    [[noreturn]] void fail(const std::string& what) const {
        throw std::runtime_error("minilpc: " + what + " near token '" + cur().s + "' #" + std::to_string(p));
    }
    std::string wantId() {
        if (cur().k != Tok::ID) fail("identifier expected");
        return t[p++].s;
    }

    NodeP node(Node::Kind k) { auto n = std::make_shared<Node>(); n->k = k; return n; }

    // assignment expression
    NodeP parseExpr() { return parseAssign(); }
    NodeP parseAssign() {
        NodeP lhs = parseBin(0);
        const char* ops[] = {"=", "+=", "&=", "*=", "|=", "-=", "<<=", ">>=", nullptr};
        for (int i = 0; ops[i]; ++i) {
            if (isPun(ops[i])) {
                ++p;
                NodeP rhs = parseAssign();
                if (lhs->k == Node::VAR && !lhs->assignOp.empty()) fail("chained assignment");
                if (lhs->k != Node::VAR && lhs->k != Node::INDEX) fail("assignment target");
                lhs->assignOp = ops[i];
                lhs->rhs = rhs;
                return lhs;
            }
        }
        return lhs;
    }
    static int precOf(const std::string& op) {
        if (op == "<" || op == ">") return 1;
        if (op == "<<" || op == ">>") return 2;
        if (op == "&") return 3;
        if (op == "+" || op == "-") return 4;
        if (op == "*" || op == "%" || op == "/") return 5;
        return 0;
    }
    NodeP parseBin(int minPrec) {
        NodeP lhs = parseUnary();
        while (true) {
            // 'upto' terminates the start expression of loop(i = 0 upto N)
            if (cur().k == Tok::ID && cur().s == "upto") break;
            if (cur().k != Tok::PUN) break;
            int pr = precOf(cur().s);
            if (pr < minPrec || pr == 0) break;
            std::string op = t[p++].s;
            NodeP rhs = parseBin(pr + 1);
            NodeP n = node(Node::BIN);
            n->str = op;
            n->kids.push_back(lhs);
            n->kids.push_back(rhs);
            lhs = n;
        }
        return lhs;
    }
    NodeP parseUnary() {
        if (isPun("-")) { ++p; NodeP n = node(Node::UN); n->str = "-"; n->kids.push_back(parseUnary()); return n; }
        if (isPun("++") || isPun("--")) {
            std::string op = t[p++].s;
            NodeP tgt = parseUnary();
            if (tgt->k != Node::VAR) fail("++/-- target");
            tgt->incDec = (op == "++") ? 1 : -1;
            tgt->post = false;
            return tgt;
        }
        return parsePostfix();
    }
    NodeP parsePostfix() {
        NodeP e = parsePrimary();
        while (true) {
            if (isPun("(") && (e->k == Node::VAR)) {
                // call of a named function / efun
                ++p;
                NodeP c = node(Node::CALL);
                c->str = e->str;
                if (!isPun(")"))
                    while (true) {
                        c->kids.push_back(parseExpr());
                        if (!eat(",")) break;
                    }
                if (!eat(")")) fail(") after args");
                e = c;
            } else if (isPun("->")) {
                ++p;
                std::string fn = wantId();
                NodeP c = node(Node::CALL);
                c->str = fn;
                c->arrow = true;
                c->kids.push_back(e);  // base object (self tag)
                if (eat("(")) {
                    if (!isPun(")"))
                        while (true) {
                            c->kids.push_back(parseExpr());
                            if (!eat(",")) break;
                        }
                    if (!eat(")")) fail(") after args");
                }
                e = c;
            } else if (isPun("[")) {
                ++p;
                NodeP idx = parseExpr();
                if (!eat("]")) fail("] after index");
                NodeP n = node(Node::INDEX);
                n->kids.push_back(e);
                n->kids.push_back(idx);
                e = n;
            } else if (isPun("++") || isPun("--")) {
                if (e->k != Node::VAR) fail("++/-- target");
                e->incDec = (cur().s == "++") ? 1 : -1;
                e->post = true;
                ++p;
            } else break;
        }
        return e;
    }
    NodeP parsePrimary() {
        if (cur().k == Tok::NUM) { NodeP n = node(Node::NUM); n->num = t[p++].n; return n; }
        if (cur().k == Tok::STR) { NodeP n = node(Node::STR); n->str = t[p++].s; return n; }
        if (isPun("(")) {
            ++p;
            // (int) expr cast - the generator emits
            //   x = (y + (int) this_object()->fd(y + 3749, y + 4704)) & mask;
            // consume the type name + ')' and continue with the operand; our
            // arithmetic coerces through toInt anyway.
            if (cur().k == Tok::ID &&
                (cur().s == "int" || cur().s == "string" || cur().s == "mixed" ||
                 cur().s == "object" || cur().s == "void") &&
                p + 1 < t.size() && t[p + 1].k == Tok::PUN && t[p + 1].s == ")") {
                ++p;  // type name
                ++p;  // ')'
                return parseUnary();
            }
            // ({ ... }) array literal - the lexer splits '({' into '(' + '{'
            if (isPun("{")) {
                ++p;
                NodeP n = node(Node::ARR);
                if (!isPun("}"))
                    while (true) {
                        n->kids.push_back(parseExpr());
                        if (!eat(",")) break;
                    }
                if (!eat("}")) fail("} after array");
                if (!eat(")")) fail(") after array");
                return n;
            }
            // ([ ... ]) mapping literal
            if (isPun("[")) {
                ++p;
                NodeP n = node(Node::MAPLIT);
                if (!isPun("]"))
                    while (true) {
                        n->kids.push_back(parseExpr());
                        if (!eat(":")) fail(": in mapping");
                        n->kids.push_back(parseExpr());
                        if (!eat(",")) break;
                    }
                if (!eat("]")) fail("] after mapping");
                if (!eat(")")) fail(") after mapping");
                return n;
            }
            NodeP e = parseExpr();
            if (!eat(")")) fail(") after (");
            return e;
        }
        if (isPun("(:")) {
            ++p;
            NodeP n = node(Node::CLO);
            if (cur().k != Tok::ID) fail("closure function");
            n->str = t[p++].s;
            while (!isPun(":)")) {
                if (cur().k == Tok::END) fail("unclosed (: ");
                n->kids.push_back(parseExpr());  // bound args (also plain ids)
                // (: sprintf, "%d" :) - the anticheat generator emits efun
                // closures with COMMA-separated bound args (sort(({x, ...}),
                // (: sprintf, "%d" :))).  Without eating the comma the parser
                // failed "expression near token ','" and 13 of ~15 observed
                // challenge scripts went SRC-FAIL -> wrong answers sent back ->
                // the anticheat blocked the character's combat (the freeze).
                if (!eat(",")) break;
            }
            ++p;  // :)
            return n;
        }
        if (cur().k == Tok::ID) {
            std::string id = t[p++].s;
            if (id == "this_object") {
                if (!eat("(") || !eat(")")) fail("this_object()");
                return node(Node::SELF);
            }
            NodeP n = node(Node::VAR);
            n->str = id;
            return n;
        }
        fail("expression");
    }

    StmtP stmt() {
        auto s = std::make_shared<Stmt>();
        if (isPun("{")) {
            ++p;
            s->k = Stmt::BLOCK;
            while (!isPun("}")) {
                if (cur().k == Tok::END) fail("unclosed block");
                s->stmts.push_back(stmt());
            }
            ++p;
            return s;
        }
        if (eatId("return")) {
            s->k = Stmt::RET;
            if (!isPun(";")) s->expr = parseExpr();
            if (!eat(";")) fail("; after return");
            return s;
        }
        if (eatId("loop")) {
            if (!eat("(")) fail("( after loop");
            // Two forms: loop (var in expr)          - element iteration
            //            loop (var = start upto end) - numeric, INCLUSIVE bound
            std::string var = wantId();
            if (eat("=")) {
                NodeP startE = parseExpr();
                if (!eatId("upto")) fail("upto in numeric loop");
                NodeP endE = parseExpr();
                if (!eat(")")) fail(") after loop header");
                auto s2 = std::make_shared<Stmt>();
                s2->k = Stmt::LOOPUPTO;
                s2->name = var;
                s2->expr = endE;
                s2->start = startE;
                s2->stmts.push_back(stmt());
                return s2;
            }
            if (!eatId("in")) fail("in after loop var");
            auto s2 = std::make_shared<Stmt>();
            s2->k = Stmt::LOOPIN;
            s2->name = var;
            s2->expr = parseExpr();
            if (!eat(")")) fail(") after loop header");
            s2->stmts.push_back(stmt());
            return s2;
        }
        if ((cur().k == Tok::ID) && (cur().s == "mixed" || cur().s == "int" || cur().s == "string" || cur().s == "object")) {
            // Declaration - possibly with multiple declarators:
            //   mixed i, mks, mask, result;
            auto first = std::make_shared<Stmt>();
            first->k = Stmt::DECL;
            std::string type = t[p++].s;
            first->name = wantId();
            first->declType = type;
            if (eat("=")) first->expr = parseExpr();
            if (!isPun(",")) {
                if (!eat(";")) fail("; after decl");
                return first;
            }
            auto blk = std::make_shared<Stmt>();
            blk->k = Stmt::BLOCK;
            blk->stmts.push_back(first);
            while (eat(",")) {
                auto d = std::make_shared<Stmt>();
                d->k = Stmt::DECL;
                d->name = wantId();
                d->declType = type;
                if (eat("=")) d->expr = parseExpr();
                blk->stmts.push_back(d);
            }
            if (!eat(";")) fail("; after decl");
            return blk;
        }
        s->k = Stmt::EXPRS;
        s->expr = parseExpr();
        if (!eat(";")) fail("; after expression");
        return s;
    }

    std::shared_ptr<Prog> program() {
        auto pg = std::make_shared<Prog>();
        while (cur().k != Tok::END) {
            eatId("override");
            if (cur().k != Tok::ID || (cur().s != "mixed" && cur().s != "int" && cur().s != "string" && cur().s != "object" && cur().s != "void"))
                fail("type at top level");
            std::string type = t[p++].s;
            std::string name = wantId();
            if (isPun("(")) {
                ++p;
                MiniFn fn;
                if (!isPun(")"))
                    while (true) {
                        if (cur().k == Tok::ID && (cur().s == "int" || cur().s == "mixed" || cur().s == "string" || cur().s == "object")) ++p;
                        std::string pn = wantId();
                        NodeP def;
                        if (eat("=")) def = parseExpr();
                        fn.params.push_back({pn, def});
                        if (!eat(",")) break;
                    }
                if (!eat(")")) fail(") after params");
                fn.body = stmt();
                pg->fns[name] = fn;
            } else {
                // global variable
                NodeP init;
                if (eat("=")) init = parseExpr();
                if (!eat(";")) fail("; after global");
                pg->globalInits.push_back({name, init});
            }
        }
        return pg;
    }
};

// ---- runtime -----------------------------------------------------------
struct Interp {
    std::shared_ptr<Prog> pg;
    std::map<std::string, Value> locals[32];
    int depth = 0;
    int64_t calls = 0;

    static Value selfVal() {
        auto a = std::make_shared<ArrayValue>();
        a->items.push_back(make_string("\x01MINILPC_SELF"));
        return a;
    }
    static bool isSelf(const Value& v) {
        if (v.index() != 5) return false;
        auto a = std::get<std::shared_ptr<ArrayValue>>(v);
        return a && !a->items.empty() && a->items[0].index() == 4 &&
               std::get<SharedString>(a->items[0]).str() == "\x01MINILPC_SELF";
    }

    [[noreturn]] void fail(const std::string& m) const { throw std::runtime_error("minilpc rt: " + m); }

    Value* lvalueRef(const NodeP& n, std::map<std::string, Value>& scope) {
        if (n->k == Node::VAR) {
            auto it = scope.find(n->str);
            if (it == scope.end()) {
                auto g = pg->globals.find(n->str);
                if (g != pg->globals.end()) return &g->second;
                fail("undefined variable " + n->str);
            }
            return &it->second;
        }
        if (n->k == Node::INDEX) {
            // containers live in scope/globals; fetch base container then index
            Value base = eval(n->kids[0], scope);
            (void)base;
            fail("indexed lvalue unsupported");
        }
        fail("bad lvalue");
    }

    static int64_t toInt(const Value& v) {
        if (v.index() == 2) return std::get<int64_t>(v);
        if (v.index() == 3) return static_cast<int64_t>(std::get<double>(v));
        if (v.index() == 4) {
            const std::string& s = std::get<SharedString>(v).str();
            return strtoll(s.c_str(), nullptr, 10);
        }
        return 0;
    }

    static int cmpValues(const Value& a, const Value& b) {
        if (a.index() == 4 && b.index() == 4) {
            const int c = std::get<SharedString>(a).str().compare(std::get<SharedString>(b).str());
            return c < 0 ? -1 : (c > 0 ? 1 : 0);
        }
        if (a.index() == 2 && b.index() == 2) {
            const int64_t x = std::get<int64_t>(a), y = std::get<int64_t>(b);
            return x < y ? -1 : (x > y ? 1 : 0);
        }
        const int64_t x = toInt(a), y = toInt(b);
        return x < y ? -1 : (x > y ? 1 : 0);
    }

    std::string sprintfImpl(const std::vector<Value>& a) {
        if (a.empty() || a[0].index() != 4) return "";
        const std::string& f = std::get<SharedString>(a[0]).str();
        std::string out;
        size_t ai = 1;
        for (size_t i = 0; i < f.size(); ++i) {
            if (f[i] != '%') { out += f[i]; continue; }
            size_t j = i + 1;
            if (j < f.size() && f[j] == '%') { out += '%'; i = j; continue; }
            std::string flags, width;
            while (j < f.size() && (f[j] == '0' || f[j] == '-' || f[j] == '+')) flags += f[j++];
            while (j < f.size() && isdigit(static_cast<unsigned char>(f[j]))) width += f[j++];
            if (j >= f.size()) break;
            char conv = f[j];
            i = j;
            Value v = ai < a.size() ? a[ai++] : Value{};
            char buf[64];
            std::string piece;
            switch (conv) {
                case 'd':
                    snprintf(buf, sizeof buf, "%lld", static_cast<long long>(toInt(v)));
                    piece = buf;
                    break;
                case 'x': case 'X': {
                    snprintf(buf, sizeof buf, conv == 'x' ? "%llx" : "%llX",
                             static_cast<long long>(toInt(v)));
                    piece = buf;
                    break;
                }
                case 's':
                    piece = v.index() == 4 ? std::get<SharedString>(v).str() : "";
                    break;
                case 'f':
                    snprintf(buf, sizeof buf, "%f", v.index() == 3 ? std::get<double>(v) : static_cast<double>(toInt(v)));
                    piece = buf;
                    break;
                default:
                    piece += '%';
                    piece += conv;
                    break;
            }
            if (!width.empty()) {
                size_t w = static_cast<size_t>(strtoul(width.c_str(), nullptr, 10));
                if (piece.size() < w) {
                    char fill = flags.find('0') != std::string::npos ? '0' : ' ';
                    piece = std::string(w - piece.size(), fill) + piece;
                }
            }
            out += piece;
        }
        return out;
    }

    Value callNamed(const std::string& fn, std::vector<Value> args, std::map<std::string, Value>& scope) {
        // builtin efuns used by challenge scripts
        if (fn == "sprintf") return make_string(sprintfImpl(args));
        if (fn == "strlen") {
            return make_int(args.empty() || args[0].index() != 4 ? 0
                             : static_cast<int64_t>(std::get<SharedString>(args[0]).str().size()));
        }
        if (fn == "sizeof") {
            if (args.empty()) return make_int(0);
            if (args[0].index() == 5) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(args[0])->items.size()));
            if (args[0].index() == 6) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(args[0])->size()));
            return make_int(0);
        }
        if (fn == "toupper" || fn == "tolower") {
            if (args.empty() || args[0].index() != 4) return make_string("");
            std::string s = std::get<SharedString>(args[0]).str();
            for (char& c : s) c = (fn == "toupper") ? static_cast<char>(toupper(static_cast<unsigned char>(c))) : static_cast<char>(tolower(static_cast<unsigned char>(c)));
            return make_string(s);
        }
        if (fn == "values") {
            if (args.empty() || args[0].index() != 6) return make_array();
            auto out = std::make_shared<ArrayValue>();
            for (auto& kv : std::get<std::shared_ptr<MappingValue>>(args[0])->items()) out->items.push_back(kv.second);
            return out;
        }
        if (fn == "keys") {
            if (args.empty() || args[0].index() != 6) return make_array();
            auto out = std::make_shared<ArrayValue>();
            for (auto& kv : std::get<std::shared_ptr<MappingValue>>(args[0])->items()) out->items.push_back(kv.first);
            return out;
        }
        if (fn == "allocate_mapping") return std::make_shared<MappingValue>();
        if (fn == "count_total") {
            // count_total(array) = SUM of the (numeric) elements - the game's
            // own usage proves it: zhenfa sums skill levels
            // (count_total(members->query_zhenfa_skill_level())), monsterd
            // counts players (count_total(obs->is_player()) over 1/0 marks),
            // family_guard counts entries (count_total(values(map))).
            int64_t sum = 0;
            if (!args.empty() && args[0].index() == 5)
                for (auto& it : std::get<std::shared_ptr<ArrayValue>>(args[0])->items) sum += toInt(it);
            return make_int(sum);
        }
        if (fn == "sort") {
            if (args.empty() || args[0].index() != 5) return make_array();
            auto items = std::get<std::shared_ptr<ArrayValue>>(args[0])->items;
            std::vector<std::pair<Value, Value>> keyed(items.size());
            if (args.size() >= 2 && args[1].index() == 8) {
                auto cl = std::get<std::shared_ptr<ClosureValue>>(args[1]);
                for (size_t i = 0; i < items.size(); ++i) {
                    std::vector<Value> call;
                    call.push_back(items[i]);
                    keyed[i] = {callClosure(cl, call), items[i]};
                }
            } else {
                for (size_t i = 0; i < items.size(); ++i) keyed[i] = {items[i], items[i]};
            }
            std::stable_sort(keyed.begin(), keyed.end(), [](const std::pair<Value, Value>& a,
                                                            const std::pair<Value, Value>& b) {
                return cmpValues(a.first, b.first) < 0;
            });
            auto out = std::make_shared<ArrayValue>();
            for (auto& kv : keyed) out->items.push_back(std::move(kv.second));
            return out;
        }
        if (fn == "this_object") return selfVal();
        // user function in this program
        auto it = pg->fns.find(fn);
        if (it != pg->fns.end()) return callUser(it->second, args);
        fail("unknown function " + fn);
    }

    // (: name, bound... :) - builtin or user function with bound leading args
    Value callClosure(const std::shared_ptr<ClosureValue>& cl, std::vector<Value> args) {
        std::vector<Value> all = cl->boundArgs;
        all.insert(all.end(), args.begin(), args.end());
        return callNamed(cl->functionName, all, locals[0]);
    }

    struct Res {
        Value v;
        bool ret = false;
    };

    Value callUser(const MiniFn& fn, const std::vector<Value>& args) {
        if (++depth > 64) fail("call depth");
        if (++calls > 200000) fail("call budget");
        std::map<std::string, Value> saved;
        const int li = depth % 32;
        locals[li].swap(saved);
        for (size_t i = 0; i < fn.params.size(); ++i) {
            if (i < args.size()) locals[li][fn.params[i].first] = args[i];
            else if (fn.params[i].second) locals[li][fn.params[i].first] = eval(fn.params[i].second, locals[li]);
            else locals[li][fn.params[i].first] = make_int(0);
        }
        Res r = execBlock(fn.body, locals[li]);
        locals[li].swap(saved);
        --depth;
        return r.ret ? r.v : Value{};
    }

    Res execBlock(const StmtP& s, std::map<std::string, Value>& scope) {
        switch (s->k) {
            case Stmt::BLOCK: {
                for (auto& st : s->stmts) {
                    Res r = execBlock(st, scope);
                    if (r.ret) return r;
                }
                return Res{};
            }
            case Stmt::RET:
                return {s->expr ? eval(s->expr, scope) : Value{}, true};
            case Stmt::DECL: {
                scope[s->name] = s->expr ? eval(s->expr, scope) : make_int(0);
                return Res{};
            }
            case Stmt::LOOPIN: {
                Value seq = eval(s->expr, scope);
                if (seq.index() == 5) {
                    auto items = std::get<std::shared_ptr<ArrayValue>>(seq)->items;  // copy: body may mutate
                    for (auto& it : items) {
                        scope[s->name] = it;
                        for (auto& st : s->stmts) {
                            Res r = execBlock(st, scope);
                            if (r.ret) return r;
                        }
                    }
                } else if (seq.index() == 6) {
                    // loop (v in mapping) yields a [key, value] PAIR per
                    // iteration - the generator template does
                    //     loop (mks in m)  result += mks[0] + mks[1];
                    // with int keys and int values, so the only consistent
                    // original-VM semantics is a two-element array per step.
                    auto kvs = std::get<std::shared_ptr<MappingValue>>(seq)->items();  // copy
                    for (auto& kv : kvs) {
                        auto pair = std::make_shared<ArrayValue>();
                        pair->items.push_back(kv.first);
                        pair->items.push_back(kv.second);
                        scope[s->name] = pair;
                        for (auto& st : s->stmts) {
                            Res r = execBlock(st, scope);
                            if (r.ret) return r;
                        }
                    }
                }
                return Res{};
            }
            case Stmt::LOOPUPTO: {
                // loop (i = start upto end): INCLUSIVE bound - the generator's
                // "0 upto 1 + ((x - y) & 3)" runs i = 0 .. 1+((x-y)&3).
                Value endV = eval(s->expr, scope);
                int64_t end = toInt(endV);
                for (int64_t i = toInt(eval(s->start, scope)); i <= end; ++i) {
                    scope[s->name] = make_int(i);
                    for (auto& st : s->stmts) {
                        Res r = execBlock(st, scope);
                        if (r.ret) return r;
                    }
                }
                return Res{};
            }
            case Stmt::EXPRS:
                eval(s->expr, scope);
                return Res{};
        }
        return Res{};
    }

    static void mapSet(const Value& mv, const Value& key, const Value& val) {
        if (mv.index() != 6) return;
        std::get<std::shared_ptr<MappingValue>>(mv)->insert(key, val);
    }
    static const Value* mapGet(const Value& mv, const Value& key) {
        if (mv.index() != 6) return nullptr;
        return std::get<std::shared_ptr<MappingValue>>(mv)->find(key);
    }

    Value applyAssign(const NodeP& np, std::map<std::string, Value>& scope) {
        Node* n = np.get();
        if (n->k == Node::VAR) {
            Value* lv = lvalueRef(np, scope);
            Value nv;
            if (n->assignOp == "=") nv = eval(n->rhs, scope);
            else if (n->assignOp == "+=") {
                Value r = eval(n->rhs, scope);
                // array += array concatenates (the generator template does
                //   a += ({ (y - y + i + 37) & mask });
                if (lv->index() == 5 && r.index() == 5) {
                    auto dst = std::get<std::shared_ptr<ArrayValue>>(*lv);
                    auto src = std::get<std::shared_ptr<ArrayValue>>(r);
                    if (dst && src)
                        for (auto& it : src->items) dst->items.push_back(it);
                    nv = *lv;
                } else if (lv->index() == 4 && r.index() == 4) {
                    // string += string: s += sprintf("%02X", x) - the generator
                    // builds the challenge answer through string concatenation.
                    nv = make_string(std::get<SharedString>(*lv).str() +
                                     std::get<SharedString>(r).str());
                } else {
                    nv = make_int(toInt(*lv) + toInt(r));
                }
            }
            else if (n->assignOp == "-=") nv = make_int(toInt(*lv) - toInt(eval(n->rhs, scope)));
            else if (n->assignOp == "*=") nv = make_int(toInt(*lv) * toInt(eval(n->rhs, scope)));
            else if (n->assignOp == "&=") nv = make_int(toInt(*lv) & toInt(eval(n->rhs, scope)));
            else if (n->assignOp == "|=") nv = make_int(toInt(*lv) | toInt(eval(n->rhs, scope)));
            else if (n->assignOp == "<<=") nv = make_int(toInt(*lv) << toInt(eval(n->rhs, scope)));
            else if (n->assignOp == ">>=") nv = make_int(toInt(*lv) >> toInt(eval(n->rhs, scope)));
            else fail("assign op " + n->assignOp);
            *lv = nv;
            return nv;
        }
        if (n->k == Node::INDEX) {
            // m[k] = v / arr[i] = v: the container behind the shared_ptr IS the
            // stored container, so mutating through the evaluated copy works.
            Value base = eval(n->kids[0], scope);
            Value key = eval(n->kids[1], scope);
            Value val = eval(n->rhs, scope);
            if (base.index() == 6) {
                mapSet(base, key, val);
                return val;
            }
            if (base.index() == 5) {
                auto& items = std::get<std::shared_ptr<ArrayValue>>(base)->items;
                int64_t i = toInt(key);
                if (i >= 0 && static_cast<size_t>(i) < items.size()) items[static_cast<size_t>(i)] = val;
                return val;
            }
            fail("index assign on non-container");
        }
        fail("assign target");
    }

    Value eval(const NodeP& np, std::map<std::string, Value>& scope) {
        Node* n = np.get();
        switch (n->k) {
            case Node::NUM: return make_int(n->num);
            case Node::STR: return make_string(n->str);
            case Node::VAR: {
                if (n->assignOp.empty() && n->incDec) {
                    Value* lv = lvalueRef(np, scope);
                    int64_t v = toInt(*lv);
                    *lv = make_int(v + n->incDec);
                    return n->post ? make_int(v) : *lv;
                }
                if (!n->assignOp.empty()) return applyAssign(np, scope);
                auto it = scope.find(n->str);
                if (it != scope.end()) return it->second;
                auto g = pg->globals.find(n->str);
                if (g != pg->globals.end()) return g->second;
                fail("undefined variable " + n->str);
            }
            case Node::BIN: {
                const int64_t l = toInt(eval(n->kids[0], scope));
                const int64_t r = toInt(eval(n->kids[1], scope));
                const std::string& op = n->str;
                if (op == "+") return make_int(l + r);
                if (op == "-") return make_int(l - r);
                if (op == "*") return make_int(l * r);
                if (op == "%") return make_int(r ? l % r : 0);
                if (op == "/") return make_int(r ? l / r : 0);
                if (op == "&") return make_int(l & r);
                if (op == "<<") return make_int(l << r);
                if (op == ">>") return make_int(l >> r);
                if (op == "<") return make_int(l < r ? 1 : 0);
                if (op == ">") return make_int(l > r ? 1 : 0);
                fail("binop " + op);
            }
            case Node::UN:
                return make_int(-toInt(eval(n->kids[0], scope)));
            case Node::ARR: {
                auto a = std::make_shared<ArrayValue>();
                for (auto& k : n->kids) a->items.push_back(eval(k, scope));
                return a;
            }
            case Node::MAPLIT: {
                auto m = std::make_shared<MappingValue>();
                Value mv = m;
                for (size_t i = 0; i + 1 < n->kids.size(); i += 2)
                    mapSet(mv, eval(n->kids[i], scope), eval(n->kids[i + 1], scope));
                return mv;
            }
            case Node::CLO: {
                auto cl = std::make_shared<ClosureValue>();
                cl->functionName = n->str;
                for (auto& k : n->kids) cl->boundArgs.push_back(eval(k, scope));
                return cl;
            }
            case Node::SELF: return selfVal();
            case Node::CALL: {
                if (n->arrow) {
                    // base->fn(args): the challenge script only ever calls its
                    // OWN functions through this_object() - verify the tag.
                    Value base = eval(n->kids[0], scope);
                    if (!isSelf(base)) fail("-> call on non-self");
                    std::vector<Value> args;
                    for (size_t i = 1; i < n->kids.size(); ++i) args.push_back(eval(n->kids[i], scope));
                    return callNamed(n->str, args, scope);
                }
                std::vector<Value> args;
                for (auto& k : n->kids) args.push_back(eval(k, scope));
                return callNamed(n->str, args, scope);
            }
            case Node::INDEX: {
                Value base = eval(n->kids[0], scope);
                Value key = eval(n->kids[1], scope);
                if (base.index() == 5) {
                    auto& items = std::get<std::shared_ptr<ArrayValue>>(base)->items;
                    int64_t i = toInt(key);
                    if (i < 0 || static_cast<size_t>(i) >= items.size()) return make_int(0);
                    return items[static_cast<size_t>(i)];
                }
                if (base.index() == 6) {
                    const Value* v = mapGet(base, key);
                    return v ? *v : Value{};
                }
                if (base.index() == 4) {
                    const std::string& s = std::get<SharedString>(base).str();
                    int64_t i = toInt(key);
                    if (i < 0 || static_cast<size_t>(i) >= s.size()) return make_int(0);
                    return make_int(static_cast<unsigned char>(s[static_cast<size_t>(i)]));
                }
                return make_int(0);
            }
        }
        return Value{};
    }
};

std::mutex& miniMx() {
    static std::mutex m;
    return m;
}
std::unordered_map<std::string, std::shared_ptr<Prog>>& miniProgs() {
    static std::unordered_map<std::string, std::shared_ptr<Prog>> m;
    return m;
}

// Parse LPC source text.  Throws std::runtime_error on syntax errors.
std::shared_ptr<Prog> parseSource(const std::string& text) {
    Lexer lx(text);
    lx.run();
    Parser ps(std::move(lx.out));
    return ps.program();
}

// Evaluate function `fn` of the program registered under `key`.
Value callMini(const std::string& key, const std::string& fn, const std::vector<Value>& args) {
    std::shared_ptr<Prog> pg;
    {
        std::lock_guard<std::mutex> lk(miniMx());
        auto it = miniProgs().find(key);
        if (it == miniProgs().end()) return Value{};
        pg = it->second;
    }
    auto fit = pg->fns.find(fn);
    if (fit == pg->fns.end()) return Value{};
    Interp ip;
    ip.pg = pg;
    return ip.callUser(fit->second, args);
}

bool hasMini(const std::string& key) {
    std::lock_guard<std::mutex> lk(miniMx());
    return miniProgs().count(key) != 0;
}
void registerMini(const std::string& key, std::shared_ptr<Prog> pg) {
    std::lock_guard<std::mutex> lk(miniMx());
    miniProgs()[key] = std::move(pg);
}
void unregisterMini(const std::string& key) {
    std::lock_guard<std::mutex> lk(miniMx());
    miniProgs().erase(key);
}

// True when the content is LPC source text (printable) rather than a binary
// program image.
bool looksLikeSource(const std::string& s) {
    size_t n = s.size() < 64 ? s.size() : 64;
    if (n == 0) return false;
    size_t printable = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == '\n' || c == '\r' || c == '\t' || (c >= 32 && c < 127)) ++printable;
    }
    return printable == n;
}

}  // namespace minilpc
