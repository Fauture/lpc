# -*- coding: utf-8 -*-
"""Patch 3: (a) exclusive/self time in the CPU profiler + configurable window,
(b) replace tryCallOtherObject's full-registry suffix scan with the ordered
    key index (measured 10-14.5M hasSuffix iterations per 5s during a login).
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


# --------------------------------------------------- (a) self time + window
rep(
    """struct CpuProfSlot {
    const char* name;
    std::atomic<int64_t> calls{0};
    std::atomic<int64_t> ns{0};
    std::atomic<int64_t> extra{0};   // site-specific counter (loop iterations, ...)
};""",
    """struct CpuProfSlot {
    const char* name;
    std::atomic<int64_t> calls{0};
    std::atomic<int64_t> ns{0};      // inclusive: this scope plus everything it called
    std::atomic<int64_t> self{0};    // exclusive: this scope minus instrumented children
    std::atomic<int64_t> extra{0};   // site-specific counter (loop iterations, ...)
};""")

rep(
    """    CpuProfSlot* s = new CpuProfSlot{n, {}, {}, {}};""",
    """    CpuProfSlot* s = new CpuProfSlot{n, {}, {}, {}, {}};""")

rep(
    """struct CpuProfGuard {
    CpuProfSlot* s = nullptr;
    int64_t t0 = 0;
    explicit CpuProfGuard(const char* n) {
        static const std::atomic<bool>& on = dynProbeFlag("LPCVM_CPUPROF");
        if (!on.load(std::memory_order_relaxed)) return;
        s = cpuProfGet(n);
        t0 = cpuProfNowNs();
    }
    ~CpuProfGuard() {
        if (!s) return;
        s->calls.fetch_add(1, std::memory_order_relaxed);
        s->ns.fetch_add(cpuProfNowNs() - t0, std::memory_order_relaxed);
    }
};""",
    """// Active guard stack, so a site's SELF time can exclude the children it
// called (the inclusive totals alone over-count heavily: one login window
// reported 15.8s of "op8" inside 5s of wall clock because every nested
// cross-object call is counted again).
struct CpuProfFrame {
    CpuProfSlot* slot;
    int64_t startNs;
    int64_t childNs;
};
std::vector<CpuProfFrame>& cpuProfStack() { static thread_local std::vector<CpuProfFrame> v; return v; }

struct CpuProfGuard {
    CpuProfSlot* s = nullptr;
    explicit CpuProfGuard(const char* n) {
        static const std::atomic<bool>& on = dynProbeFlag("LPCVM_CPUPROF");
        if (!on.load(std::memory_order_relaxed)) return;
        s = cpuProfGet(n);
        cpuProfStack().push_back(CpuProfFrame{s, cpuProfNowNs(), 0});
    }
    ~CpuProfGuard() {
        if (!s) return;
        int64_t now = cpuProfNowNs();
        CpuProfFrame f = cpuProfStack().back();
        cpuProfStack().pop_back();
        int64_t d = now - f.startNs;
        int64_t selfNs = d - f.childNs;
        if (selfNs < 0) selfNs = 0;
        s->calls.fetch_add(1, std::memory_order_relaxed);
        s->ns.fetch_add(d, std::memory_order_relaxed);
        s->self.fetch_add(selfNs, std::memory_order_relaxed);
        if (!cpuProfStack().empty()) cpuProfStack().back().childNs += d;
    }
};""")

rep(
    """        int64_t c = s->calls.exchange(0), ns = s->ns.exchange(0), ex = s->extra.exchange(0);
        if (!c && !ex) continue;
        std::cerr << "  " << s->name << ": " << (ns / 1000000) << "ms over " << c
                  << " calls, avg " << (c ? ns / c : 0) << "ns";
        if (ex) std::cerr << ", extra=" << ex;
        std::cerr << "\\n";""",
    """        int64_t c = s->calls.exchange(0), ns = s->ns.exchange(0), ex = s->extra.exchange(0);
        int64_t selfNs = s->self.exchange(0);
        if (!c && !ex) continue;
        std::cerr << "  " << s->name << ": incl " << (ns / 1000000) << "ms / self "
                  << (selfNs / 1000000) << "ms over " << c
                  << " calls, avg " << (c ? ns / c : 0) << "ns";
        if (ex) std::cerr << ", extra=" << ex;
        std::cerr << "\\n";""")

rep(
    """        if (dynProbeFlag("LPCVM_CPUPROF").load(std::memory_order_relaxed)) {
            static int64_t cpuProfLastMs = 0;
            int64_t nowP = nowMs();
            if (cpuProfLastMs == 0) cpuProfLastMs = nowP;
            if (nowP - cpuProfLastMs >= 5000) {
                cpuProfLastMs = nowP;
                cpuProfDump();
            }
        }""",
    """        if (dynProbeFlag("LPCVM_CPUPROF").load(std::memory_order_relaxed)) {
            static const int64_t profWinMs = [] {
                const char* v = std::getenv("LPCVM_CPUPROF_MS");
                int64_t n = v ? std::atoll(v) : 5000;
                return n < 200 ? 200 : n;
            }();
            static int64_t cpuProfLastMs = 0;
            int64_t nowP = nowMs();
            if (cpuProfLastMs == 0) cpuProfLastMs = nowP;
            if (nowP - cpuProfLastMs >= profWinMs) {
                cpuProfLastMs = nowP;
                cpuProfDump();
            }
        }""")

# ------------------------------------------------ (b) indexed suffix lookup
rep(
    """// Registry maps that maintain the index from their own operations, so no
// registration site can forget to update it.""",
    """// Same lookup as regHasKeySuffix, but returns the VM (O(log n) through the
// ordered key index).  tryCallOtherObject's fallback otherwise scans EVERY
// registry key with a std::string hasSuffix - measured 10-14.5M iterations per
// 5s window during a login, i.e. hundreds of ms of pure string comparison.
std::shared_ptr<OriginalVm> regFindVmByKeySuffix(const std::string& suffix) {
    std::string nn = normKey(suffix);
    RegIdxGuard g;
    auto it = regIndex().objBySuffix.lower_bound(nn);
    for (; it != regIndex().objBySuffix.end(); ++it) {
        const std::string& k = it->first;
        if (k.size() >= nn.size()) {
            if (k.compare(k.size() - nn.size(), nn.size(), nn) == 0) {
                auto vit = regIndex().vmsByKey.find(k);
                if (vit != regIndex().vmsByKey.end()) return vit->second.vm;
                return std::shared_ptr<OriginalVm>();
            }
            if (k.compare(0, std::min(k.size(), nn.size()),
                          nn.substr(0, std::min(k.size(), nn.size()))) > 0)
                break;
        }
    }
    return std::shared_ptr<OriginalVm>();
}

// Registry maps that maintain the index from their own operations, so no
// registration site can forget to update it.""")

rep(
    """        std::shared_ptr<OriginalVm> hit;
        CpuProfExtra _cpuProfSuffix("callOther.suffixScan");
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
        }""",
    """        std::shared_ptr<OriginalVm> hit;
        CpuProfExtra _cpuProfSuffix("callOther.suffixScan");
        static const std::atomic<bool>& idxSuffixFlag = dynProbeFlag("LPCVM_HOMEFIX");
        if (idxSuffixFlag.load(std::memory_order_relaxed)) {
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
        }""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s (%d lines)' % (P, d.count('\r\n') + 1))
