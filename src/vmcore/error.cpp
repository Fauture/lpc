// ===========================================================================
// vmcore/error.cpp — 错误通道 (vm_error 0x8097b22 → vm_errorQuit → vm_traceError)
//
// 原版铁律: 一切失败都留痕。被 catch 的错误也打印, 然后按 catch 语义继续;
// 引擎里不存在静默吞错。新核的统一出口: 打印 (对象/函数/帧链/PCB tid 全量
// 上下文), 然后抛 VmError; 解释器的 catch 链恢复, 无人捕获则终止当前链
// (调度器记 [chain-abort] 留痕 — 不是吞掉)。
// ===========================================================================
#include <iostream>
#include <stdexcept>

#include "pcb.h"
#include "vmcore.h"

namespace lpc {
namespace vmcore {

namespace {
struct VmErrorSig : std::runtime_error {
    explicit VmErrorSig(const std::string& m) : std::runtime_error(m) {}
};
thread_local VmErrorSig* t_pendingError = nullptr;
} // namespace

[[noreturn]] void vmError(const std::string& what) {
    VmPcb* pcb = currentPcb();
    // 原版 vm_error 的文案自带结尾 '\n'（vm_stdprintf 语义）。若原样拼接，
    // 上下文 " @ obj::fn tid=.. frames=[..]" 会被这个换行顶到下一行，日志里
    // 只剩半条错误串、定位不到现场 —— 先剥掉结尾换行再拼。
    std::string msg = what;
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
        msg.pop_back();
    // ---- 留痕: 谁、在哪、什么错、帧链 ----
    {
        std::string obj = "<driver>";
        std::string fn = "<global>";
        std::string chain;
        if (pcb) {
            if (!pcb->frames.empty()) {
                const VmFrame& f = pcb->frames.back();
                fn = f.fn ? f.fn->name : "<ctx>";
                obj = f.obj ? f.obj->key() : (f.prog ? f.prog->name() : "<driver>");
            }
            chain = " tid=" + std::to_string(pcb->tid);
            chain += " frames=[" + pcb->frameChainText(8) + "]";
        }
        std::cerr << "[vm-error] " << msg << " @ " << obj << "::" << fn << chain << "\n";
        std::cerr.flush();
    }
    throw VmErrorSig(what);
}

} // namespace vmcore
} // namespace lpc
