# 调度器重构：现状、卡点与方案（分支 refactor/ldmud-vm-model）

## 1. 为什么调度和解释器改不动

`src/original_vm.cpp` 19.5k 行，里面同时住着：

| 角色 | 现在的载体 | 问题 |
|---|---|---|
| 解释器执行态 | `OriginalVm::frames_ / stack_ / code_ / meta_ / ip_ / catches_ / returnValue_` | **一个 VM 对象同时承载多条链**，所以每次进入都要 `switchToChain()` 把上一家的状态 stash 出去、把这一家的搬进来 |
| 每条链的转存 | `OriginalVm::runStates_`（按 `chainSeq` 键）+ `activeKey_` | 就是上面"搬来搬去"的存放处；搬错一次就出现"丢尾段 / 没角色列表 / 没关联账号"这类事故 |
| 进程控制块 | `Scheduler::Process`（155 行，嵌在同一个文件里） | 持有 `shared_ptr<OriginalVm>`，而 `OriginalVm` 是**该文件匿名命名空间内的类型** → 跨单元无法引用，调度器只能留在 19.5k 行文件里 |
| 调度器 | `Scheduler`（同文件匿名命名空间） | `runProcess/parkCurrent/resumeProcess` 直接读写上面的执行态成员 |

由此派生出的所有"冲突 / 数据无法共享"症状：

1. `switchToChain()` 在 8 处被调用（`runInit`、`executeFunctionFromOutside`、`executeFunctionOnStack`、`resumeProcess`、`resumeCaller`、`resumeRunLoop`、`runProcess`…），每一处都必须"记得"搬状态；漏一处就丢帧。
2. 网络线程要碰 VM ⇒ 只能靠 `execMutex_`（16 处）、`execDepth_`、`runDetached`（32 处）、`objVarsSnapshot` 把关；这是"多线程 + 共享可变执行态"的必然代价。
3. 挂起（`SuspendSignal`）后，链的续跑可能落在**另一条链**上（`/gs/clone/char/user.o do_operate create_content save`），于是连接级门只数"已归属的链"就漏掉它 → 这就是 9101 的真正来源。

公开驱动没有这些问题：LDMud 的 `struct pcb_s` **自己拥有** stack/frame/catch 上下文，调度器只管 PCB 池和就绪队列，VM 的"当前上下文"就是正在跑的那个 PCB。**没有可共享的执行态，就没有需要搬的状态，也就没有冲突。**

## 2. 已完成（本轮，已验证）

* `LPCVM_TEMPKEY` 改成可用 `_probe_on\LPCVM_TEMPKEY` **运行时开关**（原来只能靠启动 env，而一次重启要 20 分钟）。抓到的证据：
  `[tempkey] IN /gs/clone/char/login.o::lite_set_temp key="operating"`（每次连击 15 次写、30 次读）——
  **重入标记确实是登录 clone 上的 `lite_set_temp("operating", …)`**。
* 探针 + 连接级命令门（上一批提交）：`ev=OVERLAP` 恒为 0、门零卡顿、`basic_char_info` 关联行写出、客户端进游戏。

## 3. 已试过并证伪（别再重复）

| 做法 | 结果 |
|---|---|
| 读对象 temp 映射 `liteTempTruthy("operating")` | 登录 clone 的 18 个 objVar 里只有 2 个映射（`register_time create_time` / 空），**没有 operating** → 存储不在这里 |
| 把命令 VM 注册给连接（`registerClientScriptVm(dcno, runVmRaw)`） | **回归**：客户端停在 `login(5)`、`0xF061 body=4`。原因是"最近一次派发的 VM"经常不是登录 clone（登录链跑在别的对象/daemon 上），读到脏状态后门把登录包也拦了 20s |
| `shared_ptr<void>` 把 PCB 挪出文件 | 需要上百处 `static_cast`（`p->vm->frames_`、`p->timerEntry->cancelled`…），不划算 |
| 按 VM 归属续跑链 / 150ms settle 窗口 | 前者拦死登录；后者饿死 threadd 队列等待者（启动到不了 listening） |

## 4. 真正的重构方案（按依赖顺序，每步独立可验证）

**Step A：PCB 长出执行态（核心，消除"搬状态"）**
把 `OriginalVm` 的执行态成员整体搬进一个 `ExecState`，PCB 持有它：

```cpp
struct ExecState {                  // 原 OriginalVm 的执行态成员
    std::vector<Frame> frames;
    std::vector<StackEntry> stack, detachedStack;
    const std::vector<OriginalInsn>* code; const std::vector<uint8_t>* meta;
    size_t ip; ResumeCond cond; bool parked;
    std::vector<CatchRecord> catches; int catchDepth;
    Value returnValue; bool returned;
};
struct Process { ...; std::unique_ptr<ExecState> es; };   // 每条链一份，独占
```
VM 侧只留 `ExecState* es_`（指向当前运行链的那份）。**`switchToChain()` 与整个 `runStates_` 转存机制随之删除**——没有共享就没有搬移。
机械替换量：`frames_`→`es_->frames` 等约 700–900 处，纯文本可替换，编译器逐处校验。

**Step B：调度器独立成模块**
`ExecState`/`Process`/`ResumeCond` 进 `src/vm_pcb.h`（不再持有 VM 指针，改为持有 `ExecState`），调度器进 `src/vm_sched.{h,cpp}`；CMakeLists 与 `build/lpcvm.vcxproj` 同步（`cmake -S . -B build` 重新生成）。

**Step C：删掉多线程残留**
`execMutex_` / `execDepth_` / `runDetached` / `objVarsSnapshot` 全部消失；网络线程只通过 `spawn`/`execOnMain` 排队。

**Step D：连接级命令语义落到 PCB**
`Process::ownerCno` 由 PCB 自己带（执行态归属明确后这是天然字段），门的判据变成"该连接的 PCB 是否都结束"——不再需要 VM 注册、不再有 9101。

## 5. 每步验收（缺一不可）

```
# 起服（清库；约 15-20 分钟到 listening，20 分钟不到即视为失败）
python -X utf8 _run_probe.py <dir> 86400 "LPCVM_SEND=1;LPCVM_TAIL=1;LPCVM_CHAINTRACE=1"
python -X utf8 _rapid2.py 100 <name> 20     # 真机 20 连击建号
python -X utf8 _acc.py <dir> <name>         # 四项：弹窗 / 0xF061 body / gid_info / basic_char_info
```
通过线：`ev=OVERLAP` 为 0、`[chain-gate]` 为 0、`basic_char_info` 有行、客户端 `screen after: ingame`。

## 6. 铁律（本轮踩过的坑）

* 任何改动只要**启动到不了 listening** 或**登录失败**，立刻整体回退（本轮 VM 注册、settle 窗口都是这样被否掉的）。
* 不要用逐段 Python 字符串替换去"修"已损坏的文件：一旦锚点错位就把类头切碎（本轮发生过一次），直接 `git checkout HEAD -- src/original_vm.cpp` 重来。
* 改源码必须保持 CRLF（用 `_patch_src.py`），否则 MSVC 吞花括号、报假错。
