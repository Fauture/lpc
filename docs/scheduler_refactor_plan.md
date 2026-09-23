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

## 7. Step A（执行态进 ExecState）本轮进展与可复现配方

目标：把 `OriginalVm` 的 19 个执行态成员（`frames_/stack_/detachedStack_/code_/meta_/ip_/
cond_/parked_/catches_/catchDepth_/returnValue_/returned_/funcLookup_/lastOp_/lastIp_/
lastEfunIndex_/savedObjVarsForNextCall_/onResume_/resumeValue_`）搬进 `struct ExecState`，
VM 只留 `ExecState* es_`，于是 `switchToChain()` 从"搬 19 个成员"变成指针转移，
`runStates_` 也不再需要转存（这是"冲突 / 无法共享数据"的结构性解法）。

**已经跑通的部分（`_vmchain_codegen/refactor_stepA_v2.py`）**

1. 只替换 **Scheduler 类体之外** 与 **OriginalVm 成员块之外** 的引用
   （关键教训：无差别替换会把结构体成员声明一起改掉，例如
   `std::vector<std::shared_ptr<Process>> parked_;`、`sockWaiters_`、`timers_` ⇒ 调度器被摧毁，
   报 C2065 "未声明的标识符" 一堆）；
2. 于是 1242 处成员引用改成 `es_->member`，54 处"通过别的 VM 指针访问"
   （`vm->frames_`、`p->parkedVm->frames_`）改走 `->es_->member`；
3. `VmRunState` 缩减为 `std::shared_ptr<ExecState> es; int64_t ownerTid;`；
4. `stashRunState/restoreRunState/switchToChain` 重写为所有权转移（shared_ptr），
   不再逐个 `std::move` 19 个字段；
5. 剩余编译错误：**2 个**（两处裸 `!parked_` 位于 lambda 内，需要写成 `!es_->parked`），
   另需把调度器自己的 `parked_`（vector）声明补回类作用域。

**为什么本轮没有落地**：最后两次修补又引入了新的破坏（这类文本手术一次只能动一个
语义单元），而仓库红线是"每一步都要能起服、能登录"。因此本轮结束在**已验证可用的
提交状态**（`git checkout HEAD -- src/original_vm.cpp` 后重建通过），Step A 的配方与
脚本留给下一次以"小步 + 每步编译 + 起服验收"的方式落地。

**下一次的正确做法**

* 用宏过渡一次到位：在 `ExecState` 之上加
  `#define frames_ es_->frames`（19 个），Scheduler 类体用 `#undef`/`#define` 单独处理
  它自己的 `parked_`。这样"搬移"只需一次编译即可验证语义，之后的 `es_->` 明文替换是
  纯机械重命名，可以分多次提交、每次都能起服。
* 或者按成员分批：先搬 `frames_`（611 处），再搬 `stack_`（315 处）… 每批一次构建+验收。
* 无论哪种，**保护区间必须按行号集合计算**（Scheduler 类体、VM 成员块、结构体定义），
  不能用全局正则。

## 8. Step A4（把宏改成访问器）——已实测回归，不要重做

想法：把 19 个迁移宏 `#define frames_ es_->frames` 改成类内访问器
`std::vector<Frame>& frames_() { return es_->frames; }`（宏变成 `#define frames_ frames_()`），
让"执行态在哪"由函数决定，为下一步"按链选状态"铺路。

实测（同一构建、同一账号、20 连击建号、连跑 3 次）

| 版本 | 9112 维护中 | 9101 处理中 | 9113 名字已存在 | basic_char_info | 客户端 |
|---|---|---|---|---|---|
| A3（宏，已提交 `84ea3c6`） | 0 | 7 | 3 | 1 行 | ingame(6) |
| A4（访问器） | **1（三次全部 1）** | **9** | 3 | 1 行 | ingame(7) |

结论：A4 稳定引入 1 次 9112 「数据库目前正在维护中（原因：9112）」，且 9101 从 7 升到 9，
属于**确定性回归**（不是噪声）。9112 的来源是"某个 filed 等待者被错误唤醒/漏唤醒"
（`filed.c::do_operate` 返回 -2），也就是说访问器版本改变了唤醒时序。

因此：**宏垫片是当前语义的一部分，不能简单换成访问器**。若要继续这条路，必须先把
"哪个链的状态"显式化（`Process` 直接持有 `ExecState`，`runProcess` 进入时设置），
让访问器有确定的落点，而不是依赖 `es_` 这个共享指针在唤醒路径上的时序。

回退记录：`git checkout`/备份恢复后重建（18:12:10 的 Release 二进制），
`src/` 与 HEAD `84ea3c6` 一致，A3 验收数据（0×9112）保持不变。

## 9. 链状态（Step A3）与链键（A4/A5）的完整实测对照

测试方法：真机 `_rapid2.py 100 <name> 20`（登录→建角页→名字→一次 `{ENTER 20}`），
每个版本连跑 3 次，`_acc.py` 统计客户端弹窗与数据库行。

| 版本 | 说明 | 9112 维护中 | 9101 处理中 | 9113 名字已存在 | basic_char_info | 客户端 |
|---|---|---|---|---|---|---|
| A3（`84ea3c6`，当前 HEAD） | 链状态表永久持有执行态，`es_` 只换指向 | **0 / 0 / 0** | 6 / 6 / 6 | 3 / 3 / 3 | 1 行 | ingame |
| A4 | 把 19 个宏换成类内访问器 `frames_()` | 1 / 1 / 1 | 9 / 9 / 9 | 3 / 3 / 3 | 1 行 | ingame |
| A5 | 在 `runProcess` 里保存/恢复 `chainKey` | 1 / 1 / 1 | 7 / 7 / 7 | 3 / 3 / 3 | 1 行 | ingame |

结论（都是确定性差异，不是噪声）：

1. **A3 是当前最优且已提交的状态**：0 次 9112、6 次 9101、关联行写出、客户端进游戏。
2. **A4 与 A5 都稳定引入 1 次 9112**（`filed.do_operate` 返回 -2 → 「数据库目前正在
   维护中（原因：9112）」）。二者唯一的共同点：都改变了"VM 在入口处安装哪条链的
   执行状态"这一时序。
3. `chainKey` 在本文件里**只有** 6 处用途——全部是 `switchToChain(lpc::scheduler().chainKey())`
   入口切换（`executeFunctionFromOutside` / `executeFunctionOnStack` / `resumeProcess` /
   `resumeCaller` / `resumeRunLoop` / `enterChain`）。也就是说"谁在跑"完全由这个
   thread-local 决定，而 A3 里它**从不恢复**（`runProcess` 只置位）。
4. 因此下一步只剩两个明确的实验，二选一做完再谈删 `switchToChain`：
   * **清空式**：一次运行结束后把 chainKey 置 `-1`（而不是恢复成上一个进程的 key），
     让嵌套/跨对象入口落到 `defaultEs_`；
   * **绑定式**：让 `Process` 直接持有 `shared_ptr<ExecState>`，调度器进入时把
     `vm->es_` 设为该进程的链状态，`switchToChain(key)` 只在"key 正是当前进程的链"时才
     生效，否则不动（这样 A4 的访问器才有确定落点）。
   两者都必须用上面同一套 3 连跑对照来做判定（0×9112 才算通过）。

## 10. 验收规程修正（重要：之前所有弹窗计数都是错的）

### 10.1 计数 bug
`9101/9112/9113` 是 0x1FE5 报文 **hex 载荷里的 GBK 文本**，在日志里表现为
`39313031` / `39313132` / `39313133`，**不是** ASCII 的 `9101`。我（和 `_acc.py`）
一直按字面文本搜索，所以报出来的数字与真实弹窗无关：

| 版本 | 旧（错误）计数 | 正确计数（每轮 9101 / 9113 / 9112） |
|---|---|---|
| A6（`ecaa40d`，已提交） | “0 / 6 / 0” | **8/6/1 · 4/2/1 · 0/1/0** |
| A4b（访问器+A6） | “1 / 9 / 0” | **7/3/0 · 7/4/0 · 0/0/0** |

结论：第 8、9 节里基于旧计数的版本对比（A3/A4/A5 的 9112 差异）**作废**，
A4“确定性回归”的结论不成立——A4b 与 A6 在正确计数下几乎相同（总 9101 14 vs 15，
9113 7 vs 7，9112 都是 0）。

### 10.2 测试规程（按用户要求）
* **一个版本一次起服；每次登录用不同账号**（账号 1-100，密码=账号），
  即“3 次验证 = 3 个账号各登录一次”，不是同一账号连跑 3 次。
* 起服会清空 `dl_ddb_1_test`，所以每个账号都是全新角色状态，不必挑“没用过的账号”。
* 每一轮必须确认**走的是建角流程**（日志里有 `args=[create_new_char ...]`；
  `args=[load_existed_char ...]` 表示载入已有角色，那一轮不算数）。
* 统计用 `_acc6.py`（按运行分段 + 正确的 GBK hex 计数），
  判据：**9112 = 0**、9113 属于原版行为、客户端能进游戏。

### 10.3 复用工具
| 工具 | 用途 |
|---|---|
| `_rapid2.py <acct> <name> <n>` | 真机登录 + 建角 + n 连击 ENTER |
| `_acc6.py <logdir> [names]` | 按轮统计弹窗（唯一可信的弹窗计数） |
| `_flowgate4.py <logdir> <name>` | 该轮是否为建角流程 |

## 11. Step C 测量：VM 到底会不会被非调度线程进入？

`execMutex_` / `execDepth_` / `runDetached` 存在的唯一理由是"一个 VM 可能被多个线程进入"
（代码注释自己这么写）。要删它们，先要证明**没有**非调度线程进入。

做法：在 5 个 guard 点（`fromOutside` / `onStack` / `resume` / `resumeRunLoop` /
`resumeCaller`）加 `noteVmEntry()` 探针，打印 `schedulerThread=0/1`（`LPCVM_ENTRYDBG`
可运行时开关），前 40 次进入采样。

实测（起服 + 账号 1/2/3 各一次 20 连击建号）：

| 阶段 | schedulerThread=1 | schedulerThread=0 |
|---|---|---|
| 启动到 listening | 40 | **0** |
| 真机建号连击 | （采样已满，无新增非调度进入） | **0** |

同批次的验收数据（3 个不同账号）：每轮 `creates=11~12`、`9101 = 7/6/0`、
`9113 = 3/3/0`、**`9112 = 0`**，三轮全部 `ingame`。

结论：
1. 现在所有 LPC 入口都在调度器线程上（`create_thread` 三个 spawn 点、timer、构造、
   网络包 `execOnMain`），锁在热路径上只是空操作开销。
2. 剩下 3 处 OS 线程是：`create_thread` 的 telnet 回退分支（`isTelnetThread`）、
   两处 pending-threads 的 `workers` 线程池；它们要么走 `spawn`，要么在真实服务里不触发。
3. 下一批改动（Step C 的正式落地）：删掉 `execMutex_`/`execDepth_`/`ExecGuard` 与
   `runDetached` 的解锁/重锁逻辑，只保留 `es_->detachedStack` 的语义；然后处理那 3 处线程。
   每批仍按"编译 → 起服 → 3 个不同账号（9112=0）"验收。

## 12. Step C2：剩下的 OS 线程入口只在 telnet 路径上

对 3 处残留入口逐个查证（不是猜）：

| 入口 | 谁调用 | 真实游戏路径是否触发 |
|---|---|---|
| `drainPendingOriginalThreads`（builtins.cpp:1299） | **只有 telnet 新客户端**接入时（`telnet_new_client`） | 否 |
| `runPendingOriginalThreads`（original_vm.cpp:16922） | **没有任何调用者**（仓库内全量搜索） | 否 |
| `pendingThreads()` 队列 | **没有任何 push**（全量搜索 `push_back/emplace` 为 0） | 否 |
| `create_thread` 的 `isTelnetThread` 回退分支 | telnet 守护线程（`telnet_daemon` / `buildin_telnet_daemon`） | 否 |

日志佐证：`vLock` / `vEntry` / `vA6c` / `stepA6` 四份真机日志里
`pending-thr=0`、`pending threads=0`、`[thread] run=0`。

结论：
1. **游戏客户端路径上已经没有非调度线程进入 VM**（与第 11 节 `noteVmEntry` 的
   40/40 + 全部真实负载为 0 一致）；
2. 仅存的非调度入口是 **telnet（GM 后台）**，它在调度器线程之外进 VM 是设计遗留；
3. 因此 Step C1 的"锁只在非调度线程加"正好覆盖 telnet，而游戏路径不再付锁开销——
   这就是当前能安全达到的最优形态；
4. 若要彻底删掉锁，必须先把 telnet 入口改成调度器进程（`spawn` + 在进程上携带
   telnet client id），并用 `noteVmEntry` 复测"非调度进入=0"。那是有独立风险的
   一步（telnet 的 `input_string()` 走 thread-local client id），留待单独做。

### 现在还留着的多线程机制（及其存在理由）
| 机制 | 现状 |
|---|---|
| `execMutex_` / `execDepth_` | 仅 telnet 路径实际加锁；游戏路径空操作 |
| `runDetached` 的解锁/重取 | 已按"实际持锁次数"重写，锁为 0 时自动跳过 |
| `objVarsSnapshot` | 仅上面那些 pending 线程用；游戏路径不走 |
| `create_thread` telnet 回退分支 | telnet 专用 |
| 每连接一个 recv 线程、每服务一个 accept 线程 | 网络层遗留，Step F |

## 13. Step C 完成：多线程机制全部拆掉（C1–C4）

| 批次 | 提交 | 内容 | 依据 |
|---|---|---|---|
| C1 | `f4f3724` | `ExecGuard` 改为只在**非调度线程**加锁，`runDetached` 的释放/重取按"实际持锁次数" | 第 11 节测量：非调度进入 0 |
| C2 | `2b6f52c` | telnet 守护线程 → `Scheduler::spawnTelnet`（client id 随**进程**走，`runProcess` 作用域守卫发布/恢复） | 第 12 节：唯一剩下的非调度入口是 telnet |
| C3 | `c336b6b` | 删除 pending-thread 死代码（两个 worker 池、队列、声明、builtins 调用） | 队列无写入者、无调用者、日志 0 次 |
| C4 | `6732054` | **删除 `execMutex_`/`execDepth_`/`ExecGuard`/6 处 guard/`runDetached` 的锁循环** | C2+C3 之后本文件不再创建会进 VM 的线程 |

保留的东西（不是遗留，是语义）：
* `runDetached` 把调用者 stack/frames 移入/移出 `es_->detachedStack`——让嵌套跨对象调用
  拿到干净栈，属**状态**语义；
* 4 处 `std::thread`（xdbc 探测/守护、调试钩子）都不进入 VM；
* `noteVmEntry` 探针（`LPCVM_ENTRYDBG`）留着，用于后续任何改动后复测"非调度进入=0"。

验收（每个批次都是 3 个不同账号、各一次 20 连击建号）：`9112` 恒为 0、
`9101` 0~9（随客户端重发次数波动）、`9113` 0~4，客户端能进游戏。
源文件 `original_vm.cpp` 从 19,460 行降到 19,308 行。

## 14. Step F（网络层）现状与下一步

现状（`game_service.cpp`）：
* 每服务一个 accept 线程 + 每连接一个 recv 线程；recv 循环已是
  "100ms select + 非阻塞 recv"（无空转），错误/关闭路径有明确日志；
* 客户端包在 recv 线程解析，`dispatchCommand` 先过连接级命令门
  （`waitClientCommandIdle`：驱动链/脚本 `operating`/settle），再
  `feedClient -> execOnMain`，由 `fut.get()` 阻塞到该命令链结束——即
  "一个连接一次命令"由**阻塞**实现，`ev=OVERLAP` 恒为 0。

下一步（需要独立一轮起服+验收，风险中等）：
1. 把 accept 与每连接读取改成调度器 socket 扫描（对齐原版
   `vm_scanSocketInMainLoop`）：调度器循环里 `select` 监听 socket 与客户端 socket，
   读到的字节放进连接缓冲；
2. 包解析留在缓冲之上，命令派发改为"投递到调度器邮箱"，由调度器在
   上一命令链结束后派发下一包（邮箱版门，取代 `fut.get()` 阻塞）；
3. 验收：`ev=OVERLAP=0`、`[chain-gate]=0`、3 个账号 `9112=0` 且能进游戏，
   并复测 `noteVmEntry` 非调度进入=0。

## 15. 验收装置本身的可信性（审计结果）

前面两次测试方法错误（同账号连跑、弹窗计数按字面文本搜索）都由用户指出。
为确认现在这套装置可信，对最近几批验收数据做了审计（`_harness_audit.py` /
`_acct_in_log.py`）：

| 批次 | 日志中出现的账号 | 每轮创建的角色 | 每轮是否载入自己的角色 |
|---|---|---|---|
| `vF1` | **7, 8, 9**（恰好三个） | vfA / vfB / vfC | 是 |
| `vC4` | 4, 5, 6 | v4A / v4B / v4C | 是 |
| `vC3` | 1, 2, 3 | vcA / vcB / vcC | 是 |
| `vTel` | 7, 8, 9 | vtA / vtB / vtC | 是（vtA 出现两次属重连） |
| `vLock` | 4, 5, 6 | vlA / vlB / vlC | 是 |
| `vA6c` | 4–9（六轮） | va6A…va6F | 是 |

即：**每次登录都是不同账号、每轮都走建角流程并创建自己的角色**，
没有"载入已有角色直接进游戏"混入统计。

固定规程（每批改动必跑）
1. 起服（清库；~15-20 分钟到 listening，20 分钟不到即视为失败）；
2. 账号 1–100 中取**三个不同账号**，各跑 `_rapid2.py <acct> <name> 20`（每个账号
   一个新角色名）；
3. `_acc6.py <logdir>` 统计（按运行分段 + GBK hex 计数）；
4. 通过线：**9112 = 0**、`ev=OVERLAP = 0`、`[chain-gate] = 0`、客户端进游戏；
5. 需要时复测 `noteVmEntry`（`LPCVM_ENTRYDBG`）：非调度线程进入必须为 0。
