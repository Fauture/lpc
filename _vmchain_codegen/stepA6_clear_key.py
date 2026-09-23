#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A6 experiment: clear the chain key after a run instead of restoring it.

    python -X utf8 _vmchain_codegen/stepA6_clear_key.py

`runProcess` sets the thread-local chain key to the process's own `chainSeq`, and
the interpreter uses that key at its six entry points to install the right
per-chain execution state.  A3 never cleared it (so a finished process's key
lingered), A5 restored the PREVIOUS process's key - and A5 measured 3/3 runs with
1x9112, i.e. restoring is worse.

This variant CLEARS it instead: after a run returns, the key is `-1`, so a nested
or cross-object entry that has no key of its own falls to `defaultEs_` (a clean
state) rather than inheriting whichever chain happened to run before.

Judged by the same yardstick as every other variant: 3 real-client 20x ENTER runs
must show 0x9112.
"""
import io

SRC = r'src\original_vm.cpp'

OLD = '''    if (p->tid >= 0) setChainTid(p->tid);
    if (p->chainSeq > 0) setChainKey(p->chainSeq);'''

NEW = '''    if (p->tid >= 0) setChainTid(p->tid);
    // The chain key selects the per-chain execution state at the interpreter's
    // entry points.  It is CLEARED (not restored to the previous process's key)
    // when this run returns: an entry with no key of its own then starts from a
    // clean state instead of inheriting an unrelated chain's frames.  Restoring
    // the previous key was measured to be worse than leaving it stale (3/3 runs
    // with one 9112 vs 0 in the stale version), so clearing is the variant under
    // test here.  See docs/scheduler_refactor_plan.md section 9.
    struct ChainKeyScope {
        ~ChainKeyScope() { lpc::scheduler().setChainKey(-1); }
    } chainKeyScope;
    if (p->chainSeq > 0) lpc::scheduler().setChainKey(p->chainSeq);'''


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
    print('OK chain key is cleared after each run')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
