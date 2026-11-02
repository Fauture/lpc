// ===========================================================================
// vmcore/object.cpp — VmProgram 构建 / Registry / clone / destruct
//
// 语义来源（全部已实证，搬运时保持不变）:
//   * 程序合并: mergeInheritedFunctions 的 walk（祖先优先、父表字符串表
//     共享、累积 baseFunOffset/baseVarOffset、前向让位真实现、零字节 DLT
//     占位符重指同名真实现 — own 先于 inherited）。
//   * PROG_VARS 槽位: vm_getObjVar (0x80BC5C0) 的 walk — valueNo(raw[0xa])
//     + 累积 baseVarOffset。
//   * 继承文件解析: pak 优先，其次磁盘树 (loadInheritFileForMerge)。
//   * 键规范化: canonicalObjKey 的 8 种拼写收敛为注册期建立的别名表 —
//     命名入口 O(1) 点查，运行期身份一律指针。
// ===========================================================================
#include "object.h"

#include <cstring>
#include <set>

#include "driver_model.h"
#include "pak_loader.h"

namespace lpc {
namespace vmcore {

// ---------------------------------------------------------------------------
// path helpers (loader-side naming only)
// ---------------------------------------------------------------------------
static bool endsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::string relOf(std::string p) {
    if (p.rfind("pak:", 0) == 0) p.erase(0, 4);
    if (!p.empty() && p.front() == '/') p.erase(p.begin());
    if (!p.empty() && p.back() == '/') p.pop_back();
    return p;
}

// ---------------------------------------------------------------------------
// VmProgram
// ---------------------------------------------------------------------------
VmProgram::VmProgram(std::shared_ptr<const OriginalObject> src,
                     const InheritLoader& loadInherit)
    : src_(std::move(src)) {
    // Own (root-table) functions at their .o indexes.  The hidden records
    // (index < 0 or the flag-word records before hiddenFunctionCount) never
    // enter the table — their "index" is a flag word, not an index.
    for (const auto& fn : src_->functions) {
        if (fn.index < 0) continue;
        byIndex_[fn.index] = &fn;
        if (!fn.code.empty()) byName_[fn.name] = &fn;
    }
    // ---- inherited walk (ancestors first; later merges override) ----
    std::map<std::string, std::shared_ptr<const OriginalObject>> loaded;
    std::unordered_map<std::string, const OriginalFunction*> inhByName;
    // Create-chain collection (deduped per program path, resolved through the
    // merged table AFTER the DLT repoint below).
    std::vector<std::pair<int, std::string>> createKeys;
    std::set<std::string> seenPrograms;
    std::function<void(const OriginalObject&, int, int)> walk;
    walk = [&](const OriginalObject& parent, int accFun, int accVar) {
        for (const auto& inh : parent.inherits) {
            std::shared_ptr<const OriginalObject> parentObj;
            auto lit = loaded.find(inh.path);
            if (lit != loaded.end()) {
                parentObj = lit->second;
            } else {
                try {
                    parentObj = loadInherit(inh.path);
                } catch (...) {
                    continue;   // failed to load the inherited file
                }
                if (!parentObj) continue;
                loaded[inh.path] = parentObj;
            }
            const int funOff = accFun + inh.baseFunOffset;
            const int varOff = accVar + inh.baseVarOffset;
            // Parent's own inheritance first (its function indexes already
            // include them), then the parent's functions.
            walk(*parentObj, funOff, varOff);
            // Construction data: the parent's init segment runs after its
            // own ancestors' (runInheritedInitsOf order).
            initSegs_.push_back(InitSeg{parentObj, varOff, funOff});
            if (seenPrograms.insert(inh.path).second) {
                for (const auto& fn : parentObj->functions) {
                    if (fn.name == "create" && fn.index >= 0 && !fn.code.empty())
                        createKeys.push_back({funOff + fn.index, inh.path});
                }
            }
            auto parentTable =
                std::make_shared<const std::vector<std::string>>(parentObj->strings);
            for (const auto& fn : parentObj->functions) {
                if (fn.index < 0) continue;
                const int childIndex = funOff + fn.index;
                auto existing = byIndex_.find(childIndex);
                if (existing == byIndex_.end()) {
                    auto inhFn = std::make_shared<OriginalFunction>(fn);
                    // Inherited functions resolve shared-string indexes against
                    // the PARENT's table, obj-vars against the accumulated
                    // baseVarOffset, local-index calls against baseFunOffset.
                    inhFn->strings = parentTable;
                    inhFn->varOffset = varOff;
                    inhFn->funOffset = funOff;
                    inherited_.push_back(inhFn);
                    byIndex_[childIndex] = inhFn.get();
                    if (!inhFn->code.empty()) inhByName[inhFn->name] = inhFn.get();
                } else if (!fn.code.empty() && existing->second->code.empty()) {
                    // A forward declaration merged before the real
                    // implementation must not shadow it (name.c declares
                    // query_entire_dbase, dbase.c implements it).
                    auto inhFn = std::make_shared<OriginalFunction>(fn);
                    inhFn->strings = parentTable;
                    inhFn->varOffset = varOff;
                    inhFn->funOffset = funOff;
                    inherited_.push_back(inhFn);
                    existing->second = inhFn.get();
                    if (!inhFn->code.empty()) inhByName[inhFn->name] = inhFn.get();
                }
            }
        }
    };
    walk(*src_, 0, 0);
    // ---- DLT repoint: zero-byte placeholders → real implementation ----
    // The .o's table is the whole constitution's: a program that only
    // REFERENCES a function contributes a zero-byte placeholder, and a call
    // landing there silently does nothing (iid.c create_iid's query/lite_set).
    // The original resolves DLT references to the defining constitution —
    // own/root definitions first, then the most-derived inherited one.
    for (auto& kv : byIndex_) {
        const OriginalFunction* f = kv.second;
        if (!f || !f->code.empty()) continue;
        auto own = byName_.find(f->name);
        if (own != byName_.end() && !own->second->code.empty()) {
            kv.second = own->second;
            continue;
        }
        auto inh = inhByName.find(f->name);
        if (inh != inhByName.end()) kv.second = inh->second;
    }
    // ---- name index: later inherits override earlier ones (most derived) ----
    for (const auto& kv : byIndex_) {
        const OriginalFunction* f = kv.second;
        if (!f || f->code.empty()) continue;
        // key order = baseFunOffset order ⇒ LAST same-name write is the most
        // derived implementation (matches findFunctionAny's take-the-last rule).
        byName_[f->name] = f;
    }
    // ---- create chain (vm_f_construct_object2's initOrderList walk) ----
    // Inherited creates resolve through the MERGED table at funOff+index (the
    // walk above collected them); the OWN program's creates are appended
    // directly from its own records — a merged lookup at the same index could
    // return an inherited create and the own one would never run (the
    // duowen-daoren menu registration bug).
    for (const auto& ck : createKeys) {
        const OriginalFunction* f = fnAtIndex(ck.first);
        if (f && f->name == "create" && !f->code.empty())
            createFns_.push_back(f);
    }
    for (const auto& fn : src_->functions) {
        if (fn.name != "create" || fn.index < 0 || fn.code.empty()) continue;
        bool dup = false;
        for (const OriginalFunction* f : createFns_)
            if (f == &fn) { dup = true; break; }
        if (!dup) createFns_.push_back(&fn);
    }
    // ---- PROG_VARS slots (vm_getObjVar 0x80BC5C0) ----
    //     slot = valueNo (raw[0xa], int16) + accumulated baseVarOffset,
    //     searched over every constitution of the program (own + inherited),
    //     first match wins — qualification "prog::name" must match too.
    std::function<void(const OriginalObject&, int, bool)> varWalk;
    varWalk = [&](const OriginalObject& prog, int base, bool isRoot) {
        for (const auto& rec : prog.progVars) {
            std::string rprog, rname;
            {
                const size_t sep = rec.name.find("::");
                if (sep == std::string::npos) rname = rec.name;
                else { rprog = rec.name.substr(0, sep); rname = rec.name.substr(sep + 2); }
            }
            std::string k = rprog.empty() ? rname : rprog + "::" + rname;
            const int slot = base +
                static_cast<int16_t>(rec.raw[0xa] | (rec.raw[0xb] << 8));
            // Later walks (the root program is walked LAST) would overwrite an
            // inherited slot; only insert the FIRST hit per qualified name.
            if (varSlots_.find(k) == varSlots_.end()) varSlots_[k] = slot;
        }
        for (const auto& inh : prog.inherits) {
            std::shared_ptr<const OriginalObject> pobj;
            try {
                std::string rel = relOf(inh.path);
                if (endsWith(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
                try {
                    pobj = loadInherit(rel);
                } catch (...) { continue; }
            } catch (...) { continue; }
            if (pobj) varWalk(*pobj, base + inh.baseVarOffset, false);
        }
        (void)isRoot;
    };
    varWalk(*src_, 0, true);

    // ---- destructor chain (vm_f_destruct_object2 0x80f9284) ----
    // initOrderList walked BACKWARDS (most derived first); each segment runs
    // its OWN "destruct" record — the .o function table is the WHOLE
    // constitution's (own + inherited expansion), so a segment's destructor
    // lives in its own program at its LOCAL index and lands in the merged
    // table at funOff + index.  Segments without one are skipped
    // (v3 == 0xFFFF in the original).  Measured: npc.o hiddenCount == 0 —
    // the hidden-flag theory was wrong; these are plain visible records.
    auto collectSegDtor = [&](const std::shared_ptr<const OriginalObject>& prog,
                              int funOff, int varOff) {
        for (const auto& f : prog->functions) {
            if (f.index < 0 || f.name != "destruct" || f.code.empty()) continue;
            auto copy = std::make_shared<OriginalFunction>(f);
            copy->strings =
                std::make_shared<const std::vector<std::string>>(prog->strings);
            copy->varOffset = varOff;
            copy->funOffset = funOff;
            ownedDtorCopies_.push_back(std::move(copy));
            destructFns_.push_back(ownedDtorCopies_.back().get());
            return;   // one destructor record per segment
        }
    };
    // Own (most derived) segment first — the original starts at the LAST
    // initOrderList entry.
    collectSegDtor(src_, 0, 0);
    for (auto it = initSegs_.rbegin(); it != initSegs_.rend(); ++it)
        collectSegDtor(it->prog, it->funOffset, it->varOffset);
}

const OriginalFunction* VmProgram::fnAtIndex(int index) const {
    auto it = byIndex_.find(index);
    return it == byIndex_.end() ? nullptr : it->second;
}

const OriginalFunction* VmProgram::findFn(const std::string& name) const {
    auto it = byName_.find(name);
    return it == byName_.end() ? nullptr : it->second;
}

int VmProgram::varSlotByName(const std::string& qualifiedName) const {
    auto it = varSlots_.find(qualifiedName);
    return it == varSlots_.end() ? -1 : it->second;
}

// ---------------------------------------------------------------------------
// VmObject
// ---------------------------------------------------------------------------
VmObject::VmObject(std::shared_ptr<const VmProgram> prog, std::string key)
    : prog_(std::move(prog)), key_(std::move(key)) {
    vars_.assign(static_cast<size_t>(prog_->globalVarCount()), make_int(0));
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------
Registry& Registry::instance() {
    static Registry r;
    return r;
}

std::string Registry::canonicalKey(const std::string& virtualPath,
                                   const std::string& searchRoot) {
    // Same convergence canonicalObjKey performed by enumeration, expressed as
    // ONE deterministic key: "<root>/<rel>.o" for disk loads, "pak:<rel>.o"
    // for pak loads.  Every other accepted spelling is an alias (registered
    // alongside), never a separate identity.
    std::string rel = relOf(virtualPath);
    if (endsWith(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    // Extension-less binding paths ("/gs/daemons/moved") are aliases too;
    // the canonical form always carries .o.
    if (rel.find_last_of('.') == std::string::npos ||
        rel.substr(rel.find_last_of('.') + 1).find('/') != std::string::npos)
        rel += ".o";
    if (pakMounted()) return "pak:" + rel;
    return searchRoot.empty() ? rel : searchRoot + "/" + rel;
}

std::shared_ptr<const VmProgram> Registry::loadProgram(const std::string& virtualPath,
                                                       const std::string& searchRoot) {
    std::string rel = relOf(virtualPath);
    if (endsWith(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    auto it = programs_.find(rel);
    if (it != programs_.end()) return it->second;

    // Load the .o bytes: the mounted pak first, then the extracted tree.
    std::shared_ptr<OriginalObject> src;
    if (pakMounted()) {
        std::vector<uint8_t> bytes;
        if (pakReadFile(pakVirtualPrefix() + "/" + rel, bytes)) {
            try {
                src = std::make_shared<OriginalObject>(loadOriginalObjectFromBytes(bytes));
                src->sourceName = "/" + rel;
            } catch (...) {
            }
        }
    }
    if (!src) {
        try {
            std::string cand = searchRoot.empty() ? rel : searchRoot + "/" + rel;
            src = std::make_shared<OriginalObject>(loadOriginalObject(cand));
        } catch (...) {
            return nullptr;
        }
    }
    if (!src) return nullptr;

    // Program building needs inherited programs: resolve them through this
    // same registry's raw-table cache (pak first, disk fallback).
    auto progRaw = std::make_shared<VmProgram>(src, [this](const std::string& vpath) {
        return loadInheritProgram(vpath);
    });
    auto prog = std::shared_ptr<const VmProgram>(progRaw);
    progRaw->setSelf(prog);
    programs_[rel] = prog;
    return prog;
}

// Resolve an inherited program (merge walk / PROG_VARS walk): pak first, then
// the extracted tree, cached by rel path.
std::shared_ptr<const OriginalObject> Registry::loadInheritProgram(const std::string& vpath) {
    std::string rel = relOf(vpath);
    if (endsWith(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    auto it = programs_.find(rel);
    if (it != programs_.end()) {
        // Hand out the loader's shared OriginalObject.  programs_ stores
        // VmProgram; the src is kept alive by it.
        return std::shared_ptr<const OriginalObject>(it->second, &it->second->src());
    }
    // Load the bytes the same way loadProgram does, but WITHOUT building a
    // full merged program (the walk only needs the raw table).
    std::shared_ptr<OriginalObject> src;
    if (pakMounted()) {
        std::vector<uint8_t> bytes;
        if (pakReadFile(pakVirtualPrefix() + "/" + rel, bytes)) {
            try {
                src = std::make_shared<OriginalObject>(loadOriginalObjectFromBytes(bytes));
                src->sourceName = "/" + rel;
            } catch (...) {
            }
        }
    }
    if (!src) {
        try {
            std::string cand = diskRoot_.empty() ? rel : diskRoot_ + "/" + rel;
            src = std::make_shared<OriginalObject>(loadOriginalObject(cand));
        } catch (...) {
            return nullptr;
        }
    }
    if (!src) return nullptr;
    // Cache the RAW table (not a merged program) for the walk.
    inherits_[rel] = src;
    return src;
}

std::shared_ptr<const VmProgram> Registry::findProgram(const std::string& virtualPath) const {
    std::string rel = relOf(virtualPath);
    if (endsWith(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    auto it = programs_.find(rel);
    return it == programs_.end() ? nullptr : it->second;
}

// Every accepted spelling of one object, registered alongside the canonical
// key: with/without leading slash, .c/.o, extension-less, "pak:" forms, and
// the loader's sourceName spellings.  The alias table is THE name resolution
// data — findByName is two hash lookups, never a scan.
static void registerAliases(const std::string& canonical,
                            std::unordered_map<std::string, std::string>& aliases) {
    std::string base = relOf(canonical);
    std::vector<std::string> forms;
    auto addForm = [&](const std::string& f) {
        if (f.empty()) return;
        for (const auto& x : forms)
            if (x == f) return;
        forms.push_back(f);
    };
    addForm(base);
    if (endsWith(base, ".o")) addForm(base.substr(0, base.size() - 2) + ".c");
    const size_t slash = base.find_last_of('/');
    const std::string leaf = (slash == std::string::npos) ? base : base.substr(slash + 1);
    if (leaf.find('.') == std::string::npos) {
        addForm(base + ".o");
        addForm(base + ".c");
    }
    const size_t n = forms.size();
    for (size_t i = 0; i < n; ++i) addForm("/" + forms[i]);
    const size_t n2 = forms.size();
    for (size_t i = 0; i < n2; ++i) addForm("pak:" + forms[i]);
    for (const auto& f : forms) aliases[f] = canonical;
    aliases[canonical] = canonical;
}

void Registry::registerObject(const std::string& key, VmObject* obj) {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    objects_[key] = obj;
    registerAliases(key, aliases_);
    // The loader's sourceName ("/gs/x/x.o") is another accepted spelling.
    if (obj && obj->prog()) {
        std::string sn = relOf(obj->prog()->name());
        if (!sn.empty() && key.compare(0, 6, "clone:") != 0) {
            aliases_[sn] = key;
            aliases_["/" + sn] = key;
            if (endsWith(sn, ".o")) {
                aliases_[sn.substr(0, sn.size() - 2) + ".c"] = key;
                aliases_["/" + sn.substr(0, sn.size() - 2) + ".c"] = key;
                const std::string stem = sn.substr(0, sn.size() - 2);
                aliases_[stem] = key;
                aliases_["/" + stem] = key;
            }
        }
    }
}

void Registry::unregisterObject(const std::string& key) {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    objects_.erase(key);
    // Aliases pointing at the key die with it (identity is gone).
    for (auto it = aliases_.begin(); it != aliases_.end();) {
        if (it->second == key) it = aliases_.erase(it);
        else ++it;
    }
}

VmObject* Registry::find(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    auto it = objects_.find(key);
    return it == objects_.end() ? nullptr : it->second;
}

std::vector<VmObject*> Registry::allObjects() const {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    std::vector<VmObject*> out;
    out.reserve(objects_.size());
    for (const auto& kv : objects_) out.push_back(kv.second);
    return out;
}

VmObject* Registry::findByName(const std::string& name) const {
    if (name.empty()) return nullptr;
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    auto it = objects_.find(name);
    if (it != objects_.end()) return it->second;
    auto al = aliases_.find(name);
    if (al != aliases_.end()) {
        auto ot = objects_.find(al->second);
        if (ot != objects_.end()) return ot->second;
    }
    // Original engine spells every object path WITH a leading '/' ("/gs/...",
    // "/daemons/..."), and object_name(ob,1)->query_entire_dbase() style
    // call_other-by-path hands us "/pak:..." / "/clone:..." spellings.  Our
    // registry keys are the "pak:..." / "clone:..." forms — retry without
    // the leading slash before giving up (scand.do_init's scan table,
    // npc/basic.c::create's import(...) and every by-path call depend on
    // this normalization).
    if (name.front() == '/') {
        std::string bare = name.substr(1);
        it = objects_.find(bare);
        if (it != objects_.end()) return it->second;
        al = aliases_.find(bare);
        if (al != aliases_.end()) {
            auto ot = objects_.find(al->second);
            if (ot != objects_.end()) return ot->second;
        }
    }
    return nullptr;
}

void Registry::markInitialized(const std::string& key) {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    initialized_.insert(key);
    constructing_.erase(key);
}
bool Registry::isInitialized(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    return initialized_.count(key) != 0;
}
void Registry::markConstructing(const std::string& key) {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    constructing_.insert(key);
}
bool Registry::isConstructing(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    return constructing_.count(key) != 0;
}
bool Registry::anyConstructing() const {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    return !constructing_.empty();
}
void Registry::clearConstructing(const std::string& key) {
    std::lock_guard<std::recursive_mutex> lk(mtx_);
    constructing_.erase(key);
}

Value Registry::makeObjectValue(VmObject* obj) {
    if (!obj) return make_int(0);
    Value v = make_object(obj->key());
    auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
    ov->vmObj = obj;                       // the pointer IS the identity
    ov->alive = obj->aliveFlag();          // liveness of the instance
    return v;
}

// ---------------------------------------------------------------------------
// clone / destruct
// ---------------------------------------------------------------------------
VmObject* cloneObject(VmObject* tmpl) {
    if (!tmpl) return nullptr;
    static std::atomic<int64_t> seq{0};
    auto& reg = Registry::instance();
    // clone key from the TEMPLATE's canonical program path, never from a
    // source-clone key ("clone:clone:..." never happens: strip any existing
    // "clone:" and "#seq" from the program spelling).
    std::string progKey = tmpl->key();
    const size_t hash = progKey.find('#');
    if (hash != std::string::npos) progKey.resize(hash);
    if (progKey.compare(0, 6, "clone:") == 0) progKey.erase(0, 6);
    const std::string key = "clone:" + progKey + "#" + std::to_string(seq.fetch_add(1));
    VmObject* inst = new VmObject(tmpl->prog()->self(), key);
    inst->templateObj = tmpl;              // object_template() answers this
    parkVmObject(inst);                    // immortal lifetime (raw-pointer identity)
    reg.registerObject(key, inst);
    return inst;
}

bool destructObject(VmObject* obj) {
    if (!obj || obj->destructed()) return false;
    obj->markDestructed();
    return true;
}

} // namespace vmcore
} // namespace lpc
