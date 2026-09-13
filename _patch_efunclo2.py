# -*- coding: utf-8 -*-
"""original_vm.cpp half of the efun-closure fix (value.h already patched)."""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


A = B("""                            cl->functionName = names[in.a];
                            cl->funcIndex = in.a;
                            cl->func = nullptr;   // efun: resolved by name at call time""")
BB = B("""                            cl->functionName = names[in.a];
                            cl->funcIndex = in.a;
                            cl->func = nullptr;   // efun: resolved by name at call time
                            cl->efunClosure = true;""")
assert d.count(A) == 1, 'creation anchor %d' % d.count(A)
d = d.replace(A, BB)

C = B("""                    } else if (cl->funcIndex >= 0 && !cl->func) {""")
D = B("""                    } else if (cl->funcIndex >= 0 && !cl->func && !cl->efunClosure) {""")
assert d.count(C) == 1, 'case5 anchor %d' % d.count(C)
d = d.replace(C, D)

# preserve the flag wherever a closure is copied (both indentation variants)
for ind in (' ' * 24, ' ' * 20):
    old = B(ind + "cl->functionName = src->functionName;\n"
            + ind + "cl->funcIndex = src->funcIndex;\n"
            + ind + "cl->func = src->func;\n"
            + ind + "cl->owner = src->owner;")
    new = B(ind + "cl->functionName = src->functionName;\n"
            + ind + "cl->funcIndex = src->funcIndex;\n"
            + ind + "cl->func = src->func;\n"
            + ind + "cl->efunClosure = src->efunClosure;\n"
            + ind + "cl->owner = src->owner;")
    n = d.count(old)
    print('copy-helper indent %d: %d occurrence(s)' % (len(ind), n))
    if n:
        d = d.replace(old, new)

io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
print('efunClosure refs:', d.count(b'efunClosure'))
