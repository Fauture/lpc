// ===========================================================================
// vmcore/interp.h — 解释器入口 + 栈操作 (全局栈 + 当前 PCB, 原版模型)
//
//   * run(pcb): 跑到边界帧弹出 (整条链结束 / 全局 init 段结束)。
//   * runNested(pcb, depth): 嵌套执行 — 跑到帧数回到 depth (vm_apply 从
//     efun 内调用 LPC 的原版递归模型: 同一栈同一 PCB, 多一层 funContext)。
//   * 栈操作全部经当前 PCB — 对应原版 vm_push*OfPCB / vm_getPCB*Argument。
//   * 阻塞 efun 抛 SuspendSignal: 状态原封留在 PCB 里, 调度器挂起,
//     唤醒后 vmResume 从保存的 ip 续跑 — vm_pendCurrentProcess 模型。
// ===========================================================================
#pragma once
#include <unordered_map>
#include "pcb.h"

namespace lpc {
namespace vmcore {
namespace Interp {

// Return codes of vmReturnFromFun (call.cpp).
enum {
    RET_CONTINUE = 0,   // caller stream restored; keep running the loop
    RET_CHAIN_END = 1   // boundary/context frame finished; loop returns
};

// ---- stack via PCB ---------------------------------------------------------
inline StackItem makeItem(Value v) {
    StackItem it;
    it.value = std::move(v);
    return it;
}
inline void pushItem(VmPcb* pcb, StackItem&& it) {
    pcb->stack.push_back(std::move(it));
}
inline void pushValue(VmPcb* pcb, Value v) {
    StackItem it;
    it.value = std::move(v);
    pcb->stack.push_back(std::move(it));
}
inline StackItem popItem(VmPcb* pcb) {
    StackItem it = std::move(pcb->stack.back());
    pcb->stack.pop_back();
    return it;
}
inline Value popValue(VmPcb* pcb) {
    return popItem(pcb).value;
}

// The shared "global variable" space (the driver's global program).
std::vector<Value>& sharedGlobals();
Value& sharedGlobal(int index);

// Master (global) program hooks — set once at master load, used by op7-bit7
// (global function table, operand verbatim), op23-bit7 and closure fallback.
void setMasterProgram(const VmProgram* prog, VmObject* obj);
const VmProgram* masterProgram();
VmObject* masterObject();
const OriginalFunction* findMasterFunction(int index);

// Reference (vptr) resolution: a ref item reads/writes its live slot.
// depth caps ref-to-ref chains (self-cycles bail out at 32).
Value derefItem(VmPcb* pcb, const StackItem& it, int depth = 0);
// Store into a ref item (assign through an lvalue); returns false when the
// item is not a writable ref.
bool storeItem(VmPcb* pcb, const StackItem& it, const Value& v);

// ---- loops -----------------------------------------------------------------
void run(VmPcb* pcb);
void runNested(VmPcb* pcb, size_t depth);

} // namespace Interp
} // namespace vmcore
} // namespace lpc
