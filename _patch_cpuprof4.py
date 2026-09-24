# -*- coding: utf-8 -*-
"""Patch 4: make the indexed lookups the DEFAULT.

Measured on the real client (RDP): with LPCVM_HOMEFIX armed
  build_magic_pennant_skills  853ms -> 242ms
  callOther self              664ms -> 6ms      (per 1s window)
  callOther.suffixScan iters  10,258,680 -> 871
  findFunctionAny self        205ms -> 22ms
Semantics are unchanged (ownsFunctionFast is the same pointer test,
findFunctionAnyIdx reproduces findFunctionAny's first-match/last-match rules,
regFindVmByKeySuffix mirrors the hasSuffix scan through the ordered key index),
so the fix is now on by default; LPCVM_NO_HOMEFIX restores the old paths for A/B.
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


# one helper next to the registry index helpers
rep(
    """// Same lookup as regHasKeySuffix, but returns the VM (O(log n) through the""",
    """// The indexed lookups (ownsFunctionFast / findFunctionAnyIdx /
// regFindVmByKeySuffix) are the DEFAULT: they are semantically identical to the
// linear scans and remove an O(registry) / O(constitution) walk from the hottest
// cross-object path.  Measured on the real client with LPCVM_HOMEFIX armed:
// build_magic_pennant_skills 853ms -> 242ms, callOther self 664ms -> 6ms and
// callOther.suffixScan 10,258,680 -> 871 iterations per 1s window.
// LPCVM_NO_HOMEFIX restores the old linear paths for A/B.
bool objIdxFixOn() {
    static const std::atomic<bool>& off = dynProbeFlag("LPCVM_NO_HOMEFIX");
    return !off.load(std::memory_order_relaxed);
}

// Same lookup as regHasKeySuffix, but returns the VM (O(log n) through the""")

rep(
    """        static const std::atomic<bool>& homeFixI = dynProbeFlag("LPCVM_HOMEFIX");
        if (homeFixI.load(std::memory_order_relaxed)) return findFunctionAnyIdx(name);""",
    """        if (objIdxFixOn()) return findFunctionAnyIdx(name);""")

rep(
    """    static const std::atomic<bool>& homeFixFlag = dynProbeFlag("LPCVM_HOMEFIX");
    const bool homeFix = homeFixFlag.load(std::memory_order_relaxed);""",
    """    const bool homeFix = objIdxFixOn();""")

rep(
    """        static const std::atomic<bool>& idxSuffixFlag = dynProbeFlag("LPCVM_HOMEFIX");
        if (idxSuffixFlag.load(std::memory_order_relaxed)) {""",
    """        if (objIdxFixOn()) {""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s' % P)
