// ===========================================================================
// vmcore/driver_model.cpp — env 模型 + 定时器注册表 (driver_model.h 的实现)
// ===========================================================================
#include <algorithm>
#include <unordered_map>

#include "driver_model.h"

#include "call.h"
#include "vmcore.h"

namespace lpc {
namespace vmcore {

// ---------------------------------------------------------------------------
// env (container) model
// ---------------------------------------------------------------------------
namespace {
struct EnvLodgerIndex {
    std::unordered_map<std::string, std::vector<std::string>> lodgers;  // container -> children
    std::unordered_map<std::string, std::string> containerOf;           // child -> container
};
EnvLodgerIndex& envLodgerIndex() {
    static EnvLodgerIndex idx;
    return idx;
}
std::mutex& envPendMutex() { static std::mutex m; return m; }
std::unordered_map<std::string, std::string>& envByKey() {
    static std::unordered_map<std::string, std::string> m;
    return m;
}

// The canonical registry key of any accepted spelling ("" when unknown).
std::string canonKey(const std::string& k) {
    if (k.empty()) return k;
    if (Registry::instance().find(k)) return k;
    // Find the registered object BY NAME (alias table), then use its key —
    // one object, one key, no scans.
    VmObject* o = Registry::instance().findByName(k);
    return o ? o->key() : k;
}

void envLodgerMove(const std::string& child, const std::string& dest) {
    if (child.empty()) return;
    auto& idx = envLodgerIndex();
    auto co = idx.containerOf.find(child);
    if (co != idx.containerOf.end()) {
        auto lo = idx.lodgers.find(co->second);
        if (lo != idx.lodgers.end()) {
            auto& v = lo->second;
            auto pos = std::find(v.begin(), v.end(), child);
            if (pos != v.end()) v.erase(pos);
            if (v.empty()) idx.lodgers.erase(lo);
        }
        idx.containerOf.erase(co);
    }
    if (dest.empty()) return;
    idx.lodgers[dest].push_back(child);   // tail-append (vm_f_move_object)
    idx.containerOf[child] = dest;
}
} // namespace

void envStoreKey(const std::string& selfKey, const std::string& destKey) {
    const std::string self = canonKey(selfKey);
    const std::string dest = canonKey(destKey);
    {
        std::lock_guard<std::mutex> lk(envPendMutex());
        envLodgerMove(self, dest);
    }
    if (self.empty()) return;
    {
        std::lock_guard<std::mutex> lk(envPendMutex());
        if (dest.empty()) envByKey().erase(self);
        else envByKey()[self] = dest;
    }
}

std::string envLookupKey(const std::string& obKey) {
    std::lock_guard<std::mutex> lk(envPendMutex());
    auto it = envByKey().find(canonKey(obKey));
    if (it != envByKey().end()) return it->second;
    return std::string();
}

std::vector<std::string> envLodgerKeysOf(const std::string& containerKey) {
    const std::string c = canonKey(containerKey);
    std::vector<std::string> out;
    {
        std::lock_guard<std::mutex> lk(envPendMutex());
        auto lo = envLodgerIndex().lodgers.find(c);
        if (lo == envLodgerIndex().lodgers.end()) return out;
        out = lo->second;
    }
    // A destructed object leaves its container's list (the original's
    // destruct unlinks it); ours are dropped lazily here.
    std::vector<std::string> live;
    live.reserve(out.size());
    for (const std::string& k : out) {
        if (Registry::instance().findByName(k)) live.push_back(k);
        else {
            std::lock_guard<std::mutex> lk(envPendMutex());
            envLodgerMove(k, "");
        }
    }
    out.swap(live);
    return out;
}

// ---------------------------------------------------------------------------
// timer registry
// ---------------------------------------------------------------------------
namespace {
std::mutex g_timerMtx;
// Ordered by triggerAt (the original's sorted timer list); name lookups walk
// this list for the first match (same-name timers coexist).
std::vector<std::shared_ptr<TimerEntry>> g_timerList;
std::unordered_map<int64_t, std::shared_ptr<TimerEntry>> g_timerById;
std::atomic<int64_t> g_timerSeq{1};

int64_t steadyMsNow() {
    // The SCHEDULER's shared clock (see efun_adapt steadyMs): timer fire
    // times must be on the same axis as the scheduler's nowMs().
    return msSinceStart();
}
} // namespace

std::shared_ptr<TimerEntry> timerFindByName(const std::string& name) {
    std::lock_guard<std::mutex> lk(g_timerMtx);
    for (const auto& e : g_timerList)
        if (e->name == name) return e;
    return nullptr;
}
std::shared_ptr<TimerEntry> timerFindById(int64_t id) {
    std::lock_guard<std::mutex> lk(g_timerMtx);
    auto it = g_timerById.find(id);
    return it == g_timerById.end() ? nullptr : it->second;
}
void timerUnregister(const std::shared_ptr<TimerEntry>& e) {
    if (!e) return;
    std::lock_guard<std::mutex> lk(g_timerMtx);
    if (e->cancelled) *e->cancelled = true;   // cancels the scheduled fire
    for (auto it = g_timerList.begin(); it != g_timerList.end(); ++it)
        if (*it == e) { g_timerList.erase(it); break; }
    auto it = g_timerById.find(e->id);
    if (it != g_timerById.end() && it->second == e) g_timerById.erase(it);
}

int64_t timerSchedule(const std::string& name, const std::string& callback,
                      VmObject* owner, int64_t timeoutSecs,
                      std::vector<Value> boundArgs,
                      const lpc::Ref<ClosureValue>& efunCb) {
    auto entry = std::make_shared<TimerEntry>();
    entry->id = g_timerSeq.fetch_add(1);
    entry->name = name;
    entry->callback = callback;
    entry->efunCb = efunCb;
    entry->owner = owner;
    entry->timeoutSecs = timeoutSecs;
    entry->triggerAt = static_cast<int64_t>(std::time(nullptr)) + timeoutSecs;
    entry->cancelled = std::make_shared<std::atomic<bool>>(false);
    {
        std::lock_guard<std::mutex> lk(g_timerMtx);
        // Re-arming a NAMED timer for the SAME object cancels the older
        // pending one: the game re-arms repeating timers from INSIDE their
        // own callback, and pure coexist semantics doubled the population
        // every period (which drowned the dba service queue behind the
        // timer's own writes).
        if (!entry->name.empty()) {
            for (auto it = g_timerList.begin(); it != g_timerList.end();) {
                const auto& old = *it;
                if (old->name == entry->name && old->owner == owner &&
                    !old->cancelled->load()) {
                    if (old->cancelled) *old->cancelled = true;
                    g_timerById.erase(old->id);
                    it = g_timerList.erase(it);
                } else {
                    ++it;
                }
            }
        }
        auto it = std::lower_bound(
            g_timerList.begin(), g_timerList.end(), entry,
            [](const std::shared_ptr<TimerEntry>& a,
               const std::shared_ptr<TimerEntry>& b) {
                return a->triggerAt < b->triggerAt;
            });
        g_timerList.insert(it, entry);
        g_timerById[entry->id] = entry;
    }
    // Schedule the fire as a scheduler process at the absolute time; the
    // cancelled flag lets delete_timer drop it before it runs.
    int64_t atMs = steadyMsNow() + timeoutSecs * 1000;
    if (atMs < steadyMsNow()) atMs = steadyMsNow() + 1000;   // wrapped: 1s
    if (efunCb)
        Scheduler::instance().spawnDelayedEfun(owner, efunCb, atMs,
                                               entry->cancelled);
    else
        Scheduler::instance().spawnDelayed(owner, callback, std::move(boundArgs),
                                           atMs, entry->cancelled);
    return entry->id;
}

double timerRemainingSecs(const std::shared_ptr<TimerEntry>& e) {
    if (!e) return 0.0;
    std::lock_guard<std::mutex> lk(g_timerMtx);
    // The original walks the sorted list accumulating each node's
    // timeoutTicks (relative deltas) up to the match: head delta = trigger -
    // now, later deltas = trigger-time gaps.
    double acc = 0.0;
    const int64_t now = static_cast<int64_t>(std::time(nullptr));
    int64_t prev = 0;
    for (const auto& ent : g_timerList) {
        const int64_t delta = (prev == 0) ? (ent->triggerAt - now)
                                          : (ent->triggerAt - prev);
        acc += static_cast<double>(delta);
        if (ent == e) break;
        prev = ent->triggerAt;
    }
    return acc;
}

} // namespace vmcore
} // namespace lpc
