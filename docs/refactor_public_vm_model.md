# lpcvm 虚拟机重构（以公开 LPC 驱动为参照）

分支：`refactor/ldmud-vm-model`（从 `88ba918` 起）
参照源码：`C:\Users\Administrator\Desktop\lpc-ref\ldmud-master\src`、`C:\Users\Administrator\Desktop\lpc-ref\fluffos-master`
（codeload.github.com 可直连，raw.githubusercontent.com 在本机 DNS 解析不到）

## 为什么重构

现在的 `src/original_vm.cpp`（1.9 万行）把三件事揉在一个类里：

1. 字节码解释器（`runLoop` + `frames_`/`stack_`/`code_`/`meta_`/`ip_`）
2. 协作式调度器（`Scheduler`：ready/timers/queue/event/socket/mutex 等待表）
3. 网络/DB 线程的入口（`execOnMain`、`runDetached`、`spawn`）

后果是每一类问题都要靠"在某处加一个判断"来修，最近连续 4 次针对同一个
症状（20 连击建号弹「正在处理中」9101）的补丁全部失败，就是这种结构的症状：

| 尝试 | 结果 |
|---|---|
| 队列按 1s 静默等待 | 9101 × 5（基线） |
| 等"本命令回复已发出 + 静默" | 9101 × 7（更差） |
| 驱动层 `commandsInFlight()` 计数 | 9101 × 9（更差） |
| 连接级 busy 探针 + 定时轮询 | 未验证（被本轮重构取代） |

共同点：**都在猜哪个环节让同一个连接的第二个命令提前派发**，没有直接观测。

## 公开驱动的模型（参照点）

* **LDMud** `src/backend.c`：主循环 `get_message()` 每次只取一条输入行，
  交给 `execute_command()` / `call_function_interactive()` 跑完，
  才回到循环取下一行。也就是说**一个 interactive 同时只有一个命令在跑**；
  输入按 interactive 缓冲，命令跑完后才被消费。
  阻塞 efun 靠 `vm_pendCurrentProcess` 挂起 PCB，主循环继续服务其他 PCB。
* **FluffOS** `src/comm.c` + `backend.c`：同样的形状——socket 只负责收字节、
  切分命令，`process_input()` 在 backend 里逐条执行。
* 两者都不存在"网络线程直接 apply LPC 命令"的路径。

我们的实现里恰好有这条路径（`game_service.cpp` 的 reader 线程 →
`pktAnalyse` → `applyClosureOnOwner` → `Scheduler::execOnMain`），
所以"同一连接两个命令重叠"在结构上是可能的，必须用观测确认它是否真的发生。

## 阶段 0（本轮，已完成代码 + 待验收）

**连接级命令链时间线探针**（`LPCVM_CHAINTRACE`，或 `_probe_on\LPCVM_CHAINTRACE`）

* `src/original_vm.h`：`vmchainBegin/End/Park/Run/RunExit/Mailbox/LiveChains/TextOf`
  声明 + `LivenessCounters::chainBegin/chainEnd/chainPark/chainResume/chainOverlap`
* `src/original_vm.cpp`：探针实现在文件末尾（`lpc` 命名空间），帧链渲染器在
  `} // namespace lpc` 之后（那里 `OriginalVm` 可见）；调度器在
  `parkCurrent` / `execOnMain` / `run()` 的 `runProcess` 前后 / `ThreadExitSignal`
  四处打点
* `src/builtins.*`：`setDispatchCno/dispatchCno`（thread_local，连接号随命令下发）
* `src/game_service.cpp`：两处派发点（accept 线程、每连接 reader 线程）
  用 `setDispatchCno` 标注连接号并打一条 `MAILBOX` 事件

输出（每个事件一行，含连接号、链号、VM 指针、入口函数、live 链数、帧链）：

```
[chain] t=<ms> ev=BEGIN|PARK|RESUME|END|DROP|OVERLAP|MAILBOX cno=<连接> key=<链号>
        vm=<VM> entry=<函数> live=<该连接未结束链数> chain=[<帧链>]
```

**关键判据**：`OVERLAP` 事件与 `chainOverlap` 计数。
公开模型下同一连接永不重叠，所以这个数必须是 0；
它一旦非 0，就带出"是哪条链、哪个入口函数、等了多久、卡在什么条件上"，
不用再从弹窗数量反推。

## 阶段 1（结构修复，待阶段 0 数据确认根因后实施）

**连接级命令邮箱**（把公开模型的"一个 interactive 一个命令"搬过来）

* reader 线程只做收发与拆包，把整包交给**该连接的命令邮箱**；
* 一个连接的邮箱同时最多有一个"在跑"的命令；命令链结束（VM 的 END 事件）
  才放行下一个；
* 派发不再依赖 reader 线程的 `fut.get()` 阻塞（那是"碰巧序列化"），
  而是邮箱的状态机保证——即使将来把派发改到调度线程也成立。

## 阶段 2（拆分，去除结构性隐患）

* 把 `Scheduler` 从 `original_vm.cpp` 抽到独立单元（公开驱动的 `backend.c` 对应物），
  解释器只保留 `OriginalVm`；
* `Process` 明确成 PCB 状态机（ready/pending/finished），
  每条链的执行态从"VM 成员 + runStates_ 转存"改为 PCB 自持；
* 去掉 `runDetached`/`execDepth_`/`execMutex_` 残留（公开驱动里没有这些概念）。

## 阶段 0/1 实测结果（2026-09-23，真机 20 连击建号）

真机流程：`_rapid2.py 100 <名字> 20`（登录账号 100 → 建角页 → 名字 → 一次
`{ENTER 20}`），随后 `_acc.py <dir> <名字>` 四项检查。

| 版本 | 9101 处理中 | 9113 名字已存在 | 角色列表 | gid_info | basic_char_info | 客户端 | OVERLAP | 门超时 |
|---|---|---|---|---|---|---|---|---|
| 88ba918（基线，chain1） | 5 | 4 | body=114 | 1 行 | **0 行** | 停在建角页 createchar(9) | 1 次（实测证） | — |
| 仅按包加门（chain5） | 10 | 3 | body=114 | 1 行 | 0 行 | 停在建角页 | 0 | 0 |
| 门 + 续链归属连接（chain6） | 6 | 4 | body=114 | 1 行 | **1 行** | **进游戏 ingame(11)** | 0 | 0 |
| 门 + 续链归属 + LPC 标记轮询（chain8） | 8 | 4 | body=114 | 1 行 | **1 行** | **进游戏 ingame(6)** | 0 | 0 |

**已确认的结构事实（都有日志证据，不再是推测）**

1. **同一连接确实会重叠**（基线）：`cno=22` 的两个 `cmd_create_new_char`
   链 `key=28538`/`key=28539`，第二条 BEGIN 时第一条 `live=1`；
   紧接着 `[lg] … load_existed_char … chain=[_process_command<-create_new_char
   <-create_new_char_succ<-check_valid_name<-get_gid<-do_operate]` —— 上一条
   create 还停在 `do_operate`（等 filed 往返）。
2. **一条 TCP 读可以带来多个包**：`feedClient()` 在**一次调用里**把整块里的
   所有完整包依次派发，所以"按块加门"无效（chain5 的 9101 反而更多）——
   门必须落在 `dispatchCommand`（每个包的唯一入口）。
3. **命令的真实生命周期跨链**：`cmd_create_new_char` 链 250–340ms 就 END 了，
   但 LPC 的 create 继续在**另一条链**（threadd/daemon 的 `do_operate`）上跑，
   所以"驱动链为空"不等于"命令结束"。续链现在会继承连接号
   （`Scheduler::spawn` 继承 `t_schedClientNo`，park 时 `vmchainTagLive` 重新归属）。
4. 加门后：**OVERLAP = 0、门超时 = 0、客户端稳定进游戏、`basic_char_info`
   关联行写出**（基线是 0 行、停在建角页）。

**仍未解决**：9101 还有 6–8 次。链路已经收敛到一点：客户端在 create 进行中
会自行重发 `create_new_char`，而 `login.o::_process_command` 的重入标记
（`lite_set_temp("operating",1)`）在这时**仍然是 1**（日志里 `[login-flow] … op=1`
紧接 9101）。

* 已实现"驱动读 LPC 标记"的通道：`OriginalVm::liteTempTruthy("operating")` +
  `registerClientScriptVm` + 门里的 `clientLpcBusy`，但实测该读取返回 **false**
  ——说明这个标记**不在对象的 temp 映射里**。
* 下一步（明确的单点任务）：定位 `lite_set_temp("operating", …)` 在
  `OriginalVm` 里的真实落点（`LPCVM_TEMPKEY` / dump 登录 clone 的 objVars，
  或按 `logind.o`/`login.o` 的 `_process_command` 调用点反查），把
  `liteTempTruthy` 指到正确的存储上；之后门就能在 LPC 自己认为"忙"时拦住包，
  9101 应归零。
* 另一个已排除的方向：这不是"门放行太早"（active=0、live=0、overlap=0 都成立），
  也不是"门拦多了"（drops=0）。

## 验收（每阶段必跑，缺一不可）

```
python -X utf8 _run_probe.py <dir> 86400 "LPCVM_SEND=1;LPCVM_TAIL=1;LPCVM_CHAINTRACE=1"
python -X utf8 _rapid2.py 100 <name> 20          # 真机 20 连击建号
python -X utf8 _acc.py <dir> <name>              # 四项：弹窗数 / 0xF061 body / gid_info / basic_char_info
```

* `chainOverlap == 0`（阶段 1 之后必须）
* 四项验收全过 + 客户端进游戏
* 每次改动真机验收通过才提交（仓库红线）

## 环境坑（继续遵守）

* 源码 UTF-8 无 BOM、以 936 代码页编译：**改源码必须保持 CRLF**；
  用 `_patch_src.py`（锚点替换，自动按目标文件行尾重写），
  脚本直接写文件把 CRLF 变 LF 会让编译器吞掉花括号。
* `original_vm.cpp` / `builtins.cpp` 里本来就有 16 / 12 个 U+FFFD
  （HEAD 版本就有，不是新引入的），不要"顺手修"，会污染 diff。
* MSBuild 增量编译有时不重编 `original_vm.cpp`（`dumpbin` 里符号仍是 UNDEF）：
  出现"改了解析不到的符号"时先 `/t:Rebuild` 或 touch 源文件。
* 到 listening 前 NPC `restore` 阶段 10–15 分钟是正常的。
