# -*- coding: utf-8 -*-
"""Patch 8: replace the per-VM name index with a per-function name HASH.

Measured problem: the per-VM `unordered_map<string, fn*>` name index added
~400MB (gs working set 1080MB -> 1500MB) because every one of the ~23k VMs built
its own map.  A 64-bit FNV-1a hash stored ON the function (8 bytes/function,
filled once by the loader) gives the same effect with no per-VM containers: the
two full scans stay byte-for-byte the same (same order, same first-match /
last-match rules, std::string compare as the final arbiter) - only the number of
string comparisons drops.
"""
import sys

P = 'src/original_vm.cpp'
H = 'src/o_loader.h'
C = 'src/o_loader.cpp'

for f in (P, H, C):
    s = open(f, encoding='utf-8', newline='').read()
    assert s.count('\r\n') == s.count('\n'), '%s is not pure CRLF' % f


def load(f):
    return open(f, encoding='utf-8', newline='').read()


def save(f, s):
    open(f, 'w', encoding='utf-8', newline='').write(s)


def rep(s, old, new, cnt=1, what=''):
    o, n = old.replace('\n', '\r\n'), new.replace('\n', '\r\n')
    c = s.count(o)
    if c != cnt:
        sys.exit('ANCHOR %d != %d in %s for:\n%s' % (c, cnt, what, old[:140]))
    return s.replace(o, n, cnt)


# ---------------------------------------------------------- o_loader.h
h = load(H)
h = rep(h, """struct OriginalFunction {
    std::string name;
    int index = -1;""",
        """struct OriginalFunction {
    std::string name;
    // FNV-1a 64 of `name`, filled once by the loader.  findFunctionAny() scans
    // every function of the constitution twice per name lookup; comparing this
    // hash first turns ~all of those std::string compares into an integer
    // compare (measured 43us -> a few us per lookup) without per-VM containers.
    uint64_t nameHash = 0;
    int index = -1;""", what=H)
h = rep(h,
        """OriginalObject loadOriginalObject(const std::string& path);""",
        """// FNV-1a 64 over the bytes of `s` (the efun dispatcher uses the same
// construction, so a missing hash is easy to spot: 0 means "not computed").
uint64_t lpcNameHash(const std::string& s);

OriginalObject loadOriginalObject(const std::string& path);""", what=H)
save(H, h)

# ---------------------------------------------------------- o_loader.cpp
c = load(C)
c = rep(c, """OriginalObject loadOriginalObjectFromBytes(const std::vector<uint8_t>& data) {""",
        """uint64_t lpcNameHash(const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char ch : s) {
        h ^= ch;
        h *= 1099511628211ull;
    }
    return h;
}

OriginalObject loadOriginalObjectFromBytes(const std::vector<uint8_t>& data) {""", what=C)
c = rep(c, """    if (!obj.functions.empty() && !obj.strings.empty()) {
        auto ownTable = std::make_shared<const std::vector<std::string>>(obj.strings);
        for (auto& fn : obj.functions)
            if (!fn.strings) fn.strings = ownTable;
    }""",
        """    if (!obj.functions.empty() && !obj.strings.empty()) {
        auto ownTable = std::make_shared<const std::vector<std::string>>(obj.strings);
        for (auto& fn : obj.functions)
            if (!fn.strings) fn.strings = ownTable;
    }
    // Fill the name hash for every function (single place where the function
    // list is final), so findFunctionAny can pre-filter on an integer compare.
    for (auto& fn : obj.functions) fn.nameHash = lpcNameHash(fn.name);""", what=C)
save(C, c)

# ---------------------------------------------------------- original_vm.cpp
d = load(P)
d = rep(d, """    // LPCVM_HOMEFIX: name -> function index, built once per VM and rebuilt when
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
        if (heavyIdxFixOn()) return findFunctionAnyIdx(name);
        CpuProfExtra _cpuProfFindScan("findFunctionAny.scan");
        for (const auto& fn : obj_.functions) {
            _cpuProfFindScan.add();
            if (fn.name == name && fn.index >= 0) return &fn;
        }""",
        """    // findFunctionAny() walks EVERY function of the constitution TWICE per
    // lookup (obj_.functions plus the whole funcsByIndex_ map, and the second
    // pass has no early exit) and runs on every cross-object call resolved by
    // name - the hottest path of a login.  A per-VM name index was tried and
    // reverted: ~23k VMs x their own map added ~400MB (gs 1080MB -> 1500MB).
    // Instead the loader stores a 64-bit name hash on each function and the two
    // scans below pre-filter on it; the scan ORDER and the std::string compare
    // as final arbiter are unchanged, so first-match / last-match semantics are
    // identical.
    const OriginalFunction* findFunctionAny(const std::string& name) const {
        CpuProfGuard _cpuProfFind("findFunctionAny");
        const bool hashFilter = heavyIdxFixOn();
        const uint64_t wantHash = hashFilter ? lpcNameHash(name) : 0;
        CpuProfExtra _cpuProfFindScan("findFunctionAny.scan");
        for (const auto& fn : obj_.functions) {
            _cpuProfFindScan.add();
            if (fn.index < 0) continue;
            if (hashFilter && wantHash && fn.nameHash != wantHash) continue;
            if (fn.name == name) return &fn;
        }""", what=P)

d = rep(d, """        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_) {
            _cpuProfFindScan.add();
            if (kv.second && kv.second->name == name && kv.second->index >= 0 &&
                !kv.second->code.empty())
                impl = kv.second;   // keep the last (highest-key) implementation
        }
        return impl;""",
        """        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_) {
            _cpuProfFindScan.add();
            const OriginalFunction* f = kv.second;
            if (!f || f->index < 0 || f->code.empty()) continue;
            if (hashFilter && wantHash && f->nameHash != wantHash) continue;
            if (f->name == name)
                impl = f;   // keep the last (highest-key) implementation
        }
        return impl;""", what=P)

# drop the now-dead stamp bumps
d = rep(d, """            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;
            bumpFnStamp();""",
        """            if (fn.index >= 0) funcsByIndex_[fn.index] = &fn;""", cnt=3, what=P)
d = rep(d, """            funcsByIndex_ = tmpl->funcsByIndex_;
            bumpFnStamp();""",
        """            funcsByIndex_ = tmpl->funcsByIndex_;""", what=P)
d = rep(d, """                        funcsByIndex_[childIndex] = inhFn.get();
                        bumpFnStamp();""",
        """                        funcsByIndex_[childIndex] = inhFn.get();""", what=P)
save(P, d)
print('patch 8 applied')
