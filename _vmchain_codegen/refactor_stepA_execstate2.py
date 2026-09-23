#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A (retry): move the VM's execution state into a per-chain ExecState.

    python -X utf8 _vmchain_codegen/refactor_stepA_execstate2.py

The first attempt replaced the VM's 19 execution members with `es_->member`
across the whole file, which also hit two identifiers that belong to the
SCHEDULER (`parked_`, a `std::vector<shared_ptr<Process>>`) and comments.  This
version is data-driven about that:

  * `parked_`: the scheduler's uses are exactly the vector operations
    (`erase/begin/end/push_back/size`, `for (... : parked_)`) plus its
    declaration; the VM's are the boolean ones (`? 1 : 0`, `= true/false`,
    `!parked_`, `isParked`).
  * `cond_`: never rewritten inside `st.`/`sig.`/`p->` member accesses and never
    on a comment line.
  * comment-only lines are left alone (the prose that documents the old design).

The 19 member definitions are replaced by `struct ExecState { ... }` plus the
single `ExecState* es_` pointer, and `VmRunState` keeps just that pointer so the
next step can turn stash/restore into a pointer swap.
"""
import io
import re

SRC = r'src\original_vm.cpp'

MEMBERS = {
    'detachedStack_': 'std::vector<StackItem> detachedStack_;',
    'savedObjVarsForNextCall_': 'std::shared_ptr<std::vector<Value>> savedObjVarsForNextCall_;',
    'frames_': 'std::vector<Frame> frames_;',
    'funcLookup_': 'const std::map<int, const OriginalFunction*>* funcLookup_ = nullptr;',
    'lastOp_': 'int lastOp_ = -1;',
    'lastEfunIndex_': 'int lastEfunIndex_ = -1;',
    'lastIp_': 'size_t lastIp_ = 0;',
    'returnValue_': 'Value returnValue_ = make_int(0);',
    'returned_': 'bool returned_ = false;',
    'catchDepth_': 'int catchDepth_ = 0;',
    'catches_': 'std::vector<CatchRecord> catches_;',
    'code_': 'const std::vector<OriginalInsn>* code_ = nullptr;',
    'meta_': 'const std::vector<uint8_t>* meta_ = nullptr;',
    'ip_': 'size_t ip_ = 0;',
    'cond_': 'ResumeCond cond_;',
    'parked_': 'bool parked_ = false;',
    'resumeValue_': 'Value resumeValue_ = make_int(0);',
    'onResume_': 'std::function<void()> onResume_;',
    'stack_': 'std::vector<StackItem> stack_;',
}

EXEC_STATE = '''    // ---- per-chain execution state -------------------------------------
    // The interpreter's whole mutable state for ONE execution chain, reached
    // through `es_` (the active chain's state) instead of living directly on the
    // VM.  One VM object hosts several chains (a create command, its
    // continuation on another object, daemons sharing a registry VM); keeping
    // this data in shared members is what forced the old
    // stashRunState()/restoreRunState() copy dance on every switch - and what
    // made a missed switch point lose frames.  See docs/scheduler_refactor_plan.md.
    struct ExecState {
        std::vector<StackItem> stack;
        std::vector<StackItem> detachedStack;
        std::shared_ptr<std::vector<Value>> savedObjVarsForNextCall;
        std::vector<Frame> frames;
        const std::map<int, const OriginalFunction*>* funcLookup = nullptr;
        int lastOp = -1;
        int lastEfunIndex = -1;
        size_t lastIp = 0;
        Value returnValue = make_int(0);
        bool returned = false;
        int catchDepth = 0;
        std::vector<CatchRecord> catches;
        const std::vector<OriginalInsn>* code = nullptr;
        const std::vector<uint8_t>* meta = nullptr;
        size_t ip = 0;
        ResumeCond cond;
        bool parked = false;
        Value resumeValue = make_int(0);
        std::function<void()> onResume;
    };
    // Every LPC entry goes through switchToChain(), but a few paths (the master,
    // runInit) execute before any chain key exists, so the default state is owned
    // here; `es_` therefore never dangles.
    ExecState defaultEs_;
    ExecState* es_ = &defaultEs_;'''

# scheduler-owned uses of these names (never rewritten)
SCHED_LINES = {1851, 1877}          # parked_.erase(...) / its declaration
SCHED_PAT = re.compile(r'(?:for\s*\([^)]*:\s*parked_\)|parked_\.(?:erase|begin|end|push_back|size))')


def is_comment(line: str) -> bool:
    s = line.lstrip()
    return s.startswith('//') or s.startswith('*') or s.startswith('/*')


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    lines = raw.decode('utf-8').split(sep)

    def_line = {}
    for i, ln in enumerate(lines):
        s = ln.strip()
        for name, decl in MEMBERS.items():
            if s == decl or (s.startswith(decl[:-1]) and ';' in s and
                             s.index(';') >= len(decl) - 1):
                def_line.setdefault(name, []).append(i)
    for name, ds in def_line.items():
        if len(ds) != 1:
            print('def count %s = %s' % (name, [d + 1 for d in ds]))
            return 1
    missing = [n for n in MEMBERS if n not in def_line]
    if missing:
        print('NOT FOUND %s' % missing)
        return 1

    def_lines = {d[0] for d in def_line.values()}
    base_sched = def_line['parked_'][0]        # scheduler vector declaration?
    # find the scheduler's vector declaration line explicitly
    sched_decl = None
    for i, ln in enumerate(lines):
        if ln.strip() == 'std::vector<std::shared_ptr<Process>> parked_;   // no automatic wakeup':
            sched_decl = i
    if sched_decl is None:
        print('scheduler parked_ declaration not found')
        return 1
    skip_lines = def_lines | {sched_decl}

    out = []
    subs = 0
    for i, ln in enumerate(lines):
        if i in skip_lines:
            if i == min(def_lines):
                out.append(EXEC_STATE.replace('\n', sep))
            continue
        new = ln
        if not is_comment(ln):
            for name in MEMBERS:
                if name == 'parked_':
                    if SCHED_PAT.search(new):
                        continue
                    pat = r'(?<![\w>])parked_\b'
                elif name == 'cond_':
                    pat = r'(?<![\w>.])(?<!st\.)(?<!sig\.)cond_\b'
                else:
                    pat = r'(?<![\w>])' + re.escape(name) + r'\b'
                new2 = re.sub(pat, 'es_->' + name, new)
                subs += len(re.findall(pat, new))
                new = new2
        out.append(new)

    text = sep.join(out)

    # VmRunState: keep only the chain's state pointer + owner tid
    i = text.find('struct VmRunState {')
    if i < 0:
        print('VmRunState not found')
        return 1
    j = text.find('};', i)
    text = text[:i] + '''struct VmRunState {
        // The chain's execution state.  It used to be a COPY of the VM's 19
        // execution members, moved in and out on every switch
        // (stashRunState/restoreRunState); carrying the state itself makes a
        // switch a pointer assignment, so a missed switch cannot lose frames.
        std::shared_ptr<ExecState> es;
        int64_t ownerTid = -1;
    };''' + text[j + 2:]

    open(SRC, 'wb').write(text.encode('utf-8'))
    print('OK ExecState installed, %d substitutions' % subs)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
