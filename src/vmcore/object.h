// ===========================================================================
// vmcore/object.h — VmProgram / VmObject / 对象注册表 (原版对象模型)
//
// 原版实证 (IDA, magic_Linux32x):
//   * 对象值 type=10 直接持有 Vm_Object_T* (v.info): vm_callInOtherFun
//     (0x8094cd8) 里目标对象零查找 — info = v3->v.info。只有字符串名
//     (type=4) 才走 vm_lookupObjectByName (0x80b114c), 且仅限名字入口。
//   * 析构即对象自身 flag: *((char*)info+13) < 0 → "Object is not found
//     (destructed)."。身份比较即指针比较 (vm_compareValue 0x80bbcb4)。
//   * 程序 (Vm_Program_T) 是只读数据: 函数表/字符串表/PROG_VARS/继承链
//     偏移; 克隆共享程序, 只新建实例变量表。
//
// 新核模型:
//   VmProgram — o_loader 产物包上一层, 加载后不可变; 模板与全部克隆共享。
//   VmObject  — 实例身份: 独立实例变量数组 + 共享程序指针 + 析构 flag。
//   Registry  — 注册表: 键(规范化路径) → VmObject*。键是加载器/名字入口
//               用的命名, 运行期身份一律是指针。没有 suffix 扫描、没有
//               tplKeyCache、没有"objByPtr 只存最后注册键"。
// ===========================================================================
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "o_loader.h"
#include "value.h"

namespace lpc {
namespace vmcore {

class VmObject;

// ---------------------------------------------------------------------------
// VmProgram — a loaded .o's read-only program, shared by the template object
// and every clone.  Built once at load time; immutable afterwards.
//
// byIndex_ 是合并后的 constitution 函数表 (own functions at their indexes +
// inherited functions at baseFunOffset+index, 前向声明让位于同 index 的真实现,
// 零字节 DLT 占位符重指到同名真实现 — 与已验证的 mergeInheritedFunctions
// 语义一致)。名字解析走 findFn (后继承覆盖先继承, 同名取最派生的非空实现)。
// ---------------------------------------------------------------------------
class VmProgram {
public:
    // Build from loader output.  `loadInherit` resolves an inherit's virtual
    // path to its program (pak first, then the extracted tree) — supplied by
    // the registry so program building needs no global state.
    using InheritLoader = std::function<std::shared_ptr<const OriginalObject>(
        const std::string& vpath)>;

    VmProgram(std::shared_ptr<const OriginalObject> src, const InheritLoader& loadInherit);

    const OriginalObject& src() const { return *src_; }
    const std::string& name() const { return src_->sourceName; }
    uint32_t globalVarCount() const { return src_->globalVarCount; }
    uint16_t efunCount() const { return src_->efunCount; }

    // Merged-table lookup: index resolves against the WHOLE constitution.
    const OriginalFunction* fnAtIndex(int index) const;
    // Name lookup: own definitions first (most derived), then the most
    // derived inherited implementation (later inherits override earlier).
    const OriginalFunction* findFn(const std::string& name) const;

    // PROG_VARS slot by name (valueNo + accumulated baseVarOffset) — the
    // original vm_getObjVar (0x80BC5C0) walk, precomputed at build time.
    int varSlotByName(const std::string& qualifiedName) const;

    // ---- construction data (initOrderList; vm_f_construct_object2) ----
    // Flattened inherit walk, PARENTS-FIRST execution order: ancestors of an
    // inherit run before the inherit itself; each segment runs under its own
    // constitution (own string table + accumulated baseVarOffset).
    struct InitSeg {
        std::shared_ptr<const OriginalObject> prog;
        int varOffset;
    };
    const std::vector<InitSeg>& initSegments() const { return initSegs_; }
    // The create() chain: every program along the inherit chain whose create
    // is not shadowed (resolved through the merged table at funOff+index),
    // base-first, with the OWN program's creates appended directly (their
    // own records — a merged-table lookup could return an inherited create
    // sharing the same index).
    const std::vector<const OriginalFunction*>& createChain() const { return createFns_; }

    // The live shared_ptr every VmObject holds — keeps the program alive.
    std::shared_ptr<const VmProgram> self() const { return self_; }
    void setSelf(std::shared_ptr<const VmProgram> s) { self_ = std::move(s); }

private:
    std::shared_ptr<const OriginalObject> src_;
    std::map<int, const OriginalFunction*> byIndex_;
    // Inherited function copies owned here (they carry the parent's string
    // table and accumulated offsets; the loader's own records are const).
    std::vector<std::shared_ptr<OriginalFunction>> inherited_;
    // name → real implementation (most derived, non-empty code) for findFn.
    std::unordered_map<std::string, const OriginalFunction*> byName_;
    // PROG_VARS resolution (valueNo + baseVarOffset), built by the build walk.
    std::map<std::string, int> varSlots_;
    // Construction data (see the accessors above).
    std::vector<InitSeg> initSegs_;
    std::vector<const OriginalFunction*> createFns_;
    std::shared_ptr<const VmProgram> self_;
};

// ---------------------------------------------------------------------------
// VmObject — an instance.  The object VALUE is the pointer to this struct
// (原版 v.info); its identity IS the pointer, its liveness IS destructed_.
//
// 克隆 = new VmObject(共享程序) + 全新变量数组 — 浅实例, 零拷贝零合并。
// 析构分两段 (原版语义): markDestructed() 置 flag (析构链还能跑 save/get_name);
// 真正释放时 closeReferences() 使所有已发出的对象值失效 (alive flag)。
// ---------------------------------------------------------------------------
class VmObject {
public:
    VmObject(std::shared_ptr<const VmProgram> prog, std::string key);

    const VmProgram* prog() const { return prog_.get(); }
    const std::string& key() const { return key_; }      // registry name (loader-only)
    std::vector<Value>& vars() { return vars_; }         // instance variables
    const std::vector<Value>& vars() const { return vars_; }

    bool destructed() const { return destructed_.load(); }
    void markDestructed() { destructed_.store(true); }
    // Reference invalidation on the real free (原版: 引用变 0)。
    std::shared_ptr<std::atomic<bool>> aliveFlag() const { return alive_; }
    void closeReferences() { if (alive_) alive_->store(false); }

    // ---- env (容器/房客链) — 由 efuns (move_object/lodger_object) 维护 ----
    std::string envKey;                        // container key ("" = none)
    std::vector<VmObject*> lodgers;            // contents, tail-link order
    // The object this one was cloned from (object_template()); null for a
    // loaded (non-clone) object.
    VmObject* templateObj = nullptr;

    // this_object() name for LPC identity (program path shared by clones is
    // prog->name(); the INSTANCE key tells clones apart).
    const std::string& thisName() const { return key_; }

private:
    std::shared_ptr<const VmProgram> prog_;
    std::string key_;                          // registry key / instance name
    std::vector<Value> vars_;                  // instance variables
    std::atomic<bool> destructed_{false};
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
};

// ---------------------------------------------------------------------------
// Registry — 键 → VmObject*。加载器/名字入口专用; 运行期对象值一律指针。
// 键规范化: 虚拟路径的 ".c"/无扩展名/带根 前缀统一到同一规范键, 命名入口
// (find_object / call_other-by-name) 先规范化再点查 — 没有 suffix 扫描。
// ---------------------------------------------------------------------------
class Registry {
public:
    static Registry& instance();

    // Program cache: one VmProgram per loaded .o (shared by template+clones).
    // Loads via o_loader; the inherit loader resolves parent programs through
    // the same cache.  Returns null when the file cannot be loaded.
    std::shared_ptr<const VmProgram> loadProgram(const std::string& virtualPath,
                                                 const std::string& searchRoot);
    std::shared_ptr<const VmProgram> findProgram(const std::string& virtualPath) const;

    // Object registration.  key = canonical registry key ("pak:gs/x/y.o",
    // "clone:gs/x/y.o#<seq>", master "<main>").  Registers by exact key only.
    void registerObject(const std::string& key, VmObject* obj);
    void unregisterObject(const std::string& key);
    VmObject* find(const std::string& key) const;          // exact canonical key
    VmObject* findByName(const std::string& name) const;   // alias + lookup

    // Construction state: loading and constructing are separate steps
    // (vm_f_load_object -> vm_loadObject split).  `initialized` = the
    // construction process finished (global init + create chain ran).
    void markInitialized(const std::string& key);
    bool isInitialized(const std::string& key) const;
    void markConstructing(const std::string& key);
    bool isConstructing(const std::string& key) const;
    void clearConstructing(const std::string& key);

    // Disk tree root for pak-miss fallback (set from the vfs profile at boot).
    void setDiskRoot(const std::string& root) { diskRoot_ = root; }
    const std::string& diskRootOf() const { return diskRoot_; }

    // Snapshot of every registered object (compat layer's objects()/
    // child_objects/neighbour_object walk; order unspecified like the
    // original's hash-table walk).
    std::vector<VmObject*> allObjects() const;

    // Public wrapper of the inherit-program resolver (dynamic image loads
    // build a program through the same cache).
    std::shared_ptr<const OriginalObject> loadInheritProgramPublic(
        const std::string& vpath) {
        return loadInheritProgram(vpath);
    }

    // Object values: type-10 value holding the pointer.
    static Value makeObjectValue(VmObject* obj);

    // Canonical key of a virtual path (".c"→".o", leading "/", root prefixes,
    // extension-less bindings, "obj/" prefix variants).  One object, one key.
    static std::string canonicalKey(const std::string& virtualPath,
                                    const std::string& searchRoot);

private:
    Registry() = default;
    // Resolve an inherited program (merge/PROG_VARS walks): pak first, then
    // the disk tree; cached by rel path.  Returns the RAW loader table.
    std::shared_ptr<const OriginalObject> loadInheritProgram(const std::string& vpath);

    mutable std::recursive_mutex mtx_;
    std::string diskRoot_;
    std::unordered_map<std::string, std::shared_ptr<const VmProgram>> programs_;
    // Raw (unmerged) inherited-program cache for the build walks.
    std::unordered_map<std::string, std::shared_ptr<const OriginalObject>> inherits_;
    std::unordered_map<std::string, VmObject*> objects_;
    // Canonical-alias table: every accepted spelling of an object → its
    // canonical key (built at registration; the ONLY name resolution data).
    std::unordered_map<std::string, std::string> aliases_;
    std::unordered_set<std::string> initialized_;
    std::unordered_set<std::string> constructing_;
};

// Create a clone instance of a registered object (clone_object): fresh
// VmObject sharing the target's program, registered under
// "clone:<progkey>#<seq>".  Returns null when the template is not loaded.
VmObject* cloneObject(VmObject* tmpl);

// Destruct an object (destruct_object): mark, run the destructor chain (the
// caller drives LPC), then unregister + close references on the real free.
bool destructObject(VmObject* obj);

} // namespace vmcore
} // namespace lpc
