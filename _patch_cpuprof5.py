# -*- coding: utf-8 -*-
"""Patch 5: narrow the default.

The all-on default hung the boot at "还没有从 CONFIG_D 取得服务器所在区组名" (the
runtime A/B was fine because it was armed AFTER boot).  The two risky changes are
now opt-in again and only the proven suffix-index fix stays default:

  default ON   : regFindVmByKeySuffix in tryCallOtherObject  (the 664ms/1s scan;
                 this alone is what the profile attributed the pennant's 853ms to)
  LPCVM_HOMEFIX: + ownsFunctionFast, + findFunctionAnyIdx, + object-wide cache warm
  LPCVM_NO_OBJIDX: restore the old linear suffix scan (A/B)

The object-wide cache warm in originalHomeVmOf is DROPPED: it assumed every
function of an object resolves to the same VM, which is false when several VMs
share one OriginalObject (proto vs live instance - exactly the taskd case), so it
could hand a callee the wrong instance's variables.
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


# 1. helper: suffix index ON by default, the rest opt-in
rep(
    """bool objIdxFixOn() {
    static const std::atomic<bool>& off = dynProbeFlag("LPCVM_NO_HOMEFIX");
    return !off.load(std::memory_order_relaxed);
}""",
    """bool objIdxFixOn() {
    static const std::atomic<bool>& off = dynProbeFlag("LPCVM_NO_OBJIDX");
    return !off.load(std::memory_order_relaxed);
}

// The heavier rewrites (own-function index, name index, object-wide cache warm)
// stay OPT-IN: enabling them from process start hung the boot at
// "还没有从 CONFIG_D 取得服务器所在区组名", while arming them after boot was fine.
bool heavyIdxFixOn() {
    static const std::atomic<bool>& on = dynProbeFlag("LPCVM_HOMEFIX");
    return on.load(std::memory_order_relaxed);
}""")

rep(
    """        if (objIdxFixOn()) return findFunctionAnyIdx(name);""",
    """        if (heavyIdxFixOn()) return findFunctionAnyIdx(name);""")

rep(
    """    const bool homeFix = objIdxFixOn();""",
    """    const bool homeFix = heavyIdxFixOn();""")

# 2. drop the unsafe object-wide cache warm
rep(
    """    if (homeFix && hit) {
        // Warm the WHOLE winning object at once: one registry scan then serves
        // every function of that constitution, instead of one scan per function
        // (a login packet builder touches hundreds of previously-uncalled
        // functions of ~26 skill objects).
        for (const auto& f : hit->object().functions) cache[&f] = hit;
    }
    return hit;""",
    """    // NOTE: an "warm every function of the winning object" shortcut was tried
    // here and REMOVED: it assumed all functions of one OriginalObject resolve
    // to the same VM, which is false when several VMs share that object (proto
    // vs live instance), so it could bind a callee to the wrong instance.
    return hit;""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s' % P)
