# -*- coding: utf-8 -*-
"""Patch 6: replace the (boot-breaking) suffix index with a memo of the ORIGINAL scan.

Why the index approach was wrong: tryCallOtherObject's fallback picks the first
registry key (iteration order of reg.vms) that hasSuffix(rel).  `rel` for the most
common case ("gs/daemons/configd.o") matches BOTH "/gs/daemons/configd.o" and
"pak:gs/daemons/configd.o", and those are different instances (live vs proto).
GsRegIndex::vmsByKey/objBySuffix are keyed by the NORMALIZED key, so they keep
only one of them -> the indexed lookup returned the wrong instance and the boot
hung at "还没有从 CONFIG_D 取得服务器所在区组名".

The fix below keeps the scan EXACTLY as it was (same order, same predicate, same
winner) and only memoizes its RESULT per `rel`, invalidated by a registry
generation counter that every register/unregister bumps.
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


# 1. registry generation counter
rep(
    """GsRegIndex& regIndex() { static GsRegIndex i; return i; }""",
    """GsRegIndex& regIndex() { static GsRegIndex i; return i; }

// Bumped by every register/unregister.  A memo of a registry-wide scan is only
// valid while this value is unchanged.
std::atomic<uint64_t>& regGeneration() {
    static std::atomic<uint64_t> g{1};
    return g;
}""")

rep(
    """        idx.objBySuffix[nk] = nk;
    }
}""",
    """        idx.objBySuffix[nk] = nk;
    }
    regGeneration().fetch_add(1, std::memory_order_relaxed);
}""")

rep(
    """void regIndexUnregister(const std::string& key) {""",
    """void regIndexUnregister(const std::string& key) {
    regGeneration().fetch_add(1, std::memory_order_relaxed);""")

# 2. memo helpers (placed next to the other registry helpers)
rep(
    """// The indexed lookups (ownsFunctionFast / findFunctionAnyIdx /
// regFindVmByKeySuffix) are the DEFAULT""",
    """// Memo for tryCallOtherObject's suffix fallback: key -> (VM or "no match").
// The scan itself stays byte-for-byte the original one; only its result is
// cached, and any registry change (regGeneration) invalidates the whole memo.
struct SuffixMemoEntry {
    std::weak_ptr<OriginalVm> vm;
    bool found = false;
    uint64_t gen = 0;
};
std::unordered_map<std::string, SuffixMemoEntry>& suffixMemo() {
    static std::unordered_map<std::string, SuffixMemoEntry> m;
    return m;
}
std::mutex& suffixMemoMx() {
    static std::mutex m;
    return m;
}

// The indexed lookups (ownsFunctionFast / findFunctionAnyIdx /
// regFindVmByKeySuffix) are the DEFAULT""")

# 3. the fallback: memo first, original scan on a miss
rep(
    """        if (objIdxFixOn()) {
            hit = regFindVmByKeySuffix(rel);
            _cpuProfSuffix.add();
        } else {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                _cpuProfSuffix.add();
                if (hasSuffix(kv.first, rel)) {
                    if (vmEnvTrace() && funName == "get_my_info")
                        std::cerr << "[call_other] -> " << kv.first << " (suffix)\\n";
                    hit = kv.second;
                    break;
                }
            }
        }""",
    """        // LPCVM_NO_OBJIDX restores the un-memoized scan (A/B).
        const uint64_t regGen = regGeneration().load(std::memory_order_relaxed);
        bool haveMemo = false;
        if (objIdxFixOn()) {
            std::lock_guard<std::mutex> lk(suffixMemoMx());
            auto mit = suffixMemo().find(rel);
            if (mit != suffixMemo().end()) {
                if (mit->second.gen == regGen) {
                    if (!mit->second.found) {
                        haveMemo = true;             // memoized "no match at all"
                    } else {
                        hit = mit->second.vm.lock();
                        haveMemo = (hit != nullptr);
                    }
                } else {
                    suffixMemo().erase(mit);
                }
            }
        }
        if (!haveMemo) {
            hit.reset();
            {
                std::lock_guard<std::recursive_mutex> rl(registryMutex());
                for (auto& kv : reg.vms) {
                    _cpuProfSuffix.add();
                    if (hasSuffix(kv.first, rel)) {
                        if (vmEnvTrace() && funName == "get_my_info")
                            std::cerr << "[call_other] -> " << kv.first << " (suffix)\\n";
                        hit = kv.second;
                        break;
                    }
                }
            }
            if (objIdxFixOn()) {
                std::lock_guard<std::mutex> lk(suffixMemoMx());
                SuffixMemoEntry e;
                e.vm = hit;
                e.found = (hit != nullptr);
                e.gen = regGen;
                suffixMemo()[rel] = e;
            }
        }""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s' % P)
