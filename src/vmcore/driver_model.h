// ===========================================================================
// vmcore/driver_model.h — 驱动模型设施 (env 容器模型 / set_timer 注册表)
//
// 这些是"原版就有"的驱动侧模型, 不是旧核的补丁机制:
//   * env/lodger 模型 — vm_f_move_object (0x80f5348) 把对象链到容器
//     pLogderObject 链尾, vm_f_lodger_object (0x80f41c8) 沿链遍历;
//     env_object 读容器的注册键。键一律是 Registry 的规范键。
//   * 定时器注册表 — vm_putTimerIntoTimerList (0x80aa12c) 的按绝对触发
//     时间有序表; 同名定时器可共存, get_timer/delete_timer 取首个匹配;
//     重挂同名定时器 (同对象) 替换旧条目 (游戏从回调内部重挂 repeating
//     定时器的模式依赖此规则, 否则每周期翻倍)。
// ===========================================================================
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {
namespace vmcore {

class VmObject;
struct VmPcb;

// ---- env (container) model — keyed by canonical registry key -------------
void envStoreKey(const std::string& selfKey, const std::string& destKey);
std::string envLookupKey(const std::string& obKey);
// The container's lodgers in the driver's chain order (tail-append); entries
// whose object is gone are dropped lazily.
std::vector<std::string> envLodgerKeysOf(const std::string& containerKey);

// ---- timer registry (set_timer / get_timer / delete_timer) ---------------
struct TimerEntry {
    int64_t id = 0;
    std::string name;         // empty when registered with 0/ref as arg2
    std::string callback;     // function name, for the get_timer mapping
    int64_t timeoutSecs = 0;  // delay in seconds
    int64_t triggerAt = 0;    // absolute trigger time (seconds since epoch)
    VmObject* owner = nullptr;   // scoping key of the name-replace rule
    // Non-null when the callback is an EFUN closure ((: call_other, ob,
    // "fn" :)): the fire runs the efun on a dedicated process instead of
    // resolving `callback` in the owner's program table.
    lpc::Ref<ClosureValue> efunCb;
    std::shared_ptr<std::atomic<bool>> cancelled;
};
std::shared_ptr<TimerEntry> timerFindByName(const std::string& name);
std::shared_ptr<TimerEntry> timerFindById(int64_t id);
void timerUnregister(const std::shared_ptr<TimerEntry>& e);
// Register + schedule the callback (a scheduler process firing at trigger
// time).  A NAMED re-arm for the same owner replaces the older pending entry.
// `efunCb` carries an efun-closure callback (see TimerEntry::efunCb).
// Returns the new timer id.
int64_t timerSchedule(const std::string& name, const std::string& callback,
                      VmObject* owner, int64_t timeoutSecs,
                      std::vector<Value> boundArgs,
                      const lpc::Ref<ClosureValue>& efunCb =
                          lpc::Ref<ClosureValue>());
// get_timer's "timeout": the accumulated-ticks walk of the ordered list,
// in seconds (the matched timer's absolute trigger offset from now).
double timerRemainingSecs(const std::shared_ptr<TimerEntry>& e);

// ---- construction entry points (efun_adapt.cpp) --------------------------
// load_object / clone_object: construct through the plan machinery; the
// current process parks on "obj-init:<key>" (external callers block on a
// promise instead).
Value vmLoadObjectValue(const std::string& vpath,
                        const std::vector<Value>& createArgs,
                        const std::string& progImage = std::string());
Value vmCloneObjectValue(const std::string& vpath,
                         const std::vector<Value>& createArgs);
// vm_callFunPtr: invoke a closure through its OWNER's current table.
Value vmInvokeClosure(VmPcb* pcb, const lpc::Ref<ClosureValue>& cl,
                      const std::vector<Value>& extra);

// ---- minilpc (the anti-cheat challenge source evaluator; original_vm.cpp)
// NOTE: the evaluator lives in namespace lpc::minilpc (the extracted asset
// keeps its original namespace).
} // namespace vmcore
namespace minilpc {
struct Prog;
bool looksLikeSource(const std::string& s);
std::shared_ptr<Prog> parseSource(const std::string& text);
void registerMini(const std::string& key, std::shared_ptr<Prog> pg);
} // namespace minilpc
namespace vmcore {

// ---- object lifetime (the compat layer owns these) ------------------------
// VmObjects are immortal once created (frames/values hold RAW pointers —
// the original's object table keeps Vm_Object_T alive the same way; a
// destructed object is a flag + reference invalidation, not a free).
void parkVmObject(VmObject* obj);
// Registry key of an object pointer (both VmObject* and its program's
// OriginalObject* are recorded); "" when unknown.
std::string vmKeyOfObjectPtr(const void* objPtr);
// deferred real-free list: an object destructed from inside its own call
// chain leaves the registry only after the issuing process finished.
void vmFlushPendingFrees(const void* proc);

} // namespace vmcore
} // namespace lpc
