#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A4: replace the migration macros with real accessors.

    python -X utf8 _vmchain_codegen/stepA4b_accessors.py

The shim `#define frames_ es_->frames` (19 of them) works but hides the storage
behind the preprocessor, so nothing can ask for a DIFFERENT execution state (a
parked chain's own context, a per-PCB context).  Each old name becomes a member
function of OriginalVm:

    std::vector<Frame>& frames_() { return es_->frames; }

and the macro becomes `#define frames_ frames()` so the ~1400 existing uses keep
compiling while every access now crosses a function boundary - the single place
that decides where the active chain's state lives.
"""
import io

SRC = r'src\original_vm.cpp'

# (name, type, ExecState member)
MEMBERS = [
    ('frames_', 'std::vector<Frame>', 'frames'),
    ('stack_', 'std::vector<StackItem>', 'stack'),
    ('detachedStack_', 'std::vector<StackItem>', 'detachedStack'),
    ('savedObjVarsForNextCall_', 'std::shared_ptr<std::vector<Value>>', 'savedObjVarsForNextCall'),
    ('funcLookup_', 'const std::map<int, const OriginalFunction*>*', 'funcLookup'),
    ('lastOp_', 'int', 'lastOp'),
    ('lastEfunIndex_', 'int', 'lastEfunIndex'),
    ('lastIp_', 'size_t', 'lastIp'),
    ('returnValue_', 'Value', 'returnValue'),
    ('returned_', 'bool', 'returned'),
    ('catchDepth_', 'int', 'catchDepth'),
    ('catches_', 'std::vector<CatchRecord>', 'catches'),
    ('code_', 'const std::vector<OriginalInsn>*', 'code'),
    ('meta_', 'const std::vector<uint8_t>*', 'meta'),
    ('ip_', 'size_t', 'ip'),
    ('cond_', 'ResumeCond', 'cond'),
    ('parked_', 'bool', 'parked'),
    ('resumeValue_', 'Value', 'resumeValue'),
    ('onResume_', 'std::function<void()>', 'onResume'),
]


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')

    anchor = '    ExecState* es_ = &defaultEs_;'
    if 'lvalue' in t and 'std::vector<Frame>& frames_()' in t:
        print('accessors already present')
    else:
        a = anchor.replace('\n', sep)
        if t.count(a) != 1:
            print('es_ anchor COUNT %d' % t.count(a))
            return 1
        acc = ['',
               '    // ---- execution-state accessors -----------------------------------',
               '    // The interpreter state lives in the ACTIVE chain\'s ExecState.  These',
               '    // wrappers are the only place that knows how to reach it, so the storage',
               '    // can change underneath (per-PCB contexts, a parked chain\'s own state)',
               '    // without touching the ~1400 uses.  See docs/scheduler_refactor_plan.md.',
               '    ExecState& state() { return *es_; }']
        for name, typ, mem in MEMBERS:
            acc.append('    %s& %s() { return es_->%s; }' % (typ, name, mem))
        t = t.replace(a, a + sep.join(acc), 1)

    # macro block -> function-like aliases
    i = t.find('// ---- Stage 1 migration shims')
    j = t.find('#define onResume_ es_->onResume', i)
    if i >= 0 and j >= 0:
        j = t.index(sep, j) + len(sep)
        block = ['// ---- migration aliases ---------------------------------------------',
                 '// The old member names now call the accessors above: every access goes',
                 '// through one function that decides where the active chain\'s state lives.',
                 '']
        for name, _typ, _mem in MEMBERS:
            block.append('#define %s %s()' % (name, name))
        block.append('')
        t = t[:i] + sep.join(block) + t[j:]
        print('macro block rewritten as function aliases')
    else:
        print('macro block not found (%d/%d)' % (i, j))

    open(SRC, 'wb').write(t.encode('utf-8'))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
