# -*- coding: utf-8 -*-
"""Move Frame::maxArgNo after `strings` so the 5-field aggregate initializers
({fn, bp, argNum, localCount, strings}) keep working."""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""    int argNum = 0;
    // MAX argument count (declared params + defaulted params) as recorded by
    // the last check_args_num (op 15) of this frame.  resort_stack needs it to
    // decide which of the inline-pushed defaults belong to parameters that were
    // NOT passed (see the op-71 handler).
    int maxArgNo = 0;
    int localCount = 0;
    // String table of the PROGRAM the function belongs to.  Inherited
    // functions carry their own program's strings (shared-string indexes are
    // resolved against the callee's table, not the current object's).
    const std::vector<std::string>* strings = nullptr;
""")
NEW = B("""    int argNum = 0;
    int localCount = 0;
    // String table of the PROGRAM the function belongs to.  Inherited
    // functions carry their own program's strings (shared-string indexes are
    // resolved against the callee's table, not the current object's).
    const std::vector<std::string>* strings = nullptr;
    // MAX argument count (declared params + defaulted params) as recorded by
    // the last check_args_num (op 15) of this frame.  resort_stack needs it to
    // decide which of the inline-pushed defaults belong to parameters that were
    // NOT passed (see the op-71 handler).  Kept AFTER `strings` on purpose: the
    // Frame aggregate initializers are positional ({fn, bp, argNum, localCount,
    // strings}).
    int maxArgNo = 0;
""")
assert d.count(OLD) == 1, 'OLD not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
after = d.count(b'\r\n')
print('written; CRLF', after, 'bareLF', d.count(b'\n') - after)
