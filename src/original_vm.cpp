// ===========================================================================
// original_vm.cpp — 驱动兼容层 (彻底重写版)
//
// 本文件不再包含任何执行引擎。全部 public API 的实现转调新执行核心
// (src/vmcore/: PCB / 全局栈 / 帧 / 对象模型 / 调用原语 / 调度器 / efun
// 适配层); 旧核的 per-object ExecState、runDetached、switchToChain、
// runStates_、savedObjVars 五套切换、suffix 扫描、tplKeyCache 在这里没有
// 对应物。
//
// 保留的驱动侧资产 (原版就有, 与执行核心无关):
//   * 诊断体系: dynProbeFlag / liveness / vmchain 探针 / flow 探针 /
//     per-connection 命令门 (waitClientCommandIdle) — 观测面, 不含 VM 内部。
//   * minilpc: 反外挂挑战脚本 (check_server) 的 LPC 源求值器
//     (vm_asset_minilpc.inc, 自包含)。
//   * AntiCheatScript: 0x5108 载荷的解码/求值 (vm_asset_anticheat.inc)。
// 语义依据: 原版 magic_Linux32x (IDA 地址标注)。
// ===========================================================================
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "lzss.h"

#include "vmcore/call.h"
#include "vmcore/driver_model.h"
#include "vmcore/interp.h"
#include "vmcore/object.h"
#include "vmcore/values.h"
#include "vmcore/vmcore.h"

#include "builtins.h"
#include "interpreter.h"
#include "o_loader.h"
#include "original_vm.h"
#include "pak_loader.h"
#include "xdbc.h"

namespace lpc {

// ===========================================================================
// 1. runtime-toggleable diagnostics
// ===========================================================================
const std::atomic<bool>& dynProbeFlag(const char* name) {
    static std::mutex regMx;
    static std::map<std::string, std::shared_ptr<std::atomic<bool>>> reg;
    static std::mutex refreshMx;
    static int64_t lastRefresh = 0;
    std::lock_guard<std::mutex> lk(regMx);
    auto it = reg.find(name);
    if (it == reg.end()) {
        auto flag = std::make_shared<std::atomic<bool>>(
            std::getenv(name) != nullptr);
        // The marker file (_probe_on\<NAME>) re-arms a probe on a LIVE
        // server (a reboot costs ~11 minutes); re-read at most once a second.
        std::thread([flag, name = std::string(name)] {
            for (;;) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (std::filesystem::exists("_probe_on/" + name))
                    flag->store(true);
            }
        }).detach();
        it = reg.emplace(name, std::move(flag)).first;
    }
    return *it->second;
}

LivenessCounters& liveness() {
    static LivenessCounters c;
    return c;
}

int64_t msSinceStart() { return vmcore::msSinceStart(); }

int64_t usNow() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch())
        .count();
}

// ---- reply-delivery flow probe (LPCVM_FLOW) -------------------------------
namespace {
bool flowOn() {
    static const bool on = (std::getenv("LPCVM_FLOW") != nullptr);
    return on;
}
std::map<std::string, std::pair<int64_t, std::string>>& flowParks() {
    static std::map<std::string, std::pair<int64_t, std::string>> m;
    return m;
}
std::mutex& flowMtx() { static std::mutex m; return m; }
}  // namespace

void flowProbePark(const char* key, const char* fn) {
    if (!flowOn() || !key) return;
    std::lock_guard<std::mutex> lk(flowMtx());
    flowParks()[key] = {usNow(), fn ? fn : "?"};
    std::cerr << "[flow] t=" << usNow() << " ev=PARK cno=" << dispatchCno()
              << " tid=" << vmcore::currentTid()
              << " key=" << key << " fn=" << (fn ? fn : "?") << "\n";
    std::cerr.flush();
}

void flowProbeWake(const char* key, const char* fn) {
    if (!flowOn() || !key) return;
    int64_t waited = -1;
    std::string pfn = "?";
    {
        std::lock_guard<std::mutex> lk(flowMtx());
        auto it = flowParks().find(key);
        if (it != flowParks().end()) {
            waited = usNow() - it->second.first;
            pfn = it->second.second;
            flowParks().erase(it);
        }
    }
    std::cerr << "[flow] t=" << usNow() << " ev=WAKE cno=" << dispatchCno()
              << " tid=" << vmcore::currentTid()
              << " key=" << key << " fn=" << (fn ? fn : "?")
              << " parkedFn=" << pfn << " waited=" << waited << "us\n";
    std::cerr.flush();
}

void flowProbeReply(int64_t cno, int64_t cmd, int64_t len) {
    if (!flowOn()) return;
    std::cerr << "[flow] t=" << usNow() << " ev=REPLY cno=" << cno
              << " cmd=0x" << std::hex << cmd << std::dec << " len=" << len
              << "\n";
    std::cerr.flush();
}

// ===========================================================================
// 2. object lifetime (park / pointer identity / deferred free)
// ===========================================================================
namespace {
std::mutex& objParkMx() { static std::mutex m; return m; }
std::vector<std::unique_ptr<vmcore::VmObject>>& objPark() {
    static std::vector<std::unique_ptr<vmcore::VmObject>> v;
    return v;
}
std::unordered_map<const void*, std::string>& objKeyByPtr() {
    static std::unordered_map<const void*, std::string> m;
    return m;
}

struct PendingFree {
    std::string key;
    const void* proc;
};
std::vector<PendingFree>& pendingFrees() {
    static std::vector<PendingFree> v;
    return v;
}
}  // namespace

namespace vmcore {
void parkVmObject(VmObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lk(objParkMx());
    objPark().emplace_back(obj);
    objKeyByPtr()[obj] = obj->key();
    if (obj->prog()) objKeyByPtr()[&obj->prog()->src()] = obj->key();
}

// ---------------------------------------------------------------------------
// Orphaned-contents cascade.  The original driver garbage-collects objects
// that lose their last reference when their owner is destructed — a logged-
// out character's carried items/pets are collected and their destructors run
// (iid.c::destruct -> unregister_iid on the master's global iid map).  Our
// port keeps objects immortal, so those iids stayed registered forever and
// every re-login restore hit register_iid -> duplicate ->
// when_iid_duplicated(deny) -> "原因码9202/9211" on login.  Walk the dying
// object's instance variables and free the per-character clones
// (item/pet/guard/child) it held: each clone's mudlib destructor runs in its
// own scheduler process, then the clone leaves the registry.
// ---------------------------------------------------------------------------
static bool isOwnedCloneKey(const std::string& k) {
    if (k.find('#') == std::string::npos) return false;   // clones only
    return k.find("/clone/item/") != std::string::npos ||
           k.find("/clone/pet/") != std::string::npos ||
           k.find("/clone/char/guard") != std::string::npos ||
           k.find("/clone/char/child") != std::string::npos;
}
static void collectOwnedClones(const Value& v,
                               std::set<const void*>& seen,
                               std::vector<vmcore::VmObject*>& out,
                               int depth) {
    if (depth > 8) return;
    switch (v.index()) {
    case 7: {
        auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
        if (!ov) return;
        vmcore::VmObject* o = ov->vmObj;
        if (!o && !ov->programName.empty() && ov->programName != "<this>")
            o = vmcore::Registry::instance().findByName(ov->programName);
        if (!o || !seen.insert(static_cast<const void*>(o)).second) return;
        if (isOwnedCloneKey(o->key())) {
            out.push_back(o);
            // recurse into the item (nested bags/gems) but not into rooms/NPCs
            for (const auto& vv : o->vars())
                collectOwnedClones(vv, seen, out, depth + 1);
        }
        return;
    }
    case 5: {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
        if (!arr || !seen.insert(static_cast<const void*>(arr.get())).second) return;
        for (const auto& e : arr->items)
            collectOwnedClones(e, seen, out, depth + 1);
        return;
    }
    case 6: {
        auto mp = std::get<std::shared_ptr<MappingValue>>(v);
        if (!mp || !seen.insert(static_cast<const void*>(mp.get())).second) return;
        mp->forEachActive([&](const Value&, const Value& val) {
            collectOwnedClones(val, seen, out, depth + 1);
        });
        return;
    }
    default: return;
    }
}
static void freeOrphanedContents(vmcore::VmObject* o) {
    std::set<const void*> seen;
    std::vector<vmcore::VmObject*> owned;
    for (const auto& v : o->vars())
        collectOwnedClones(v, seen, owned, 0);
    static const std::atomic<bool>& cscDbg = dynProbeFlag("LPCVM_CASCADE");
    if (cscDbg.load(std::memory_order_relaxed)) {
        std::cerr << "[cascade] " << o->key() << " owned=" << owned.size();
        std::cerr.flush();
        for (auto* c : owned) std::cerr << " " << (c ? c->key() : "?");
        std::cerr << "\n";
        std::cerr.flush();
    }
    for (auto* c : owned) {
        if (!c || c->destructed()) continue;
        c->markDestructed();
        // The mudlib destructor (iid.c::destruct -> unregister_iid) must run
        // while the object is still registered — master's iid_map holds a
        // live ObjectValue the destructor compares against this_object().
        // Closing references first (an earlier attempt) nulled that value
        // and the unregister silently no-opped.  Instead defer the real
        // free to the destructor process's own chain end.
        vmcore::VmPcb* sp =
            vmcore::Scheduler::instance().spawn(c, "destruct", {});
        pendingFrees().push_back(
            PendingFree{c->key(), static_cast<const void*>(sp)});
    }
}

std::string vmKeyOfObjectPtr(const void* objPtr) {
    std::lock_guard<std::mutex> lk(objParkMx());
    auto it = objKeyByPtr().find(objPtr);
    return it == objKeyByPtr().end() ? std::string() : it->second;
}
void vmFlushPendingFrees(const void* proc) {
    auto& pf = pendingFrees();
    if (pf.empty()) return;
    std::vector<PendingFree> now;
    for (auto it = pf.begin(); it != pf.end();) {
        if (it->proc == proc) {
            now.push_back(*it);
            it = pf.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& e : now) {
        vmcore::VmObject* obj = vmcore::Registry::instance().findByName(e.key);
        static const std::atomic<bool>& cscDbg0 = dynProbeFlag("LPCVM_CASCADE");
        if (cscDbg0.load(std::memory_order_relaxed)) {
            std::cerr << "[cascade] flush proc=" << e.proc << " key=" << e.key
                      << " found=" << (obj ? 1 : 0)
                      << " destructed=" << (obj ? (obj->destructed() ? 1 : 0) : -1)
                      << "\n";
            std::cerr.flush();
        }
        if (obj && obj->destructed()) {
            // Free the carried items/pets first (their destructors unregister
            // the iids — see freeOrphanedContents), then the owner itself.
            freeOrphanedContents(obj);
            // The object leaves the registry here: every ObjectValue that
            // carries its flag stops being an object (the original driver's
            // reference invalidation on a real free).
            obj->closeReferences();
            vmcore::Registry::instance().unregisterObject(obj->key());
            vmcore::Registry::instance().clearConstructing(obj->key());
        }
    }
}
}  // namespace vmcore

// ===========================================================================
// 3. per-connection command-chain probe + gate
// ===========================================================================
namespace {
struct ChainLive {
    int64_t clientNo = -1;
    int64_t chainKey = -1;
    const void* vm = nullptr;
    std::string entry;
    int64_t beginMs = 0;
    bool parked = false;
    bool exited = false;
};
std::mutex& chainLiveMtx() { static std::mutex m; return m; }
std::map<int64_t, ChainLive>& chainLive() {
    static std::map<int64_t, ChainLive> m;
    return m;
}
bool chainTraceOn() {
    static const std::atomic<bool>& f = dynProbeFlag("LPCVM_CHAINTRACE");
    return f.load(std::memory_order_relaxed);
}
const char* chainKindName(int k) {
    switch (k) {
        case 0: return "runtime";
        case 1: return "timer";
        case 2: return "queue";
        case 3: return "event";
        case 4: return "object";
        case 5: return "socket";
        case 6: return "mutex";
        case 7: return "service";
        default: return "?";
    }
}
void chainEmit(const char* ev, int64_t cno, int64_t key, const void* vm,
               const std::string& entry, const std::string& extra,
               const std::string& chain) {
    std::cerr << "[chain] t=" << lpc::msSinceStart() << "ms ev=" << ev
              << " cno=" << cno << " key=" << key << " vm=" << vm;
    if (!entry.empty()) std::cerr << " entry=" << entry;
    if (!extra.empty()) std::cerr << " " << extra;
    std::cerr << " live=" << lpc::vmchainLiveChains(cno)
              << " chain=[" << chain << "]\n";
    std::cerr.flush();
}
}  // namespace

void vmchainBegin(int64_t clientNo, int64_t chainKey, const void* vm,
                  const std::string& entry) {
    liveness().chainBegin.fetch_add(1, std::memory_order_relaxed);
    std::string overlap;
    {
        std::lock_guard<std::mutex> lk(chainLiveMtx());
        auto& m = chainLive();
        if (clientNo >= 0) {
            // THE invariant of the public driver model: one connection runs
            // one command at a time.  Any other un-ENDed chain of this
            // connection is the 9101 「正在处理中」 cause.
            for (const auto& kv : m) {
                if (kv.second.clientNo != clientNo) continue;
                if (kv.second.chainKey == chainKey) continue;
                overlap =
                    "OVERLAP_WITH key=" + std::to_string(kv.second.chainKey) +
                    " vm=" +
                    std::to_string(reinterpret_cast<uintptr_t>(kv.second.vm)) +
                    " entry=" + kv.second.entry +
                    " ageMs=" +
                    std::to_string(lpc::msSinceStart() - kv.second.beginMs);
                break;
            }
        }
        ChainLive cl;
        cl.clientNo = clientNo;
        cl.chainKey = chainKey;
        cl.vm = vm;
        cl.entry = entry;
        cl.beginMs = lpc::msSinceStart();
        m[chainKey] = cl;
    }
    if (!overlap.empty()) {
        liveness().chainOverlap.fetch_add(1, std::memory_order_relaxed);
        if (chainTraceOn())
            chainEmit("OVERLAP", clientNo, chainKey, vm, entry, overlap,
                      vmchainTextOf(vm));
    } else if (chainTraceOn()) {
        chainEmit("BEGIN", clientNo, chainKey, vm, entry, std::string(),
                  vmchainTextOf(vm));
    }
}

void vmchainEnd(int64_t clientNo, int64_t chainKey, const void* vm,
                const std::string& entry, bool completed) {
    if (completed)
        liveness().chainEnd.fetch_add(1, std::memory_order_relaxed);
    const bool traced = chainTraceOn();
    int64_t age = 0;
    bool known = false;
    int64_t emitCno = clientNo;
    std::string emitEntry = entry;
    {
        std::lock_guard<std::mutex> lk(chainLiveMtx());
        auto& m = chainLive();
        auto it = m.find(chainKey);
        if (it != m.end()) {
            age = lpc::msSinceStart() - it->second.beginMs;
            known = it->second.clientNo >= 0;
            emitCno = it->second.clientNo;
            emitEntry = it->second.entry;
            m.erase(it);
        }
    }
    if (!traced) {
        // Wake the per-connection gate even untraced: a connection may
        // dispatch its next command only when this one is really over.
        chainWaitNotify();
        return;
    }
    std::string extra = "completed=" + std::to_string(completed ? 1 : 0);
    if (age > 0) extra += " ageMs=" + std::to_string(age);
    chainEmit(completed ? "END" : "DROP", known ? emitCno : -1, chainKey, vm,
              emitEntry, extra, vmchainTextOf(vm));
    chainWaitNotify();
}

void vmchainPark(int64_t clientNo, int64_t chainKey, const void* vm,
                 const std::string& entry, int condKind,
                 const std::string& condKey) {
    liveness().chainPark.fetch_add(1, std::memory_order_relaxed);
    const bool traced = chainTraceOn();
    {
        std::lock_guard<std::mutex> lk(chainLiveMtx());
        auto& m = chainLive();
        auto it = m.find(chainKey);
        if (it != m.end()) {
            it->second.parked = true;
            it->second.exited = true;
        } else {
            ChainLive cl;
            cl.clientNo = clientNo;
            cl.chainKey = chainKey;
            cl.vm = vm;
            cl.entry = entry;
            cl.beginMs = lpc::msSinceStart();
            cl.parked = true;
            cl.exited = true;
            m[chainKey] = cl;
        }
    }
    if (!traced) return;
    std::string what =
        std::string("wait=") + chainKindName(condKind) + " key=" + condKey;
    chainEmit("PARK", clientNo, chainKey, vm, entry, what, vmchainTextOf(vm));
}

void vmchainRun(int64_t chainKey, const void* vm) {
    int64_t resumeCno = -1;
    bool wasParked = false;
    std::string resumeEntry;
    const bool traced = chainTraceOn();
    {
        std::lock_guard<std::mutex> lk(chainLiveMtx());
        auto it = chainLive().find(chainKey);
        if (it != chainLive().end() && it->second.exited) {
            wasParked = it->second.parked;
            resumeCno = it->second.clientNo;
            resumeEntry = it->second.entry;
            it->second.parked = false;
            it->second.exited = false;
        }
    }
    if (!wasParked) return;
    liveness().chainResume.fetch_add(1, std::memory_order_relaxed);
    if (traced)
        chainEmit("RESUME", resumeCno, chainKey, vm, resumeEntry,
                  std::string(), vmchainTextOf(vm));
}

void vmchainRunExit(int64_t chainKey, bool parked) {
    std::lock_guard<std::mutex> lk(chainLiveMtx());
    auto it = chainLive().find(chainKey);
    if (it == chainLive().end()) return;
    it->second.exited = true;
    it->second.parked = parked;
}

void vmchainTagLive(int64_t clientNo, int64_t chainKey) {
    if (clientNo < 0 || chainKey < 0) return;
    std::lock_guard<std::mutex> lk(chainLiveMtx());
    auto it = chainLive().find(chainKey);
    if (it != chainLive().end()) it->second.clientNo = clientNo;
}

void vmchainMailbox(int64_t clientNo, int64_t chainKey, const char* what) {
    if (!chainTraceOn()) return;
    chainEmit("MAILBOX", clientNo, chainKey, nullptr, std::string(),
              std::string(what ? what : ""), std::string());
}

int vmchainLiveChains(int64_t clientNo) {
    if (clientNo < 0) return 0;
    std::lock_guard<std::mutex> lk(chainLiveMtx());
    int n = 0;
    for (const auto& kv : chainLive())
        if (kv.second.clientNo == clientNo && !kv.second.exited) ++n;
    return n;
}

std::string vmchainTextOf(const void* vm) {
    const vmcore::VmPcb* p = static_cast<const vmcore::VmPcb*>(vm);
    return p ? p->frameChainText(12) : std::string();
}

void registerClientScriptVm(int64_t cno, const void* vm) {
    // Kept for header compatibility: the new core attributes chains to their
    // connection through the PCB (setDispatchCno + execOnMain), so the
    // script-side busy observable is the connection's live-chain count.
    (void)cno;
    (void)vm;
}

bool clientLpcBusy(int64_t cno) {
    // The script's own re-entrancy guard (`lite_set_temp("operating",1)`)
    // can outlive the driver's bookkeeping for a command: its phases run on
    // chains the driver tags separately.  The LPC-side busy observable is
    // therefore the connection's live (un-exited) chain count.
    return vmchainLiveChains(cno) > 0;
}

namespace {
std::mutex& chainGateMtx() { static std::mutex m; return m; }
std::condition_variable& chainGateCv() {
    static std::condition_variable cv;
    return cv;
}
std::atomic<int>& cmdActiveCount(int64_t cno) {
    static std::mutex m;
    static std::map<int64_t, std::atomic<int>*> tbl;
    std::lock_guard<std::mutex> lk(m);
    auto it = tbl.find(cno);
    if (it != tbl.end()) return *it->second;
    auto* p = new std::atomic<int>(0);
    tbl[cno] = p;
    return *p;
}
}  // namespace

void chainWaitNotify() { chainGateCv().notify_all(); }

void cmdActiveAttach(int64_t cno) {
    if (cno < 0) return;
    cmdActiveCount(cno).fetch_add(1, std::memory_order_acq_rel);
}

void cmdActiveDetach(int64_t cno) {
    if (cno < 0) return;
    if (cmdActiveCount(cno).fetch_sub(1, std::memory_order_acq_rel) <= 1) {
        chainGateCv().notify_all();
    }
}

bool waitClientCommandIdle(int64_t clientNo, int timeoutMs) {
    if (clientNo < 0) return true;
    const int64_t totalMs = timeoutMs > 0 ? timeoutMs : 20000;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(totalMs);
    // The "still busy" state changes in several places and not all of them
    // can notify; notifications only shorten the wait, a 100ms poll bounds a
    // missed one.
    const auto pollMs = std::chrono::milliseconds(100);
    for (;;) {
        const int active = cmdActiveCount(clientNo).load(std::memory_order_acquire);
        const int live = vmchainLiveChains(clientNo);
        const bool lpc = clientLpcBusy(clientNo);
        if (active <= 0 && live <= 0 && !lpc) return true;
        if (std::chrono::steady_clock::now() >= deadline) {
            std::cerr << "[chain-gate] cno=" << clientNo << " still busy after "
                      << totalMs << "ms (active=" << active
                      << ", liveChains=" << live
                      << ", lpcOperating=" << (lpc ? 1 : 0) << ")\n";
            std::cerr.flush();
            return false;
        }
        std::unique_lock<std::mutex> lk(chainGateMtx());
        chainGateCv().wait_for(lk, pollMs);
    }
}

// ===========================================================================
// 4. current process identity
// ===========================================================================
int64_t schedulerCurrentTid() { return vmcore::currentTid(); }

// ===========================================================================
// 5. minilpc — the anti-cheat challenge source evaluator (verbatim asset)
// ===========================================================================
#include "vm_asset_minilpc.inc"

// ===========================================================================
// 6. AntiCheatScript — 0x5108 payload evaluator (verbatim asset)
// ===========================================================================
#include "vm_asset_anticheat.inc"

bool antiCheatCheckServer(const std::string& cryptedBuf, int& x, int& y,
                          int& answer) {
    if (cryptedBuf.size() < 4) return false;
    const auto* p = reinterpret_cast<const uint8_t*>(cryptedBuf.data());
    const uint32_t origLen = (static_cast<uint32_t>(p[0]) << 24) |
                             (static_cast<uint32_t>(p[1]) << 16) |
                             (static_cast<uint32_t>(p[2]) << 8) |
                             static_cast<uint32_t>(p[3]);
    std::vector<uint8_t> body(p + 4, p + cryptedBuf.size());
    std::vector<uint8_t> script;
    if (origLen == 0) {
        script = body;
    } else if (!lzssDecompress(body, origLen, script)) {
        return false;
    }
    if (script.size() > 1554) {
        if (dynProbeFlag("LPCVM_AC").load(std::memory_order_relaxed))
            std::cerr << "[ac] script too big: " << script.size()
                      << " > 1554\n";
        return false;
    }
    int code = 102;
    for (size_t i = 0; i < script.size(); ++i) {
        if (script[i] != static_cast<uint8_t>(code))
            script[i] ^= static_cast<uint8_t>(code);
        code = (((code * 2) + 7) ^ 71) & 255;
    }
    if (script.empty()) return false;
    if (dynProbeFlag("LPCVM_AC").load(std::memory_order_relaxed)) {
        std::cerr << "[ac-src] <<<\n"
                  << std::string(script.begin(), script.end())
                  << "\n[ac-src] >>>\n";
        std::cerr.flush();
    }
    {
        AntiCheatScript acs;
        const bool trace = dynProbeFlag("LPCVM_AC").load(std::memory_order_relaxed);
        bool ok = acs.parse(std::string(script.begin(), script.end()));
        if (!ok) std::cerr << "[ac] parse failed\n";
        if (ok) { ok = acs.eval("get_x", {}, x); if (!ok) std::cerr << "[ac] get_x failed\n"; }
        if (ok) { ok = acs.eval("get_y", {}, y); if (!ok) std::cerr << "[ac] get_y failed\n"; }
        if (ok) {
            ok = acs.eval("challenge_server", {x, y}, answer);
            if (!ok) std::cerr << "[ac] challenge_server failed\n";
        }
        if (!ok) {
            if (trace) {
                std::cerr << "[ac] challenge eval failed; script:\n"
                          << std::string(script.begin(), script.end()) << "\n";
                std::cerr.flush();
            }
            // NEVER leave the client without an answer.
            x = 0;
            y = 0;
            answer = 0;
        }
    }
    if (dynProbeFlag("LPCVM_AC").load(std::memory_order_relaxed))
        std::cerr << "[ac] x=" << x << " y=" << y << " answer=" << answer << "\n";
    return true;
}

// ===========================================================================
// 7. boot (runOriginalObject): the master program as a construct process
// ===========================================================================
namespace {
std::string masterRegistryKey(const std::string& searchRoot) {
    return searchRoot.empty() ? "<main>" : searchRoot + "/<main>";
}
vmcore::VmObject* findMasterVmObject() {
    auto& reg = vmcore::Registry::instance();
    if (reg.findByName("<main>")) return reg.findByName("<main>");
    for (vmcore::VmObject* o : reg.allObjects()) {
        if (o->key().find("<main>") != std::string::npos) return o;
    }
    return nullptr;
}
}  // namespace

int runOriginalObject(const OriginalObject& obj, std::ostream& out,
                      const std::string& searchRoot) {
    auto& reg = vmcore::Registry::instance();
    reg.setDiskRoot(searchRoot);
    const std::string mainKey = masterRegistryKey(searchRoot);
    // Efun-adapter boot wiring (event peek/consume hooks, socket poll
    // backend) — once, before any process runs.
    vmcore::wireEfunLayer();

    // The master program from the loader output; inherited programs resolve
    // through the same registry cache as every load.
    auto src = std::make_shared<OriginalObject>(obj);
    auto progRaw = std::make_shared<vmcore::VmProgram>(
        src, [](const std::string& vpath) {
            return vmcore::Registry::instance().loadInheritProgramPublic(vpath);
        });
    auto prog = std::shared_ptr<const vmcore::VmProgram>(progRaw);
    progRaw->setSelf(prog);

    auto* master = new vmcore::VmObject(prog, mainKey);
    vmcore::parkVmObject(master);
    reg.registerObject(mainKey, master);
    // Set BEFORE the master runs: its own init code calls global functions
    // (call_in_user_fun bit7), which resolve against this program.
    vmcore::Interp::setMasterProgram(prog.get(), master);

    // The master is a SCHEDULER construct process, exactly like the original
    // driver (vm_mainLoop runs from the very start; the master's startup
    // code - load_all, create, post_init - shares the CPU with the daemon
    // processes it creates, both through its relinquish() calls and the
    // runTicks time slice).
    reg.markConstructing(mainKey);
    auto plan = vmcore::buildConstructPlan(master, {});
    vmcore::VmPcb* masterPcb = vmcore::Scheduler::instance().spawnConstruct(
        std::move(plan));
    // runTicks safety net (the boot code also yields cooperatively through
    // relinquish()): the countdown must START at the slice size or the first
    // instruction yields.
    if (masterPcb) {
        masterPcb->yieldEvery = 2000000;
        masterPcb->yieldCountdown = masterPcb->yieldEvery;
    }

    out << "Original .o VM finished.\n";
    return 0;
}

int runOriginalObjectInit(const OriginalObject& obj, const std::string& searchRoot) {
    // Driver-side one-shot init (not used by the pak boot path): run the
    // construction synchronously on a scratch process.
    auto& reg = vmcore::Registry::instance();
    reg.setDiskRoot(searchRoot);
    const std::string mainKey = masterRegistryKey(searchRoot);
    auto src = std::make_shared<OriginalObject>(obj);
    auto progRaw = std::make_shared<vmcore::VmProgram>(
        src, [](const std::string& vpath) {
            return vmcore::Registry::instance().loadInheritProgramPublic(vpath);
        });
    auto prog = std::shared_ptr<const vmcore::VmProgram>(progRaw);
    progRaw->setSelf(prog);
    auto* master = new vmcore::VmObject(prog, mainKey);
    vmcore::parkVmObject(master);
    reg.registerObject(mainKey, master);
    vmcore::Interp::setMasterProgram(prog.get(), master);
    try {
        auto plan = vmcore::buildConstructPlan(master, {});
        vmcore::VmPcb scratch;
        vmcore::vmRunConstructionSync(&scratch, plan);
        reg.markInitialized(mainKey);
    } catch (...) {
        return 1;
    }
    return 0;
}

Value callMasterProgramFunction(const std::string& funName,
                                const std::vector<Value>& args,
                                const std::string& searchRoot) {
    (void)searchRoot;
    vmcore::VmObject* master = findMasterVmObject();
    if (!master) return make_int(0);
    // Route through the scheduler: VM execution is single-threaded.  Nested
    // calls from a running process run directly; calls from network/telnet/
    // mysql threads spawn a scheduler process and block on its result.
    return vmcore::Scheduler::instance().execOnMain(master, funName, args);
}

std::string masterSearchRoot() {
    vmcore::VmObject* master = findMasterVmObject();
    if (!master) return "";
    const std::string& key = master->key();
    const size_t p = key.rfind("/<main>");
    if (p != std::string::npos) return key.substr(0, p);
    return "";
}

// ===========================================================================
// 8. registry compatibility surface
// ===========================================================================
int loadObjectIntoRegistry(const std::string& virtualPath,
                           const std::string& searchRoot,
                           const std::vector<Value>& createArgs) {
    if (!searchRoot.empty())
        vmcore::Registry::instance().setDiskRoot(searchRoot);
    Value v = vmcore::vmLoadObjectValue(virtualPath, createArgs);
    return v.index() == 7 ? 1 : 0;
}

Value findObjectInRegistry(const std::string& virtualPath,
                           const std::string& searchRoot) {
    (void)searchRoot;
    vmcore::VmObject* o = vmcore::Registry::instance().findByName(virtualPath);
    static const std::atomic<bool>& foDbg = dynProbeFlag("LPCVM_FODB");
    if (foDbg.load(std::memory_order_relaxed) &&
        virtualPath.find("/gs/zone/") != std::string::npos) {
        std::cerr << "[fobj] '" << virtualPath << "' -> "
                  << (o ? o->key() : "0") << "\n";
        std::cerr.flush();
    }
    if (!o) return make_int(0);
    return vmcore::Registry::makeObjectValue(o);
}

bool objectValueAlive(const Value& v) {
    if (v.index() != 7) return false;
    auto ov = std::get<std::shared_ptr<ObjectValue>>(v);
    if (!ov) return false;
    return !ov->alive || ov->alive->load(std::memory_order_relaxed);
}
bool objectValueFreed(const Value& v) {
    return v.index() == 7 && !objectValueAlive(v);
}

Value cloneObjectInRegistry(const std::string& virtualPath,
                            const std::string& searchRoot,
                            const std::vector<Value>& createArgs) {
    (void)searchRoot;
    return vmcore::vmCloneObjectValue(virtualPath, createArgs);
}

bool destructObjectByKey(const std::string& keyIn) {
    if (keyIn.empty()) return false;
    auto& reg = vmcore::Registry::instance();
    vmcore::VmObject* obj = reg.findByName(keyIn);
    if (!obj) return false;
    if (obj->destructed()) {
        // Re-entrant destruct_object(<same key>) from inside the destructor
        // chain: the OUTER call owns the registry teardown, so leave the
        // entry in place — the chain keeps calling back into the object
        // (me->save()).
        return true;
    }
    // Phase 1 "marked": the object stays registered while its destructor
    // chain runs — user.o::destruct -> userd::destroy_user -> me->save() is
    // the ONLY dbase writer and resolves `me` through the registry.
    obj->markDestructed();
    const OriginalFunction* dtr = obj->prog()->findFn("destruct");
    {
        // LPCVM_LIFE (once-cached env, driver context only): was the mudlib
        // destructor actually resolvable?  A MISSING here means the whole
        // logout save chain (destruct -> userd::destroy_user -> me->save)
        // silently never runs while destruct_object still reports success.
        static const bool lifeProbe2 = [] {
            const char* e = std::getenv("LPCVM_LIFE");
            return e && *e;
        }();
        if (lifeProbe2)
            std::cerr << "[life] destruct-chain " << keyIn << " dtr="
                      << (dtr ? "FOUND" : "MISSING (destructor never runs!)")
                      << "\n";
    }
    if (dtr) {
        vmcore::VmPcb* pcb = vmcore::currentPcb();
        if (pcb) {
            // 原版 vm_f_destruct_object：destruct 链在当前进程内联跑（压上
            // 下文帧，同一扁平循环），收尾（关闭引用、移出注册表）挂在链
            // 返回点。这里同构：压帧 + 帧上 retHook 收尾 + efunTakeover——
            // destruct 链内的阻塞 efun（me->save() → filed）park 后 resume
            // 照常走完链再收尾，C++ 无续延（vmApplySync 的递归在 park 时
            // 会销毁收尾路径，boot 曾因此静默死亡）。
            vmPushFunCall(pcb, obj, dtr, {});
            const std::string okey = obj->key();
            vmcore::vmHookCallbackWhenReturn(pcb, [okey](Value& rv) {
                // LPCVM_LIFE: the destructor frame RETURNED (the whole
                // user.o::destruct body ran to its end, whichever branch).
                {
                    static const bool lifeProbe3 = [] {
                        const char* e = std::getenv("LPCVM_LIFE");
                        return e && *e;
                    }();
                    if (lifeProbe3)
                        std::cerr << "[life] destruct-chain " << okey
                                  << " BODY-DONE\n";
                }
                auto& reg2 = vmcore::Registry::instance();
                if (vmcore::VmObject* o = reg2.findByName(okey)) {
                    vmcore::VmPcb* pcb2 = vmcore::currentPcb();
                    bool executing = false;
                    if (pcb2) {
                        for (const auto& fr : pcb2->frames)
                            if (fr.obj == o) { executing = true; break; }
                    }
                    if (executing) {
                        pendingFrees().push_back(
                            PendingFree{okey, static_cast<const void*>(pcb2)});
                    } else {
                        freeOrphanedContents(o);
                        o->closeReferences();
                        reg2.unregisterObject(okey);
                        reg2.clearConstructing(okey);
                    }
                }
                rv = make_int(1);   // destruct_object 的返回值
            });
            pcb->efunTakeover = true;
            return true;
        }
        vmcore::Scheduler::instance().execOnMain(obj, "destruct", {});
    }
    // Phase 2 "really freed": an object still executing (its frames on the
    // current process's chain) is only flagged — code that runs AFTER
    // destruct_object(this_object()) in the same chain must still work
    // (charge items' apply() prints the item name after consuming it).  The
    // registry removal waits for the issuing process to finish
    // (vmFlushPendingFrees, called by the scheduler at chain end).
    vmcore::VmPcb* pcb = vmcore::currentPcb();
    bool executing = false;
    if (pcb) {
        for (const auto& fr : pcb->frames)
            if (fr.obj == obj) { executing = true; break; }
    }
    if (executing) {
        pendingFrees().push_back(
            PendingFree{obj->key(), static_cast<const void*>(pcb)});
        return true;
    }
    freeOrphanedContents(obj);
    obj->closeReferences();
    reg.unregisterObject(obj->key());
    reg.clearConstructing(obj->key());
    return true;
}

std::string canonicalObjectPath(const std::string& key) {
    auto& reg = vmcore::Registry::instance();
    if (vmcore::VmObject* o = reg.findByName(key)) return o->key();
    return vmcore::Registry::canonicalKey(key, reg.diskRootOf());
}

const void* objectIdentityOf(const std::string& name) {
    vmcore::VmObject* o = vmcore::Registry::instance().findByName(name);
    return o ? static_cast<const void*>(o) : nullptr;
}

std::string regKeyOfObjectPtr(const void* objPtr) {
    return vmcore::vmKeyOfObjectPtr(objPtr);
}

std::string objectTemplateKey(const std::string& key) {
    vmcore::VmObject* o = vmcore::Registry::instance().findByName(key);
    if (!o || !o->templateObj) return std::string();
    return o->templateObj->key();
}

Value makeLodgerArray(const std::string& containerKey) {
    Value arr = make_array();
    auto av = std::get<std::shared_ptr<ArrayValue>>(arr);
    for (const std::string& k : vmcore::envLodgerKeysOf(containerKey))
        av->items.push_back(make_object(k));
    return arr;
}

// ===========================================================================
// 9. instance variables by NAME (vm_getObjVar / save_object)
// ===========================================================================
namespace {
vmcore::VmObject* vmOfObjValue(const Value& obj) {
    if (obj.index() == 4) {
        const std::string& nm = std::get<SharedString>(obj).str();
        return nm.empty() ? nullptr
                          : vmcore::Registry::instance().findByName(nm);
    }
    if (obj.index() != 7) return nullptr;
    auto ov = std::get<std::shared_ptr<ObjectValue>>(obj);
    if (!ov) return nullptr;
    if (ov->vmObj) return ov->vmObj;
    if (!ov->programName.empty())
        return vmcore::Registry::instance().findByName(ov->programName);
    return nullptr;
}
}  // namespace

Value objVarGet(const Value& obj, const std::string& name) {
    vmcore::VmObject* o = vmOfObjValue(obj);
    if (!o) return Value{};
    const int slot = o->prog()->varSlotByName(name);
    if (slot < 0 || static_cast<size_t>(slot) >= o->vars().size())
        return Value{};
    return o->vars()[static_cast<size_t>(slot)];
}

bool objVarSet(const Value& obj, const std::string& name, const Value& v) {
    vmcore::VmObject* o = vmOfObjValue(obj);
    if (!o) return false;
    const int slot = o->prog()->varSlotByName(name);
    if (slot < 0 || static_cast<size_t>(slot) >= o->vars().size())
        return false;
    o->vars()[static_cast<size_t>(slot)] = v;
    return true;
}

std::string objVarsToString(const Value& obj) {
    vmcore::VmObject* o = vmOfObjValue(obj);
    if (!o) return std::string();
    // save_object(): "prog::var=value\n" for every non-hidden variable slot
    // of the object's OWN program that has a PROG_VARS record; undefined
    // slots are not saved.
    std::string out;
    const auto& pv = o->prog()->src().progVars;
    for (const auto& rec : pv) {
        const size_t slot = static_cast<size_t>(
            static_cast<int16_t>(rec.raw[0xa] | (rec.raw[0xb] << 8)));
        if (slot >= o->vars().size()) continue;
        const Value& v = o->vars()[slot];
        if (v.index() == 0) continue;   // undefined -> not saved
        out += rec.name.empty() ? ("::" + std::to_string(slot)) : rec.name;
        out += "=";
        out += to_string(v, true);
        out += "\n";
    }
    return out;
}

// ===========================================================================
// 10. closure application
// ===========================================================================
Value applyClosureOnOwner(const Value& closure, const std::vector<Value>& args) {
    if (closure.index() != 8) return make_int(0);
    auto cl = std::get<std::shared_ptr<ClosureValue>>(closure);
    if (!cl) return make_int(0);
    std::vector<Value> all = cl->boundArgs;
    all.insert(all.end(), args.begin(), args.end());
    vmcore::VmObject* owner = cl->ownerKey.empty()
        ? nullptr
        : vmcore::Registry::instance().findByName(cl->ownerKey);
    if (owner)
        return vmcore::Scheduler::instance().execOnMain(owner, cl->functionName,
                                                        all);
    return callMasterProgramFunction(cl->functionName, all, masterSearchRoot());
}

// Public entry point (interpreter.h): call a closure value from inside an
// efun implementation (builtins.cpp), on the process that invoked the efun.
// The function resolves through the closure OWNER's CURRENT merged table —
// never a cached pointer (vm_callFunPtr re-addresses through pObject on
// every call).
Value callClosureForEfun(const Value& closure, const std::vector<Value>& args) {
    if (closure.index() != 8) return Value{};
    auto cl = std::get<std::shared_ptr<ClosureValue>>(closure);
    if (!cl) return Value{};
    vmcore::VmPcb* pcb = vmcore::currentPcb();
    return vmcore::vmInvokeClosure(pcb, cl, args);
}

// ===========================================================================
// 11. master defines / heart beat / scheduler entry
// ===========================================================================
bool masterDefineLookup(const std::string& name, std::string& expansion) {
    const vmcore::VmProgram* master = vmcore::Interp::masterProgram();
    if (!master) return false;
    // The .o define table pads names to a fixed width with trailing spaces;
    // compare trimmed.
    for (const auto& d : master->src().defines) {
        const size_t e = d.first.find_last_not_of(" \t\r\n");
        const std::string dn =
            (e == std::string::npos) ? "" : d.first.substr(0, e + 1);
        if (dn == name) {
            expansion = d.second;
            return true;
        }
    }
    return false;
}

int postHeartBeatAll() {
    static const bool hbOn = (std::getenv("LPCVM_NO_HB") == nullptr);
    if (!hbOn) return 0;
    vmcore::VmObject* master = findMasterVmObject();
    if (!master) return 0;
    if (!master->prog()->findFn("post_heart_beat")) return 0;
    // Pass the master object as `me`, exactly like the original driver's
    // vm_apply(master, "post_heart_beat", thisObject).
    vmcore::Scheduler::instance().execOnMain(
        master, "post_heart_beat", {vmcore::Registry::makeObjectValue(master)});
    return 1;
}

Value callOriginalObjectFunction(const std::string& oPath,
                                 const std::string& funName,
                                 const std::vector<Value>& args,
                                 const std::string& searchRoot) {
    (void)searchRoot;
    vmcore::VmObject* o = vmcore::Registry::instance().findByName(oPath);
    if (!o) return make_int(0);
    return vmcore::Scheduler::instance().execOnMain(o, funName, args);
}

void schedulerRun() { vmcore::Scheduler::instance().run(); }

void registerServiceSocket(int fd, std::function<bool(unsigned int fd)> cb) {
    vmcore::Scheduler::instance().addServiceSocket(fd, std::move(cb));
}

} // namespace lpc
