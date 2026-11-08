# 原版 VM 核心处理（IDA 实证，magic_Linux32x）

> 来源：`magic_Linux32x.i64`（imagebase 0x8048000），全部经 Hex-Rays 反编译核对。
> 原版源码路径证据：`/home/cc/xxxx/gameshell/src/vm/vm_shell.c`、`vm_fun.c`、`vm_loop.c`。
> 本文覆盖：主循环链、调度器、时间片、控制标志、进程生命周期、timer/invoke 机制。
> 操作码逐条语义见 `docs/操作码语义参考.md`；efun 采集暂未做。

## 1. 核心处理链总览

```
OS 线程
 └─ vm_mainLoop (0x80adea8)                 调度器心脏, while(!vm_requestStop)
     ├─ vm_getVmMutex()                     全局 VM 锁（每轮重取）
     ├─ vm_scheduleProcessInternal(0x80a8a54) 摘进程 → 执行 → 处理返回码
     ├─ vm_scanSocketInMainLoop (0x80afb9c)  ★ 每轮泵 socket I/O
     ├─ vm_scanTelnet                        ★ 每轮泵 telnet
     ├─ 四类 tick 统计: scheduleProcessTicks / ScanSocketTicks /
     │   ScanTelnetTicks / IdleTicks (runtimeState 1/2/3/0)
     ├─ vm_forceVMRelinquish>0 → sleep 该值(上限10)个 tick
     └─ 无活动进程且无 invoke → 按 min(timer超时, relinquishTick) 等事件
```

关键结论：**原版主循环每跑完一轮进程就在同一持锁窗口内泵 socket/telnet I/O**——
这就是原版客户端点击即时响应（一次点击必中"停止"按钮）的机制根源。

## 2. vm_scheduleProcessInternal（0x80a8a54，调度核心）

每轮做的事：

1. `vm_processAllRespondingInQueue()`；无活动进程/timer/invoke 直接返回。
2. `v2 = now - vm_lastTick`（本轮逝去 tick），用于 timer 扣减。
3. **时间片 quota**：
   - `runTicks = 10`（活动进程 ≤9 时）
   - 进程数 >9 时 `runTicks = clamp(100/进程数, 2, …)`
   - 循环执行直到 `now - 本轮开始 >= 20 * runTicks`（即 ≤10 进程时每轮调度最多
     连续执行 200 tick，进程越多单进程配额越小，下限 40 tick）。
4. **上下文装载**（进进程前）：PCB 的 `funCp/funContext/funContextSize/ctrlFlags/
   inFd/outFd/rpcClient/sp/stack/stackSize` 装入全局 VM 寄存器
   （`vm_funCp/vm_sp/vm_stack/...`）；`vm_updateCurrentFunCallContext()`。
5. **resumeHook**：PCB->resumeHook 非空则先调一次（返回 0 继续跑，非 0 当返回码）。
6. 执行体：`vm_enterThreadTick = now; vm_doActions(pcb)`；
   结束后 `holdTicks += 耗时; totalTicks += 耗时`。
7. **上下文卸载**（切出/终止时）：`ctrlFlags/sp/funCp` 存回 PCB，`vm_stack=0`。

### 返回码语义（v18，调度器据此处置进程）

| 返回码 | 含义 | 调度器处置 |
|---|---|---|
| 0 | 正常让出（时间片跑完/自愿） | holdTicks=0，`vm_pActiveProcess = pNext`（环形队列轮转） |
| -8101 | 进程自杀/finish | 摘环，`returnCode=-1`，vm_deleteProcess |
| -8102 | 内部终止/执行中止 | 摘环，`returnCode=-4`，vm_deleteProcess |
| -8103 | suspend | 摘环，`flags\|=2`（挂起，恢复走 suspendLevel 递减） |
| -8104 | pend（等事件/对象） | 摘环，`flags\|=4`；有 pPendOnObject 则挂到对象等待链 |
| -8105 | VM 清环境 | vm_clearEnvironment，返回 0 |
| -8106 | VM 关机 | `vm_alreadyShutdown=1`，clearEnvironment，返回 -99 |
| -8107 | 重启处理器（processor 变更） | vm_doActions 内自旋重跑，不出调度层 |
| 84 | ctrlFlags&0x80 | （调试/线程特殊路径） |

## 3. vm_doActions（执行包装，vm_fun.c）

```
totalTicks = 10000            默认 cost 上限
flags&0x200000(callback)      → totalTicks = 120000
vm_startTimer(0)
loop:
  vm_newCatch(...) + _setjmp  异常恢复点；longjmp -3 → unwind catch 链后重跑
  flags&0x40                  → vm_returnFromEfun()（efun 挂起恢复）
  holdTicks > 0x927C0(600000) → "Non-delay routine cost too much time" 强杀
  ctrlFlags 各位分发（同下表）
  -8107                       → while{ 取指合法则 vm_processor(pcb) } 自旋
  callback 超时               → vm_error("Non-delay routine (callback) cost too much time")
```

## 4. vm_regularProcessor（0x80978c4，取指分派主循环）

```c
v1 = 2048; vm_switchOutThread = 0;
do {
    if (vm_ctrlFlags) { …各位消费 → actionState(-8103/-8104/-8102/-8107/-8101/84)，非 0 break; }
    if (--v1 <= 0) { v1 = 1024; if (vm_isTimerTimeout(0)) break; }   // 时间片检查
    if (pcb->nextActionHook) { hook(pcb->pNextActionCookie); 清零; }  // 每条指令前的注入钩子
    else {
        node = &cs->nodes[ip];  ip++;                                  // 取指
        ai = &actionInfoTable[node->actionNo];
        if (ai->attrib & 0x80000000) {                                 // metaData 寻址模式
            off = node->par16 | (node->par8 << 16);
            if (ai->parameters == 2) entry(md[off], md[off+4]);
            else                     entry(md[off], par8);
        } else {                                                       // 直接数模式
            parameters==0 → entry();
            parameters==1 → entry(par16);
            parameters==2 → entry(par16, par8);
        }
    }
} while (!vm_switchOutThread);   // handler 内置位 → 切出
return actionState;
```

要点：
- **时间片**：每 2048 条指令查一次 `vm_isTimerTimeout`，触发后降频为每 1024 条一查。
- **参数寻址**：attrib bit31 决定参数来自 metaData 段（par16|par8<<16 为字节偏移）
  还是节点内 immediate（par16/par8）。op.txt 的 257/258 = 0x101/0x102
  （metaData 位|1/2 参数）。
- **vm_switchOutThread** 由 handler（如 efun/切片路径）置位，主循环随即返回。
- `vm_ctrlFlags` 位表：0x02=suspend、0x04=pend、0x08=终止、0x10=efun 返回后继续、
  0x20=processor 变更（-8107）、0x40=finish（-8101）、0x80=特殊（84）。

### 分发表

- `actionInfoTable`：全局表，`vm_getActionInfo(0x80a369c)` 做边界检查 `op <= 0x56`。
- `vm_initActions (0x8095df0)`：扫表至哨兵 `actionNo == 0xF0`；
  按 `vm_confEnableJit` 选 `vm_jitProcessor`（"jit"）或 `vm_regularProcessor`（"regular"）
  写入全局 `vm_processor`；切换时 `vm_ctrlFlags |= 0x20`（-8107 让 doActions 自旋换轨）。
- `vm_setProcessor (0x8095da0)`：同上，允许外部强制指定（传 0 用配置默认）。

## 5. vm_run（0x8098b20，脚本入口）

1. `vm_runningInScheduler()` 检查（主循环内跑脚本 → VM_RUNNING_IN_MAIN_LOOP）。
2. `vm_getVmMutex` → `vm_buildScratch` → 装载：`vm_loadBinary`（二进制 .o）
   或 `vm_yyparse`（源码编译）→ `vm_genXAction/genXAction(0x1D finish)` →
   `vm_optimize` → `vm_transformXActions` 产出 `Vm_Code_Segment_T`。
3. 优化后若节点全为 op46(null_action)/op29(finish_action) → 空程序直接返回。
4. `pdb_allocateMemory(12)` 分配 PCB attribs → `vm_spawnProcess` →
   `vm_setProcessAttrib` → `vm_setThreadPriority(-1)` → `vm_resumeProcess` →
   `vm_bringupMainLoop`（唤醒调度事件）；endHook 为空则 `vm_waitProcessEnd` 同步等待。

## 6. 进程（PCB）模型

- 全部 PCB 在 `vm_allPCB[]` 数组（`vm_maxProcess` 上限），活动进程组成
  **环形双向链表** `vm_pActiveProcess`（pPrev/pNext 互指成环），
  `vm_currentActiveProcess` 计数。
- PCB 关键字段：`flags/sp/funCp/funContext/stack/stackSize/ctrlFlags/inFd/outFd/
  rpcClient/holdTicks/totalTicks/suspendLevel/pPendOnObject/resumeHook/returnCode/
  catchContext/catchCp/nextActionHook`。
- flags 位：0x01|0x02=挂起态、0x04=已 pend、0x40、0x80=在 timer 链、
  0x100=主进程标记（scheduleProcessInternal 只对它跑 timer 扣减）、
  0x100000=不可永久 pend 守卫、0x200000=callback 型（不可 pend/受 120K 限）、
  0x400000。
- 生命周期函数：
  - `vm_startProcess (0x80aa7c4)`：插环尾，holdTicks=0。
  - `vm_resumeProcess (0x80aac10)`：suspendLevel>0 递减；否则清挂起位插环尾。
  - `vm_pendCurrentProcess (0x80a9784)`：置 `vm_ctrlFlags|=4`（-8104）。
  - `vm_bringProcessToNext (0x80a97ec)`：把某 PCB 挪到当前活跃位之后（优先调度）。
  - 摘环统一动作：`pPrev->pNext=pNext; pNext->pPrev=pPrev; --vm_currentActiveProcess`。

## 7. 切片与响应模型（结论）

原版"不卡"由三层保证：

1. **指令级切片**：regularProcessor 每 2048 条查 timer 超时，切出经
   `vm_switchOutThread`/ctrlFlags 返回调度层。
2. **每轮调度泵 I/O**：vm_mainLoop 每轮（不管有没有进程可跑）都 scanSocket+scanTelnet，
   新命令在任意轮次间隙进入。
3. **进程间配额**：单进程每轮最多 20×runTicks（≤10 进程时 200 tick ≈ 数毫秒级），
   跑完即轮转到下一进程。

我们重实现（src/vmcore/sched.cpp）的对齐情况：用户 pcb yieldEvery=20000 对应第 1 层
（原版更细）；主循环回泵 I/O 对应第 2 层；第 3 层的按进程数缩放配额未实现。

## 8. 值类型与栈约定（各 handler 反编译归纳）

| type | 含义 |
|---|---|
| 0 | undefined（number=0） |
| 1 | void |
| 2 | int |
| 3 | real |
| 4 | string |
| 5 | buffer |
| 6 | array |
| 7 | mapping |
| 8 | ptr/reference（指向 Value，attrib&1=const） |
| 9 | function pointer（Vm_Fun_Ptr_T） |
| 10(0xA) | object（带 0x8000 destructed / 0x2000 / 0x4000 附加位） |
| 20 | loop 控制块（36B，initForLoop 分配，见下） |

栈约定：`vm_sp` 向**下**生长，push = `--vm_sp` 后写 `vm_stack[vm_sp]`；
二元运算右操作数在 `stack[sp]`、左在 `stack[sp+1]`，结果写回左槽后 pop 掉右槽。
`vm_sp==0` 即栈底，越界统一报 "Stack overflow when calculating."。

loop 控制块（36B）字段（DWORD 序）：`[0]`模式(1=数字步进/-1 反向/901=mapping 键/
902=数组 foreach)、`[1]`循环变量槽指针、`[2]`步长、`[3]`结束标志、`[4]`attrib、
`[5]`当前值/键数组、`[6]`终止值/键下标、`[7]`元素下标、`[8]`计数。
foreach 数组（902）给循环变量写 3 元数组 `{元素, 键, 下标}`。
