# -*- coding: utf-8 -*-
"""Patch 7: make the two remaining indexed lookups the default (opt-out).

After the boot hang was traced to the suffix-index rewrite (now replaced by the
generation-validated memo), ownsFunctionFast / findFunctionAnyIdx can be enabled
from process start.  They become default ON, with LPCVM_NO_HOMEFIX as the opt-out
for A/B.  regFindVmByKeySuffix is deleted: it is the trap that hung the boot
(normalized-key index keeps only one of the live/proto spellings).
"""
import sys

P = 'src/original_vm.cpp'
d = open(P, encoding='utf-8', newline='').read()
assert d.count('\r\n') == d.count('\n'), 'file is not pure CRLF'


def A(s):
    return s.replace('\n', '\r\n')


def rep(old, new, cnt=1):
    global d
    o, n = A(old), A(new)
    c = d.count(o)
    if c != cnt:
        sys.exit('ANCHOR %d != %d for:\n%s' % (c, cnt, old[:140]))
    d = d.replace(o, n, cnt)


rep(
    """// The heavier rewrites (own-function index, name index, object-wide cache warm)
// stay OPT-IN: enabling them from process start hung the boot at
// "还没有从 CONFIG_D 取得服务器所在区组名", while arming them after boot was fine.
bool heavyIdxFixOn() {
    static const std::atomic<bool>& on = dynProbeFlag("LPCVM_HOMEFIX");
    return on.load(std::memory_order_relaxed);
}""",
    """// The two O(constitution) rewrites (own-function range test, name index) are
// default ON too; the boot hang attributed to "the heavier rewrites" turned out
// to come from the suffix-index rewrite, which is gone (see the memo above).
// LPCVM_NO_HOMEFIX restores the linear paths for A/B.
bool heavyIdxFixOn() {
    static const std::atomic<bool>& off = dynProbeFlag("LPCVM_NO_HOMEFIX");
    return !off.load(std::memory_order_relaxed);
}""")

# delete the trap
i = d.find(A("""// Same lookup as regHasKeySuffix, but returns the VM (O(log n) through the"""))
j = d.find(A("""// Registry maps that maintain the index from their own operations"""))
if i < 0 or j < 0 or j < i:
    sys.exit('cannot locate regFindVmByKeySuffix block (i=%d j=%d)' % (i, j))
block = d[i:j]
if 'regFindVmByKeySuffix' not in block:
    sys.exit('unexpected block:\n%s' % block[:200])
d = d[:i] + A("""// NOTE: an index-based rewrite of the suffix fallback (regFindVmByKeySuffix via
// GsRegIndex::objBySuffix / vmsByKey) was tried here and DELETED: those indexes
// are keyed by the NORMALIZED key, so "/gs/x.o" (live) and "pak:gs/x.o" (proto)
// collapse to one entry and the lookup returned the wrong instance - the boot
// hung at "还没有从 CONFIG_D 取得服务器所在区组名".  The registry iteration
// order of the original scan is part of its semantics; memoize it instead.

""") + d[j:]

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s' % P)
