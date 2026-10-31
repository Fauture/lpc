// ===========================================================================
// vmcore/call.h — 调用原语的核内声明 (interp.cpp 使用)
// ===========================================================================
#pragma once
#include <future>

#include "pcb.h"

namespace lpc {
namespace vmcore {

// (call.cpp) 弹帧; op = 72 (return_from_fake_fun) / 73 (return_from_fun)。
// 返回 Interp::RET_CONTINUE / RET_CHAIN_END (interp.h)。
int vmReturnFromFun(VmPcb* pcb, int op);

// (call.cpp) 参数已在栈上就位 (编译器压好的 op7/op5/op8 布局, param0 在顶):
// 直接压帧, bp = 栈顶。passedArgs = 实参数 (帧 argNum — check_args_num 的
// 依据), padArgs = 外部入口插入的零填充槽数 (编译器路径为 0)。
// resumeCurrentStream=true: 帧返回当前流; false: 边界帧 (链结束)。
// 返回 false 仅当栈空 (argc==0 且栈空)。
bool pushFrameInPlace(VmPcb* pcb, VmObject* obj, const VmProgram* prog,
                      const OriginalFunction* fn, int passedArgs,
                      bool resumeCurrentStream, int padArgs = 0);

// (efun_adapt.cpp) efun 分发:
//   dispatchEfun: 弹 argc 个参数 (deref 引用; ref 型 efun 保留原始项),
//     结果压栈 (void efun 不压); 阻塞 efun 内部 park (throw SuspendSignal)。
//   dispatchEfunByName: 已收集参数, 返回结果 (不压栈)。
//   dispatchEfunClosure: efun 闭包调用 (call_fun_ptr 的 efun 分支);
//     弹 argc 个参数项, 连同闭包 boundArgs 分发, 结果压栈。
void dispatchEfun(VmPcb* pcb, int efunIndex, int argc);
Value dispatchEfunByName(VmPcb* pcb, const std::string& name,
                         std::vector<Value>& args);
Value dispatchEfunClosure(VmPcb* pcb, const std::shared_ptr<ClosureValue>& cl,
                          int argc);

// (call.cpp) 广播的压栈形态 (op-8 数组调用): 启动逐元素 hook 链;
// 全部 miss 时直接把结果数组压栈, 否则最后一个元素的 hook 把数组作为
// 返回值压到操作数栈上 (跨 park 由 hook 闭包续链)。
void vmCallOtherArrayPush(VmPcb* pcb, const std::vector<Value>& targets,
                          const std::string& funName,
                          const std::vector<Value>& args);

// (call.cpp) C++ 调用方 (efun 回调/驱动 apply) 在当前 PCB 上同步跑完一个
// 被调函数: 压帧 → 嵌套循环到帧弹 → 弹出返回值。vm_apply 的原版递归模型。
Value vmApplySync(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                  const std::vector<Value>& args,
                  const struct VmProgram* progOverride = nullptr);

// (call.cpp) 原版 vm_callInUserFun/vm_callFunPtr 语义的压帧原语：压参 +
// 压帧（返回当前流），调用方立即返回（配 efunTakeover）——阻塞 efun 在
// 被调帧里 park 时 C++ 无续延可失。
bool vmPushFunCall(VmPcb* pcb, VmObject* obj, const OriginalFunction* fn,
                   const std::vector<Value>& args,
                   const struct VmProgram* progOverride = nullptr);

// (efun_adapt.cpp) 全局 init 段驱动: fn==nullptr 的上下文帧 + run + 弹帧
// (return_from_fake_fun 结束段, 不弹上下文帧 — 本函数弹)。
void vmRunGlobalInit(VmPcb* pcb, VmObject* obj);

// ---------------------------------------------------------------------------
// 构造计划 (load_object / clone_object / master boot) —
// 原版 vm_f_construct_object2 的 initOrderList 语义:
//   1. 本程序全局初始化段 (yieldable, 帧模型下天然可 park/resume)
//   2. 继承链初始化段: 递归收集 (父先子后), 每段带累计 baseVarOffset 与
//      该程序自己的字符串表
//   3. create 链: 沿继承链 base-first 收集每个程序的 create (合并表键
//      funOff+index 解析; 同程序去重; 本程序自身 create 直接追加),
//      createArgs 只传给声明了参数的 create
// 完成后: 注册表标记 initialized, 唤醒 "obj-init:<key>" 上挂起的加载方。
// ---------------------------------------------------------------------------
struct ConstructStep {
    enum class Kind { Init, Create } kind = Kind::Init;
    // Init: the segment to run under a context frame (fn == nullptr).
    const std::vector<OriginalInsn>* code = nullptr;
    const std::vector<uint8_t>* meta = nullptr;
    int varOffset = 0;
    const std::vector<std::string>* strings = nullptr;
    // Create: the function to call as a normal frame.
    const OriginalFunction* fn = nullptr;
    std::vector<Value> args;
    // Init 段的伪函数 (原版没有——原版 init 段跑在调用进程执行流上, 步进由
    // hookCallbackWhenReturn 推进): 把段包成普通帧, 与 create 同一机制
    // (park/resume/retHook 全走通用帧模型)。plan 持有保活。
    std::shared_ptr<OriginalFunction> fakeFn;
};

struct ConstructPlan {
    VmObject* obj = nullptr;             // the object under construction
    std::vector<ConstructStep> steps;
    size_t idx = 0;
    // True while a step's run is in flight (a park escaped with the step
    // unfinished).  When the resume's vmResume RETURNS, the in-flight step
    // has completed — vmDriveConstruction closes it and continues.
    bool inFlight = false;
    std::string doneEvent;               // "obj-init:<key>"
    bool finished = false;
    // Set when a SYNCHRONOUS driver-side load (loadObjectIntoRegistry) waits
    // on this construction: the scheduler fulfils it at plan completion.
    std::shared_ptr<std::promise<Value>> resultPromise;
    // LPC-level construction failure (an uncaught error in the create chain):
    // the waiters answer 0 instead of a phantom object.
    bool failed = false;
};

// Prebuild the full plan for a program's construction (the C++ walk of the
// inherit chain — no LPC runs during the walk).
std::shared_ptr<ConstructPlan> buildConstructPlan(VmObject* obj,
                                                  std::vector<Value> createArgs);

// ---------------------------------------------------------------------------
// 原版 vm_f_construct_object2 (0x80f90b4) 的 inline 构造:
//   * 每一步 (init 段伪函数 / create) 压一个普通帧 (resumeCurrentStream=true,
//     保存调用者流 — 即 vm_callInUserFunBody 当前栈直调);
//   * 步返回时 retHook 推进下一步 (vm_hookCallbackWhenReturn 的 ok1/ok2);
//   * 全部完成: 注册表标记 + 唤醒 doneEvent + pop 计划栈, 对象值经 rv
//     成为调用者 efun 的返回值;
//   * 步里的阻塞 efun park 当前进程 (同 tid), resume 后 hook 链自动续 —
//     runProcess 无需任何构造特殊处理。嵌套构造 = constructStack。
// ---------------------------------------------------------------------------
void vmStartInlineConstruction(VmPcb* pcb,
                               const std::shared_ptr<ConstructPlan>& plan);
// 驱动层同步版 (scratch PCB, 调度线程 boot): 同一 hook 机制, Interp::run 跑
// 到链尾; park 逃出按失败处理 (与旧同步装载一致)。
Value vmRunConstructionSync(VmPcb* scratch,
                            const std::shared_ptr<ConstructPlan>& plan);

} // namespace vmcore
} // namespace lpc
