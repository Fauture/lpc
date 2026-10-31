// ===========================================================================
// vmcore/pcb.cpp — current process + tid pool
//
// vm_getCurrentPCB (0x80a9310): the interpreter's every stack push/pop goes
// through the CURRENT PCB.  thread_local — each service thread has its own
// current process (the original's per-PCB state is per-thread here because
// services run their LPC on several OS threads).
// ===========================================================================
#include "pcb.h"

#include <iostream>

#include "vmcore.h"

namespace lpc {
namespace vmcore {

namespace {
thread_local VmPcb* t_currentPcb = nullptr;
}

VmPcb* currentPcb() { return t_currentPcb; }

VmPcbSetter::VmPcbSetter(VmPcb* pcb) : prev_(t_currentPcb) { t_currentPcb = pcb; }
VmPcbSetter::~VmPcbSetter() { t_currentPcb = prev_; }

int64_t currentTid() {
    VmPcb* p = currentPcb();
    return p ? p->tid : -1;
}

std::string currentPreviousObject() {
    VmPcb* p = currentPcb();
    return p ? p->previousObjectName : std::string();
}

VmObject* currentObject() {
    VmPcb* p = currentPcb();
    if (!p || p->frames.empty()) return nullptr;
    // this_object() is the CURRENT FRAME's object — a cross-object call is a
    // frame push, so the innermost frame always carries the right identity.
    return p->frames.back().obj;
}

std::string VmPcb::frameChainText(size_t maxFrames) const {
    std::string out;
    size_t n = 0;
    for (auto it = frames.rbegin(); it != frames.rend() && n < maxFrames; ++it, ++n) {
        if (!out.empty()) out += " ";
        out += (it->fn ? it->fn->name : std::string("<ctx>"));
    }
    return out;
}

// ---------------------------------------------------------------------------
// TidPool — 0..1023 唯一分配, 生命期不变, 归还复用; 池空返回 -1 (调用方排队)。
// 上限必须 ≥ get_system_info["max_threads"]（filed/configd 按它创建
// ev_filed<tid/32> 事件组——tid 超组即 -101，filed 请求全灭）。
namespace {
constexpr int64_t kTidPoolSize = 1024;
}

TidPool::TidPool() {
    free_.reserve(static_cast<size_t>(kTidPoolSize));
    for (int64_t t = kTidPoolSize - 1; t >= 0; --t) free_.push_back(t);
}

int64_t TidPool::alloc() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (free_.empty()) return -1;   // 池空: 调用方把进程排队 (原版语义)
    int64_t t = free_.back();
    free_.pop_back();
    return t;
}

void TidPool::release(int64_t tid) {
    if (tid < 0 || tid >= kTidPoolSize) return;
    std::lock_guard<std::mutex> lk(mtx_);
    free_.push_back(tid);
}
// get_thread_id 必须对每个进程返回不同值 (configd read_etc 的
// "ev_readetcd"+tid/32 事件掩码依赖), 且 tid 复用不得把死链状态带到新链 —
// 新核里链状态就是 PCB 本身, 池只管编号。
// ---------------------------------------------------------------------------

} // namespace vmcore
} // namespace lpc
