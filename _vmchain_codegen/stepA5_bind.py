#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A5: the running process explicitly OWNS the VM's active chain state.

    python -X utf8 _vmchain_codegen/stepA5_explicit_owner.py

Why: Step A4 (turning the shim macros into accessors) was measured to be a
DETERMINISTIC regression - 3/3 runs produced exactly one 9112 「数据库目前正在维护中」
(a filed waiter woken with the wrong result) and 9101 went 6 -> 9, while the
macro version produced 0x9112 and 6x9101 in 3/3 runs.  The difference is that the
accessor versions changed WHEN the active state is reached relative to the wake
path, because "which chain is active" was only implied by `activeKey_`/`es_`.

This step removes the implication: a process records the chain key it runs
(`Process::chainSeq`, which is already set for every spawn and every wake), and
the scheduler binds the VM's active state to that key when it runs the process.
`switchToChain()` then only ever installs a state the RUNNING PROCESS owns.

Not part of this step: deleting switchToChain entirely.  A process can nest
entries (a create command running on the login clone, its continuation on
another object), so the key is bound at each scheduler entry point rather than
only at spawn.
"""
import io

SRC = r'src\original_vm.cpp'

ANCHOR = '    int64_t activeChainKey() const { return activeKey_; }'
NEW = '''    int64_t activeChainKey() const { return activeKey_; }
    // Bind this VM's active execution state to the chain the SCHEDULER is running
    // (rpc/scheduler side only; the interpreter never calls this).  `key` is the
    // running process's own chain id, so the state a VM exposes is always the one
    // its current process owns - the property the accessor refactor (Step A4)
    // needs, and the reason wake paths no longer depend on when `es_` was set.
    void bindActiveChain(int64_t key) {
        if (key < 0) return;
        switchToChain(key);
    }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    a = ANCHOR.replace('\n', sep)
    if 'bindActiveChain' in t:
        print('already present')
        return 0
    if t.count(a) < 1:
        print('anchor not found')
        return 1
    t = t.replace(a, NEW.replace('\n', sep), 1)
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK bindActiveChain added')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
