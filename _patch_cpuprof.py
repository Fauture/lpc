# -*- coding: utf-8 -*-
"""Add LPCVM_CPUPROF (per-site inclusive CPU profiler) + LPCVM_HOMEFIX
(O(1) ownsFunction + whole-object cache warm) to src/original_vm.cpp.

Keeps CRLF (MSVC needs it: LF makes the compiler swallow braces -> C3254/C2838).
Every anchor must match exactly once, otherwise the script aborts untouched.
"""
import sys

P = 'src/original_vm.cpp'
d = open(P, encoding='utf-8', newline='').read()
assert '\r\n' in d and d.count('\r\n') == d.count('\n'), 'file is not pure CRLF'


def A(s):
    return s.replace('\n', '\r\n')


def rep(old, new, cnt=1):
    global d
    o, n = A(old), A(new)
    c = d.count(o)
    if c != cnt:
        sys.exit('ANCHOR %d != %d for:\n%s' % (c, cnt, old[:120]))
    d = d.replace(o, n, cnt)


# ---------------------------------------------------------------- 1. profiler
rep(
    """    return s->on;
}

// Cached copies of the LPCVM_* trace/diagnostic environment switches.""",
    """    return s->on;
}

// ---------------------------------------------------------------------------
// LPCVM_CPUPROF: inclusive per-call-site CPU profiler.
//
// Why it exists: [op-prof] charges the gap between two interpreter iterations
// to the PREVIOUS opcode, and that gap includes any park/resume (scheduler and
// DB wait) - so its numbers cannot separate CPU from waiting.  This profiler
// times NAMED C++ scopes with a RAII guard and dumps calls / total ms / avg ns
// for each site every 5s while the (dyn-switchable) probe is armed.
// ---------------------------------------------------------------------------
namespace {
struct CpuProfSlot {
    const char* name;
    std::atomic<int64_t> calls{0};
    std::atomic<int64_t> ns{0};
    std::atomic<int64_t> extra{0};   // site-specific counter (loop iterations, ...)
};
std::vector<CpuProfSlot*>& cpuProfSlots() { static std::vector<CpuProfSlot*> v; return v; }
std::mutex& cpuProfMx() { static std::mutex m; return m; }

CpuProfSlot* cpuProfGet(const char* n) {
    std::lock_guard<std::mutex> lk(cpuProfMx());
    for (CpuProfSlot* s : cpuProfSlots())
        if (std::strcmp(s->name, n) == 0) return s;
    CpuProfSlot* s = new CpuProfSlot{n, {}, {}, {}};
    cpuProfSlots().push_back(s);
    return s;
}
inline int64_t cpuProfNowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}
}  // namespace

struct CpuProfGuard {
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
};

// Bare counter (same switch) for work that is not a scope (loop iterations).
struct CpuProfExtra {
    CpuProfSlot* s = nullptr;
    explicit CpuProfExtra(const char* n) {
        static const std::atomic<bool>& on = dynProbeFlag("LPCVM_CPUPROF");
        if (on.load(std::memory_order_relaxed)) s = cpuProfGet(n);
    }
    inline void add(int64_t k = 1) { if (s) s->extra.fetch_add(k, std::memory_order_relaxed); }
};

void cpuProfDump() {
    std::lock_guard<std::mutex> lk(cpuProfMx());
    if (cpuProfSlots().empty()) return;
    std::vector<CpuProfSlot*> v = cpuProfSlots();
    std::sort(v.begin(), v.end(), [](CpuProfSlot* a, CpuProfSlot* b) {
        return a->ns.load() > b->ns.load();
    });
    std::cerr << "[cpuprof] --- per-site inclusive CPU (last window) ---\\n";
    for (CpuProfSlot* s : v) {
        int64_t c = s->calls.exchange(0), ns = s->ns.exchange(0), ex = s->extra.exchange(0);
        if (!c && !ex) continue;
        std::cerr << "  " << s->name << ": " << (ns / 1000000) << "ms over " << c
                  << " calls, avg " << (c ? ns / c : 0) << "ns";
        if (ex) std::cerr << ", extra=" << ex;
        std::cerr << "\\n";
    }
    std::cerr.flush();
}

// Cached copies of the LPCVM_* trace/diagnostic environment switches.""")

# ------------------------------------------------- 2. O(1) ownsFunction variant
rep(
    """    bool ownsFunction(const OriginalFunction* fn) const {
        for (const auto& f : obj_.functions)
            if (&f == fn) return true;
        auto it = funcsByIndex_.find(fn->index);
        return it != funcsByIndex_.end() && it->second == fn;
    }""",
    """    bool ownsFunction(const OriginalFunction* fn) const {
        for (const auto& f : obj_.functions)
            if (&f == fn) return true;
        auto it = funcsByIndex_.find(fn->index);
        return it != funcsByIndex_.end() && it->second == fn;
    }
    // Same predicate, O(1): OriginalObject::functions is a contiguous vector, so
    // "is fn one of MY functions" is a pointer-range test instead of a walk over
    // every function of the constitution.  ownsFunction() is called once per
    // registered VM by originalHomeVmOf(), whose loop runs over the WHOLE
    // registry (~23k VMs), so the linear form costs ~100ns x 23k per lookup.
    bool ownsFunctionFast(const OriginalFunction* fn) const {
        if (!fn) return false;
        const auto& v = obj_.functions;
        if (!v.empty() && fn >= v.data() && fn < v.data() + v.size()) return true;
        auto it = funcsByIndex_.find(fn->index);
        return it != funcsByIndex_.end() && it->second == fn;
    }""")

# ---------------------------------------------------- 3. callFunctionByName
rep(
    """    Value callFunctionByName(const std::string& name, const std::vector<Value>& args) {
        // Per-process run state (the original's per-PCB frame stack).""",
    """    Value callFunctionByName(const std::string& name, const std::vector<Value>& args) {
        CpuProfGuard _cpuProfByName("callByName");
        // Per-process run state (the original's per-PCB frame stack).""")

# --------------------------------------------------- 4. startInlineFunction
rep(
    """        checkCallDepth(fn.name);
        std::shared_ptr<std::vector<Value>> autoVars;""",
    """        CpuProfGuard _cpuProfInline("startInline");
        checkCallDepth(fn.name);
        std::shared_ptr<std::vector<Value>> autoVars;""")

# ----------------------------------------------------- 5. callEfunWithArgs
rep(
    """    Value callEfunWithArgs(int efunIndex, const std::vector<Value>& args) {
        stackDbgCheck("efun");""",
    """    Value callEfunWithArgs(int efunIndex, const std::vector<Value>& args) {
        CpuProfGuard _cpuProfEfun("efun");
        stackDbgCheck("efun");""")

# ------------------------------------------------------ 6. originalHomeVmOf
rep(
    """    static std::unordered_map<const OriginalFunction*, const OriginalVm*> cache;
    static std::mutex mtx;
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = cache.find(fn);
        if (it != cache.end()) return it->second;
    }
    const OriginalVm* hit = nullptr;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());""",
    """    CpuProfGuard _cpuProfHome("homeVm");
    CpuProfExtra _cpuProfMiss("homeVm.miss");
    CpuProfExtra _cpuProfScan("homeVm.scan");
    static std::unordered_map<const OriginalFunction*, const OriginalVm*> cache;
    static std::mutex mtx;
    static const std::atomic<bool>& homeFixFlag = dynProbeFlag("LPCVM_HOMEFIX");
    const bool homeFix = homeFixFlag.load(std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = cache.find(fn);
        if (it != cache.end()) return it->second;
    }
    _cpuProfMiss.add();
    const OriginalVm* hit = nullptr;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());""")

rep(
    """        int bestInit = -1;
        for (auto& kv : objectRegistry().vms) {
            if (!kv.second || !kv.second->ownsFunction(fn)) continue;""",
    """        int bestInit = -1;
        for (auto& kv : objectRegistry().vms) {
            _cpuProfScan.add();
            if (!kv.second) continue;
            if (!(homeFix ? kv.second->ownsFunctionFast(fn)
                          : kv.second->ownsFunction(fn))) continue;""")

rep(
    """    std::lock_guard<std::mutex> lk(mtx);
    cache[fn] = hit;
    return hit;
}""",
    """    std::lock_guard<std::mutex> lk(mtx);
    cache[fn] = hit;
    if (homeFix && hit) {
        // Warm the WHOLE winning object at once: one registry scan then serves
        // every function of that constitution, instead of one scan per function
        // (a login packet builder touches hundreds of previously-uncalled
        // functions of ~26 skill objects).
        for (const auto& f : hit->object().functions) cache[&f] = hit;
    }
    return hit;
}""")

# ------------------------------------------------------- 7. homeVmMutable
rep(
    """std::shared_ptr<OriginalVm> homeVmMutable(const OriginalFunction* fn) {
    const OriginalVm* h = originalHomeVmOf(fn);
    if (!h) return nullptr;
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    for (auto& kv : objectRegistry().vms)
        if (kv.second.get() == h) return kv.second;""",
    """std::shared_ptr<OriginalVm> homeVmMutable(const OriginalFunction* fn) {
    CpuProfGuard _cpuProfHomeMut("homeVmMutable");
    CpuProfExtra _cpuProfHomeMutScan("homeVmMutable.scan");
    const OriginalVm* h = originalHomeVmOf(fn);
    if (!h) return nullptr;
    std::lock_guard<std::recursive_mutex> rl(registryMutex());
    for (auto& kv : objectRegistry().vms) {
        _cpuProfHomeMutScan.add();
        if (kv.second.get() == h) return kv.second;
    }""")

# ----------------------------------------------------- 8. tryCallOtherObject
rep(
    """                         const std::string& callerThisName) {
    RegScanT _regScan;""",
    """                         const std::string& callerThisName) {
    CpuProfGuard _cpuProfOther("callOther");
    RegScanT _regScan;""")

rep(
    """        std::shared_ptr<OriginalVm> hit;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                if (hasSuffix(kv.first, rel)) {""",
    """        std::shared_ptr<OriginalVm> hit;
        CpuProfExtra _cpuProfSuffix("callOther.suffixScan");
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {
                _cpuProfSuffix.add();
                if (hasSuffix(kv.first, rel)) {""")

# ------------------------------------------------------------ 9. opcode cases
for op, name in ((5, 'call_fun_ptr'), (6, 'call_in_efun'), (7, 'call_in_user_fun'),
                 (8, 'call_in_other_fun_ex')):
    rep('            case %d: { // %s\n' % (op, name),
        '            case %d: { // %s\n                CpuProfGuard _cpuProfOp%d("op%d");\n'
        % (op, name, op, op))

# --------------------------------------------------------- 10. loadObjectIntoRegistry
rep(
    """                           const std::vector<Value>& createArgs) {
    if (searchRoot.empty() || virtualPath.empty()) return 0;""",
    """                           const std::vector<Value>& createArgs) {
    CpuProfGuard _cpuProfLoadObj("loadObject");
    if (searchRoot.empty() || virtualPath.empty()) return 0;""")

# ------------------------------------------------------------- 11. dump hook
rep(
    """        // LPCVM_MEMSTAT: once per second, census the big allocators.
        if (std::getenv("LPCVM_MEMSTAT")) {""",
    """        // LPCVM_CPUPROF: dump the per-site inclusive CPU profiler every 5s.
        if (dynProbeFlag("LPCVM_CPUPROF").load(std::memory_order_relaxed)) {
            static int64_t cpuProfLastMs = 0;
            int64_t nowP = nowMs();
            if (cpuProfLastMs == 0) cpuProfLastMs = nowP;
            if (nowP - cpuProfLastMs >= 5000) {
                cpuProfLastMs = nowP;
                cpuProfDump();
            }
        }
        // LPCVM_MEMSTAT: once per second, census the big allocators.
        if (std::getenv("LPCVM_MEMSTAT")) {""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s (%d lines)' % (P, d.count('\r\n') + 1))
