# -*- coding: utf-8 -*-
"""Patch 2: find out where the login's CPU really goes.

1. instrument findFunctionAny (guard + scan counter) - it walks EVERY function
   of the constitution twice per name lookup, on every cross-object call.
2. instrument executeFunctionFromOutsideLocked (interpreter share).
3. global executed-instruction counter, dumped with each [cpuprof] window.
4. LPCVM_SLOW_OP: dyn-armable + configurable threshold + no 100ms rate limit
   (the rate limit hid all but 6 samples inside the 718ms window).
5. LPCVM_HOMEFIX also enables an indexed findFunctionAny (name -> function),
   so the same boot can A/B it.
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


# ------------------------------------------------ 1. instruction counter + dump
rep(
    """void cpuProfDump() {
    std::lock_guard<std::mutex> lk(cpuProfMx());""",
    """std::atomic<int64_t> g_insnCount{0};

void cpuProfDump() {
    int64_t insns = g_insnCount.exchange(0, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(cpuProfMx());""")

rep(
    """    std::cerr << "[cpuprof] --- per-site inclusive CPU (last window) ---\\n";""",
    """    std::cerr << "[cpuprof] --- per-site inclusive CPU (last window), insns="
              << insns << " ---\\n";""")

# ------------------------------------------- 2. count instructions in runLoop
rep(
    """            if (slowOpOn) {
                using namespace std::chrono;
                int64_t nowNs = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
                int64_t prevDur = nowNs - slowOpPrevNs;
                slowOpPrevNs = nowNs;
                if (prevDur > 8000 && (nowNs - slowOpLastReport) > 100000000) {""",
    """            if (insnCntOn) g_insnCount.fetch_add(1, std::memory_order_relaxed);
            if (slowOpOn) {
                using namespace std::chrono;
                int64_t nowNs = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
                int64_t prevDur = nowNs - slowOpPrevNs;
                slowOpPrevNs = nowNs;
                if (prevDur > slowOpMinNs && (slowOpNoLimit || (nowNs - slowOpLastReport) > 100000000)) {""")

# --------------------------------------------- 3. slow-op switches (dyn + tuned)
rep(
    """        static const bool slowOpOn = (std::getenv("LPCVM_SLOW_OP") != nullptr);""",
    """        // LPCVM_SLOW_OP is now a DYN probe (armable on a running server via
        // _probe_on\\LPCVM_SLOW_OP) with a configurable threshold and no 100ms
        // rate limit: inside the 718ms build_magic_pennant_skills window the old
        // rate limit reported only 6 of the thousands of slow calls.
        static const std::atomic<bool>& slowOpFlag = dynProbeFlag("LPCVM_SLOW_OP");
        const bool slowOpOn = slowOpFlag.load(std::memory_order_relaxed);
        static const std::atomic<bool>& insnCntFlag = dynProbeFlag("LPCVM_CPUPROF");
        const bool insnCntOn = insnCntFlag.load(std::memory_order_relaxed);
        static const int64_t slowOpMinNs = [] {
            const char* v = std::getenv("LPCVM_SLOW_OP_US");
            return v ? std::atoll(v) * 1000 : 8000;
        }();
        static const bool slowOpNoLimit = (std::getenv("LPCVM_SLOW_OP_NOLIMIT") != nullptr);""")

# ---------------------------------------- 4. instrument findFunctionAny + index
rep(
    """    const OriginalFunction* findFunctionAny(const std::string& name) const {
        for (const auto& fn : obj_.functions)
            if (fn.name == name && fn.index >= 0) return &fn;""",
    """    // LPCVM_HOMEFIX: name -> function index, built once per VM and rebuilt when
    // the merged table changes.  findFunctionAny() otherwise walks EVERY
    // function of the constitution TWICE per lookup (obj_.functions plus the
    // whole funcsByIndex_ map, and the second pass has no early exit) comparing
    // std::string names - and it runs on every cross-object call resolved by
    // name, i.e. on the hottest path of the login.
    mutable std::unordered_map<std::string, const OriginalFunction*> nameIdxFirst_;
    mutable std::unordered_map<std::string, const OriginalFunction*> nameIdxImpl_;
    mutable uint64_t fnStamp_ = 0;
    mutable uint64_t nameIdxStamp_ = ~0ull;
    void bumpFnStamp() const { ++fnStamp_; }
    void buildNameIdx() const {
        nameIdxFirst_.clear();
        nameIdxImpl_.clear();
        for (const auto& fn : obj_.functions)
            if (fn.index >= 0) nameIdxFirst_.emplace(fn.name, &fn);        // first wins
        for (const auto& kv : funcsByIndex_)
            if (kv.second && kv.second->index >= 0 && !kv.second->code.empty())
                nameIdxImpl_[kv.second->name] = kv.second;                 // last wins
        nameIdxStamp_ = fnStamp_;
    }
    const OriginalFunction* findFunctionAnyIdx(const std::string& name) const {
        if (nameIdxStamp_ != fnStamp_) buildNameIdx();
        auto it = nameIdxFirst_.find(name);
        if (it != nameIdxFirst_.end()) return it->second;
        auto it2 = nameIdxImpl_.find(name);
        return it2 == nameIdxImpl_.end() ? nullptr : it2->second;
    }
    const OriginalFunction* findFunctionAny(const std::string& name) const {
        CpuProfGuard _cpuProfFind("findFunctionAny");
        static const std::atomic<bool>& homeFixI = dynProbeFlag("LPCVM_HOMEFIX");
        if (homeFixI.load(std::memory_order_relaxed)) return findFunctionAnyIdx(name);
        CpuProfExtra _cpuProfFindScan("findFunctionAny.scan");
        for (const auto& fn : obj_.functions) {
            _cpuProfFindScan.add();
            if (fn.name == name && fn.index >= 0) return &fn;
        }""")

rep(
    """        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_)
            if (kv.second->name == name && kv.second->index >= 0 && !kv.second->code.empty())
                impl = kv.second;   // keep the last (highest-key) implementation
        return impl;
    }""",
    """        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_) {
            _cpuProfFindScan.add();
            if (kv.second && kv.second->name == name && kv.second->index >= 0 &&
                !kv.second->code.empty())
                impl = kv.second;   // keep the last (highest-key) implementation
        }
        return impl;
    }""")

# stamp bumps at every funcsByIndex_ write site
rep(
    """            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;""",
    """            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
            bumpFnStamp();""", 3)
rep(
    """            funcsByIndex_ = tmpl->funcsByIndex_;""",
    """            funcsByIndex_ = tmpl->funcsByIndex_;
            bumpFnStamp();""")
rep(
    """                        funcsByIndex_[childIndex] = inhFn.get();""",
    """                        funcsByIndex_[childIndex] = inhFn.get();
                        bumpFnStamp();""")

# ------------------------------- 5. instrument the outside-entry (interpreter)
rep(
    """    Value executeFunctionFromOutsideLocked(const OriginalFunction& fn, const std::vector<Value>& args) {
        VmNestGuard nestGuard("outside");""",
    """    Value executeFunctionFromOutsideLocked(const OriginalFunction& fn, const std::vector<Value>& args) {
        CpuProfGuard _cpuProfEfo("execOutside");
        VmNestGuard nestGuard("outside");""")

open(P, 'w', encoding='utf-8', newline='').write(d)
print('patched %s (%d lines)' % (P, d.count('\r\n') + 1))
