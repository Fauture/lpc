# -*- coding: utf-8 -*-
"""Reference interpreter for the generated anti-cheat challenge script.

Usage:  python -X utf8 _ac_eval.py < script.txt      -> "x y answer"
        python -X utf8 _ac_eval.py --check            -> validate over every
                                                         captured real script

Grammar handled (the exact shapes the client's generate_function() emits):
  * top level:  [override] <type> name(par [= default], ...) { body }
  * statements: type decl lists (mixed i, m, a, s;), assignments (name = / +=,
    m[e] = / +=, a[i] = / +=), loop (i = A upto B) {..}, loop (mks in m) {..},
    return expr;
  * expressions: full C precedence, ({ ... }) array literals, [index],
    this_object()->name(args), builtins: allocate_mapping, count_total,
    sizeof, strlen, keys, values, sort, sprintf, toupper/tolower, copy_value.
  * values: int | string | array | mapping (mapping = list of (k,v) pairs in
    insertion order, exactly like the driver's mapping).
"""
import os
import re
import sys

# ---------------------------------------------------------------- values
_DBG_PRIMARY = bool(os.environ.get('ACDBG'))

INT, STR, ARR, MAP = 'int', 'str', 'arr', 'map'


class Val(object):
    __slots__ = ('kind', 'num', 's', 'items', 'pairs')

    def __init__(self, kind=INT, num=0, s='', items=None, pairs=None):
        self.kind = kind
        self.num = num
        self.s = s
        self.items = items if items is not None else []
        self.pairs = pairs if pairs is not None else []

    def as_int(self):
        if self.kind == INT:
            return self.num
        if self.kind == STR:
            return len(self.s)
        if self.kind == ARR:
            return len(self.items)
        if self.kind == MAP:
            return len(self.pairs)
        return 0


def V(n):
    return Val(INT, num=int(n))


def VS(text):
    return Val(STR, num=len(text), s=text)


def add_value(cur, rhs):
    """`cur += rhs` as the driver's operator does it (array append / string
    concat / int add), returning the new value for the caller to store."""
    if cur.kind == ARR and rhs.kind == ARR:
        return Val(ARR, items=cur.items + rhs.items)
    if cur.kind == STR or rhs.kind == STR:
        return VS((cur.s if cur.kind == STR else str(cur.num)) +
                  (rhs.s if rhs.kind == STR else str(rhs.num)))
    return V(cur.as_int() + rhs.as_int())


# ---------------------------------------------------------------- lexer
TOKEN_RE = re.compile(r"""
    (?P<ws>\s+)
  | (?P<num>0[xX][0-9a-fA-F]+|\d+)
  | (?P<str>"(?:[^"\\]|\\.)*")
  | (?P<id>[A-Za-z_]\w*)
  | (?P<op>\+=|-=|\*=|/=|%=|==|!=|<=|>=|&&|\|\||<<|>>|\+\+|--|->|::|\(\:|:\)|.)
""", re.VERBOSE)


class Tok(object):
    __slots__ = ('kind', 'text')

    def __init__(self, kind, text):
        self.kind = kind
        self.text = text

    def __repr__(self):
        return '%s(%r)' % (self.kind, self.text)


def lex(src):
    out = []
    for m in TOKEN_RE.finditer(src):
        kind = m.lastgroup
        text = m.group()
        if kind == 'ws':
            continue
        if kind == 'str':
            raw = text[1:-1]
            raw = raw.replace('\\"', '"').replace('\\\\', '\\').replace('\\r', '\r').replace('\\n', '\n')
            out.append(Tok('str', raw))
        elif kind == 'num':
            out.append(Tok('num', int(text, 0)))
        elif kind == 'id':
            out.append(Tok('id', text))
        else:
            out.append(Tok('op', text))
    # `&=` / `|=` come through as two tokens with this regex; merge them.
    merged = []
    i = 0
    while i < len(out):
        if (out[i].kind == 'op' and out[i].text in ('&', '|') and i + 1 < len(out) and
                out[i + 1].kind == 'op' and out[i + 1].text == '='):
            merged.append(Tok('op', out[i].text + '='))
            i += 2
            continue
        merged.append(out[i])
        i += 1
    out = merged
    out.append(Tok('eof', ''))
    return out


# ---------------------------------------------------------------- parser
TYPES = ('mixed', 'int', 'string', 'array', 'mapping', 'float', 'object', 'buffer')


class Parser(object):
    def __init__(self, toks):
        self.t = toks
        self.i = 0

    def peek(self, k=0):
        return self.t[min(self.i + k, len(self.t) - 1)]

    def at(self, text):
        tk = self.peek()
        return tk.text == text and tk.kind in ('op', 'id')

    def next(self):
        tk = self.t[self.i]
        self.i += 1
        return tk

    def eat(self, text):
        if self.at(text):
            self.i += 1
            return True
        return False

    def expect(self, text):
        if not self.eat(text):
            raise SyntaxError('expected %r at %r' % (text, self.peek()))


class Interp(object):
    def __init__(self, src):
        self.funcs = {}
        self.parse(src)

    # ---- parse ----
    def parse(self, src):
        p = Parser(lex(src))
        while p.peek().kind != 'eof':
            if p.at('override'):
                p.next()
            # collect tokens until '{' at depth 0
            head = []
            while not (p.at('{') or p.peek().kind == 'eof'):
                head.append(p.next())
            if p.peek().kind == 'eof':
                break
            body_toks = self.skip_block(p)
            # name = last id before '('
            names = [k for k in head if k.kind == 'id']
            paren = None
            for idx, k in enumerate(head):
                if k.text == '(':
                    paren = idx
                    break
            if paren is None or not names:
                continue
            # the identifier right before '(' is the function name
            fname = None
            for idx in range(paren - 1, -1, -1):
                if head[idx].kind == 'id':
                    fname = head[idx].text
                    break
            params, defs = self.parse_params(head[paren + 1:])
            if fname:
                self.funcs[fname] = (params, defs, body_toks)

    def skip_block(self, p):
        assert p.at('{')
        p.next()
        depth = 1
        out = []
        while depth:
            tk = p.next()
            if tk.kind == 'eof':
                break
            if tk.text == '{':
                depth += 1
            elif tk.text == '}':
                depth -= 1
                if depth == 0:
                    break
            out.append(tk)
        return out

    @staticmethod
    def parse_params(toks):
        # the parameter list ends at the ')' that closes the '(' we started after
        depth = 1
        inner = []
        for tk in toks:
            t = tk.text if isinstance(tk.text, str) else ''
            if t in ('(', '[', '{'):
                depth += 1
            elif t in (')', ']', '}'):
                depth -= 1
                if depth == 0:
                    break
            inner.append(tk)
        # split on top-level commas
        groups, cur, depth = [], [], 0
        for tk in inner:
            t = tk.text if isinstance(tk.text, str) else ''
            if t in ('(', '[', '{'):
                depth += 1
            elif t in (')', ']', '}'):
                depth -= 1
            if t == ',' and depth == 0:
                groups.append(cur)
                cur = []
                continue
            cur.append(tk)
        groups.append(cur)
        names, defs = [], []
        for g in groups:
            if not g:
                continue
            eq = None
            for idx, tk in enumerate(g):
                if tk.text == '=':
                    eq = idx
                    break
            decl = g if eq is None else g[:eq]
            dflt = 0
            if eq is not None and eq + 1 < len(g):
                dflt = g[eq + 1].text if g[eq + 1].kind == 'num' else 0
            ids = [tk.text for tk in decl if tk.kind == 'id']
            if ids:
                names.append(ids[-1])
                defs.append(dflt)
        return names, defs

    # ---- execute ----
    def call(self, fname, args):
        if fname not in self.funcs:
            return V(0)
        params, defs, body = self.funcs[fname]
        env = {}
        for i, nm in enumerate(params):
            env[nm] = args[i] if i < len(args) else V(defs[i])
        return Exec(self, env).block(body)

    def eval(self, fname, args):
        return self.call(fname, args).as_int()


class Exec(object):
    def __init__(self, interp, env):
        self.it = interp
        self.env = env

    # -------- statements --------
    def one_or_block(self, p, extra_env):
        """The generated loops are `loop (...) stmt;` WITHOUT braces."""
        if p.at('{'):
            body = self.it.skip_block(p)
            return (body, True)
        start = p.i
        end = self.scan_stmt(p)
        toks = p.t[start:end]
        p.i = end
        return (toks, False)

    def one_or_block_run(self, spec, extra_env):
        """Build a thunk that runs one loop iteration.

        The body runs against the SAME environment as the loop (this mirrors
        the compiled code, where the body is straight-line code in the caller's
        frame).  Running it on a copied dict - which the first version did -
        threw away every write that was not an in-place mutation of a shared
        array/mapping.
        """
        toks, is_block = spec

        def run():
            ex = Exec(self.it, extra_env)
            if is_block:
                ex.block(toks)
            else:
                sub = Parser(list(toks) + [Tok('eof', '')])
                ex.stmt(sub)
        return run

    def scan_stmt(self, p):
        """Consume one statement (to a top-level ';' or a balanced block).

        The loop headers the client generates look like
        `loop (i = 0 upto 1 + ((x + y) & 3))` - after the header the very next
        token is the body, which is why this has to stop at a TOP-LEVEL ';'
        while skipping every balanced () [] {} group.
        """
        depth = 0
        while p.peek().kind != 'eof':
            tk = p.peek()
            if tk.text == '{':
                self.it.skip_block(p)
                if depth == 0:
                    return p.i
                continue
            if tk.text in ('(', '['):
                depth += 1
                p.next()
                continue
            if tk.text in (')', ']'):
                if depth == 0:
                    return p.i
                depth -= 1
                p.next()
                continue
            p.next()
            if tk.text == ';' and depth <= 0:
                return p.i
        return p.i

    def block(self, toks):
        p = Parser(list(toks) + [Tok('eof', '')])
        result = V(0)
        while p.peek().kind != 'eof':
            r = self.stmt(p)
            if r is not None:
                result = r
        return result

    def stmt(self, p):
        tk = p.peek()
        if tk.kind == 'id' and tk.text in TYPES:
            p.next()
            while True:
                nm = p.next().text
                val = V(0)
                if p.eat('='):
                    val = self.expr(p)
                self.env[nm] = val
                if p.eat(','):
                    continue
                break
            p.eat(';')
            return None
        if tk.kind == 'id' and tk.text == 'loop':
            p.next()
            p.expect('(')
            var = p.next().text
            if p.eat('in'):
                coll = p.next().text
                p.expect(')')
                run = self.one_or_block_run(self.one_or_block(p, self.env), self.env)
                src = self.env.get(coll, Val())
                pairs = list(src.pairs) if src.kind == MAP else []
                for (k, v) in pairs:
                    self.env[var] = Val(ARR, items=[k, v])
                    run()
                # The loop variable keeps its last value (vm_cmpAndJmpInLoop's
                # final setVar); it is NOT removed after the loop.
                return None
            p.expect('=')
            a = self.ternary(p).as_int()      # stop before 'upto' / 'in'
            if p.eat('upto'):
                b = self.ternary(p).as_int()
                p.expect(')')
                run = self.one_or_block_run(self.one_or_block(p, self.env), self.env)
                # The compiled loop (init_for_loop/cond_jmp_for_loop/
                # step_jmp_for_loop, vm_cmpAndJmpInLoop) keeps its own counter
                # and ASSIGNS the loop variable on every step, so the body
                # always sees the counter value - an `i++` in the body is
                # overwritten by the next step.
                for v in range(a, b + 1):
                    self.env[var] = V(v)
                    run()
                # The break step assigns the counter before testing it, so the
                # variable is left at B + 1 after the loop.
                self.env[var] = V(b + 1)
                return None
            if p.eat('in'):
                coll = p.next().text
                p.expect(')')
                run = self.one_or_block_run(self.one_or_block(p, self.env), self.env)
                src = self.env.get(coll, Val())
                pairs = list(src.pairs) if src.kind == MAP else []
                for (k, v) in pairs:
                    self.env[var] = Val(ARR, items=[k, v])
                    run()
                # The loop variable keeps its last value (vm_cmpAndJmpInLoop's
                # final setVar); it is NOT removed after the loop.
                return None
            raise SyntaxError('bad loop')
        if tk.kind == 'id' and tk.text == 'return':
            p.next()
            v = self.expr(p)
            p.eat(';')
            return v
        if tk.kind == 'id' and tk.text in ('break', 'continue'):
            p.next()
            p.eat(';')
            return None
        if tk.text in ('{',):
            body = self.it.skip_block(p)
            return Exec(self.it, dict(self.env)).block(body)
        # assignment or expression
        if p.peek().kind == 'id' and p.peek(1).text in ('=', '+=', '-=', '&=', '|=', '[', '++', '--'):
            nm = p.next().text
            if p.at('['):
                p.next()
                key = self.expr(p)
                p.expect(']')
                # `a[i] = ...` where a is an ARRAY assigns the ELEMENT.  Treating
                # it as a mapping (the old code replaced the array with a fresh
                # mapping) silently dropped the store, so `a[i] = (a[i] - ...)`
                # left the array untouched and every answer was wrong.
                dst = self.env.get(nm)
                if dst is None or dst.kind not in (ARR, STR, MAP):
                    dst = Val(MAP, pairs=[])
                    self.env[nm] = dst
                if dst.kind == ARR:
                    i = key.as_int()
                    if p.eat('='):
                        val = self.expr(p)
                        if 0 <= i < len(dst.items):
                            dst.items[i] = val
                        elif i == len(dst.items):
                            dst.items.append(val)
                    elif p.eat('+='):
                        val = self.expr(p)
                        if 0 <= i < len(dst.items):
                            dst.items[i] = add_value(dst.items[i], val)
                    p.eat(';')
                    return None
                if p.eat('='):
                    val = self.expr(p)
                    self.map_put(dst, key, val)
                elif p.eat('+='):
                    val = self.expr(p)
                    cur = self.map_get(dst, key)
                    self.map_put(dst, key, V(cur.as_int() + val.as_int()))
                p.eat(';')
                return None
            if p.eat('='):
                self.env[nm] = self.expr(p)
                p.eat(';')
                return None
            if p.eat('+='):
                rhs = self.expr(p)
                cur = self.env.get(nm)
                if cur is None:
                    cur = V(0)
                    self.env[nm] = cur
                # MUTATE the existing value object: loop bodies run on a copied
                # dict, so a rebind here would be lost after the iteration and
                # `loop (...) a += ({ ... });` would append nothing.
                if cur.kind == ARR and rhs.kind == ARR:
                    cur.items.extend(rhs.items)
                elif cur.kind == STR or rhs.kind == STR:
                    cur.s = (cur.s if cur.kind == STR else str(cur.num)) + \
                            (rhs.s if rhs.kind == STR else str(rhs.num))
                    cur.num = len(cur.s)
                    cur.kind = STR
                else:
                    cur.num += rhs.as_int()
                p.eat(';')
                return None
            if p.eat('&='):
                self.env[nm] = V(self.env.get(nm, V(0)).as_int() & self.expr(p).as_int())
                p.eat(';')
                return None
            if p.eat('|='):
                self.env[nm] = V(self.env.get(nm, V(0)).as_int() | self.expr(p).as_int())
                p.eat(';')
                return None
        v = self.expr(p)
        p.eat(';')
        return v

    @staticmethod
    def map_put(m, key, val):
        for pair in m.pairs:
            if pair[0].as_int() == key.as_int() and pair[0].kind == key.kind:
                pair[1] = val
                return
        m.pairs.append([key, val])

    @staticmethod
    def map_get(m, key):
        for pair in m.pairs:
            if pair[0].as_int() == key.as_int() and pair[0].kind == key.kind:
                return pair[1]
        return V(0)

    # -------- expressions (C precedence) --------
    def expr(self, p):
        return self.assign_expr(p)

    def assign_expr(self, p):
        left = self.ternary(p)
        if p.at('='):
            p.next()
            right = self.assign_expr(p)
            return right
        return left

    def ternary(self, p):
        c = self.binary(p, 0)
        if p.at('?'):
            p.next()
            a = self.ternary(p)
            p.expect(':')
            b = self.ternary(p)
            return a if c.as_int() else b
        return c

    LEVELS = [('||',), ('&&',), ('|',), ('^',), ('&',),
              ('==', '!='), ('<', '>', '<=', '>='), ('<<', '>>'),
              ('+', '-'), ('*', '/', '%')]

    def binary(self, p, level):
        if level >= len(self.LEVELS):
            return self.unary(p)
        left = self.binary(p, level + 1)
        while True:
            tk = p.peek()
            if tk.text not in self.LEVELS[level]:
                return left
            op = p.next().text
            right = self.binary(p, level + 1)
            left = self.apply_bin(op, left, right)

    @staticmethod
    def apply_bin(op, a, b):
        if op == '+':
            if a.kind == STR or b.kind == STR:
                text = (a.s if a.kind == STR else str(a.num)) + (b.s if b.kind == STR else str(b.num))
                return VS(text)
            if a.kind == ARR and b.kind == ARR:
                return Val(ARR, items=a.items + b.items)
            return V(a.as_int() + b.as_int())
        x, y = a.as_int(), b.as_int()
        if op == '-':
            return V(x - y)
        if op == '*':
            return V(x * y)
        if op == '/':
            return V(x // y if y else 0)
        if op == '%':
            return V(x % y if y else 0)
        if op == '&':
            return V(x & y)
        if op == '|':
            return V(x | y)
        if op == '^':
            return V(x ^ y)
        if op == '<<':
            return V(x << y)
        if op == '>>':
            return V(x >> y)
        if op == '==':
            return V(1 if x == y else 0)
        if op == '!=':
            return V(1 if x != y else 0)
        if op == '<':
            return V(1 if x < y else 0)
        if op == '>':
            return V(1 if x > y else 0)
        if op == '<=':
            return V(1 if x <= y else 0)
        if op == '>=':
            return V(1 if x >= y else 0)
        if op == '&&':
            return V(1 if (x and y) else 0)
        if op == '||':
            return V(1 if (x or y) else 0)
        return V(0)

    def unary(self, p):
        if p.eat('-'):
            return V(-self.unary(p).as_int())
        if p.eat('!'):
            return V(0 if self.unary(p).as_int() else 1)
        if p.eat('++'):
            return self.unary(p)
        if p.eat('('):
            # cast?  `(int) expr`
            if p.peek().kind == 'id' and p.peek().text in TYPES and p.peek(1).text == ')':
                p.next()
                p.next()
                return self.unary(p)
            v = self.expr(p)
            p.eat(')')
            return self.postfix(p, v)
        return self.postfix(p, self.primary(p))

    def postfix(self, p, v):
        while True:
            if p.at('++') or p.at('--'):
                # i++ used as a value: yield the current value, then bump the
                # variable (LPC post-increment).  self._last_ident is set by
                # primary() when it reads a variable.
                inc = p.at('++')
                p.next()
                nm = getattr(self, '_last_ident', None)
                if nm and nm in self.env:
                    self.env[nm] = V(self.env[nm].as_int() + (1 if inc else -1))
                self._last_ident = None
                return v
            if p.at('['):
                p.next()
                idx = self.expr(p)
                p.expect(']')
                v = self.index(v, idx)
                continue
            if p.at('->'):
                p.next()
                nm = p.next().text
                p.expect('(')
                args = self.args(p)
                v = self.it.call(nm, args)
                continue
            return v

    def args(self, p):
        out = []
        if p.at(')'):
            p.next()
            return out
        while True:
            out.append(self.expr(p))
            if p.eat(','):
                continue
            break
        if not p.eat(')'):
            # tolerate generators that nest `({ ... })` inside a call
            p.eat(')')
        return out

    @staticmethod
    def index(v, idx):
        i = idx.as_int()
        if v.kind == ARR:
            return v.items[i] if 0 <= i < len(v.items) else V(0)
        if v.kind == STR:
            return V(ord(v.s[i])) if 0 <= i < len(v.s) else V(0)
        if v.kind == MAP:
            for pair in v.pairs:
                if pair[0].as_int() == i and pair[0].kind == idx.kind:
                    return pair[1]
            return V(0)
        return V(0)

    def primary(self, p):
        tk = p.peek()
        if tk.text == '(:':                 # closure literal - value is unused
            while not p.at(':)') and p.peek().kind != 'eof':
                p.next()
            p.eat(':)')
            return Val(ARR, items=[])
        if _DBG_PRIMARY:
            sys.stderr.write('  primary at %d %r buf=%r\n' % (p.i, tk, [x.text for x in p.t[:8]]))
        if tk.kind == 'num':
            p.next()
            return V(tk.text)
        if tk.kind == 'str':
            p.next()
            return VS(tk.text)
        if tk.text == '(':
            p.next()
            if p.at('{'):
                p.next()
                items = []
                while not p.at('}') and p.peek().kind != 'eof':
                    items.append(self.expr(p))
                    if p.eat(','):
                        continue
                    break
                p.eat('}')
                p.eat(')')
                return Val(ARR, items=items)
            if p.at(':'):                      # (: name, args :) closure literal
                p.next()
                while not p.at(':') and p.peek().kind != 'eof':
                    p.next()
                if p.at(':'):
                    p.next()
                else:
                    p.expect(')')
                    return Val(ARR, items=[])
                p.expect(')')
                return Val(ARR, items=[])
            v = self.expr(p)
            p.eat(')')
            return v
        if tk.text == '{':
            # `({ ... })` reached without the leading '(' (some generators emit
            # the braces only): treat it as an array literal, NEVER as a block -
            # a block here is what silently swallowed the loop body.
            p.next()
            items = []
            while not p.at('}') and p.peek().kind != 'eof':
                items.append(self.expr(p))
                if p.eat(','):
                    continue
                break
            p.eat('}')
            p.eat(')')
            return Val(ARR, items=items)
        if tk.kind == 'id':
            p.next()
            nm = tk.text
            if p.at('('):
                p.next()
                args = self.args(p)
                return self.builtin(nm, args)
            if nm in self.env:
                self._last_ident = nm
                return self.env[nm]
            if nm == 'this_object':
                return Val()
            return V(0)
        raise SyntaxError('unexpected token %r' % (tk,))

    def builtin(self, nm, args):
        if nm == 'allocate_mapping':
            return Val(MAP, pairs=[])
        if nm == 'count_total':
            total = 0
            for a in args:
                if a.kind == ARR:
                    for x in a.items:
                        total += x.as_int()
                else:
                    total += a.as_int()
            return V(total)
        if nm == 'sizeof':
            return V(args[0].as_int() if args else 0)
        if nm == 'strlen':
            return V(len(args[0].s) if args and args[0].kind == STR else (args[0].as_int() if args else 0))
        if nm == 'keys':
            return Val(ARR, items=[k for (k, _v) in (args[0].pairs if args else [])])
        if nm == 'values':
            return Val(ARR, items=[_v for (_k, _v) in (args[0].pairs if args else [])])
        if nm == 'sort':
            if args and args[0].kind == ARR:
                return Val(ARR, items=sorted(args[0].items, key=lambda x: x.as_int()))
            return Val(ARR, items=[])
        if nm == 'sprintf':
            fmt = args[0].s if args and args[0].kind == STR else ''
            vals = [a.as_int() for a in args[1:]]
            try:
                text = fmt % tuple(vals) if vals else fmt
            except Exception:
                text = ''
            return VS(text)
        if nm in ('toupper', 'tolower'):
            s = args[0].s if args and args[0].kind == STR else ''
            return VS(s.upper() if nm == 'toupper' else s.lower())
        if nm == 'copy_value':
            return args[0] if args else V(0)
        if nm == 'member_array':
            return V(-1)
        if nm in self.it.funcs:
            return self.it.call(nm, args)
        return V(0)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--check':
        import _ac_check
        return _ac_check.run()
    src = sys.stdin.read()
    itp = Interp(src)
    x = itp.eval('get_x', [])
    y = itp.eval('get_y', [])
    ans = itp.eval('challenge_server', [V(x), V(y)])
    print('%d %d %d' % (x, y, ans))
    return 0


if __name__ == '__main__':
    sys.exit(main() or 0)
