# -*- coding: utf-8 -*-
"""An EFUN closure created by op 23 (: call_other, path, "fn" :) carries
func = nullptr and funcIndex = the EFUN's index in the global table.  call_fun_ptr
then ran its generic "legacy closure without a func pointer" resolution
(funcsByIndex_[funcIndex], else findMasterFunction(funcIndex)) BEFORE the efun
fallback.  When the executing VM's merged table happens to own a function at
that same index, the closure silently ran THAT function: taskd's
(*funcs["tip"])(me, task_info) returned int 1 instead of instruction_1's tip
string, so every task log/tip came out empty.  Mark efun closures and let them
reach the efun-by-name fallback.
"""
import io

# ---- value.h (LF) ---------------------------------------------------------
V = 'src/value.h'
d = io.open(V, 'rb').read()
OLD = ("""    // Object that owns this closure (the program whose bytecode created it).
    // When the closure is invoked from another VM (e.g. the master's post_init
    // loop calling a daemon's do_init), call_fun_ptr resolves the function in
    // this owner instead of the current VM.  Null for efun closures.
    const struct OriginalObject* owner = nullptr;
""").encode('ascii')
NEW = ("""    // Object that owns this closure (the program whose bytecode created it).
    // When the closure is invoked from another VM (e.g. the master's post_init
    // loop calling a daemon's do_init), call_fun_ptr resolves the function in
    // this owner instead of the current VM.  Null for efun closures.
    const struct OriginalObject* owner = nullptr;
    // True when this closure wraps a BUILTIN EFUN (create_function_value with
    // the global-table bit and no matching master function, e.g.
    // (: call_other, path, "fn" :) or (: create_thread, ... :)).  funcIndex is
    // then the EFUN's slot number, which COLLIDES with real function indexes in
    // a VM's merged table: call_fun_ptr's funcsByIndex_[funcIndex] fallback
    // would silently run that unrelated function (taskd's (*funcs["tip"])(...)
    // returned int 1 instead of instruction_1's tip string -> every task
    // log/tip empty).  Such closures must go to the efun-by-name fallback.
    bool efunClosure = false;
""").encode('ascii')
assert d.count(OLD) == 1, 'value.h anchor not found (%d)' % d.count(OLD)
io.open(V, 'wb').write(d.replace(OLD, NEW))
print('value.h patched')

# ---- original_vm.cpp (CRLF) ----------------------------------------------
P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


# 1. mark the closure at creation
A = B("""                            cl->functionName = names[in.a];
                            cl->funcIndex = in.a;
                            cl->func = nullptr;   // efun: resolved by name at call time""")
BB = B("""                            cl->functionName = names[in.a];
                            cl->funcIndex = in.a;
                            cl->func = nullptr;   // efun: resolved by name at call time
                            cl->efunClosure = true;""")
assert d.count(A) == 1, 'creation anchor not found (%d)' % d.count(A)
d = d.replace(A, BB)

# 2. skip the funcIndex resolution for efun closures (case 5 / call_fun_ptr)
C = B("""                    } else if (cl->funcIndex >= 0 && !cl->func) {""")
D = B("""                    } else if (cl->funcIndex >= 0 && !cl->func && !cl->efunClosure) {""")
assert d.count(C) == 1, 'case5 anchor not found (%d)' % d.count(C)
d = d.replace(C, D)

# 3. preserve the flag in the two closure-copy helpers
E = B("""                        cl->functionName = src->functionName;
                        cl->funcIndex = src->funcIndex;
                        cl->func = src->func;
                        cl->owner = src->owner;""")
F = B("""                        cl->functionName = src->functionName;
                        cl->funcIndex = src->funcIndex;
                        cl->func = src->func;
                        cl->efunClosure = src->efunClosure;
                        cl->owner = src->owner;""")
n = d.count(E)
assert n == 2, 'copy helpers found %d times' % n
d = d.replace(E, F)

G = B("""                    cl->functionName = src->functionName;
                    cl->funcIndex = src->funcIndex;
                    cl->func = src->func;
                    cl->owner = src->owner;""")
H = B("""                    cl->functionName = src->functionName;
                    cl->funcIndex = src->funcIndex;
                    cl->func = src->func;
                    cl->efunClosure = src->efunClosure;
                    cl->owner = src->owner;""")
n = d.count(G)
assert n == 2, 'copy helpers(2) found %d times' % n
d = d.replace(G, H)

io.open(P, 'wb').write(d)
print('original_vm.cpp patched; CRLF', d.count(b'\r\n'),
      'bareLF', d.count(b'\n') - d.count(b'\r\n'))
