#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A5: restore the previous chain key when a process finishes its run.

    python -X utf8 _vmchain_codegen/stepA5_restore_key.py

THE BUG THIS FIXES
------------------
`Scheduler::runProcess` did:

    if (p->tid >= 0)      setChainTid(p->tid);        // saved/restored by callers
    if (p->chainSeq > 0)  setChainKey(p->chainSeq);   // NEVER restored

but the interpreter reads the chain key at FIVE entry points
(`switchToChain(lpc::scheduler().chainKey())` in executeFunctionFromOutside,
executeFunctionOnStack, resumeProcess, resumeCaller, resumeRunLoop, enterChain)
to decide which chain's execution state to install on the VM.

So after a process finished, its key stayed installed; the next entry with no key
of its own - a nested entry, a cross-object call, a wake path - then switched the
VM to THAT chain's state.  The VM could end up holding another chain's frames,
which is exactly the class of failure this refactor is meant to remove, and it is
what made the accessor refactor (Step A4) a deterministic 9112 regression while
the macro version merely got lucky.

FIX: save the previous key and restore it when the run returns, like `chainTid`
already does.  Scope-bound so every return path (normal, park, slice, exit) is
covered.
"""
import io

SRC = r'src\original_vm.cpp'

OLD = '''    if (p->tid >= 0) setChainTid(p->tid);
    if (p->chainSeq > 0) setChainKey(p->chainSeq);'''

NEW = '''    if (p->tid >= 0) setChainTid(p->tid);
    // The chain key is what the interpreter uses to install the right per-chain
    // execution state (see switchToChain at the entry points).  It MUST be
    // restored on the way out: a finished process used to leave its key behind,
    // and the next nested/cross-object entry then switched the VM to another
    // chain's state - frames from the wrong chain, which is the failure mode this
    // refactor exists to remove.
    struct ChainKeyScope {
        int64_t prev;
        bool armed;
        ChainKeyScope() : prev(lpc::scheduler().chainKey()), armed(false) {}
        ~ChainKeyScope() {
            if (armed) lpc::scheduler().setChainKey(prev);
        }
    } chainKeyScope;
    if (p->chainSeq > 0) {
        lpc::scheduler().setChainKey(p->chainSeq);
        chainKeyScope.armed = true;
    }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    o, n = OLD.replace('\n', sep), NEW.replace('\n', sep)
    if 'ChainKeyScope' in t:
        print('already present')
        return 0
    if t.count(o) != 1:
        print('anchor COUNT %d' % t.count(o))
        return 1
    t = t.replace(o, n, 1)
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK chain key is saved/restored around runProcess')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
