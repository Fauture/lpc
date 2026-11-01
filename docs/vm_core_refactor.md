# VM 核心重构文档（refactor/vm-core，v2 — 彻底重构版）

> 依据：原版 `magic_Linux32x` IDA 实证 + 当前源码逐结构核实。本文档是重构的唯一执行依据。
> v1 作废：v1 的阶段 A~E 是在现有 OriginalVm/Scheduler 上加字段、改指针、逐站替换并保留旧行为——
> 那是修补，不是重构。本版改为：**按原版模型新建 VM 核心（新代码、新结构），旧执行引擎整体退役**。

## 一、重构立场（与 v1 的根本区别）

1. **不是改造 original_vm.cpp，是新建执行核心并替换它。**
   `original_vm.cpp`（2.46 万行）中的执行引擎——OriginalVm 的 per-object ExecState、
   runDetached、switchToChain、runStates_、chainSeq、链盗用、savedObjVars/funcLookupSwitched/
   thisNameOverride、clone 共享模板——全部是"每对象一个 VM + 跨对象跳转"错误模型的衍生物。
   该模型下修一个边界就要补一层 switch/restore（病根 #6 已实锤：补丁堆积）。**衍生物不迁移，直接消亡。**
2. **保留的资产只有两类**：
   - 已实证正确的知识：.o 格式解析（o_loader）、opcode 语义（两年逐 opcode 对齐的成果）、
     efun 底座（xdbc/mysql/socket/file，与 VM 内部无关）、驱动外围（game_service、
     pkt_protocol、四服务框架）。
   - 对外 API 面：`original_vm.h` 的 public 函数签名（驱动侧 126 个调用点依赖的兼容面）。
3. **新核语义对齐原版，查证以 IDA 为准**：本文所有地址可复查；语义不明必须查 IDA
   （`docs/ida_mcp_使用.md`），禁止猜测、禁止"能用就行"。

## 二、原版架构（IDA 实证，地址可复查）——新核的唯一蓝本

### 1. 执行状态 = 一个全局栈 + 一个 PCB
- 解释器全局只有一套执行栈：`vm_stack`（0x8661964）/ `vm_sp`（0x86619cc）。
- PCB 是进程的唯一状态载体：`vm_getCurrentPCB`（0x80a9310）、`vm_getPCB`（0x80a93d4）；
  栈值压取全部经 PCB（`vm_push*OfPCB` / `vm_getPCB*Argument` 系列）；
  PCB 携带 tid（`vm_f_get_thread_id` 0x80ed79c 返回 PCB->tid）、euid（`vm_getPCBEuid`）、
  错误状态（`vm_setPCBLastError`）。
- **没有任何"第二执行状态"**：阻塞 = PCB 挂起（记录唤醒条件），唤醒 = 同一栈同一帧链继续。

### 2. 对象值就是指针
- 对象值 type=10 直接持有 `Vm_Object_T*`（`v.info`）：`vm_callInOtherFun`（0x8094cd8）
  里目标对象**零查找**——`info = v3->v.info`；只有字符串名（type=4）才走
  `vm_lookupObjectByName`（0x80b114c），且仅限加载器/名字入口。
- 析构即对象自身 flag（`*((char*)info+13) < 0` → `vm_error("Object is not found (destructed).")`）。
- 身份比较即指针比较（`vm_compareValue` 0x80bbcb4 case 6..0xA）。

### 3. call_other = 压帧，不是跳转
`vm_callInOtherFun` → `vm_callInUserFunBody(0, funOffset, info, ...)`（0x8093eb0）：
- 在**当前栈**上压一个绑定 (目标对象, 目标程序, funOffset) 的 funContext；
- 被调方 `vm_returnFromFun` 弹帧回到调用点；需要后处理时用
  `vm_hookCallbackWhenReturn`（0x8094e70，见 `vm_callInOtherFunEx` 0x80bd0fc）。
- 全程 `vm_sp`/`vm_stackTopValue` 操作同一栈——**没有第二 ExecState、没有跳转、没有桥接**。

### 4. 错误必须留痕
- 一切失败走 `vm_error`（0x8097b22）→ `vm_errorQuit` → `vm_traceError`：
  被 catch 的错误也打印，然后按 catch 语义继续。**引擎里不存在静默吞错**。

### 5. 调用家族（全部同一模型，全部收敛到压帧）
| 原版函数 | 地址 | 用途 |
|---|---|---|
| vm_callInUserFun / Body | 0x8094080 / 0x8093eb0 | 本对象/继承调用 |
| vm_callFunPtr | 0x809441c | 函数指针（闭包）调用 |
| vm_callInOtherFun(Ex) | 0x8094cd8 / 0x80bd0fc | call_other |
| vm_callInOtherArray2/3 | 0x80bcf14 / 0x80bcdec | call_other 广播（return hook 收集结果） |
| vm_callOSFun / callInEfun / callInInitFun / callInFakeFun | 0x8093420 / 0x8093d98 / 0x8094258 / 0x8094178 | 特殊入口 |

## 三、当前实现的病根（全部已实证，新核中必须结构性消解）

| # | 病根 | 位置（已核实） | 已实锤后果 |
|---|---|---|---|
| 1 | 每对象一个 OriginalVm，各有 ExecState（stack/frames/ip，`es_` 指针视图 + `runStates_` 链表 + `switchToChain`，original_vm.cpp:11287-11371） | original_vm.cpp | 跨对象调用=跳转到另一 VM，身份需重解析 |
| 2 | 跨对象调用 = `target->callFunctionByName` + `runDetached` 跳栈 + `noteParkedCaller`/`callerVm`/`callers_` 链盗用桥接（original_vm.cpp:9836-9948, 1822-1932） | 126 个调用点 | 战斗卡死（init 链死亡）、交易蒸发、丢物品刷新异常 |
| 3 | 对象值 = 字符串键（`ObjectValue::programName`，value.h:322）；克隆工厂靠四轮 suffix 扫描 + tplKeyCache 挑模板（original_vm.cpp:22096-22189） | clone 工厂 | 解析到错误实例（按键序决定），NPC 菜单丢失（Fangzhang-Island boatman） |
| 4 | Process.tid 手工补丁三处；`chainSeq`/`tlsChainTid`/`tlsChainKey`/`execOwnerTid_` 并存 | Process 结构（1822-1932）、ChainTidScope 等 | xlibd args_stack 孤儿、事件掩码互踩 |
| 5 | LPC catch / 调度器 catch 静默吞错（已补 [lpc-catch]/[sched-catch] 留痕，f109700） | runLoop / Scheduler | 战斗死因不可见 |
| 6 | 上下文切换靠 save/restore：`savedObjVars`/`savedFuncLookup`/`funcLookupSwitched`/`thisNameOverride`/`autoVars` 五套机制叠加（Frame 1155-1238、startInlineFunction 10334-10589） | startInlineFunction | 补丁堆积，任何新边界都要重新踩一遍 |

## 四、目标架构：新建 `src/vmcore/` 执行核心

新核从零构建，结构命名对齐原版概念。**不改动 .o 字节码格式与 opcode 编号**——opcode 语义按
已验证的实现逐个搬运，但状态来源全部改为"PCB + 当前帧"，不存在任何 VM 成员切换。

### 核心结构（新文件 src/vmcore/）

1. **`Vm_Process`（PCB）**（vmcore/pcb.h）——执行状态的唯一载体：
   - 栈区间（全局 `vm_stack` 上的 base/sp，或独立栈段——以 IDA 确认原版阻塞后的栈恢复方式为准）；
   - 帧链、ip/code/meta、catch 链、resumeValue；
   - tid、euid、lastError、previousObject、事件等待条件（kind/key/mask/wakeAtMs）；
   - park = PCB 挂起并登记唤醒条件；wake = 回 ready 队列；resume = 恢复 sp/ip 继续跑。
2. **`Vm_FunContext`（帧）**（vmcore/frame.h）——上下文住进帧：
   - 绑定 (实例 `Vm_Object*`，只读程序 `Vm_Program*`，funOffset/varOffset)；
   - bp/argNum/localCount/strings/retCode/retMeta/retIp/loops 等现有 Frame 已验证的布局字段；
   - **解释器的 this/变量表/函数表永远从当前帧取**。帧空 = 驱动兜底上下文。
   - 不存在 savedObjVars/funcLookupSwitched/thisNameOverride——帧切换即上下文切换，无需恢复。
3. **`Vm_Object`（实例）+ `Vm_Program`（只读程序）**（vmcore/object.h）：
   - `Vm_Program`：复用 o_loader 产出的程序数据（functions/strings/progVars/constitution
     基偏移/inherit 链），加载后不可变，模板与全部克隆共享。
   - `Vm_Object`：实例身份。持有独立实例变量数组 + 指向共享 `Vm_Program` + 析构 flag。
     克隆 = new Vm_Object + 新变量数组，零拷贝零合并。
   - 对象值 type=10 直接存 `Vm_Object*`；析构后引用按原版 flag 语义失效
     （现有 objectValueAlive/objectValueFreed 语义保留，判定源换成实例 flag）。
4. **调用原语**（vmcore/call.cpp）——唯一调用路径：
   - `vmCallInUserFunBody(pcb, obj, prog, funOffset, args)`：压帧（vm_callInUserFunBody 0x8093eb0 同构）；
   - `vmReturnFromFun(pcb)`：弹帧，恢复调用者流；
   - `vmHookCallbackWhenReturn(pcb, hook)`：返回钩子（替代一切 runDetached 后处理；
     数组广播用 hook 收集结果，ret 压值与 results 包装的冲突在 hook 内消解）；
   - 本地调用 / 继承调用（::fn 按 constitution funOffset 压帧）/ call_other / 闭包 /
     广播 / call_in_other_fun_ex ——全部收敛到这三个原语。**新核里 op-8 handler 只有一处实现**，
     "126 个调用点"在新核中不存在：驱动侧走 API 面，VM 内部走原语。
5. **调度器**（vmcore/sched.cpp）：PCB 表 + tid 池（0..127 唯一分配、生命期不变、结束归还，
   耗尽后 ≥128 单调兜底——语义按 IDA 复核 vm_getPCB 的分配回收）；park/wake/定时器/队列/
   事件掩码全部挂 PCB。不存在 parkedVm/callerVm/callers_/runStates_/chainSeq/
   tlsChainTid——**跨对象阻塞调用下 park 的是当前 PCB，被调函数帧就在它的栈上**，
   唤醒后弹帧回调用点，战斗卡死/交易蒸发/链盗用整类问题结构性不存在。
6. **错误通道**（vmcore/error.cpp）：`vmError()` 统一出口 → 留痕打印 → catch 链恢复 /
   未捕获则终止当前链。调度器不再有吞错 catch（f109700 语义并入设计）。
7. **解释器**（vmcore/interp.cpp）：全局栈 + 当前 PCB；opcode 分发逐个从现有实现搬运
   （语义已实锋试错成果），改动仅限状态来源：this/vars/funTable/stack 一律 PCB+帧。

### 对外 API 面（兼容层，签名不变）

`original_vm.h` 的 public 函数（callMasterProgramFunction、loadObjectIntoRegistry、
cloneObjectInRegistry、findObjectInRegistry、applyClosureOnOwner、objVarGet/Set、
schedulerRun、antiCheatCheckServer……）**签名保持**，内部实现改为转调新核。
game_service.cpp / builtins.cpp / main.cpp 等驱动侧代码不感知内核替换。

## 五、病根 → 新核结构消解表

| 病根 | 新核中的状态 |
|---|---|
| #1 per-object ExecState | 不存在：执行状态只住 PCB；OriginalVm 类消失 |
| #2 runDetached/链盗用/桥接 | 不存在：call_other 压帧于当前 PCB，park/resume 即挂起/恢复 |
| #3 字符串键身份/suffix 扫描/tplKeyCache | 不存在：对象值=指针，名字解析只在加载入口（vm_lookupObjectByName 同位） |
| #4 tid 补丁/chainSeq/tls 双轨 | 不存在：tid 即 PCB 池字段，链状态即 PCB 本身，无第二套 key |
| #5 静默吞错 | 设计内建：vmError 统一留痕，调度器无吞错 catch |
| #6 save/restore 上下文 | 不存在：上下文=帧，压帧/弹帧天然配对，无任何成员切换 |

## 六、施工顺序（纯施工，中途不启动服务）

- 以下阶段是**代码施工顺序**，不设中途启动、不设中途回归：测试不是重写阶段的事。
- **整个新核全部写完 + 驱动切换完成后，一次性构建并启动**。启动是重写完成的验收，
  不是施工过程的手段。
- 构建与运行规矩（仅最终启动时适用，硬性）：
- 构建前 unset 大小写四个 `HTTP(S)_PROXY`（大小写重名致 MSB6001）；
  `python -X utf8 tools/_build.py`（先停服务，脚本已内置杀进程）。
- 服务日志目录一律 `tmp/<前缀>_<时间戳>`，**每次全新唯一名称，严禁复用旧目录**。
- 启动判定以 **8160 端口打开**为准，启动期间不杀进程（`docs/BUILD_AND_RUN.md` §3）。
- 数据库红线照旧：启动前清空 `dl_ddb_1_test`，其余只读，参考库永不写入。

### 阶段 0：契约冻结
- 列全 `original_vm.h` public API 为兼容面；梳理驱动侧对内部结构的越界引用
  （t_efunCallerVm、scheduler() 内部字段等），逐个标注归属（应改走 API/应移入新核）。
- 产出：API 契约清单（写进 vmcore.h 注释），驱动侧无需改动。

### 阶段 1：新核骨架 + 程序/对象模型
- 新建 src/vmcore/{vmcore.h, pcb.h, object.h, call.cpp, sched.cpp, error.cpp, interp.cpp}；
- `VmProgram` 由 o_loader 产出（o_loader 不动，加载结果直接作为只读程序数据）；
  `VmObject` 实例化/析构/克隆、对象表（指针→实例）、按名查找（仅加载入口）；
  tid 池。

### 阶段 2：解释器核心（opcode 搬运）
- 全局栈 + PCB + 帧模型下的 opcode 分发：逐 opcode 从现有实现搬运（保持编号、
  操作数布局、栈效应一致），this/vars/funTable/strings 一律从当前帧取。
- 搬运重点（历史踩坑最密处，逐一对照现有实现语义）：帧弹回与 catch 恢复、
  check_args_num/resort_stack、参数缺省填充（undefined 而非 0）、
  call_global_fn 不加 baseFunOffset、隐藏函数记录跳过。

### 阶段 3：调用家族 + park/resume
- 三个调用原语落地；op-8/call_in_other_fun_ex/闭包/广播/继承全部走压帧；
  阻塞 efun 抛 SuspendSignal → PCB park，wake 后弹帧回调用点；return hook 后处理。

### 阶段 4：调度器
- PCB 调度器（ready/timer/queue/event/mutex/服务 socket）；execOnMain 语义保留
  （外部线程注入执行）；启动/登录/战斗/交易的 LPC 链全部跑在新调度器上。

### 阶段 5：efun 移植 + 驱动切换（主战场）
- efun 表分批搬入新核（底座直通：xdbc/mysql/socket/file 不动；只把"当前 VM"
  参数换成"当前 PCB"）；错误通道统一到 vmError。
- 全部 public API 内部实现切换到新核，驱动侧代码不改。

### 阶段 6：旧核退役删除 + 首次启动
- 删除：OriginalVm 执行引擎、Scheduler 旧 Process 字段（parkedVm/callerVm/callers_/
  chainSeq/tls 双轨）、runDetached/switchToChain/runStates_、savedObjVars/
  funcLookupSwitched/thisNameOverride/autoVars、suffix memo/tplKeyCache、
  字符串键对象值（ObjectValue 改存指针）。
- 兼容层收敛：original_vm.h 只保留驱动仍需的入口，其余内联到调用方或删除。
- **验收（重写完成的唯一一次启动）**：全库构建 0 error、grep 无旧核符号残留；
  启动 gs：8160 端口打开、启动日志无错误、可登录进世界。不过就修到过。

## 七、施工纪律

- 重写期间不启动服务、不写测试脚本；重写完成（阶段 6 尾）才构建 + 启动。
- opcode 语义搬运以现有实现为基准、以 IDA 为仲裁：搬运结果与现有实现行为不一致时，
  先怀疑搬运错误；现有实现本身存疑时查 IDA 定夺，禁止靠猜。
- 不引入旧版遗留机制：runDetached、链盗用、switch/restore、字符串键身份、
  suffix 扫描、手工 tid 补丁、双轨链 key——新核中一律不存在；原版有的
  （PCB、压帧、return hook、对象指针、tid 池、错误留痕）才有。
- 内存底线：克隆为浅实例（共享只读程序）。
- 本文档地址即 IDA 索引；语义不明必查 IDA，禁止猜测。
