# -*- coding: utf-8 -*-
"""Patch resort_stack (op 71) so only the defaults of REALLY missing
parameters are kept (src/original_vm.cpp is CRLF - patch on bytes)."""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


before = d.count(b'\r\n')
print('CRLF', before, 'bareLF', d.count(b'\n') - before)

OLD = B("""                int defaultPushed = static_cast<int>(stack_.size()) - 1 - f.bp;
                if (defaultPushed > 0) {
                    std::vector<StackItem> defaults;
                    for (int i = 0; i < defaultPushed; ++i)
                        defaults.push_back(stack_[static_cast<size_t>(f.bp + 1 + i)]);
""")

NEW = B("""                int defaultPushed = static_cast<int>(stack_.size()) - 1 - f.bp;
                if (defaultPushed > 0) {
                    // The compiler emits the default-value block for ALL
                    // parameters that HAVE a default - it cannot know at compile
                    // time how many were actually passed - so the block also
                    // pushes the defaults of parameters the caller DID supply.
                    // Those extras must be DISCARDED here: only
                    // `maxArgNo - argNum` of them belong to missing parameters,
                    // and because the block is emitted in FORWARD parameter
                    // order they are its LAST `missing` entries.
                    //
                    // Without this slice set_talk_drama(talk_npc, actors,
                    // content, func) - 5 declared params, `mixed func = 1`,
                    // `string para = ""`, called with 4 args - handed `func`'s
                    // default (int 1) to `para`.  build_talk_drama_buffer then
                    // wrote strlen(para)=1 and (buffer)1 = 00 00 00 01 as the
                    // drama header, so the client read list_count = 0 and played
                    // NO story at all (no drama, no story actors).
                    int keep = defaultPushed;
                    if (f.maxArgNo > 0) {
                        int missing = f.maxArgNo - f.argNum;
                        if (missing < 0) missing = 0;
                        if (missing < defaultPushed) keep = missing;
                    }
                    if (keep < 0) keep = 0;
                    std::vector<StackItem> defaults;
                    for (int i = defaultPushed - keep; i < defaultPushed; ++i)
                        defaults.push_back(stack_[static_cast<size_t>(f.bp + 1 + i)]);
""")

assert d.count(OLD) == 1, 'OLD block not found exactly once (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)

OLD2 = B("""                    for (auto& a : args) stack_.push_back(a);
                    f.argNum += defaultPushed;
""")
NEW2 = B("""                    for (auto& a : args) stack_.push_back(a);
                    f.argNum += keep;
""")
assert d.count(OLD2) == 1, 'OLD2 not found exactly once (%d)' % d.count(OLD2)
d = d.replace(OLD2, NEW2)

io.open(P, 'wb').write(d)
after = d.count(b'\r\n')
print('written; CRLF', after, 'bareLF', d.count(b'\n') - after)
print('f.maxArgNo refs:', d.count(b'f.maxArgNo'), '| keep:', d.count(b'f.argNum += keep;'))
