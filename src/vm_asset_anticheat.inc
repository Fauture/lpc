// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Minimal evaluator for the anti-cheat challenge script the CLIENT sends.
//
// The payload of 0x5108 decodes to LPC SOURCE such as (captured verbatim from
// the real client, [ac-src] in the gs log):
//
//   override mixed fk(int x = 8719, int y = 5219)
//   { return ((sort(({ x, x * 2, x * 3, x * 4 }), (: sprintf, "%d" :))[1] + x)
//             * 2 + y) & 0xFFFF; }
//   override mixed fj(int x = 3813, int y = 279)
//   { return ((sort(({ x, x * 2, x * 3, x * 4 }), (: sprintf, "%d" :))[2] + x)
//             * 2 * y) & 0xFFFF; }
//   int challenge_server(int x, int y) {
//       mixed i, mks, mask, result; mixed m, a, s;
//       mask = 0x7FFFFF;
//       m = allocate_mapping(16);
//       loop (i = 0 upto 1 + ((x + y) & 3)) m[mask & (x + y + i + 70)] =
//           ((x + 540) * (x + 245)) & mask;
//       a = ({ });
//       loop (i = 0 upto 1 + ((y + x) & 3)) a += ({ (x + y + i + 89) & mask });
//       s = "1176";
//       loop (mks in m) m[mks[0]] = (mks[1] + mks[0]) & mask;
//       y = (x + (int) this_object()->ff(x + 2624, y + 7799)) & mask;
//       result = 0;
//       loop (mks in m) result += mks[0] + mks[1];
//       result += count_total(a);
//       result += x + y;
//       result += strlen(s);
//       result &= mask;
//       return result;
//   }
//   int get_x(){ return 307; }
//   int get_y(){ return 417; }
//
// The server must answer me->send(8625, x, y, answer) (0x21B1); the client
// verifies the answer and, when it never arrives, logs itself out after a few
// minutes (the reported 3-5 minute disconnect).  This class parses that subset
// and evaluates it - no compiler needed.
// ---------------------------------------------------------------------------
// Minimal LPC interpreter for the anti-cheat challenge script the CLIENT sends.
//
// The payload of 0x5108 decodes to LPC SOURCE (not a compiled .o), e.g.:
//
//   override mixed fm(int x = 6622, int y = 6817) { return ((sort(({ x, x*2,
//     x*3, x*4 }), (: sprintf, "%d" :))[2] + x) * 2 - y) & 0xFFFF; }
//   int challenge_server(int x, int y) { mixed i, mks, mask, result;
//     mixed m, a, s; mask = 0x7F; m = allocate_mapping(16);
//     loop (i = 0 upto 1 + ((x * y) & 3)) m[mask & (y + x + i + 50)] = ...;
//     a = ({ }); loop (i = 0 upto 1 + ((y + x) & 3)) a += ({ ... });
//     s = "6015"; x = (x + (int) this_object()->fm(x + 7816, y + 8)) & mask;
//     result = 0; loop (mks in m) result += mks[0] + mks[1];
//     result += count_total(a); result += x + y; result += strlen(s);
//     result &= mask; return result; }
//   int get_x(){ return 9170;} int get_y(){ return 8140;}
//
// The server must answer me->send(8625, x, y, answer) (0x21B1); without it the
// client walks itself back to the login screen after a few minutes - the
// reported 3-5 minute disconnect.  check_server.c pushes this source through
// load_object("/challenge.c", 0, src), which our dynamic-object path cannot
// execute, so the script is interpreted here instead.
//
// Ported from _ac_eval.py, which was validated against every challenge script
// captured from the live client (21/21).
class AntiCheatScript {
public:
    struct Tok {
        char kind;          // 'n' number, 's' string, 'i' ident, 'o' operator, 'e' eof
        std::string text;
        int64_t num = 0;
    };
    struct Val {
        char kind = 'i';          // 'i' int, 's' string, 'a' array, 'm' mapping
        int64_t num = 0;
        std::string s;
        std::vector<Val> items;   // array
        std::vector<std::pair<Val, Val>> pairs;   // mapping (insertion order)
        int64_t asInt() const {
            if (kind == 'i') return num;
            if (kind == 's') return static_cast<int64_t>(s.size());
            if (kind == 'a') return static_cast<int64_t>(items.size());
            if (kind == 'm') return static_cast<int64_t>(pairs.size());
            return 0;
        }
    };

    bool parse(const std::string& src) {
        toks_ = lex(src);
        return parseProgram();
    }

    bool eval(const std::string& fn, const std::vector<int64_t>& args, int& out) {
        std::map<std::string, Func>::iterator it = funcs_.find(fn);
        if (it == funcs_.end()) return false;
        std::map<std::string, Val> env;
        for (size_t i = 0; i < it->second.params.size(); ++i)
            env[it->second.params[i]] = V(args.size() > i ? args[i] : it->second.defs[i]);
        Exec ex(this, env);
        Val r = ex.runBody(it->second.body);
        out = static_cast<int>(static_cast<uint32_t>(r.asInt()));
        return true;
    }

private:
    struct Func {
        std::vector<std::string> params;
        std::vector<int64_t> defs;
        std::vector<Tok> body;
    };
    struct Parser {
        std::vector<Tok> t;
        size_t i = 0;
        const Tok& peek(size_t k = 0) const {
            size_t idx = i + k;
            if (idx >= t.size()) idx = t.size() ? t.size() - 1 : 0;
            return t[idx];
        }
        bool at(const std::string& s) const {
            const Tok& tk = peek();
            return (tk.kind == 'o' || tk.kind == 'i') && tk.text == s;
        }
        bool eat(const std::string& s) {
            if (!at(s)) return false;
            ++i;
            return true;
        }
        void next() { if (i + 1 < t.size()) ++i; }
    };

    std::vector<Tok> toks_;
    std::map<std::string, Func> funcs_;

    // ---------------------------------------------------------------- lexer
    static bool isIdStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
    static bool isIdChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

    static std::vector<Tok> lex(const std::string& src) {
        std::vector<Tok> out;
        size_t i = 0;
        const size_t n = src.size();
        while (i < n) {
            char c = src[i];
            if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
            if (std::isdigit(static_cast<unsigned char>(c))) {
                size_t a = i;
                if (c == '0' && i + 1 < n && (src[i + 1] == 'x' || src[i + 1] == 'X')) {
                    i += 2;
                    while (i < n && std::isxdigit(static_cast<unsigned char>(src[i]))) ++i;
                } else {
                    while (i < n && std::isdigit(static_cast<unsigned char>(src[i]))) ++i;
                }
                Tok tk; tk.kind = 'n'; tk.text = src.substr(a, i - a);
                tk.num = static_cast<int64_t>(std::strtoll(tk.text.c_str(), nullptr, 0));
                out.push_back(tk);
                continue;
            }
            if (c == '"') {
                ++i;
                std::string lit;
                while (i < n && src[i] != '"') {
                    if (src[i] == '\\' && i + 1 < n) {
                        ++i;
                        lit += (src[i] == 'n') ? '\n' : (src[i] == 'r' ? '\r' : src[i]);
                        ++i;
                        continue;
                    }
                    lit += src[i++];
                }
                if (i < n) ++i;
                Tok tk; tk.kind = 's'; tk.text = lit;
                out.push_back(tk);
                continue;
            }
            if (isIdStart(c)) {
                size_t a = i;
                while (i < n && isIdChar(src[i])) ++i;
                Tok tk; tk.kind = 'i'; tk.text = src.substr(a, i - a);
                out.push_back(tk);
                continue;
            }
            // closures first: (: ... :)
            if (c == '(' && i + 1 < n && src[i + 1] == ':') {
                Tok tk; tk.kind = 'o'; tk.text = "(:";
                out.push_back(tk);
                i += 2;
                continue;
            }
            if (c == ':' && i + 1 < n && src[i + 1] == ')') {
                Tok tk; tk.kind = 'o'; tk.text = ":)";
                out.push_back(tk);
                i += 2;
                continue;
            }
            static const char* three[] = {"<<=", ">>="};
            bool matched = false;
            for (const char* op : three) {
                if (src.compare(i, 3, op) == 0) {
                    Tok tk; tk.kind = 'o'; tk.text = op;
                    out.push_back(tk);
                    i += 3;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            static const char* two[] = {"+=", "-=", "*=", "/=", "%=", "==", "!=", "<=", ">=",
                                        "&&", "||", "<<", ">>", "++", "--", "->", "&=", "|=", "^="};
            for (const char* op : two) {
                if (src.compare(i, 2, op) == 0) {
                    Tok tk; tk.kind = 'o'; tk.text = op;
                    out.push_back(tk);
                    i += 2;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            {
                Tok tk; tk.kind = 'o'; tk.text = std::string(1, c);
                out.push_back(tk);
                ++i;
            }
        }
        Tok eof; eof.kind = 'e'; eof.text = "";
        out.push_back(eof);
        return out;
    }

    // ---------------------------------------------------------------- parse
    bool parseProgram() {
        Parser p;
        p.t = toks_;
        while (p.peek().kind != 'e') {
            if (p.at("override")) p.next();
            std::vector<Tok> head;
            while (!p.at("{") && p.peek().kind != 'e') head.push_back(p.t[p.i]), p.next();
            if (p.peek().kind == 'e') break;
            std::vector<Tok> body = skipBlock(p);
            // function name = last ident before '('
            size_t paren = std::string::npos;
            for (size_t k = 0; k < head.size(); ++k)
                if (head[k].kind == 'o' && head[k].text == "(") { paren = k; break; }
            if (paren == std::string::npos) continue;
            std::string fname;
            for (size_t k = paren; k-- > 0;) {
                if (head[k].kind == 'i') { fname = head[k].text; break; }
                if (k == 0) break;
            }
            std::vector<Tok> paramToks(head.begin() + static_cast<long>(paren) + 1, head.end());
            Func f;
            parseParams(paramToks, f);
            f.body = body;
            if (!fname.empty()) funcs_[fname] = f;
        }
        return !funcs_.empty();
    }

    std::vector<Tok> skipBlock(Parser& p) {
        std::vector<Tok> out;
        if (!p.at("{")) return out;
        p.next();                       // consume '{'
        int depth = 0;
        while (p.peek().kind != 'e') {
            Tok tk = p.peek();
            if (tk.kind == 'o' && tk.text == "{") ++depth;
            if (tk.kind == 'o' && tk.text == "}") {
                if (depth == 0) { p.next(); break; }
                --depth;
            }
            out.push_back(tk);
            p.next();
        }
        return out;
    }

    static void parseParams(const std::vector<Tok>& toks, Func& f) {
        // stop at the ')' closing our '('
        std::vector<Tok> inner;
        int depth = 1;
        for (size_t k = 0; k < toks.size(); ++k) {
            const Tok& tk = toks[k];
            if (tk.kind == 'o' && tk.text == "(") ++depth;
            else if (tk.kind == 'o' && tk.text == ")") {
                if (--depth == 0) break;
            } else if (tk.kind == 'o' && (tk.text == "{" || tk.text == "[")) ++depth;
            else if (tk.kind == 'o' && (tk.text == "}" || tk.text == "]")) --depth;
            inner.push_back(tk);
        }
        std::vector<std::vector<Tok>> groups;
        std::vector<Tok> cur;
        int d2 = 0;
        for (size_t k = 0; k < inner.size(); ++k) {
            const Tok& tk = inner[k];
            if (tk.kind == 'o' && (tk.text == "(" || tk.text == "[" || tk.text == "{")) ++d2;
            if (tk.kind == 'o' && (tk.text == ")" || tk.text == "]" || tk.text == "}")) --d2;
            if (tk.kind == 'o' && tk.text == "," && d2 == 0) {
                groups.push_back(cur);
                cur.clear();
                continue;
            }
            cur.push_back(tk);
        }
        groups.push_back(cur);
        for (size_t g = 0; g < groups.size(); ++g) {
            const std::vector<Tok>& grp = groups[g];
            if (grp.empty()) continue;
            size_t eq = grp.size();
            for (size_t k = 0; k < grp.size(); ++k)
                if (grp[k].kind == 'o' && grp[k].text == "=") { eq = k; break; }
            int64_t dflt = 0;
            if (eq != grp.size() && eq + 1 < grp.size() && grp[eq + 1].kind == 'n')
                dflt = grp[eq + 1].num;
            std::string name;
            for (size_t k = 0; k < eq && k < grp.size(); ++k)
                if (grp[k].kind == 'i') name = grp[k].text;
            if (!name.empty()) {
                f.params.push_back(name);
                f.defs.push_back(dflt);
            }
        }
    }

    // ------------------------------------------------------------ execution
    static Val V(int64_t n) { Val v; v.kind = 'i'; v.num = n; return v; }
    static Val VS(const std::string& s) {
        Val v; v.kind = 's'; v.s = s; v.num = static_cast<int64_t>(s.size()); return v;
    }
    static Val VARR(std::vector<Val> items) { Val v; v.kind = 'a'; v.items = std::move(items); return v; }
    static Val VMAP() { Val v; v.kind = 'm'; return v; }

    class Exec {
    public:
        // The environment MUST be held BY REFERENCE.  A value member made
        // every loop-body mutation vanish (arrays never grew), which is why
        // the anti-cheat answer was wrong even though 0x21B1 was sent.
        Exec(AntiCheatScript* it, std::map<std::string, Val>& env) : it_(it), env_(env) {}

        Val runBody(const std::vector<Tok>& body) {
            Parser p;
            p.t = body;
            Tok eof; eof.kind = 'e'; eof.text = "";
            p.t.push_back(eof);
            Val result = V(0);
            while (p.peek().kind != 'e') {
                Val r;
                if (stmt(p, r)) result = r;
                else break;
            }
            return result;
        }

    private:
        AntiCheatScript* it_;
        std::map<std::string, Val>& env_;
        // Name of the variable the LAST primary() read - `x++` needs it to know
        // which counter to bump (cleared by every other literal/list access).
        std::string lastIdent_;

        static bool isType(const std::string& s) {
            return s == "mixed" || s == "int" || s == "string" || s == "array" ||
                   s == "mapping" || s == "float" || s == "object" || s == "buffer";
        }

        void mapPut(Val& m, const Val& key, const Val& val) {
            for (size_t k = 0; k < m.pairs.size(); ++k)
                if (m.pairs[k].first.asInt() == key.asInt() && m.pairs[k].first.kind == key.kind) {
                    m.pairs[k].second = val;
                    return;
                }
            m.pairs.push_back(std::make_pair(key, val));
        }
        Val mapGet(const Val& m, const Val& key) {
            for (size_t k = 0; k < m.pairs.size(); ++k)
                if (m.pairs[k].first.asInt() == key.asInt() && m.pairs[k].first.kind == key.kind)
                    return m.pairs[k].second;
            return V(0);
        }
        // `cur += rhs` exactly as the driver's operator does it (array append,
        // string concat, int add).
        static Val addValue(const Val& cur, const Val& rhs) {
            if (cur.kind == 'a' && rhs.kind == 'a') {
                std::vector<Val> items = cur.items;      // rhs may alias cur
                for (size_t k = 0; k < rhs.items.size(); ++k) items.push_back(rhs.items[k]);
                return VARR(items);
            }
            if (cur.kind == 's' || rhs.kind == 's') {
                std::string text = (cur.kind == 's' ? cur.s : std::to_string(cur.num)) +
                                   (rhs.kind == 's' ? rhs.s : std::to_string(rhs.num));
                return VS(text);
            }
            return V(cur.num + rhs.asInt());
        }

        // one loop body: either `{ ... }` or a single statement (the generated
        // loops have NO braces - `loop (...) stmt;`)
        struct LoopBody {
            std::vector<Tok> toks;
            bool isBlock = false;
        };
        LoopBody oneOrBlock(Parser& p) {
            LoopBody lb;
            if (p.at("{")) {
                lb.toks = it_->skipBlock(p);
                lb.isBlock = true;
                return lb;
            }
            size_t start = p.i;
            size_t end = scanStmt(p);
            lb.toks.assign(p.t.begin() + static_cast<long>(start), p.t.begin() + static_cast<long>(end));
            p.i = end;
            return lb;
        }
        // The body MUST run against the caller's environment.  Building a copy
        // (`std::map env = extra;`) discarded every write the body made, so
        // `loop (...) a += ({...});` appended nothing and the answer was wrong.
        void runLoopBody(const LoopBody& lb) {
            Exec ex(it_, env_);
            if (lb.isBlock) {
                ex.runBody(lb.toks);
                return;
            }
            Parser p;
            p.t = lb.toks;
            Tok eof; eof.kind = 'e'; eof.text = "";
            p.t.push_back(eof);
            Val r;
            ex.stmt(p, r);
        }
        size_t scanStmt(Parser& p) {
            int depth = 0;
            while (p.peek().kind != 'e') {
                Tok tk = p.peek();
                if (tk.kind == 'o' && tk.text == "{") {
                    it_->skipBlock(p);
                    if (depth == 0) return p.i;
                    continue;
                }
                if (tk.kind == 'o' && (tk.text == "(" || tk.text == "[")) { ++depth; p.next(); continue; }
                if (tk.kind == 'o' && (tk.text == ")" || tk.text == "]")) {
                    if (depth == 0) return p.i;
                    --depth;
                    p.next();
                    continue;
                }
                p.next();
                if (tk.kind == 'o' && tk.text == ";" && depth <= 0) return p.i;
            }
            return p.i;
        }

        bool stmt(Parser& p, Val& result) {
            const Tok& tk = p.peek();
            if (tk.kind == 'i' && tk.text == "loop") {
                p.next();
                if (!p.eat("(")) return false;
                std::string var = p.peek().text;
                p.next();
                if (p.eat("in")) {
                    std::string coll = p.peek().text;
                    p.next();
                    if (!p.eat(")")) return false;
                    LoopBody lb = oneOrBlock(p);
                    std::map<std::string, Val>::iterator cit = env_.find(coll);
                    std::vector<std::pair<Val, Val>> pairs;
                    if (cit != env_.end() && cit->second.kind == 'm') pairs = cit->second.pairs;
                    for (size_t k = 0; k < pairs.size(); ++k) {
                        std::vector<Val> pairItems;
                        pairItems.push_back(pairs[k].first);
                        pairItems.push_back(pairs[k].second);
                        env_[var] = VARR(pairItems);
                        runLoopBody(lb);
                    }
                    // The loop variable keeps its last value after the loop,
                    // exactly like vm_cmpAndJmpInLoop's final setVar.
                    return true;
                }
                if (!p.eat("=")) return false;
                int64_t a = 0;
                if (!ternary(p, a)) return false;
                if (!p.eat("upto")) return false;
                int64_t b = 0;
                if (!ternary(p, b)) return false;
                if (!p.eat(")")) return false;
                LoopBody lb = oneOrBlock(p);
                // `loop (i = A upto B)`: the compiled form (init_for_loop /
                // cond_jmp_for_loop / step_jmp_for_loop, vm_cmpAndJmpInLoop)
                // keeps its own counter and ASSIGNS the variable on every step
                // (inclusive of B), so a body-side `i++` is overwritten by the
                // next step; the variable is left at B + 1 when the loop exits.
                for (int64_t v = a; v <= b; ++v) {
                    env_[var] = V(v);
                    runLoopBody(lb);
                }
                env_[var] = V(b + 1);
                return true;
            }
            if (tk.kind == 'i' && tk.text == "return") {
                p.next();
                if (!expr(p, result)) return false;
                p.eat(";");
                return true;
            }
            if (tk.kind == 'i' && (tk.text == "break" || tk.text == "continue")) {
                p.next();
                p.eat(";");
                return true;
            }
            if (tk.kind == 'i' && isType(tk.text)) {
                p.next();
                for (;;) {
                    // `mixed i, mks, mask;` are ALL declarators; a following
                    // TYPE keyword (`array a; mixed i;`) starts a new statement.
                    if (p.peek().kind != 'i' || isType(p.peek().text)) break;
                    std::string nm = p.peek().text;
                    p.next();
                    Val v = V(0);
                    if (p.eat("=")) {
                        if (!expr(p, v)) return false;
                    }
                    env_[nm] = v;
                    if (p.eat(",")) continue;
                    break;
                }
                p.eat(";");
                return true;
            }
            // assignment / indexed assignment / expression
            if (tk.kind == 'i') {
                const Tok& nx = p.peek(1);
                if (nx.kind == 'o' && (nx.text == "=" || nx.text == "+=" || nx.text == "-=" ||
                                       nx.text == "&=" || nx.text == "|=" || nx.text == "[" ||
                                       nx.text == "++" || nx.text == "--")) {
                    std::string nm = tk.text;
                    p.next();
                    if (p.at("[")) {
                        p.next();
                        Val key;
                        if (!expr(p, key)) return false;
                        if (!p.eat("]")) return false;
                        // `a[i] = ...` on an ARRAY assigns the ELEMENT.  The old
                        // code replaced any non-mapping with a fresh mapping, so
                        // `a[i] = (a[i] - ...) & mask` threw the store away and
                        // every generated answer came out wrong.
                        Val& dst = env_[nm];
                        if (dst.kind == 'a') {
                            int64_t ix = key.asInt();
                            if (p.eat("=")) {
                                Val val;
                                if (!expr(p, val)) return false;
                                if (ix >= 0 && static_cast<size_t>(ix) < dst.items.size())
                                    dst.items[static_cast<size_t>(ix)] = val;
                                else if (ix == static_cast<int64_t>(dst.items.size()))
                                    dst.items.push_back(val);
                            } else if (p.eat("+=")) {
                                Val val;
                                if (!expr(p, val)) return false;
                                if (ix >= 0 && static_cast<size_t>(ix) < dst.items.size())
                                    dst.items[static_cast<size_t>(ix)] =
                                        addValue(dst.items[static_cast<size_t>(ix)], val);
                            }
                            p.eat(";");
                            return true;
                        }
                        if (dst.kind != 'm') { dst = VMAP(); }
                        if (p.eat("=")) {
                            Val val;
                            if (!expr(p, val)) return false;
                            mapPut(dst, key, val);
                        } else if (p.eat("+=")) {
                            Val val;
                            if (!expr(p, val)) return false;
                            mapPut(dst, key, V(mapGet(dst, key).asInt() + val.asInt()));
                        }
                        p.eat(";");
                        return true;
                    }
                    if (p.eat("=")) {
                        Val v;
                        if (!expr(p, v)) return false;
                        env_[nm] = v;
                        p.eat(";");
                        return true;
                    }
                    if (p.eat("+=")) {
                        Val rhs;
                        if (!expr(p, rhs)) return false;
                        Val& cur = env_[nm];
                        // MUTATE in place.  Rebinding `cur` (cur = VS(...) /
                        // cur = V(...)) threw the update away whenever the body
                        // ran on a copied map, so `loop (...) a += ({...});`
                        // appended nothing and the anti-cheat answer was wrong.
                        if (cur.kind == 'a' && rhs.kind == 'a') {
                            // copy first: rhs may alias cur
                            std::vector<Val> add = rhs.items;
                            for (size_t k = 0; k < add.size(); ++k) cur.items.push_back(add[k]);
                        } else if (cur.kind == 's' || rhs.kind == 's') {
                            std::string add = (rhs.kind == 's') ? rhs.s : std::to_string(rhs.num);
                            cur.s = (cur.kind == 's' ? cur.s : std::to_string(cur.num)) + add;
                            cur.num = static_cast<int64_t>(cur.s.size());
                            cur.kind = 's';
                        } else {
                            cur.num += rhs.asInt();
                        }
                        p.eat(";");
                        return true;
                    }
                    if (p.eat("&=")) {
                        Val rhs; if (!expr(p, rhs)) return false;
                        env_[nm] = V(env_[nm].asInt() & rhs.asInt());
                        p.eat(";");
                        return true;
                    }
                    if (p.eat("|=")) {
                        Val rhs; if (!expr(p, rhs)) return false;
                        env_[nm] = V(env_[nm].asInt() | rhs.asInt());
                        p.eat(";");
                        return true;
                    }
                }
            }
            if (tk.kind == 'o' && tk.text == "{") {
                std::vector<Tok> body = it_->skipBlock(p);
                // Braces must NOT isolate writes: LPC blocks share the enclosing
                // scope, and the generated scripts rely on that.
                Exec ex(it_, env_);
                result = ex.runBody(body);
                return true;
            }
            if (!expr(p, result)) return false;
            p.eat(";");
            return true;
        }

        // ---- expressions ----
        struct Level { const char* ops[6]; };
        bool expr(Parser& p, Val& out) { return assign(p, out); }
        bool assign(Parser& p, Val& out) {
            if (!ternaryVal(p, out)) return false;
            if (p.eat("=")) {
                Val rhs;
                if (!assign(p, rhs)) return false;
                out = rhs;
            }
            return true;
        }
        // NOTE: this must stay Val-valued.  An int64-only variant silently
        // turned `s[x % strlen(s)]` into V(0) (the string was reduced to its
        // length before the index was taken), so every script that indexes its
        // digit string produced a wrong answer.
        bool ternaryVal(Parser& p, Val& out) {
            if (!binary(p, 0, out)) return false;
            if (p.eat("?")) {
                Val a, b;
                if (!ternaryVal(p, a)) return false;
                if (!p.eat(":")) return false;
                if (!ternaryVal(p, b)) return false;
                out = out.asInt() ? a : b;
            }
            return true;
        }
        bool ternary(Parser& p, int64_t& out) {
            Val v;
            if (!ternaryVal(p, v)) return false;
            out = v.asInt();
            return true;
        }
        static const std::vector<std::vector<std::string>>& levels() {
            static const std::vector<std::vector<std::string>> lv = {
                {"||"}, {"&&"}, {"|"}, {"^"}, {"&"},
                {"==", "!="}, {"<", ">", "<=", ">="}, {"<<", ">>"},
                {"+", "-"}, {"*", "/", "%"}
            };
            return lv;
        }
        bool binary(Parser& p, size_t level, Val& out) {
            if (level >= levels().size()) return unary(p, out);
            if (!binary(p, level + 1, out)) return false;
            for (;;) {
                const std::vector<std::string>& ops = levels()[level];
                bool hit = false;
                for (size_t k = 0; k < ops.size(); ++k) {
                    if (p.at(ops[k])) {
                        std::string op = ops[k];
                        p.next();
                        Val rhs;
                        if (!binary(p, level + 1, rhs)) return false;
                        out = applyBin(op, out, rhs);
                        hit = true;
                        break;
                    }
                }
                if (!hit) return true;
            }
        }
        static Val applyBin(const std::string& op, const Val& a, const Val& b) {
            if (op == "+") {
                if (a.kind == 's' || b.kind == 's')
                    return VS((a.kind == 's' ? a.s : std::to_string(a.num)) +
                              (b.kind == 's' ? b.s : std::to_string(b.num)));
                if (a.kind == 'a' && b.kind == 'a') {
                    std::vector<Val> items = a.items;
                    for (size_t k = 0; k < b.items.size(); ++k) items.push_back(b.items[k]);
                    return VARR(items);
                }
                return V(a.asInt() + b.asInt());
            }
            int64_t x = a.asInt(), y = b.asInt();
            if (op == "-") return V(x - y);
            if (op == "*") return V(x * y);
            if (op == "/") return V(y ? x / y : 0);
            if (op == "%") return V(y ? x % y : 0);
            if (op == "&") return V(x & y);
            if (op == "|") return V(x | y);
            if (op == "^") return V(x ^ y);
            if (op == "<<") return V(x << y);
            if (op == ">>") return V(x >> y);
            if (op == "==") return V(x == y);
            if (op == "!=") return V(x != y);
            if (op == "<") return V(x < y);
            if (op == ">") return V(x > y);
            if (op == "<=") return V(x <= y);
            if (op == ">=") return V(x >= y);
            if (op == "&&") return V(x && y);
            if (op == "||") return V(x || y);
            return V(0);
        }
        bool unary(Parser& p, Val& out) {
            if (p.eat("-")) {
                if (!unary(p, out)) return false;
                out = V(-out.asInt());
                return true;
            }
            if (p.eat("!")) {
                if (!unary(p, out)) return false;
                out = V(out.asInt() ? 0 : 1);
                return true;
            }
            if (p.eat("++")) return unary(p, out);
            if (p.at("(")) {
                // cast?  (int) expr
                const Tok& t1 = p.peek(1);
                const Tok& t2 = p.peek(2);
                if (t1.kind == 'i' && isType(t1.text) && t2.kind == 'o' && t2.text == ")") {
                    p.next(); p.next(); p.next();
                    return unary(p, out);
                }
            }
            return postfix(p, out);
        }
        bool postfix(Parser& p, Val& out) {
            if (!primary(p, out)) return false;
            for (;;) {
                if (p.at("++") || p.at("--")) {
                    // `s[i++ % sizeof(s)]` - the generated scripts bump a loop
                    // counter INSIDE an expression.  Remember which variable was
                    // read and apply the side effect now.
                    bool inc = p.at("++");
                    p.next();
                    if (!lastIdent_.empty()) {
                        std::map<std::string, Val>::iterator vit = env_.find(lastIdent_);
                        if (vit != env_.end()) vit->second = V(vit->second.asInt() + (inc ? 1 : -1));
                    }
                    lastIdent_.clear();
                    return true;
                }
                if (p.at("[")) {
                    p.next();
                    Val idx;
                    if (!expr(p, idx)) return false;
                    if (!p.eat("]")) return false;
                    out = index(out, idx);
                    continue;
                }
                if (p.at("->")) {
                    p.next();
                    std::string fn = p.peek().text;
                    p.next();
                    if (!p.eat("(")) return false;
                    std::vector<Val> args;
                    if (!p.at(")")) {
                        for (;;) {
                            Val a;
                            if (!expr(p, a)) return false;
                            args.push_back(a);
                            if (p.eat(",")) continue;
                            break;
                        }
                    }
                    p.eat(")");
                    out = callFn(fn, args);
                    continue;
                }
                if (p.at("(")) {           // builtin call reached through an ident
                    p.next();
                    std::vector<Val> args;
                    if (!p.at(")")) {
                        for (;;) {
                            Val a;
                            if (!expr(p, a)) return false;
                            args.push_back(a);
                            if (p.eat(",")) continue;
                            break;
                        }
                    }
                    p.eat(")");
                    continue;
                }
                return true;
            }
        }
        Val index(const Val& v, const Val& idx) const {
            int64_t i = idx.asInt();
            if (v.kind == 'a') return (i >= 0 && static_cast<size_t>(i) < v.items.size()) ? v.items[static_cast<size_t>(i)] : V(0);
            if (v.kind == 's') return (i >= 0 && static_cast<size_t>(i) < v.s.size())
                                         ? V(static_cast<unsigned char>(v.s[static_cast<size_t>(i)])) : V(0);
            if (v.kind == 'm') {
                for (size_t k = 0; k < v.pairs.size(); ++k)
                    if (v.pairs[k].first.asInt() == i && v.pairs[k].first.kind == idx.kind)
                        return v.pairs[k].second;
                return V(0);
            }
            return V(0);
        }
        Val callFn(const std::string& fn, const std::vector<Val>& args) {
            std::map<std::string, Func>::iterator it = it_->funcs_.find(fn);
            if (it == it_->funcs_.end()) return V(0);
            std::map<std::string, Val> env;
            for (size_t k = 0; k < it->second.params.size(); ++k)
                env[it->second.params[k]] = (k < args.size()) ? args[k] : V(it->second.defs[k]);
            Exec ex(it_, env);
            return ex.runBody(it->second.body);
        }
        Val builtin(const std::string& nm, const std::vector<Val>& args) {
            if (nm == "allocate_mapping") return VMAP();
            if (nm == "count_total") {
                int64_t total = 0;
                for (size_t k = 0; k < args.size(); ++k) {
                    if (args[k].kind == 'a') {
                        for (size_t j = 0; j < args[k].items.size(); ++j) total += args[k].items[j].asInt();
                    } else {
                        total += args[k].asInt();
                    }
                }
                return V(total);
            }
            if (nm == "sizeof") return V(args.empty() ? 0 : args[0].asInt());
            if (nm == "strlen") {
                if (args.empty()) return V(0);
                return V(args[0].kind == 's' ? static_cast<int64_t>(args[0].s.size()) : args[0].asInt());
            }
            if (nm == "keys" || nm == "values") {
                std::vector<Val> items;
                if (!args.empty() && args[0].kind == 'm')
                    for (size_t k = 0; k < args[0].pairs.size(); ++k)
                        items.push_back(nm == "keys" ? args[0].pairs[k].first : args[0].pairs[k].second);
                return VARR(items);
            }
            if (nm == "sort") {
                std::vector<Val> items;
                if (!args.empty() && args[0].kind == 'a') items = args[0].items;
                std::sort(items.begin(), items.end(), [](const Val& a, const Val& b) {
                    return a.asInt() < b.asInt();
                });
                return VARR(items);
            }
            if (nm == "sprintf") {
                std::string fmt = (!args.empty() && args[0].kind == 's') ? args[0].s : std::string();
                std::string text;
                size_t argi = 1;
                for (size_t k = 0; k < fmt.size(); ++k) {
                    if (fmt[k] != '%' || k + 1 >= fmt.size()) { text += fmt[k]; continue; }
                    size_t j = k + 1;
                    std::string spec = "%";
                    while (j < fmt.size() && std::strchr("-+ #0", fmt[j])) spec += fmt[j++];
                    while (j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j]))) spec += fmt[j++];
                    if (j < fmt.size() && fmt[j] == '.') {
                        spec += fmt[j++];
                        while (j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j]))) spec += fmt[j++];
                    }
                    if (j >= fmt.size()) break;
                    char type = fmt[j];
                    spec += type;
                    k = j;
                    int64_t v = (argi < args.size()) ? args[argi++].asInt() : 0;
                    char buf[256];
                    if (type == 's') {
                        snprintf(buf, sizeof(buf), "%s", std::to_string(v).c_str());
                    } else if (type == 'c') {
                        snprintf(buf, sizeof(buf), spec.c_str(), static_cast<int>(v & 0xFF));
                    } else {
                        snprintf(buf, sizeof(buf), spec.c_str(), static_cast<long long>(v));
                    }
                    text += buf;
                }
                return VS(text);
            }
            if (nm == "toupper" || nm == "tolower") {
                std::string s = (!args.empty() && args[0].kind == 's') ? args[0].s : std::string();
                for (size_t k = 0; k < s.size(); ++k)
                    s[k] = static_cast<char>(nm == "toupper" ? std::toupper(static_cast<unsigned char>(s[k]))
                                                            : std::tolower(static_cast<unsigned char>(s[k])));
                return VS(s);
            }
            if (nm == "copy_value") return args.empty() ? V(0) : args[0];
            if (nm == "member_array") return V(-1);
            if (it_->funcs_.find(nm) != it_->funcs_.end()) return callFn(nm, args);
            return V(0);
        }
        bool primary(Parser& p, Val& out) {
            const Tok& tk = p.peek();
            if (tk.kind == 'n') { p.next(); out = V(tk.num); return true; }
            if (tk.kind == 's') { p.next(); out = VS(tk.text); return true; }
            if (tk.kind == 'o' && tk.text == "(:") {     // closure literal
                while (!(p.peek().kind == 'o' && p.peek().text == ":)") && p.peek().kind != 'e') p.next();
                p.eat(":)");
                out = VARR(std::vector<Val>());
                return true;
            }
            if (tk.kind == 'o' && tk.text == "(") {
                p.next();
                if (p.at("{")) {                        // ({ ... }) array literal
                    p.next();
                    std::vector<Val> items;
                    while (!p.at("}") && p.peek().kind != 'e') {
                        Val v;
                        if (!expr(p, v)) return false;
                        if (v.kind == 'a') {
                            for (size_t k = 0; k < v.items.size(); ++k) items.push_back(v.items[k]);
                        } else {
                            items.push_back(v);
                        }
                        if (p.eat(",")) continue;
                        break;
                    }
                    p.eat("}");
                    p.eat(")");
                    out = VARR(items);
                    return true;
                }
                if (p.at(":")) {                        // (: name, args :)
                    p.next();
                    while (!p.at(":") && p.peek().kind != 'e') p.next();
                    p.eat(":");
                    p.eat(")");
                    out = VARR(std::vector<Val>());
                    return true;
                }
                if (!expr(p, out)) return false;
                p.eat(")");
                return true;
            }
            if (tk.kind == 'o' && tk.text == "{") {     // ({ ... } reached bare
                p.next();
                std::vector<Val> items;
                while (!p.at("}") && p.peek().kind != 'e') {
                    Val v;
                    if (!expr(p, v)) return false;
                    items.push_back(v);
                    if (p.eat(",")) continue;
                    break;
                }
                p.eat("}");
                p.eat(")");
                out = VARR(items);
                return true;
            }
            if (tk.kind == 'i') {
                p.next();
                std::string nm = tk.text;
                if (p.at("(")) {
                    p.next();
                    std::vector<Val> args;
                    if (!p.at(")")) {
                        for (;;) {
                            Val a;
                            if (!expr(p, a)) return false;
                            args.push_back(a);
                            if (p.eat(",")) continue;
                            break;
                        }
                    }
                    p.eat(")");
                    out = builtin(nm, args);
                    return true;
                }
                std::map<std::string, Val>::iterator it = env_.find(nm);
                lastIdent_ = nm;
                if (it != env_.end()) { out = it->second; return true; }
                out = V(0);
                return true;
            }
            return false;
        }
    };
};
