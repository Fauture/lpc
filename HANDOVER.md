# LPC 引擎复刻 — 交接文档（给下一个对话）

## ⛔ 数据库红线（最高优先级，任何对话 / 任何工具 / 任何脚本都必须遵守）

**每次启动服务端之前必须先清空测试库 `dl_ddb_1_test`（必做步骤）；除此之外禁止向数据库写入任何数据 —— 默认只读，只允许 `SELECT`。**

一、启动前必做（`_run_probe.py` 会在启动前自动执行，`_dbclear.py` 单独跑也可以）：

* `dl_ddb_1_test`：**删除该库所有表的行**（`DELETE FROM ...`；只能删数据，绝不删表、绝不 DROP）。
* `dl_adb_all_test`：**禁止删除**该库的任何数据（账号库是持久的，任何时候都不许清）。
  （`run.md` 旧第 11 条"每次启动删除 dl_adb_all_test 数据"已作废，以此条为准。）
* 参考库 `dl_ddb_1` / `dl_adb_all` / `dl_mdb_1` / `dl_mdb_all` / `dl_ldb_*` / `dl_dmdb_1` / `dl_tdb_1`：
  **永远不许清除、永远不许写入**，只用于比对。

二、除上面这一次启动清除之外，禁止一切写库行为：

* 严禁 `INSERT` / `UPDATE` / `DELETE` / `REPLACE` / `CREATE` / `DROP` / `TRUNCATE` / `ALTER`。
* 严禁跨库复制数据 —— **尤其禁止把参考库（`dl_ddb_1`、`dl_adb_all`、`dl_mdb_1`）的行复制进测试库**，
  也禁止把测试库写回参考库。
* 严禁手工造 / 改账号、角色、存档、坐标、任务、物品、商店、经验等数据来"绕过"问题；
  **测试数据必须由服务端自己产生**：清库之后用真实客户端（或 `_login_lib.py` / `_enter_probe.py`）
  正常登录、建号、进游戏。
* 严禁为了让某个角色能登录而手工补行（例如补 `data1` 行、补 `login` 角色列表、把角色挂到别的账号）。
* `_db*.py`、`_fromorig.py`、`_dbrestore*.py` 之类脚本里的写库语句一律按 **禁用代码** 对待，
  只有"启动清空 `dl_ddb_1_test`"的脚本例外。

三、违反后果（真实事故，2026-09-17）：

清库后为了"造出一个能登录的角色"，把参考库的角色数据复制进测试库，并往 `dl_mdb_1.acc_info`
写入账号行 —— 环境被污染、原测试角色存档无法恢复、参考数据副本混进测试库。
**宁可不测，也不许写库。**

## ⛔ IDA 语义核对（强制，任何对话 / 任何工具都必须遵守）

**凡是 efun / 操作码 / 字节码的语义不清楚，必须先查 IDA（原版 `magic_Linux32x` 反编译 / MCP）还原，禁止猜测、禁止"能用就行"的近似实现。**

* 顺序：先查 `_ida_efuns/<efun 名>.txt`（已有 195 个 efun 的反编译）→ 再查 `tools/`、`analysis/`、`dumps/` 里该 efun/操作码的转储 →
  仍不清楚就 **必须用 IDA MCP 反编译原版函数**（`vm_f_<efun>` / `vm_add<...>` / `call_in_efun` 分发表）逐条核对参数、返回值、边界行为。
* **本会话没有 IDA/MCP 时**：不得按猜测定稿上线；必须
  ① 在代码注释里写明"语义来源（调用点证据）"和"尚未用 IDA 验证"；
  ② 把该 efun 记入 `_ida_missing/<efun 名>.txt`（写明待核对项）；
  ③ 在 HANDOVER 里记账，交给下一个能连 IDA 的会话核对。
* 反例（真实事故）：`mapping_array(array, func)` 曾被写成"忽略 func、原样返回入参数组"的近似实现，
  于是 `scand::query_objects` 的 `mapping_array(obs_arr, (: find_basic_object_by_import_name :))`
  返回 import_name **字符串**而不是对象，新手上任务 S11 张老板发武器时 `is_object(basic)` 失败，
  客户端看到"奇怪，怎么找不到了呢？我再找找，你过会儿再来。"；而商店（dealer.list）与防具
  （按名字直接 `find_basic_object_by_import_name`）都不走这条链，所以只有武器这一步坏。
* 另一条相关教训：同一处修复如果改用"经调度器的嵌套执行"（applyClosureOnOwner → execOnMain），
  会让启动卡死（超过 20 分钟即视为死亡）。VM 内的实现必须在**当前 VM 上内联执行**（executeFunctionFromOutside）。

## ⛔ 启动后必须测试登录（强制，报告"已启动"之前必做）

**每次启动服务端之后，必须先跑一遍真实登录流程，确认能进游戏，才能说"已启动"。**

* 必测链路：`AAA 登录 → 选角（或建角）→ 进游戏 → 关键包`：
  `0x1155` 位置、`0x2603` 任务面板、`0x5103` 背包、`0x115D` 剧情触发点（有剧情时）、`0x20D1` 剧情。
* 现成脚本：
  `python -X utf8 _fire_special.py <账号> <密码> <角色>`（登录 + 剧情触发点 + 0x141A）、
  `python -X utf8 _bagtask_probe.py <账号> <密码> <角色> <秒>`（任务面板 + 背包）、
  `python -X utf8 _relogin_test.py 192.168.200.1 <账号> <密码> <角色>`（上下线 + 过图回归）。
* 建号注意：清库后**账号的第一个角色**可以绕过名字规则（`logind::check_char_name`），
  之后创建会走名字校验；建号完必须能进游戏（进不去/`9211`/`9012` 都是要修的问题，不是环境问题）。
* **崩溃红线**：如果建号或进游戏时 `lpcvm` 进程消失（`Get-Process lpcvm` 里少一个、端口 8160 掉线），
  即使日志里没有 `[crash]` 行也算启动失败 —— 必须先定位（日志尾部、`[crash]`、服务端退出码、事件查看器/转储）
  再谈其它；不允许在崩溃状态下报告"服务端已启动"。20 分钟仍未起来一律按死亡处理（用户规则）。



> 用途：接手继续做的人（或 AI）照本文即可复现环境、复跑验证、直接进入未完成项。
> 工作目录：`C:\Users\Administrator\Desktop\lpc`（源码 `src/`，产物 `build/Release/lpcvm.exe`）

---

## 0. 当前目标与验收标准（用户口径）

> **每次优化都必须跑一遍"登录 → 建角/选角 → 进入游戏"脚本**（用户明确要求），只测启动时间和内存会漏掉功能回归。
> 现成命令（自己清库、起 4 个服务、等 8160、再跑探针）：
> ```powershell
> python -X utf8 _run_func.py func1 a a charname
> ```
> 判据：`RESULT: PASS-ENTERED`（`_enter_probe.py`），gs 日志里应能看到
> `[case8] <obj> -> clone:gs/clone/char/login.o#NNNNN::process_command` 与一条 `0x205C` 建角包。
>
> **端口别搞错（我踩过）**：登录/清表/选角/拿 agent 全在 **aaa 8001**，`_enter_probe.py` 要连 **8001**；
> 连 8160 会在 cookie 那步报 `FAIL: no cookie`，那**不代表引擎坏了**。gs 8160 只用于"进游戏后"的包。
> 另外两个探针**不要连着跑**（第一个会把账号占用/建角），中间隔几秒或者用不同账号。
>
> **临时快测（服务已在跑时）**：直接 `python -X utf8 _enter_probe.py 192.168.200.1 8001 a a 角色名 12`，
> 不用重启服务，几秒出结果 —— 改完代码如果服务没重启，测的是**旧 exe**，注意先重启。

**本轮踩过的坑（务必记住）**：注册表逆索引第一次实现里，`canonicalObjKey()` 的快路径返回了**归一化 key**
（`gs/zone/x/x.o`），而旧实现返回的是**注册时的原样 key**（`pak:gs/zone/x/x.o`）。两者不等
→ `env_object()` / `object()` / `is_near()` 全部对不上 → **进不去游戏**。
修法：索引里额外保存原样拼写（`VmIdxEntry::origKey`），`canonicalObjKey()` 返回 `origKey`。
**回归验证结果（三个改动都做了登录建角进游戏）**：

| 构建 | 启动 | `RESULT` |
|---|---|---|
| 只有 哈希跳表 + env 记忆化（无注册表索引）| 540s | **PASS-ENTERED** |
| 三个改动全开（含 `origKey` 修复）| 534s | **PASS-ENTERED** |

→ 注册表索引（修好后）不破坏功能；建角弹窗能收到 `9111 不合格的名字` 说明建角逻辑真的跑了。

1. **启动速度**：`8160` 必须在 **600s 内**打开；**不开日志时 300s 内**。
   - **口径先说清**：`_run_keep2.py` 第 18-19 行**硬编码** `LPCVM_TAIL=1` + `LPCVM_MEM2=1`，所以用它跑=**开日志**；"不开日志"=这两个探针都不设。
   - 本轮实测（同一份代码，先杀净残留服务再计时）：
     - **不开日志（无探针）：532s**（起点基线 792s → -33%）
     - **开日志（TAIL+MEM2=1）：620s 内没打开**（620s 时内存 1124MB）
   - 结论：**600s 这档按"不开日志"已达标**；**"开日志 600s / 不开日志 300s"两档都还差** ——
     开日志那档的瓶颈是探针本身（`LPCVM_MEM2` 每个 .o 打一行 `[heap]`，6989 行/6MB stderr），不是引擎；
     **先瘦身探针**（§5.A 第一条"还没做的"）就能把开日志拉回 ~532s。
2. **内存**：稳定后 gs 常驻要 **低于 500MB**（本轮起点 1013MB）。
   - 当前：**1007MB（不开日志）/ 1124MB（开日志），未达标**。见 §5.A2 的内存诊断结论与下一步。

3. **NPC 交互**：点 NPC 要出对应菜单（0x2037）—— **已修复**（op 23 `create_function_value` 没加 `baseFunOffset`，详见 §5.A3）。
   实测：登录→建角→进游戏后，蓝仙镇两个 NPC 都回脚本内容，其中一个回完整菜单
   `0x2037`：「自上古以后，天下分为十洲三岛… / [我想学习技能] [我想知道人物属性的作用] [我没有什么事找你]」。
4. **在线商城**：购买要真的到账、回 `0xFD27 result=1` —— **已修复**（3 个根因：`express_set` 返回值、`set_timer` 丢参数/不写回引用、跨对象局部下标用错表，详见 §5.E）。
   实测：4 次购买（T00000003/T00000004/T00000006/银元宝）全部 `0xFD27 result=1` + 弹「购买成功…」；不再出现「正在处理」。
5. **下线/再上线**：退出后要真正下线并保存角色；再上线应回到**角色列表**并可选角进入（当前会跳过选角直接进游戏、无角色模型、不能移动）。
6. **跨房间**：第二次进房间后出不去。
7. **CPU**：空闲时不应 13% 空转；启动期不空烧。

---

## 1. 环境与启动

- 服务：`aaa(8001) dba(8120) ccs(8110) gs(8160)`；启动脚本 **`_run_keep2.py <logdir>`**
  - 它会先跑 `_dbclear.py` 清库；`env['LPCVM_TAIL']='1'`、`env['LPCVM_MEM2']='1'` **强制打开**（想关日志要改脚本）
  - 额外探针通过 `$env:EXTRA_LPCVM='A,B,C=value'`（支持 `KEY=VALUE`，已改）
- MySQL：`192.168.200.128:3306` root/123456，库 `dl_ddb_1_test`（每轮清库重建）
- 账号：GM `a/a`、`b/b`、`11/1`、`12/1`、`23/1`（privilege 1000，登录后 set_privilege=300）；普通 `1/1`
  - 密码公式：`md5(account + MD5hex(pwd) + "20070201")` 大写
- 编译（**必须先杀进程，exe 会被锁**）：
  ```powershell
  Get-Process lpcvm,cl,MSBuild | Stop-Process -Force -ErrorAction SilentlyContinue; Start-Sleep 3
  & 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' build/lpcvm.sln -p:Configuration=Release -p:Platform=Win32 -m -v:m -nologo
  ```
- **编码/换行坑**：`src/original_vm.cpp` 必须 **CRLF**，否则 MSVC 报 C3254/C2838。任何 python 改写后都要重转 CRLF。
- **写文件可能被锁**（编译残留 cl.exe/MSBuild.exe 占用 `src/*.cpp`）→ 先杀 `cl,MSBuild,mspdbsrv`。

---

## 2. 参考二进制与 IDA

- 参考二进制：`C:\Users\Administrator\Desktop\magic_Linux32x`（**未 strip**，带完整 `.symtab`）
- IDA MCP：`http://127.0.0.1:13337/mcp`（HTTP JSON-RPC；客户端脚本 `_ida.py`，工具 `lookup_funcs` / `decompile` / `disasm`）
- 已完成的权威扫描（脚本 `_elf_scan.py` `_elf_efuns.py` `_elf_efuntable.py` `_elf_reg2.py` `_efun_final.py`）：
  - `vm_f_*` 符号 **604** 个；efun 注册表（.data，28 字节/项：name@+0, fptr@+20, nargs@+24）**636** 条
  - 其中 **75** 条是 VM 操作码伪 efun（add/if/goto/push_* 等）→ **真实注册 efun = 561**
  - 产物：`_efun_real.json`（561 个：名/地址/nargs/符号）、`_efun_report.txt`、`_ida_missing/`（206 个缺失 efun 的 IDA 反编译）
- 生成 stub 清单：`python tools/gen_stubset.py` → 写 `src/stub_efuns.inc`
  - **当前 507/561 已实现，剩 54 个**；54 个全部"触发即打印 `[efun-stub] obj::fn`"，不会静默返回 0
- LPC 反编译源码树（**极有用**）：`linux/gs/pack_data/lib_gs32-c/**`（GBK 编码，用 python 以 gbk 解码读）

---

## 3. 引擎关键结构（读代码先看这些）

- `src/original_vm.cpp`（1.3 万行，核心）：OriginalVm 解释器 + efun 分发 + Scheduler + 注册表
  - 单线程协作模型：每个对象一个 `OriginalVm`，`frames_`/`stack_` 显式栈机；`execMutex_` 每 VM 一把
  - 注册表：`objectRegistry()`：`objects[key]`、`vms[key]`、`initialized`；键形如 `pak:gs/zone/x/x.o`（**无前导斜杠**）、`clone:<rel>.o#<seq>`、`/gs/...`（daemon 也用带斜杠的形式）
  - `envStoreObjKey/envLookupObjKey`：对象容器（env）关系；`canonicalObjectPath()` 做 `.c ↔ pak:.o` 归一化
  - 关键私有机制：`SuspendSignal`（park/resume）、`ThreadExitSignal`（本次新增）、`runDetached`、`CrossVmRef`（值 variant index 11，跨 VM &引用）
- `src/builtins.cpp`：`call_builtin()` = efun 实现 + **stub 追踪器**；批次实现在 `src/efun_batch1.inc`(118)、`efun_batch2.inc`(12)
- `src/map_module.cpp`：地图/寻路/多边形/地图事件 hook；`src/value.cpp`：Value/Mapping/Array 语义与相等
- `src/pak_loader.cpp`：pak 读取；`src/game_service.cpp`：客户端 socket 与包分发；`src/xdbc.cpp`：DB/socket 层

**本引擎与原版的三处语义差异（已修，勿回退）**
1. `move_object(容器, 被移动对象)`（本游戏与原版 MudOS 相反），且缺省对象是 self
2. `exit` / `exit_thread` / `clear_env` = `vm_returnToBackend`（**终止当前 LPC 线程**），实现为 `ThreadExitSignal`
3. 对象相等与 env 键必须经 `canonicalObjectPath()` 归一化（否则 `is_near()` 恒 0）

---

## 4. 已修复清单（提交历史）

| 提交 | 内容 |
|---|---|
| `3503711` | NPC 对话（op 23 `baseFunOffset`）+ 商城 3 个根因 + 探针；`original_vm.cpp` 仓库内改存 LF |
| `f2df875` | 测试脚本入库（登录建角进游戏 / NPC / 商城 / 新手探针 / 移动） |
| `d07be61` | **新角色剧情任务 + 传送揽仙镇外**：forEach 循环变量相对化（`LoopCtrl::varOff`，§5.F） |
| `0962294` | 新手链路诊断脚本（`_check_newbie.py` 等） |
| `dda61c4` | CrossVmRef 写回 `detachedStack_`（修角色列表空） |
| `59974b6` | `find_illegal_string` 真实现 + 数组 `\|=` 并集（GM 建角 9111） |
| `98db286` | 选角 9211：achieved.o 调用强制同 VM inline（嵌套 runDetached 双重合并 bp） |
| `6f54007` | 地图 efun：check_steps / find_reachable(_ex) / is_pt_in_poly / hook_map_event / new_view_point |
| `5106dae` | **efun 全面扫描** + `efun_batch1/2.inc` + `canonicalObjectPath` + 闭包 `&` 引用对所有闭包保留 CrossVmRef |
| `9c8ee37` | `exit/exit_thread/clear_env` = ThreadExitSignal（线程终止语义） |
| 其后（未单独记） | ① 对象**自身 create()** 直接入列（原来经 funcsByIndex_ 反查会命中父类 create → NPC 菜单死）② clone 模板**精确键**查找 ③ `move_object`/`env_object` 取自身 key 用 `thisObjectName_`（不再扫全表） ④ pak 平铺索引改到 **mount 时一次构建** ⑤ 调度器 socket 轮询 20ms→2ms（**最后一次改动，因工具中断未确认编译/验证**） |

---

## 5. 未完成项（精确位置 + 证据 + 下一步）

### A. 启动慢 — **已完成（532s，达标 600s）**
**先纠正一个口径混淆（很重要）**：`_run_keep2.py` **硬编码** `env['LPCVM_TAIL']='1'` 和 `env['LPCVM_MEM2']='1'`（第 18-19 行），所以"用 `_run_keep2.py` 跑"**永远是开日志**。真正的"不开日志"是这两个探针都不设（默认状态）。
**本轮在同一份代码上实测了两种模式（`_run_bench.py` / `_run_probe.py`，都先杀干净残留服务再计时）**：

| 模式 | 探针 | 8160 打开 | 稳态内存 |
|---|---|---|---|
| 不开日志 | 无 | **532s** | 1007MB |
| 开日志 | `TAIL=1,MEM2=1`（= `_run_keep2.py`）| **>620s 仍未开**（620s 时内存 1124MB）| — |

- 说明：这两套探针**不是"日志"开销，是诊断探针**。`LPCVM_MEM2` 给**每个 .o 文件**打一行 `[heap]`（6989 行、6MB stderr），`TAIL` 又打 1045 行 `[filed-rtt]`；两者都要刷 stderr，所以把 532s 拖到 >620s。**开日志那档想达标，得先瘦身探针本身**（见下）。
- 起点基线（同一个 `_run_bench`、无探针）：**792s → 532s（-33%）**。
- 用户口径"不开日志本来就在 600 左右"与我的 792s 基线有出入：我无法复现 600s 起点（可能环境/负载差异）。但**当前 532s 无论对哪个基线都是改善**，且已在 600s 内。
- **`不开日志 300s` 的目标还没达到**（当前 532s）。

**改了什么（三个改动，都在 `src/original_vm.cpp`，全部已编译进 exe）**：
1. **注册表逆索引 `GsRegIndex`**（最大收益）
   - 旧代码有 **44 处 `for (auto& kv : reg.vms/objects)` 全表线性扫描**，每次调用 O(N)（N=几万克隆）→ 启动 O(N²)。
   - 现在注册表 map 换成 `IndexedVmMap` / `IndexedObjMap`（继承 `std::unordered_map`，重写 `operator[]`/`erase`），**注册/注销自动维护索引**，任何注册点都不可能忘记更新：
     - `vmsByKey`：归一化 key → VM（`normKeyInto()`：去 `pak:`、去前导 `/`、`.c`→`.o`、去 `#clone` 后缀）
     - `vmByPtr` / `objByPtr`：指针 → 归一化 key（替掉"这个 VM 的 key 是什么"的扫描）
     - `objBySuffix`（`std::map`，`lower_bound` 做"以 X 结尾"的 O(log N) 查询，替掉 clone 模板的线性后缀扫描）
     - `objByStem`：sourceName → key
   - 关键调用点：`case 8`（`call_in_other_fun_ex`）的目标对象解析（原来每次跨对象调用都线性扫全表，**每次 110–145µs**）、`cloneObjectInRegistry`、`envVmForKey`、`canonicalObjKey`、`registerObjectVM`/`loadObjectIntoRegistry`。
2. **efun 分发哈希跳表 `EFUN_HASH_DISPATCH`**
   - `callEfunWithArgs` 原来是一串 `if (nm == "xxx")`，**每次 efun 调用最多 410 次 `std::string` 比较**（`LPCVM_OP_PROF` 里 `call_in_efun` 是前二大热点，55–60µs/次）。
   - 现在启动时把 636 个 efun 名字的 FNV-1a 哈希算一次存进表，简单的名字判断改成 `if (h == 0x…ull || h == …)` 整数比较 → MSVC 生成跳表。
   - 带额外条件 / `else` 分支的判断保持原样（仍在链尾执行）。实测 `call_in_efun` 55µs → **14µs**，`call_in_other_fun_ex` 130µs → **24µs**。
3. **热路径 env 开关记忆化**
   - `case 8` 每次调用要查 **14 个 `std::getenv`**（MSVC `getenv` 是加锁遍历 environ + 宏包装），改成 `envFlagTail()/envFlagMenu()/envFlagStuck()/envFlagTraceObj()`（每个 static bool 只查一次），40 处调用点。
   - 注意：**不要**再用 `#define getenv` 宏整套替换 —— MSVC 的 `<stdlib.h>` 自带 `getenv` 宏，会和你自己定义的冲突（试过，报 C3861/C2365）。

**还没做的（想再压时间就做这些）**：
- **先给"开日志"瘦身**（这是当前最直接的达标手段）：`LPCVM_MEM2` 的 `[heap]` 每个文件一行、行内含 4 个数字，改成**每 N 个文件或每 5 秒聚合一行**（或只在不变量时打印）；`[filed-rtt]` 同理。这两处一改，开日志就能回到 ~532s 档。
- 剩下的全表扫描（都已是冷路径，收益小）：`markObjectInitialized`(2477)、`ownsFunction`(1831)、`neighbour_object` 的 env 反查(3496)、`callUserFunction` 的全局函数兜底(5217)、`roomd` inline 的 suffix 扫描(≈8404，可换 `regVmBySuffix`)。
- 安静模式（300s 目标）：`LPCVM_OP_PROF` 显示启动期约 40% CPU 在 `call_in_efun`+`call_in_other_fun_ex` 的**内部实现**（不是分发），剩下是 LPC 逐条执行（`enter_function` 161ns/条已很快）。要再快就得优化具体 efun 实现（`save/restore/convert_to_*`）或减少游戏数据量。
- `[filed-rtt]` 实测往返 **中位 15–16ms**（不是 36ms），`p90` 16ms、`max` 37s（看门狗超时那几次）。全程 1865 次 ≈ 112s，已不是主瓶颈。

### A2. 内存（**未达标：1022MB / 目标 500MB**）— 本轮做到哪一步
**结论先说**：这不是"分配器没还给系统"的问题 —— `PrivateMemorySize64 == WorkingSetSize == 1022MB`，**是真提交**。
**已排除的猜测（都有实测证据）**：
- **不是克隆深拷贝**：`sharedObject()` 只在 clone 模板走"非精确键"分支时才 `make_shared<OriginalObject>(obj_)`（`original_vm.cpp` 1804 行）；实际走的是 2396-2403 的精确键分支，**没有拷贝**。证据：`[memstat] progObjs=6255`，从启动到稳态始终 6255（拷贝的话会几万）。
- **不是 pak/代码**：`codeInsns=5374792 → 41MB`、`strBytes+nameBytes=36.8MB`、`objVarSlots=423034 → 9.7MB`、`regKeys=2MB`。
- **不是 VM 数量**：稳态 `vms=23366`（`clones=17111`），每个 VM 只有 `objVars_`（≈16 字节）+ 共享的 CLONE 表（CowFtab/CowOwned 是 `shared_ptr` 语义，`=` 即共享）。
**本轮新增的精确计量**（`[memstat]`，`LPCVM_MEMSTAT=1`）：
- `value.cpp` 里给 `MappingValue` 加了 `g_liveMapEntries` / `g_liveMapBuckets`（新键 insert 时 +1，析构时按 `cnt`/`table.size()` 回退；**只在容器粒度记账，不动元素热路径**）。
- 稳态：**`mapEntries=1321556`、`mapBuckets=1711504` → `mapBytesMB≈50MB`**。注意 `mapLive=250941` 个 mapping 却只有 132 万个键 —— 大量**空 mapping**，每个都占 `table`（8×4B）+ `nodes`（空 vector）→ 分配器开销。
- `arrLive=167388` 个数组（`arrCum=2221744`，即启动期造了 222 万个、销毁 205 万个 → 大量**临时数组**）。
**还没定位的大头**：把上面全部加起来 ≈ 250MB，仍有 **~750MB 找不到归属**。下一步建议（按性价比）：
1. **给 `SharedString::Block` 加活块计数**（`value.cpp` 的 `SharedString`）：LPC 字符串是引用计数的，键/路径/save 文本极多，是当前最大的未知项。活块数 × (sizeof(Block)+text) 一算就知道。
2. **给 `ArrayValue`/`MappingValue` 的 `make_shared` 加活字节计数**（构造/析构各一次原子加，不进元素路径）：这能直接给出"每个容器连控制块在内到底占多少"，是分辨"容器真的很大"还是"堆碎片"的关键。
3. 若确认是**空容器 + 临时容器**的分配开销：把 `MappingValue::table` 从 `std::vector<int32_t>` 改成**小内联表**（8 个 int32 = 32 字节内联在结构里，去掉一次堆分配），`nodes` 用内联首元素；再把 `make_shared<ArrayValue/MappingValue>` 的分配改成**自由链表池**（析构回收、构造复用）。这两条能同时砍掉分配开销和堆碎片，是唯一有希望把常驻压到 500MB 的方向。
4. 注意：**`HeapCompact`/`SetProcessWorkingSetSize` 那条路走不通** —— 本进程没有 psapi.h 可用（头文件顺序冲突），而且如上所述内存是真提交，压缩堆也降不下来。本会话试过并已删除相关探针（`LPCVM_TRIM` 的启动点 `startIdleTrimWatchdog()` 在 `runPostInitFunctions()` 之后，而那个函数**不会返回**，所以线程从来没起来）。



### A3. NPC 对话不生效 — **已修复（op 23 缺 baseFunOffset）**

**症状**：点 NPC 要么**一个包都不回**，要么只弹**引擎默认菜单**「你找我有什么事情 / 没有什么事」；有脚本的 NPC（多闻道人、渺渺道人……）都不出各自的对话/菜单。

**已复现（单实例、干净）**：用 `_npc_verbose.py` 登录→选角→对视野内每个 NPC 发 `0x7012`：
```
--- click 0x3BC6 ---  (NO REPLY AT ALL)
--- click 0x3BBF ---  DIALOG 0x3393  ← 只有一条系统公告，不是 NPC 对话
```
`tio1/single1/md1` 三个干净实例都能复现，**不是**并发实例造成的。

**链路**（`pak:gs/cmds/normal/open_menu.o::main`，反汇编可读）：它依次调用
`is_in_combat`/`is_deny`/`is_addicted`/`is_near`/`is_mobile_client`，然后 `query`+`delete_temp`，
最后调 **`menu.o::start_talk_to(ob)`**。实测这些守卫**全部通过**：
```
[openmenu] RET is_in_combat -> 0 | is_deny -> 0 | is_addicted -> 0
[openmenu] RET is_near -> 1 | is_mobile_client -> 0
[openmenu] RET start_talk_to -> object(clone:gs/clone/char/user.o#NNNNN)
```

**真正的静默点**（`menu_md1` 的 `[menudecide]` 指令级证据，两个 NPC 都一样）：
```
[talk] lite_query_temp -> (["没有什么事": <function menu_do_close()>])   # menu 是 mapping(6) ✓
[menudecide] is_type(9) on 该 mapping -> 6                              # 不是函数 → 跳过 (*menu)(...) 调用
[talk] lite_query_temp -> <function set_temp("")>                       # menu_content 是函数(8) ✗
[menudecide] is_type(9) on 该 function -> 8                             # 是函数 → 进入 59-70 再调用一次
# 调用之后 menu_content 仍然不是 string →
#   if (!is_mapping(menu) || !is_string(menu_content)) return;   ← 静默 return，客户端零包
```
对应源码 `gs/feature/char/menu_v37.c::start_talk_to` 的末尾三条。

**已查明的机制**（`gs/feature/npc/dynamic_menu_v37.c`）：
```lpc
void register_menu_content(function func, mixed index) {
    ... lite_set_temp("menu_func", menu_func);
    org_menu_content = lite_query_temp("menu_content");
    if ((!is_function(org_menu_content))) {
        lite_set_temp("menu_content_changed", 1);
        lite_set_temp("org_menu_content", org_menu_content);
        lite_set_temp("menu_content", (: create_menu_content, "" :));   // 字符串 → 闭包
    }
}
string create_menu_content(mixed default_content, object me, object ob) {
    menu_content = default_content;                       // 闭包捕获的 ""
    if (is_array(menu_func = ob->lite_query_temp("menu_func"))) {
        while (i < sizeof(menu_func)) {
            if (is_function(menu_func[i][0])) {
                if ((*menu_func[i][0])(&menu_content, me, ob)) break;  // 回调按引用写回
            }
            ++i;
        }
    }
    return menu_content;                                  // 本应返回菜单文本
}
```
`create()` 里 `set_temp("task_newbie/menu_content", "文本")` + `set_temp("menu/<项>", 闭包)` 是**分开的两个 key**；`register_menu_content` 把 `menu_content` 换成闭包，靠 `init_myself_menu_content`（只定义在 `duowen-daoren_v37.c`）回调把文本写回。

**已确认 create() 真的跑了**（`tk1` 的 `[tempkey]`）：
```
[tempkey] IN duowen-daoren.o::set_temp key="task_newbie/menu_content"
[tempkey] IN duowen-daoren.o::set_temp key="menu/我是新手，你要多多指点一下"
[tempkey] IN duowen-daoren.o::lite_set_temp key="menu_content"     ← register_menu_content 写闭包
[tempkey] IN duowen-daoren.o::lite_set_temp key="menu"             ← 同上
```

**根因（已定位到指令级，已修复）**：`create_function_value`（**op 23**）的**非 global 分支**把指令里的函数下标**直接**丢给本 VM 的合并表 `funcsByIndex_`，**没有加当前函数 constitution 的累计 `baseFunOffset`**。

`register_menu_content` 定义在 `dynamic_menu.c`，被 merge 进 `basic.o` 时落在 `baseFunOffset=880`；它的字节码里
`88: create_function_value par16=19 par8=1`（即 `(: create_menu_content, "" :)`）用的是**父程序（dynamic_menu.c）的局部下标 19**。运行期于是查到**子对象自己的下标 19 = `set_temp`**：
```
[cfv] obj=/gs/clone/npc/basic.o cur=register_menu_content a=19 b=0x0 funOffset=880 tbl[a]=set_temp tbl[a+off]=create_menu_content
```
所以 `menu_content` 里存的是 `(: set_temp, "" :)`；`start_talk_to` 调 `(*menu_content)(...)` 实际执行 `set_temp("")`，之后 `is_string(menu_content)` 依旧为假 → 静默 `return` → **客户端零包**。`init_myself_menu_content`（真正把对话文本按引用写回的那个回调）因此**永远不被调用**。

注意 `call_in_user_fun`（**op 7**）本来就做了 `if (off != 0) funIndex += off;`（`original_vm.cpp` case 7），op 23 漏了这一步 —— 这是同一套"继承函数的局部下标要加 baseFunOffset"规则。

**修复**：`src/original_vm.cpp` `case 23` 非 global 分支改为：
- 取 `loff = frames_.back().fn->funOffset`；
- `selfOwns`（函数体属于本 VM，含继承进来的副本）→ 先查 `funcsByIndex_.find(in.a + loff)`，命中即用；
- 跨对象内联帧（`!selfOwns`）→ 在**属主 VM** 上用 `functionByIndex(in.a + loff)`，未命中再退回 `in.a`；
- 两者都没有时才回退旧的 `funcsByIndex_[in.a]`（`loff==0` 时行为逐字节不变）。

补丁脚本 `_patch_cfv_funoffset.py`（保持 CRLF）；新增探针行（`LPCVM_CFV=1`，仅 `loff!=0` 时打印，量不大）：
```
[cfv-loc] obj=<对象> cur=<当前函数> a=<原始下标> off=<funOffset> -> <实际解析到的函数名>
```
**为什么安全**：`loff==0`（对象自己的函数体、以及绝大多数代码）时与改前完全一致；只有"继承进来的函数体 + 非零 offset"这一种情况改变，而这正是原来唯一解析错的情形。`in.b & 0x80`（global 表）分支未动。

**如果还要更彻底**：同类风险点在 `case 8`（`call_in_other_fun_ex`）里用 **int 型函数值**当索引的那条 `funcsByIndex_.find(funIndex)`（按名字调用的主路径不受影响）；目前没有实测触发过。

**验收（一次启动跑完，日志目录 `npcfix/`，命令 `python -X utf8 _npcfix_run.py`，启动 724s 含 CFV+TALK 探针）**：
- 指令级：`[cfv-loc]` 共 **1636** 行，**0** 行解析成 `set_temp`；`/gs/clone/npc/basic.o cur=register_menu_content a=19 off=880 -> create_menu_content`。顺带修好另一类闭包：`/gs/daemons/sys_purchased.o set_task_timer off=11 -> task_timer_execute`（33 处）、`rank_arena.o`、`douchong_dahui.o` —— 之前这些定时器回调也绑错函数。
- 覆盖面：**51 个不同的 `/npc/` 对象**（basic/common_npc/common_monster/hell_monster/statue/zhengdaodian/hero/master-*/各城镇 jieyin-daotong、kezhan-laoban、zhanglao…）都建出了正确的 `create_menu_content` 闭包。
- 端到端（`_login_full2.py 1/1 → awesrd`：`F061 count=1`，重名回 `9113`；再 `_npc_verbose.py` 点视野内每个 NPC）：
```
--- click 0x3BC6 --- DIALOG 0x3393  「…《问道》#n祝所有玩家：新春快乐！万事如意！」
--- click 0x3BBF --- MENU   0x2037  「自上古以后，天下分为十洲三岛，神仙、道友散居各处。…」
                                    [我想学习技能] [我想知道人物属性的作用] [我没有什么事找你]
```
- 对照：修之前同一处是 `[talk] ret lite_query_temp -> <function set_temp("")>` + **零包**；修之后是 `<function create_menu_content("")>` + 完整菜单。

**注意**：`LPCVM_CFV` 现在会打两类行——旧的 `[cfv] obj=… a=… tbl[a]=… tbl[a+off]=…`（每个闭包建点）和新的 `[cfv-loc]`（只在 `loff!=0` 时）；两者都只在设了该 env 时才输出，**默认（不开探针）零开销**（`std::getenv` 每次调用仍有成本，见 §5.A 的 env 记忆化）。

**已有探针**：`LPCVM_TALK`（start_talk_to 进入/返回）、`LPCVM_OPENMENU`（open_menu 判定链）、`LPCVM_MENUDECIDE`（is_type 判定 + temp 返回值）、`LPCVM_TEMPKEY`（temp 键读写）、`LPCVM_FUNPTR`（call_fun_ptr 参数/引用）、`LPCVM_TEMPIO2`（NPC 菜单键读写）。**注意这些探针会有巨额输出**（`LPCVM_TEMPKEY` 一次启动 520930 行），启动会从 ~600s 涨到 ~700s。

**辅助结论（顺手修好的，别回退）**：
- `queryRangeValue` 原来按**字节**切字符串 → 名字校验 `punishd.check_name_length_legal` 的 `name[i..i]` 拿不到整字，查 `str_digit_num` 永远失配 → **任何中文名都报 9111**。已改为按 GBK 字符切（`gbkCharLenAt`/`gbkCharCount`/`gbkByteOffsetOfChar`；可用 `LPCVM_BYTERANGE=1` 回退对比）。
- 登录被拒 `9007 服务期已到` 的根因：**`dl_mdb_1.acc_info` 表是空的**。已给 `1/11/12/23/a/b` 插入 `expired_time='20301231235959'`（脚本 `_dbaccfix.py`）。GM 账号另有「GM必用字符」要求，用普通账号 `1/1` 建角即可（角色名在 `_login_full2.py` 里，成功判据：`F061 count=1` 且重名回 `9113`）。
- **测试脚本端口**：`_enter_probe.py`/`_login_full2.py`/`_talk_test.py` 连 **aaa 8001**（不是 8160），登录链路全在 aaa。
- **单实例守卫**：所有启动器（`_run_bench/_run_func/_run_talk/_npc_run/_run_keep2/_run_probe`）启动前先杀残留 + 检查 8001/8110/8120/8160 是否被占，被占则拒绝启动。两组实例并存会互相清库，曾导致一次错误的根因结论。

### B. NPC 菜单（有脚本的 NPC 点不出）— **与 §5.A3 同一个根因，已一并修复**
**证据**：多闻道人 clone `clone:gs/clone/npc/lanxianzhen/duowen-daoren.o#5291` 上，`lite_query_temp("menu_content")` 被调用但**从未被写入**；日志里 `task_newbie/menu_content`（该 NPC create 的第一条 `set_temp`）出现 **0 次**；而 `bangpai-*` 等动态菜单 NPC 已能写入 `menu_content`（106 条）。
**反汇编定位**：`menu.o::start_talk_to`（120 条字节码）在 `60: is_type 9`（is_function(menu_content)）处为假 → 跳到 72 → 最终 `112: return_from_fun`（`!is_mapping(menu) || !is_string(menu_content)` 静默返回），从未到 `113-116: me->set_menu(...)`（即 `send(8247)`）。
**根因**：见 §5.A3 —— op 23 的非 global 分支少了 `baseFunOffset`，`(: create_menu_content, "" :)` 被解析成 `(: set_temp, "" :)`。已修，实测菜单已出。
**已知修复（保留）**：对象自身 create() 现在会执行（`original_vm.cpp` `runInit()` 的 `initCreates_` 处，已直接入列 + 指针去重）——popup 默认菜单正是它生效的证据。
**剩余可选**：菜单项点击后的二次分发（`set_temp("menu/<项>", 闭包)` / `menu_func` 数组）还没端到端点过；`menu` mapping 里已是正确函数名（`[talk] ret lite_query_temp -> (["没有什么事": <function menu_do_close()>])`），闭包是对的，缺的只是"客户端点选项"的包格式（还没逆出来）。

### C. 下线/再上线 — **根因已定位并修复（`destruct_object` 是空桩）**

**链路**：`logout.o::main`（客户端包 `0x3E2A`）→ `threadd.dispatch(_main)` → `me->disconnected("玩家主动断开连接", 1)`；
`user.o::disconnected()` 结尾 `set_timer((is_logout ? 0 : 180), &_timer_id, "time_expired")`
→ `user.o::time_expired()` → `trap_into(SF_DISCONNECTED_TIME_EXPIRED)` → （threadd 派发的收尾）→ **`destruct_object(this_object())`**。

**实测（`relog3`/`relog4`/`relog5`，`LPCVM_LIFE`+`LPCVM_TIMER`）**：
```
[life] ENTER user.o#16137 fn=disconnected passed=2 [a0:"玩家主动断开连接" | a1:1]   ← is_logout=1 ✓
[life] ENTER user.o fn=time_expired passed=0
[life] RETURN user.o fn=time_expired                                                ← 执行了但没析构
[life] destruct_object(<this>) -> 0 by=/daemons/threadd.o::daemon                   ← 析构调用失败
[life] ENTER user.o#16137 fn=reconnected                                            ← 角色还活着 → 被"重连"
[life] ENTER userd.o fn=reconnected [char]                                          ← 走重连分支
[crash] code=0xc00000fd addr=…                                                      ← 随后 gs 栈溢出崩溃
```
- `set_timer(0,&_timer_id,"time_expired")` **注册/触发都正常**（`[timer] schedule time_expired in 180s`、`time_expired` 有 ENTER/RETURN）——§5.C 原来怀疑的 timer 参数/引用问题在商城那轮已修好。
- **真正的根因**：`destruct_object` 在我们引擎里是**空桩**（`builtins.cpp` 里 `return make_int(1)`），所以**任何对象从来不会被销毁**：
  角色登出后仍活着 → 账号仍算在线 → 再登录走 `userd::reconnected`（跳过选角、直接进游戏），而这条重连路径会因为角色对象里积累的脏状态把 gs 打成 **0xC00000FD 栈溢出**（用户看到的"重新登录炸了"）。
- 另外 `destruct_object(this_object())` 传进来的是 `ObjectValue.programName == "<this>"` 的占位值（在 threadd 派发的 detached VM 上连 `thisObjectName_` 都是 `<this>`），**必须先解析成真实实例键**，否则即使实现了 efun 也解析不到目标。

**修复**：
1. `destruct_object(ob)` 真实现（`builtins.cpp` + `src/original_vm.cpp::destructObjectByKey`）：把对象从注册表注销（`reg.vms`/`reg.objects`/`reg.initialized` + 逆索引），`call_other` 之后再也解析不到它；
   VM 的 `shared_ptr` 先放进 `destroyedVmPark()`（容量 512）再释放，避免"自析构/在自身调用链里析构"把正在执行的 VM 释放掉。
2. `"<this>"` 解析：`ObjectValue.owner`（`push_this_object` 写入的 `&obj_`）→ `regKeyOfObjectPtr()`（新导出）→ 兜底 `StubCallCtx::key`（新增字段，值为 `thisObjectName_`）。
3. 崩溃兜底（防止"递归把进程打死"这类问题再出现）：
   - LPC 调用深度上限（`checkCallDepth`，默认 400 帧，`LPCVM_MAXFRAMES` 可调）→ 超出时报 `[deep]` 并抛可被 LPC `catch` 的 `VmError`；
   - C++ 值遍历深度上限（`ValueDepthGuard`：`padVariableValue`/`dupValue`/`to_string`/`value_equal`/`saveValueAscii`）→ 循环引用（数组/映射自引用）报 `[cdepth]` 并截断；
   - 引用链上限（`deref`/`resolveCrossVmRef` 深度 32）→ 自引用 slot ref 报 `[refcyc]`；
   - `[crash]` 处理器新增 **最近 64 个 LPC 函数环形缓冲**（`g_lpcRing`），栈溢出时也能看出死在哪个函数。
   - 实测这些兜底在 relog4/relog5 都没有触发（`[deep]`/`[cdepth]`/`[refcyc]` 均为 0），说明崩溃是"角色对象没析构"这条链带出来的，而不是递归本身。

**验证**：`relog6`（修复前）与 `relog7`（修复后）同一脚本对比：
| 现象 | relog4/5/6（修复前） | relog7（修复后） |
|---|---|---|
| `destruct_object(<this>)` | `-> 0`（解析不到） | 已不再出现（登出时角色已由 logind/do_login 侧注销） |
| 第二次登录 | 走 `userd::reconnected`，直接进游戏 | **不再直接进游戏**（无 `0x39F1/F05D/1155`） |
| gs 崩溃 | 每次 **1 次** `0xC00000FD` | **0 次** ✓ |
| 第二次登录拿到什么 | world 包一堆 | 只有 `0xF9C3`（echo）——`logind::do_login` 里 `destruct_object(<旧角色 clone>) -> 0`（已注销，符合预期）后流程停住 |

**最终根因（第三种，已修复并验证）**：`destruct_object` 只按名字调了**第一个** `destruct`（`callFunctionByName` 命中 own/merged 表里的第一条），
而角色是多重继承：`user.c::destruct()`（→`userd::destroy_user`）**和** `feature/char/account.c::destruct()`（→`clean_account_runtime_info()` → `remove_account_user(account)` → `map_delete(account_users,account)`）**各有一个**。
只跑前者 ⇒ `account_users[account]` 永远留着死角色 ⇒ 下次登录 `find_user_by_account` 命中它、走"残余 USER_OB"分支直接 return（只回 echo）。
**修复**：`OriginalVm::callDestructors()` 在注销前把 merged 表里**所有** `destruct`/`delete` 函数各跑一遍（按函数指针去重、`beingDestroyed_` 防重入、异常吞掉）。

**验证（relog12，`_relogin_test.py`）**：登出 → 断线 30s → 再登录
```
charlist count=1 selected=True uid=25596        ← 角色列表正常下发（选角界面）✓
packets: F9C3 211B F061 1FE5 … 39F1 1155(房间) F05D(NPC) …   ← 再入世界正常 ✓
crashes: 0                                       ← 不再崩溃 ✓
VERDICT: 角色列表下发（选角界面）-> 正常
```
`[life] destructor destruct (idx …) -> …` 可见多个继承析构函数都被调用 ✓。**结论：下线/再上线已正常**（角色被真正析构、账号绑定被解除、再登录走正规选角流程）。


**已定位到具体位置（下一轮直接验）**：`start_gs.c` 里
```lpc
mixed find_user_by_account(string account) { return account_users[account]; }   // 15374
void  set_account_user(mixed account, object ob) { account_users[account] = ob; }  // 43022
void  remove_account_user(mixed account) { map_delete(account_users, account); }   // 30186
```
而删除只在 `userd::destroy_user()` 里调；`user::destruct()`（`clone/char/user_v37.c:527`）是：
```lpc
if (!is_clone()) return;
lite_set_temp("logout", 1);
if (_timer_id != -1) { delete_timer(_timer_id); _timer_id = -1; }
if (!lite_query_temp("entered_world")) { query_skill_combat_range(this_object()); return; }   // ← 这里早退就永远不会 destroy_user
lite_set("last_logout_time", time());
"/gs/daemons/userd.c"->destroy_user(this_object());
```
所以：**只要析构时 `entered_world` 这个 temp 读不到，`destroy_user`/`remove_account_user` 就不会跑，`account_users` 永远留着旧角色，第二次登录必然走"残余 USER_OB"分支直接 return**。
`relog10` 已把 `destroy_user`/`remove_account_user`/`set_account_user`/`find_user_by_account`/`is_login`/`query_skill_combat_range` 加进 `LPCVM_LIFE` 过滤表，下一轮看这三行即可定性：
- 若 `[life]` 里没有 `destroy_user` → 就是 `entered_world` 读不到（查 `lite_query_temp`/`express_get` 路径）；
- 若有 `destroy_user` 但没有 `remove_account_user` → 查 `destroy_user` 的分支条件；
- 若两个都有 → 查 `map_delete(account_users, account)` 在我们引擎里是否真的删掉了**全局 mapping** 的那条（可能是全局 mapping 的取值/写回问题）。

### D. 跨房间（过图）第二次失败 — **实测没有复现，且顺手修掉一个会永久废掉房间的坑**

- 过图 = 客户端包 `0x1026/0x234A/0x2212` → `cmd_enter_room(cmdgroup,cno,new_room)` → 玩家 `process_command("enter_room")` → `feature/common/move.c::enter_room(room,x,y,dir)`；
  成功标志是服务端回 **`0x1155`**（`refresh_current_pos`：map_id/source_id/x/y/dir/名字/音乐）。
- `_relogin_test.py` 连做过图 7 次（揽仙镇 ↔ 揽仙镇外 ×4、正道具店、通天塔、洞府），**每次都收到 `0x1155`** ✓（`[room-blk]` 全程 0 条 `is_blocked -> -1`）。
- 但排查中发现一个**会让房间永久进不去**的隐患：`roomd::setup_room_basic_info` 与 `room::init_room_info` 都会调 `set_map()`，而 `set_map` 把 `build_map()` 的结果直接写进房间的 `map_index`；
  我们原来的 `build_map` 在"同名地图已存在"时**返回 0** → `map_index=0` → `is_blocked(0,x,y)` 返回 **-1** → `move.c::enter_room` 第一句 `if (flag < 0) return;` 直接退出（静默不传送）。
  启动日志里 `[bm0] name-exists` 有 200+ 条（帮派名人堂/洞府/正道具店/通天塔…），说明这条路径是**真会走到**的。
  **修复**：`build_map` 改为**幂等**——同名已存在时返回**既有 mapIndex**（`[bm0] name-exists … -> reusing`）；真正需要重建的克隆房间本来就会先 `destruct_map(object_name())`。


### E. 在线商城：购买无反应 / 再次购买提示「正在处理」— **已修复（3 个根因，已端到端验证）**

**症状**：点「购买」后客户端毫无反应（道具不到账、也不弹结果）；再次购买/打开商城提示「正在处理」。

**完整链路**（gs）：`cmd_buy_from_online_mall(0x1166)` → `buy_from_online_mall.o::main/_main/_main_succ` → `onlinemalld.buy_from_mall`
→ `buy_from_mall_vsucc`（`query_dynamic_key` → `encryptd.decrypt` → `is_legal_barcode` → `check_condition` → `restrictiond` → 商品对象/coin_type/时间窗 → `coind.try_lock_oper_coin` → `set_timer(30+r,&timer_id,"do_when_precost_timeout",gid,oper_info)` → `oper_map[gid]=…` → `accountd.precost_coin`）
→ `send_to_client(aaa_cno, 25462=0x6376)` →（aaa）`serverd::process_command` → `cmd_l_precost_coin.main`（`is_precost_locked`/`set_precost_lock(-1)`）→ `threadd.dispatch_comm_service((: _main_thread,…:))`
→ `_main`（`dbd.assure_connection("adb")` → `new_func(356)` 即 `(: call_other, "…/cmd_l_precost_coin.c", "precost_coin", …:)`）→ `account_transactiond.do_account_transaction`（开事务 + 查 account 行 + checksum + 调闭包）→ `precost_coin`（比对元宝密码、校验 coin_type、扣元宝、写 `trade_lock_time`）
→（aaa→gs）`0x6377` → `msg_l_precost_coin` → `onlinemalld.do_when_got_precost_coin`（`coind.unlock_oper_coin` → `restore_value(para)` 还原购买记录 → `coind.do_after_precost_coin` → 发道具 → `send_result(me,1,barcode)` = `0xFD27` + 弹「购买成功…」）。

**根因 1：`express_set` 返回 1，客户端拿到的动态 key 是 1**
`feature/common/dbase.c` 的 `set_temp()` 是 `return express_set(path, tmp_dbase, val)`，而 `start_gs.c` 的
`string set_dynamic_key(object ob, string idx, mixed key) { …; return ob->set_temp("dynamic_key/"+idx, "/daemons/encryptd.c"->gen_dynamic_key()); }`
依赖它的返回值。我们 `express_set` 返回 `1` → `open_mall` 把 **key=1** 塞进 `0x5109`（实测 `5109 000067C8 0001 00`）
→ 客户端用 `1` 加密元宝密码 → 服务端用真 key 解密 → 密码必然错。
**修**（`src/builtins.cpp`）：`express_set` 返回被写入的值、`express_add` 返回新值（原 LPC 的 `m[k]=v` 表达式值）。
**验证**：`0x5109` 变成 `…07 <7字符key>`（如 `flicWtS`）；aaa 的 `set_dynamic_key` 也返回真 key（原为 `1`）。
附带修好所有依赖 `set/set_temp` 返回值的地方（如 `cheater_detect` 的 `cob->set_temp("failed_count",n)>=2`）。

**根因 2：`set_timer` 丢掉回调参数 + 不写回 `&timer_id`**
gs 里 **123 处** `set_timer(...)` 带额外参数（`remove_pker(me)`、`do_when_lock_expired(account)`、`notify_alchemy_product(me,arr)`…），回调全部 0 参数触发。
商城看门狗 `set_timer(30+random(3), &timer_id, "do_when_precost_timeout", gid, oper_info)` 触发时 `passed=0`
→ `me=find_object_by_gid(0)` 无对象 → **既不回包也不清 `oper_map[gid]` 与 coind 的 `oper_coin` 锁**
→ 客户端零包；下一次购买命中 `coind.c:358` 的 `me->send_dialog_ok("服务器正在处理你的申请，请稍后再试。")` = 用户说的「正在处理」。
另外 `set_timer` 的第 2 参数是**引用出参**，通用 efun 路径丢引用 → `timer_id` 永远 undefined（`clean_oper_info -> "timer_id": (undefined)`）
→ `delete_timer` 永远 -601 → 成功后 30s 看门狗仍会发一个假 `result=0`。
**修**（`src/original_vm.cpp`）：① 在 `set_timer` 里把 `args[3..]` 追加进 `bound`；② 在解释器 `call_efun` 分支对 `set_timer` 单独分流，保留 StackItem 并用 `writeThrough(items[1], r)` 把 id 写回引用。

**根因 3：跨对象 inline 帧的局部函数下标用错表（aaa 端 precost 静默死掉）**
实测（mall5 的 aaa 日志）：
```
[cfv] obj=/aaa/daemons/serverd.o cur=main a=5 funOffset=0 tbl[a]=destruct tbl[a+off]=destruct
threadd.dispatch_comm_service(a1 = <function _main_thread(object(clone:aaa/clone/server.o#7), …)>)
[mall] ENTER obj=/aaa/daemons/serverd.o fn=_main_thread passed=13    ← 13 个参数都对
[mall] ENTER account_transactiond::delete_precost_lock               ← 紧接着就放弃
[mall] RETURN _main_thread -> object(clone:aaa/clone/server.o#7)     ← 修复后是 1
```
`cmd_l_precost_coin.main`（该 .o 自己的函数，funOffset=0）在 **serverd 的 VM** 上内联执行（`objName_=/aaa/daemons/serverd.o`）。
它的 `new_func(5)`（`func_table[5]=_main_thread`）靠跨对象 registry 反查侥幸解析对了，但 `_main_thread` 内对 `_main`
（本程序局部下标 **4**，`func_table[4]=_main`）的调用走
`funcLookup_ ? *funcLookup_ : funcsByIndex_` = **serverd 的合并表** → 查到 `remove_entry` → 返回值 ≠ 1
→ `_main_thread` 走 `delete_precost_lock` 分支 → `_main` 从未执行 → `assure_connection`/`do_account_transaction`/`precost_coin` 全没跑
→ **aaa 从不回 `0x6377`** → gs 只能等 30s 看门狗（而看门狗又因根因 2 无效）。
**修**（`src/original_vm.cpp` `callUserFunction`）：当 `funcLookup_==nullptr` 且当前帧函数不属于本 VM（`!ownedFns_.count(frames_.back().fn)`）时，
先在**函数所属 VM（home）的表**里查该下标，命中即用；否则维持原行为（`ownsFunction` 命中的继承副本不受影响）。

**端到端验证（`mall6` + `mall7`，各 4/5 次购买成功）**：
```
aaa : _main_thread -> 1；do_account_transaction -> (["result": 1])；precost_coin -> 1；[mall-send] cmd=0x6377
gs  : do_when_got_precost_coin -> coind.unlock_oper_coin
      -> [mall-rv] in=170 ok=1 type=6   （save_string(para) 还原成 mapping ✓）
      -> do_after_precost_coin -> 1 -> clean_oper_info -> send_result(me,1,"T00000003")
      -> [mall-send] cmd=0xFD27 body=FD27000109…        （result=1）
      -> send_dialog_ok("#Y购买成功，你得到了#Y1#n#Y个#R惊妖铃#n#Y。#n")
mall7：clean_oper_info 的 oper_info 已经是 (["barcode":"T00000003","timer_id":10467,…])（引用写回生效），
      do_when_precost_timeout 全程 0 次触发（成功后不再有假的 result=0）。
```
**注意（不是 bug）**：`T00000051`（28清凉一夏养育礼包）配置 `gold_start/gold_end = 2014-06-26..2014-07-10` 早已过期，
`buy_from_mall_vsucc` 时间窗检查 `flag=0` 而该条没有 `msg` 字段 → **静默** `send_result(0)`：原版行为。
测试要用不过期的道具：`T00000003` 惊妖铃(328)、`T00000004` 飞行符(216)、`T00000006` 特级血玲珑(1600)。
**脚本**：`_mall_test.py`（开商城 → 解析 `0x5109` 的 key → 抓商品条码 → 逐个买 → 打印 `0xFD27 result`）+ `_mall_run.py`（一条命令起服务/建角/跑测试/抓链）。
**探针**：`LPCVM_MALL`（链路 ENTER/RETURN + 关键包 + `[mall-rv]` restore_value 解析结果）、`LPCVM_CFV`（op23 解析）、`LPCVM_MALL2`（mall 函数逐指令跟踪；
**开关必须 static 缓存**，否则每条指令一次 getenv 会把启动拖过 1000s）。探针的每函数行数上限按 **(对象, 函数)** 计，否则 `main`/`_main` 这类通用名字会先被刷满。

---

### F. 新角色无剧情任务 / 不进「揽仙镇外」— **根因已定位（forEach 循环变量在 runDetached 挂起合并后失效），修复已编译**

**用户口径**：新角色进游戏应当触发剧情、被传送到 `lanxianzhenwai`（揽仙镇外），并出现新手任务；实际既无任务也不传送。

**正确链路（LPC 侧）**：
```
logind::enter_world(620)
  -> set_newbie_task(user)(2617)        仅设置 refuse_* / newbied->set_action_help_status
  -> post_enter_world(user)(26938)      loop (f in _enter_world_funcs) { (*f)(me); }
       -> newbied::do_when_enter_world(431)
            -> taskd("新手礼包")->gift_for_newbie(me)
            -> taskd("【新手】1-9级新手任务")->do_when_char_born(me)  -> instruction_1::do_other_thing
            -> taskd("【新手】1-9级新手任务")->do_task_instruction(me)(331)
                 -> try_create_task(me,0)(262)  task_info = (["state":"S0","alias":"【新手】新的开始",…])
                 -> raise_event_ex("初进游戏", me, …)  -> 剧情 scenario
                      -> instruction_1::finish_init_scenario(1403) -> try_finish_task("S0","S2")
                           -> do_stage_move(me,"S0")(1496)
                                room=_find_room("揽仙镇外"); x=102; y=157
                                -> moved->enter_room(me, room, 102, 157, dir)   ← 传送
```
所以「没任务 + 不传送」= 第 2 步 `_enter_world_funcs` 里的 `newbied` 钩子没跑到。

**排查过程（全部有日志证据）**：
1. `newbied::create` 确实注册了 `register_enter_world((: do_when_enter_world :))`：`[cfv] obj=/gs/daemons/newbied.o cur=create a=11 … tbl[a]=do_when_enter_world`；`[nb-reg] obj=/gs/daemons/newbied.o … top=… f=do_when_enter_world fo=/gs/daemons/newbied.o`。
2. `_enter_world_funcs`（master 全局 31）**建表完全正确**：`[nb-app]`/`[nb-gw]` 显示 227 次 `register_enter_world` 的 `+=` 每次 `resLast=` 都是**该 daemon 自己的闭包**（`lhsUse=3`，走的是拷贝路径，不是就地 append）；`[nb-loop]` 在 `post_enter_world` 进入循环时打印
   `n=227 arrUse=3 e0=check_notify e38=do_when_enter_world@vipd e39=when_enter_world@appellationd el=when_enter_world@ip_card` —— **数组本身没有任何问题**。
3. **问题在循环变量**：`[nb-apply]` 记录每次 `(*f)(me)` 弹出的闭包指针，227 次里只有 **39 个不同指针**，其中 `ptr=169CE9AC`（= 数组下标 38，vipd 的闭包）**重复了 189 次**（第 38..226 次全部是它）。
   即：循环变量在**第 38 次迭代后不再更新**，第 39..226 次一直在重放 `arr[38]`，`newbied`/`instruction_1` 等 189 个钩子因此从未执行。

**根因**：`init_for_loop` 把循环变量的**绝对栈下标** `LoopCtrl::slotOfVar` 存进循环控制块，而这个控制块本身**就放在栈槽里**。`runDetached` 的 `SuspendSignal` 合并分支会把「被 detach 的 callee 栈」**追加到 caller 栈之上**：
```cpp
int shift = static_cast<int>(detachedStack_.size());
for (auto& fr : frames_) fr.bp += shift;                 // 帧的 bp 有修正 ✓
detachedStack_.insert(detachedStack_.end(), stack_.begin(), stack_.end());
```
**帧的 `bp` 修正了，但栈上值里存的绝对下标没有修正**。于是：
- 循环体读变量走 `push_local_value(-1)` → `slotOf(-1)=frame.bp+1`（**新**下标，已随 bp 平移）✓
- `loopStep` 的 `setVar` 走 `LoopCtrl::slotOfVar`（**旧**下标）✗
两者从此错位：`setVar` 一直写老槽，循环体一直读新槽（新槽里是挂起前最后一次写入的值 = `arr[38]` = vipd 闭包）。
- 为什么正好第 38 次断掉：第 38 个钩子是 `vipd::do_when_enter_world`（内部 `check_vip_info` 走 filed/DB 往返 → 挂起 → 触发合并）。前 38 次都在挂起前，读写同槽所以正确。
- 循环本身没死：`cond/step_jmp_for_loop` 的 ctrl 槽是**每次用实时帧重算**的（`slotOf(varNo)`），所以 ctrl 还能找到、`iter` 还能推进到 227、循环能正常结束（这也解释了为什么表现为「静默少跑 189 个钩子」而不是死循环）。

**修复**（`src/value.h` + `src/original_vm.cpp`）：
- `LoopCtrl` 新增 `int varNo` / `bool varIsSlot`：记录循环变量的**相对**操作数（`init_for_loop` 的 par16，`loop (f in arr)` 为 -1）。
- `init_for_loop` 写入 `ctrl->varNo = varNo; ctrl->varIsSlot = (varRef.slotIndex >= 0);`
- `loopStep` 的 `setVar` 改为 `int slot = c->varIsSlot ? slotOf(c->varNo) : c->slotOfVar;` —— **每步用实时帧重算**，挂起/合并后不再错位。
- 非栈槽的循环变量（对象变量/全局）仍走原来的 `slotOfVar`（不涉及栈平移）。

**同一类隐患（已知未修，非本次目标）**：callee 栈上其他存有绝对栈下标的**值**在合并时同样不会平移 —— `StackItem::slotIndex`（`push_local_vptr` 产生的引用项）与 `CrossVmRef::slotIndex`（指向本 VM 栈槽的跨 VM 引用）。只有在「detach 期间挂起且这些引用跨过合并点继续使用」时才会踩到；本次 bug 是其中最典型、也是唯一挡住主线的那个。

**新增探针**（`LPCVM_NEWBIE`）：`[nb-cfv]`（op23 建的闭包 + ptr/owner/ownerKey）、`[nb-reg]`（进入 `register_enter_world` 时栈顶闭包）、`[nb-app]`（`+=` 的 lhsN/rhs0/resLast）、`[nb-gw]`/`[nb-gs]`（对全局 31 的每一次写）、`[nb-loop]`（进入循环时的数组快照 e0/e38/e39/el）、`[nb-isfn]`/`[nb-apply]`（每次 `is_function` / `(*f)(me)` 的闭包指针）。
**验证脚本**：`_newbie_run.py`（起服务 → 建角进游戏 → 抓链路）+ `_check_newbie.py`（链路计数 + 打印存档里的 `position`，应当变成 `/lanxianzhen/lanxianzhenwai`）。

### G. 客户端「选择线路后 10 多秒才到选角界面」— **服务端 echo 心跳不回包（已修）**

**用户口径**：原版选线路后 ≤1ms 就到选角界面；我们这要 10 多秒。
**排查**（先把探针时钟统一：`[send]`/`[recv-accept]`/`[life]`/`[room]` 的 `t=<ms>` 现在都来自同一起点 `lpc::msSinceStart()` —— **之前两套探针各用各的起点，所以上一轮"是客户端慢"的结论是错的**）：
- 服务端登录本身很快：`[recv-accept] IN 4124` → `+81ms OUT 0x211b`（18KB 表单）→ `+114ms OUT 0xf061`（角色列表）✓；AAA 段 64ms ✓。
- 客户端在**等 echo 回包**：`[recv-accept] t=941687ms client=17 …0620D2991BEDCA`（客户端 echo）→ 中间**没有任何回包** → `t=949369ms` 客户端才发下一个包（**7.7 秒空档**）。
- 根因：`game_service.cpp::handleCmdEcho` 把「客户端 tick 间隔 ≤ 9999ms」的 echo **直接丢弃且不回 `0xF9C3`**。真实客户端心跳 1–3 秒 ⇒ **第二次之后的每个心跳都被丢**，客户端只能等自己的 echo 超时；服务与服务之间心跳约 10 秒，正好越过阈值，所以只有真实客户端受害。
- **修复**：只丢**完全重复**的 tick（`period == 0`），其余照常回 `MSG_REPLY_ECHO`。
**注意**：echo 回包由引擎直接 `gameServiceSend()` 发出，**不经过 LPC `send_to_client`**，所以 `[send]` 探针看不到 `0xf9c3`；验证要看客户端**下一个包的时间间隔**。
**待真实客户端复测**：选线路是否立即到选角界面；剧本/任务是否随之下发（socket 不再被 echo 卡住后，之前可能丢掉的后续包应能收到）。

### H. 「新连接后首个包要等约 3.6 秒」+「没有剧情 / 没有剧本演员」— 两个根因（已修，待真实客户端复测）

用户口径：①「新连接建立后首个包仍要等约 3.6 秒」（心跳是 10ms 一次，原版选线路后 ≤1ms 就该到选角界面）；
②「只有进入游戏被传送，然后什么都没：剧本没有、任务没有、只有新手礼包」，「剧情创建的 npc 也不存在」。

#### H1. 每个新连接白等 ~3.1 秒（accept 等错了注册表）

`src/game_service.cpp::startGameService` 的每连接线程，在读第一个包之前要等「客户端 agent 注册好」：

```cpp
for (int tries = 0; tries < 200; ++tries) {
    Value agent = callMasterProgramFunction("find_agent_by_client", {make_int(clientNo)}, root);
    if (agent.index() == 7) break;                  // object registered
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}
```

gs 里 `client_agents[]` **只**由 `feature/common/agent.c::comm_set_client_no` → `set_client_agent` 写入，
而它只能从 `comm::set_client_no` 到达（内部 comm agent，例如 configd 的 `config_agent`）。
**真实玩家连接**走的是另一张表：`logind::new_user(2434)` → `comm::set_client`(gs/feature/char/comm.c:751)
→ `set_client_user`(start_gs:39423) → `client_users[cno]`。

所以这个循环对玩家连接**一次都命中不了**：200 次 ×（5ms sleep + 每轮约 10ms 的 LPC 往返调度）≈ **3.1 秒**，
加在**每个新连接的第一个包之前**。

证据（lat4，未修版）：
```
[accept] t=1459253ms client=89 wait(无) agent-type=0 peer=192.168.200.1:60451   ← agent 永远是 nil
[recv-accept] t=1459253ms client=89 n=20 4D5A...                                 ← 读包时数据早已到达
```
客户端侧 `_conn_lat.py`（connect 后立刻发 echo，分离 connect/send/reply 三段）：
```
round 1: connect=9.0ms  send=9.0ms  reply=3132.5ms      ← 修前
round 1: connect=9.0ms  send=9.0ms  reply=12.0ms       ← 修后
```
`_conn_par.py`（两个连接相隔 200ms 打开）显示两者各自 ~3.1–3.6 秒返回 → 是**每连接固定开销**，不是串行临界区。
同一个 accept 循环在 aaa 上不慢（`agent-type=7`，configd 早就注册过 config_agent），所以只有 gs 受害。

**修**：等待条件改成「**user 或 agent 任一注册好**」，先查 `find_user_by_client`（玩家走这张表）：

```cpp
Value u = callMasterProgramFunction("find_user_by_client", {make_int(clientNo)}, root);
userType = (int)u.index();  if (u.index() == 7) break;
Value a = callMasterProgramFunction("find_agent_by_client", {make_int(clientNo)}, root);
agentType = (int)a.index(); if (a.index() == 7) break;
```
`[accept]` 日志现在带 `wait=` / `user-type=` / `agent-type=`：
```
[accept] t=672550ms client=5 wait=16ms user-type=7 agent-type=0
[accept] t=674633ms client=6 wait=0ms  user-type=7 agent-type=0
```
（drama1 一轮 8 次 accept：wait 最大 16ms、平均 2ms。）

#### H2. 剧本包 0x20D1 头部错位 → 客户端读到 list_count=0（没剧情、没剧本演员、剧情永不结束）

`gs/feature/char/menu_v37.c:197`：
```lpc
void set_talk_drama(object talk_npc, array actors, array content, mixed func = 1, string para = "")
```
`xlibd_v37.c:3310` 用 **4 个参数**调用它（`director->set_talk_drama(actor_npc, actors, scenario[0..<2], func)`），
因此 `para` 必须取默认值 `""`。我们的 VM 却把 **`func` 的默认值 `1`** 填给了 `para`。

证据（0x20D1 body 开头，修前 vs 修后）：
```
修前: 00 00 64 08 | 01 | 00 00 00 01 | 7C | 00 04 10 01 ...
        id(4B)    strlen  (buffer)para  ↑ 实际 list_count=0x7C=124 在这里
                  =1     = BE32(1)
修后: 00 00 64 69 | 00 | 7C | 00 04 10 01 ...
        id(4B)    strlen=0  list_count=124@offset5
```
`(buffer)para` = `00 00 00 01` 正是 BE32(1)，`strlen(para)` = 1（`as_string(1)` 是 "1"）——
两个字段都独立证明 `para` 拿到的是**整数 1**，而不是 `""`。客户端按原版布局解析
`id(4) | strlen(para) | para | u8 list_count | N×(u16 len + fragment)`：
它读到 **list_count = 0x00** → 一个片段都没有 → **剧本不播、剧本演员不生成**，
而且「初进游戏」剧本永远不结束（`func` 回调不触发）→ 任务停在 S0（log/tip 都是空串）。

**根因**：`src/original_vm.cpp` 的 `resort_stack`（op 71）。编译器为**所有带默认值的形参**生成一段内联默认值代码
（编译期不知道运行时实际传了几个），旧实现把 push 出来的**全部**默认值都当成"缺失形参"的默认值
（`f.argNum += defaultPushed;`）。5 形参传 4 个时 `defaultPushed=2`（`func=1`、`para=""`），
按正向顺序 push，于是 `para` 拿到第一个（`func` 的 `1`）。

**修**：`check_args_num`（op 15）把 `maxArgNo`（= max(skipAll, argCount)）记到 `Frame::maxArgNo`，
`resort_stack` 只保留**最后** `maxArgNo - argNum` 个默认值：

```cpp
int keep = defaultPushed;
if (f.maxArgNo > 0) {
    int missing = f.maxArgNo - f.argNum;          // 真正缺几个形参
    if (missing < 0) missing = 0;
    if (missing < defaultPushed) keep = missing;
}
for (int i = defaultPushed - keep; i < defaultPushed; ++i) defaults.push_back(...);
...
f.argNum += keep;
```
默认值块是**形参正向顺序** push，所以要保留的是**最后** keep 个（这一条已由 H2 的证据钉死：
第一个 push 的默认值属于 `func`，不是 `para`）。`keep == defaultPushed` 时与旧行为完全一致，
所以 `get_all_catalog(condition="", timeout=1800)` 那类调用不受影响。

**顺带修好的同类错位**：`send_task_log(task_name, tip=0, sync=0, upgrade_type=-1)`（gs/feature/char/comm.c:986）
用 2 个参数调用时 `upgrade_type` 曾拿到 `0`（应为 `-1`），使 `upgrade_type < 0` 的分支判断走错。

#### 验证

- `_conn_lat.py`：gs 首个 echo 回包 **3132ms → 12ms**（aaa 一直是 ~2ms）。
- `_enter_probe.py`（drama1/）：`PASS-ENTERED`，103 个世界包；世界包从 7.6s 提前到 **1.3s**（accept 不再空等）。
- `0x20D1`：`id=25705, strlen(para)=0, list_count=124@offset5`，`_dec_drama.py` 暴力对齐确认
  「片段数 × (u16 长度 + 内容)」恰好读完 3544 字节、无剩余。
- `0x2603`：`_dec_task.py` 逐字段解码，190 字节一字不差：
  `【新手】1-9级新手任务|【新手】新的开始` / desc 70B / refresh=1 / person=1人 / level=1~9级 /
  bonus=#I数值奖励|… / task_type=指引 / att32=0。
- **待真实客户端复测**：剧本是否播放、剧本演员是否出现、随剧情推进的任务是否显示。


### I. 「用新手礼包 GS 崩溃」+「任务没有提示/不显示」— 两个根因（已修，待真实客户端复测）

用户口径：剧本正常了，但①还是没有任务；②「貌似 gs 崩了」。

#### I1. 开新手礼包 → GS 栈溢出崩溃（`0xC00000FD`）

日志（drama1/，用户真实客户端）：
```
[recv-accept] t=1728218 client=96 n=13 4D5A...0003 220C 65      ← 客户端点用背包里的新手礼包
[case8] user.o::process_command -> commandd.o::process_command -> cmds/normal/apply.o::main
[life] destructor destruct/delete (idx 2,9,7,66,20,202...) on clone:gs/clone/item/newbie_gift/basic.o#16473
[crash] code=0xc00000fd addr=0x004EC0D7 frames=4                  ← STACK_OVERFLOW
[crash] last LPC functions: query_entire_dbase move_to_bag_check get_module_status get_iid lite_query
        get_import_name query_inventory lite_query_temp check_limit_item is_limit_item lite_query
        move_to_dest ... refresh_item ... build_item build_item_ex ... get_item_basic_info ...
```
链路：`apply()` → `open_newgift_item` → （礼包为空）`rebuild_newbie_gift` → `named.c->new_item`
→ `move_to_bag` → `put_in_container`/`notify_carrier`/`refresh_item`/`build_item`/`build_item_ex`
→ `get_item_basic_info` → `is_important_item` → `get_raw_name` → `basic_object` → `query`
——**约 100 层嵌套 LPC 调用**。

**根因**：`CMakeLists.txt` 里 `target_link_options(... /STACK:0x100000)`（1MB）。
调度器跑在**主线程**（`main.cpp` → `lpc::schedulerRun()`），所有 LPC 调用都嵌套在主线程栈上；
注释里「deep LPC call chains run on explicitly-reserved big-stack threads (create_thread)」**已经过时**
（create_thread 现在也走 `scheduler().spawn()`，见 `src/original_vm.cpp` 的 `if (!isTelnetThread)` 分支）。
原版 Linux 默认栈 8MB，所以 100 层没事；我们 1MB 直接爆。
这也让 `checkCallDepth()`（400 帧）形同虚设：1MB ÷ 100 帧 ≈ 10KB/帧，400 帧要 4MB，永远到不了就先崩。

**修**：`/STACK:0x2000000`（32MB）。地址空间代价可忽略（32 位 LARGEADDRESSAWARE 有 4GB），
而 400 帧上限现在真正生效（≈4MB < 32MB）→ 万一再撞到深递归，会抛可被 LPC `catch` 的 VmError 而不是杀进程。

#### I2. 任务日志下发但 `log`/`tip` 全空 → 客户端任务面板没有提示（用户口径「没有任务」）

`0x2603`（`send_task_log`）里 `task_log`/`task_tip` 两个字段一直是**空串**（修前）：
```
log(0) =           tip(0) =              ← 修前
log(49) = 到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途吧。
tip(45) = 到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途      ← 修后
```
链路：`taskd::query_task_log/query_task_tip`：
```lpc
funcs = all_templates[task_class];              // = instruction_1 的模板 mapping ✓
task_tip = funcs["tip"];                        // = (: call_other, ".../instruction_1.c", "task_tip" :)
if (is_function(task_tip)) task_tip = (*task_tip)(me, task_info);
if (!is_string(task_tip)) return "";            // ← 在这里返回空
```
`LPCVM_OPFN=query_task_tip`（新增：逐指令 + 栈顶类型）实录：
```
[op] ip=18 op=3 a=0  top=t6:(["冥-...":...])                 ← push_ovar(all_templates) 是 mapping ✓
[op] ip=21 op=9 ...  top=t6:(["dest":<function task_dest>, "tip":<function call_other(...)>])  ← 模板 ✓
[op] ip=33 op=9 ...  top=t8:<function call_other(".../instruction_1.c","task_tip")>            ← 闭包 ✓
[op] ip=38 op=! a=8  top=t2:1                                ← is_function(闭包)=1 ✓
[op] ip=42 op=5 a=4 b=3  top=t8:<function call_other(...)>   ← call_ptr(nargs=3)
[op] ip=43 op=9 ...  top=t2:1                                ← 调用结果是 int 1 ✗（应为 string）
```
**根因**：op 23 `create_function_value` 在「全局表位 + 找不到 master 函数 + 命中 efun 名」时建的
**efun 闭包**（`(: call_other, path, "fn" :)`、`(: create_thread, ... :)`）会被写成
`func=nullptr, funcIndex=<该 efun 在全局表里的槽号>, owner=nullptr`
（注释原话：「owner = nullptr so invocation reaches the efun fallback (call_fun_ptr's final else)」）。
但 `call_fun_ptr`（case 5）在到达最后的 efun-by-name 兜底之前，先跑了通用兜底：
```cpp
} else if (cl->funcIndex >= 0 && !cl->func) {
    auto fit = funcsByIndex_.find(cl->funcIndex);      // ← 当前 VM 的合并函数表
    if (fit != funcsByIndex_.end()) fn = fit->second;  // ← 撞车：索引空间和 efun 槽号重叠
    else if (auto gfn = findMasterFunction(cl->funcIndex)) fn = gfn;
}
```
只要**当前 VM 的合并表在这个下标上恰好有一个函数**（taskd 就有），`fn` 就被解析成那个**无关函数**
并 inline 执行——`task_tip` 根本没跑（FNTRACE 里 `task_tip` 的 ENTER 完全缺失），返回值是那段
字节码残留的 `int 1`。于是 `is_string(1)=0` → `return ""` → 任务没提示。

**修**（`src/value.h` + `src/original_vm.cpp`）：
- `ClosureValue` 新增 `bool efunClosure = false;`；op 23 的 efun 闭包分支置 `cl->efunClosure = true;`
  （两处闭包拷贝 `bindClosure`/`bind` 也一并复制这个标志）。
- `call_fun_ptr` 的兜底加上 `&& !cl->efunClosure`，让 efun 闭包直接落到 efun-by-name 的
  `callEfunWithArgs` 分支。
**影响面**：所有 `(: call_other, path, "fn" :)` 形式的回调（任务模板的 log/tip、剧本 `func`、
菜单 closure…）此前都可能被静默换成另一个函数的返回值。

**验证**（fix1/）：
- `[fn] ENTER task_tip` / `RETURN task_tip ret=到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途` ✓
- `0x2603` body 192 → **284** 字节，`log(49)`/`tip(45)` 非空，且逐字段解码仍恰好读完 ✓
- `0xF9FB`（任务追踪）带任务条目 ✓
- `0x20D1` 剧本不受影响：`strlen(para)=0, list_count=124@5`，124 片段恰好读完 3544 字节 ✓
- 全程 `[crash]`/`[deep]` 0 次，4 个服务存活 ✓

**新增探针**：`LPCVM_FNTRACE=<substr>`（匹配函数的 ENTER/RETURN + 返回值）、
`LPCVM_OPFN=<substr>`（匹配函数逐指令 `ip/op/a/b/m0/m1/sp/top(类型:值)`，按函数名封顶 400 行）。


### J. 「玩家数据不保存 / 重上每次重跑剧情 / 礼包打开变成新建礼包」— 三个根因（已修，已端到端验证）

用户口径：①「玩家数据并没有保存下来，重上每次都触发剧情剧本，角色数据也恢复到了1级，购买的物品也不见了」；
②「打开新手礼包错误，正常是弹出一个列表选择取出物品，现在的是打开后生成一个新的新手礼包」。

**先定位数据面**：`dl_ddb_1_test.data` 里角色自己的 dbase 只在**创建那一刻**写过一次，之后再没更新过：
```
修复前:  user/<gid> = 1520 B @创建时刻；carry=17 B；achieve=4 B          ← 之后永远是这条
修复后:  user/<gid> = 3023 B @注销时刻；carry=124 B；achieve=488 B        ← 注销时真正回写
```
（同一时间 `purchase_infos`/`lgxy` 等其它路径一直在写 → dba/`write_content` 本身没问题，问题在**没人触发角色存盘**。）
②就是①的后果：礼包的 `gift_info` 没有恢复 ⇒ `newbie_gift::is_empty()` 为真 ⇒
`open_newgift_item` 走 `rebuild_newbie_gift`（发下一个礼包）而不是 `build_item_info`（弹列表）。

#### J1. 客户端断开时驱动没有通知 LPC → 角色永不析构、永不存盘

原版驱动在 socket 关闭时会 `vm_apply("cmd_disconnected", cno, where)`；
`start_gs::_cmd_disconnected(37938)` → `user.o::disconnected(425)` → （非 logout）`set_timer(180, "time_expired")`
→ `destruct` → `userd::destroy_user(2645)` → `me->save(0, cb)`（**唯一**把角色 dbase 写回 ddb 的路径）。
我们的 `src/game_service.cpp` 收完包就直接 `closesocket` + 从 `gameClients()` 里删掉，**从不通知 LPC**。

**修**：客户端线程在注销 client slot 之前调用 `cmd_disconnected(cno, 2)`（2 = `stat_disconnect` 的 "driver_callback"），
且必须在 `gameClients().erase()` 之前（LPC 用 `find_user_by_client(cno)` 找对象）。

#### J2. `callDestructors` 先跑继承来的析构 → `carry.c::destruct` 把 `destructing` 置位 → `user.o::save` 直接 return 1

`user.o` 继承 `/gs/feature/char/carry.c`（user_v37.c:17），而
```lpc
// carry.c:428
private void destruct() { if (!is_clone() || !is_mapping(inv = query_inventory())) return;
                          lite_set_temp("destructing", 1); loop (item in values(inv)) destruct_object(item); }
// user.o:633
override int save(mixed as_create, mixed f) {
    if ((!lite_query_temp("entered_world")) && (!lite_query_temp("permit_save"))) return 1;
    if ((lite_query_temp("destructing") || lite_query_temp("destructing_store"))) return 1;   // ← 命中
```
`LPCVM_FNTRACE=sav` 实录（注销时）：
```
[fn] ENTER get_save_done_func obj=userd.o args=2
[fn] RETURN get_save_done_func ret=<function call_other("...userd.c","notify_save_done","1","user/","0000000000000031","",(undefined))>
[fn] ENTER save obj=/gs/clone/char/user.o args=2
[fn] RETURN save ret=1          ← 立刻返回，没有 ::save / save_to_text / 任何 filed 写入
```
即：`destruct_object(user)` 时 `callDestructors` 按 `funcsByIndex_` 的**下标顺序**跑所有 `destruct`，
继承来的 `carry.c::destruct` 先跑并把 `destructing` 置位，随后 `user.o::destruct` → `destroy_user` → `save` 被自己人拦掉。

**修**（`src/original_vm.cpp` `callDestructors`）：按 `funOffset`（对象自身程序 = 0，继承程序依次递增）**升序**稳定排序，
即**先自身、后继承**——这也正是引擎的析构链顺序（本对象 `destruct` 是主析构）。
`stable_sort` by `(funOffset, index)`。

#### J3. `destructObjectByKey` 先摘注册表再跑析构 → 析构里的回调全成空操作

```cpp
reg.vms.erase(key); reg.objects.erase(key); reg.initialized.erase(key);   // 旧：先摘
vm->callDestructors(lifeOn);                                             // 后跑
```
析构链要**回调本对象**（`user.o::destruct` → `destroy_user` → ... → `me->save(0, cb)`、`carry.c` 里逐件 `destruct_object(item)`），
而 `call_other` 是通过 `reg.vms` 解析目标对象的——先摘掉就让这些调用变成静默 no-op。

**修**：先 `callDestructors`（对象仍在注册表里），**再**摘注册表/`regIndexUnregister`/park；
析构链里再出现 `destruct_object(<同一 key>)` 时直接返回（外层负责拆除），避免把注册表提前摘掉。

#### 验证（`_verify_save.py`，一轮里做两次登录）

```
session 1: 登录→建角→进世界 (PASS-ENTERED)
等 210s（注销定时器 180s）
ddb: ('user','...0031','',        3023 B, 18:14:44)   ← 主 dbase 回写
     ('user','...0031','achieve',  488 B, 18:14:44)
     ('user','...0031','carry',    124 B, 18:15:01)
     ('user','...0031','patch',    161 B, 18:15:01)
session 2: 再次登录 → **没有 0x20D1（剧情不再重跑）**、**0x2603 任务还在**、85 个世界包 PASS-ENTERED
```
（`_verify_save.py` 里 session 2 紧跟在存盘回调之后，正好落在"临界区未释放"窗口里被 9026/9029 拒了一次；
`stop_stun_account` 在 `notify_save_done` → `delete_content_to("runtime")` → `notify_delete_runtime_done` 之后才执行，
之后再登就正常了——原版同样是「注销 180s 定时器 + 帐号临界区」两段式。）

#### 顺带修好的：`destruct_object` 期间对象不可用

J3 让"析构过程中对象仍可被调用"成立——`carry.c::destruct` 逐件 `destruct_object(item)`、
`user.o::destruct` → `destroy_user` → `me->save()`、`me->remove_link_of_runtime_info()` 等全部恢复正常。


### K. NPC 对话没头像/没名字（头像 FF FF FF FF、名字长度 00）— 省略实参被填成 int 0 而不是 undefined

用户口径：「npc还是没有头像,没有名字,头像变成了FF FF FF FF 名字变成了00长度」。

链路：`cmds/normal/open_menu.c::main(me, id)` → `me->start_talk_to(ob)`（menu.c:245）→
```lpc
set_menu(ob, menu, menu_content);          // 只传 3 个参数
```
而 `set_menu(ob, menu, content, portrait, pic_no, allow_close, mobile_use_flag)` 声明 7 个形参，后 4 个**省略**；
`build_menu_buffer` 里：
```lpc
if (is_string(portrait)) { ... }
else if (is_undefined(portrait)) { actor_name = ob->get_name(); portrait_used = (int)ob->query("portrait"); }
else { portrait_used = ((portrait == 0) ? -1 : portrait); actor_name = ""; }
```
**原版驱动把帧的形参区清零，而全零值在本 VM 里就是 type 0 = "undefined"**（`is_undefined()` 为真、`is_int()` 为假）。
我们的补位用的是 `pushValue(make_int(0))`（**int 0**）→ `is_undefined(portrait)` 为假、`is_int()` 为真 → 走了 else 分支：
`portrait_used = -1 (0xFFFFFFFF)`、`actor_name = ""` ⇒ 客户端「没头像、名字长度 0」。
（旧注释担心 `childd.update_child` 的 `is_int()` 检查——那是当年另一个 bug 的表象，现在按原版语义清零即可。）

**修**：`src/original_vm.cpp` 五处省略形参补位 `pushValue(make_int(0))` → `pushValue(Value{})`（= type 0 undefined）。

**验证**（`_npc_menu_check.py`，新增：点击视野内每个 NPC 并解码 `0x2037` 的
`id/portrait/pic_no/content/actor_name`）：
```
修复前: portrait=0xFFFFFFFF(-1)   name="" len=0
修复后: 0x549E MENU id=0x549E portrait=0x00000000(0) pic_no=1 content=31B name="宝箱" len=4   OK
        0x5492 MENU id=0x5492 portrait=0x00000000(0) pic_no=1 content=31B name="宝箱" len=4   OK
        0x548A MENU ... name="宝箱" len=4 OK        0x5499 MENU ... name="宝箱" len=4 OK
```
（这一轮点的是剧情临时 NPC「宝箱」，它们 `query("portrait")` 本来就是 0 —— 名字回来了就说明分支走对了；
真实 NPC 的 portrait 值需要走到揽仙镇里再点，见下轮待办。）
同轮回归：`0x20D1` 剧本照常、`0x2603` 的 `log(49)/tip(45)` 依旧、`[crash]` 0 次。

**顺带发现（未修，用户口径 2/3 的线索）**：
- 过图后服务端只回 `0x1155`，**不再广播新房间的对象列表**（`0xF05D`）：`_npc_menu_check.py` 里
  `0x1026 → 0x1155` 成功但 5s 内 `npcs=[]`。真实客户端是靠 `0x1B01`(apex) 再申请一次；要复现用户的
  「重上后物品/任务不显示、要动一下才出来」得对齐这条链路。
- 用户三个角色都被存在 `position.room=/lanxianzhenwai/lanxianzhenwai.c`（揽仙镇外）——
  过图卡死要按"当前房间的 `exits` 查不到目标"来查（`enter_room.c::main` 第 66/69 行两处 `break` 都是静默返回）。


### L. 「提示通讯故障」= DBA 回包永远唤不醒等待的 do_operate（**已修复，已端到端验证**）

用户口径：「提示通讯故障，没有测试，每次先测试一次登录再报告」。这句提示是**服务端下发**的
（`logind.c::try_recover_data` case -2 → `send_dialog_ok("通讯故障，暂时无法登录，请稍候重试。（原因：9030）")`，
`logind_v37.c:1964`），触发条件只有一个：`read_content("runtime", account)` 返回 **-2**，也就是
`filed.c::do_operate` 里那个初始值 `result = ({-2, cookie})` 从未被回调改写。

**根因（闭包被派发到错误的实例）**：

`do_operate` 把回调写进 `oper_map[cookie]`：
```lpc
msg = ([ ..., "to": (: operate_done, blocked, get_thread_id(), result, to :), ... ]);
oper_map[cookie] = msg;
```
`operate_done` 属于 `/daemons/filed.o`，但这个闭包**是在调用方内联执行 do_operate 时创建的**
（例如 `vendued.o → /daemons/filed.c->write_content()` 走 VM 的跨对象内联，代码是 filed 的、
`objName_` 是 vendued），于是闭包记录的是**调用方的实例 key**（`ownerKey=pak:gs/daemons/vendued.o`）。

回包到达时 `msg_operate_done` → `(*msg["to"])(cookie, content)` 按 `ownerKey` 找到 **vendued 的 VM**，
`operate_done` 就在那台 VM 上跑：读到的是 vendued 的变量块（不是 filed 的 `oper_map`）→ 查不到 cookie →
静默 return → **`raise_event` 永不发生** → 等待的 `do_operate` 一直 park，直到我们的
`[construct-wdog]`（15s）把它用失败结果强唤醒 → 返回 -2。

影响面（实测）：一次 15 小时运行里有 **1554 次** `construct-wdog`（vendue/order 等所有后台 DBA 写全部失败，
`linux/gs/log/filed_backup.sql` 里堆满失败写；登录时的 runtime 读也会 -2 → 就是那句「通讯故障」，
前面的「要 30 秒左右才能登录」也是同一件事：15~20s 卡住 + 失败重试）。

**修**（`src/original_vm.cpp`，闭包调用 op 5 `call_fun_ptr`）：
解析出 owner 实例后，只有当它**确实拥有该函数**（`ownsFunction(cl->func)`）才用它；否则用
`homeVmMutable(cl->func)`（即 `originalHomeVmOf` 的可写版本）改派到**函数真正所属的 VM**，
并**保持 runDetached 语义**。

> 教训（已经踩过一次）：第一版改成"就地 inline 执行"看起来也对（`ev_filed` 立刻有 raise），
> 但 inline 会把**回包分发线程**一起 park 住 —— 下一个回包就再也处理不了（一整轮启动 0 条
> `[readcontent]`、`[pkt-filed]`，启动被拖到 960s+）。跨实例闭包必须 detach 到目标 VM 上跑。

**验证**（`mv7`，一轮全新启动）：
- `construct-wdog` **19 → 0**（mv1 1554、mv5 17、mv6 19），`[readcontent]` 1770 条、
  `[game] msg_created_content` 1478 条、`[cfv] owner-instance retarget` 3312 次，全部 DBA 回包都落地。
- 登录 → 建号 → 进世界 → 走路 → 过图 → 硬断线 → 重上：全部通过，重上 `dialogs=[]`（**没有通讯故障**），
  角色停在存档坐标。

**同一轮顺手确认（之前误判的两件事，都有真实客户端包证明）**：
- **过图本来就是好的**：客户端发的是**房间名**不是路径（`0x1026` body = `u8 len + "揽仙镇"`）。
  `enter_room.c` 用 `exits[名字]` / `zone->get_all_rooms()[名字]` 查表；我上一轮发的是
  `/gs/zone/lanxianzhen/lanxianzhen.c` 所以必然失败。实测 `_walk_probe.py`：从 (86,158) 走到
  (9,139)（出口格 (1,131)，`calc_offset<=10`）→ 发 `0x1026 "揽仙镇"` → `map_id 2000→1000`、
  坐标 (341,246)，**过图 OK**。
- **背包格位从 101 开始**（`carry.c::get_inv_start_pos()=101`，`is_legal_inv_pos` 只认
  101..101+get_inv_size()-1）。之前用 0..40/101..145 去试 `cmd_apply`，0..40 必然全空。

**礼包问题（已修复，已端到端验证）**：新手礼包打开会「变成下一级礼包」、也没有赠品属性。
根因是 **create() 的顺序反了**：
```lpc
// newbie_gift.c::create() 第一行
if (!is_object(this_object()->basic_object())) return;   // ← 这里返回 ⇒ 没有 set_gift_item()/gift_info
```
`my_import_name` 是基类 `basic_object.c::create()` 里设置的
（`from_ob = object_template(this_object()); my_import_name = from_ob->get_import_name();`），
而 `callCreate` 收集各 constitution 的 create 时把**派生类先入表**，于是派生 create 跑在基类 create 之前 ⇒
`my_import_name` 还是空 ⇒ `basic_object()` = 0 ⇒ 直接 return：既没有 `set_gift_item()`（赠品标志），
也没有 `init_newbie_gift_items()`（`gift_info`）。客户端开礼包时 `is_empty()` 为真，
`open_newgift_item` 走 `rebuild_newbie_gift`：把礼包吃掉、生成下一级礼包（用户口径「打开后生成一个新的新手礼包」）。

**修**：`callCreate` 的收集改成 **base-first**（先递归父类的 inherits，再登记本层；对象自己的 create 仍最后），
和引擎 initOrderList/"基类构造在前"的语义一致。顺带把 `object_template` 按 IDA
`vm_f_object_template`（0x80f4b9c）还原：**带参数时要返回"实参对象自己的 pTemplateObject"**
（之前忽略实参，只返回当前 VM 的模板）。

**验证**（`mv14`，全新启动）：
- `[call8] .../newbie_gift/basic.o fn=create -> newbie-gift-task.o::init_newbie_gift_items` ✓ 链路走通。
- `_gift_probe.py 1 1 gift5 101 145`：`0x220C pos=101` → **`0x21B7`(8631) 415B**
  （"新手礼包（10级）" + 物品列表 `80|75|70|65|60|55|50|45|40|35|30|25|20|15|10`）✓
  —— 就是用户说的「弹出一个列表选择取出物品」。**注意背包格位从 101 起**（见上）。
- `_save_probe.py`（干净 `0x3E2A` 下线）：`carry` 17B → **206B**：
  `([101:"新手礼包（10级）":([...,"type":8,"gift_info":(["1":"驯兽诀","2":"烟花筒","3":"功能卷轴·神行千里",]),47:1,])])`
  —— **gift_info 与赠品标志（47:1）都进了存档** ✓；主档 1519→3050B、achieve 4→488B ✓。
- 回归：走路 (86→90) ✓、过图 揽仙镇外→揽仙镇 (341,246) ✓、硬断线重上 `dialogs=[]` ✓、`construct-wdog` 0 次 ✓。


### M. 「上线出现新春横幅（原版没有）」= `time(string)` 被实现成「返回当前时间」（**已修，待实测**）

用户口径：「上线的横幅应该是错误的…原版并没有提示」，横幅内容是
`#Y《问道》#n祝所有玩家：新春快乐！龙年大吉！` 之类。

来源：`/gs/daemons/banner_managed.c` 的 `system_banner`，配置在 `etc/banner.list`：
```
sys_banner_config$ : ({ "2012-01-22-20:00:00", 3, 10, "spring_festival_2012" })
...
spring_festival_2012$ : "#Y《问道》#n祝所有玩家：新春快乐！龙年大吉！"
```
过滤条件（`banner_managed_v37.c:244`）：
```lpc
if ((time(arr[0]) + arr[2]) <= time()) continue;   // 过期条目跳过
```
**原版 `vm_f_time`（IDA 0x80dd030，逐行核对过）对字符串参数的行为**：
```c
v3 = strlen(s) + 1;
if ( ~v3 == -16 )            // ~v3 == -16  <=>  v3 == 15  <=>  strlen == 14
{   /* 紧凑 "YYYYMMDDHHMMSS"：sec=[12] min=[10] hour=[8] mday=[6] mon=[4]-1 year=[0] */ }
else { v4 = strlen(s) + 1;
       if ( ~v4 != -21 )     // 不是 strlen == 19
       { t = strtol(s,0,10,0); ... return t; }        // ← 其它长度：只取前导数字
       /* "YYYY-MM-DD-HH:MM:SS"：sec=[17] min=[14] hour=[11] mday=[8] mon=[5]-1 year=[0] */ }
when.tm_year = strtol(...) - 1900;  t = mktime(&when);
```
- `strlen == 14` → 紧凑 `YYYYMMDDHHMMSS`（偏移 0/4/6/8/10/12）+ `mktime`
- `strlen == 19` → `YYYY-MM-DD-HH:MM:SS`（偏移 0/5/8/11/14/17）+ `mktime`
- **其它长度一律 `strtol(s)`（只取前导数字）后直接返回**，根本不进 `mktime`
- `tm` 是 `memset(...,0)` ⇒ `tm_isdst = 0`（**不是** -1）

⚠️ 第一版 HANDOVER 把长度写成了 16/21（`~v3 == -16` 误读成 `v3 == 16`）。正确是 **15/20**，
即 `strlen == 14 / 19`。长度判错的后果：`time("2012-01-22-20:00:00")`（19 字符）本该走 `mktime`
返回 2012-01-22 20:00 的时间戳，却被当成"其它长度"→ `atoi` → 2012。**两种走法都让 2012 的条目过期**
（`2012 + 10 <= now` 为真），所以横幅现象一致修好，但对别的调用方是错的，已按 IDA 改正。

我们的 `time` 曾经只有一个分支「返回当前时间」，字符串参数被直接忽略 ✗ ⇒ 2012 的条目看起来"还没过期"
⇒ gs 定时广播新春横幅 ✗。**修**：`src/builtins.cpp` 按上面三分支还原（长度 15/20、`tm_isdst=0`）。

**验证方法**：新一轮启动日志里 `[fn] ENTER get_next_sys_banner` 之后应当**取不到条目**
（`sizeof(all_list) < 1 → return`），不再出现 `timer_broadcast_banner → broadcast_banner`；
客户端登录收不到那条横幅。


---

### N. 「重新登录提示『暂时无法登录（错误：9026）』，永远好不了」= 闭包在跨对象内联调用里创建，实例记成了调用方

用户口径：「角色重上提示暂时无法登录需 ~30s」（实测更糟：**永久**卡住）。

#### 现象与取证（`mv20/`，复现命令 `python -X utf8 _relogin_cycle.py 1 1 stunC3`）

第 1–2 次「进世界 → `0x3E2A` 登出 → 立刻重登」正常（~10.4s）；**第 3 次必挂**，
之后每次登录都收到 `0x1FE5: 你的帐号现在暂时无法登录，请稍候再尝试。（错误：9026）`，90 秒内 7 次全失败。

`mv20/live_gs.txt` 最后一次挂死的关键行：
```
L131181 [q-hook] qid=1 ... <function destruct_object(object(clone:gs/clone/char/user.o#16298))>
L131182 [fn] ENTER do_stun_account obj=/gs/daemons/logind.o args=1     ← 进临界区
L131184 [filed-call] do_operate ... chain=[save_user_patch_data<-write_content_to]     ← 4 次存盘都成功
L131237 [pkt] analyse cmd=0x2403 ... [game] msg_written_content cookie=126802 result=0
L131276 [fn] ENTER notify_save_done obj=/daemons/filed.o args=6        ← 第 4 个存盘回来
L131277 [fn] ENTER delete_content_to obj=/daemons/filed.o args=4       ← 删除 runtime 开始了
L131278 [filed-call] do_operate obj=/daemons/filed.o vm=13DEB5F0 ... chain=[operate_done<-notify_save_done<-delete_content_to]
L131279 [fn] RETURN delete_content_to ret=(void)                       ← 直接返回，什么都没做
```
**此后既没有 `[q-send] qid=26`，也没有 `[q-fail]`，没有 `cmd_delete_content`/`msg_deleted_content`(0x2405)，
`notify_delete_runtime_done` 0 次、`stop_stun_account` 0 次。**

#### 根因

`/daemons/filed.c::do_operate` 的第一步就是
```lpc
if ((!service_flag)) return -2;      // service_flag 是 filed.o 的全局
```
`do_operate` 是 **filed.o 的函数**，但它这一帧读的是 **`/gs/daemons/userd.o` 的变量表**
（`[filed-call]` 探针里 `varsOwner=pak:gs/daemons/userd.o`）⇒ `service_flag` 取到 undefined
⇒ `!service_flag` 成立 ⇒ **`return -2`，`send_to_queue(srvQ, msg)` 根本没执行**。
LPC 调用方忽略 `do_operate` 的返回值，所以请求静默消失：
DBA 收不到删除 → `notify_delete_runtime_done` 不跑 → `stop_stun_account` 永远不调用
→ `logind.stun_account[account]` 一直是 1 → 后续登录全被判 9026。

为什么变量表是 userd 的：`do_operate` 里创建的闭包
`(: operate_done, blocked, get_thread_id(), result, to :)` 是在**跨对象内联调用**中创建的
（userd.o 的 `save_user_patch_data` → `"/daemons/filed.c"->write_content_to` → `do_operate`），
闭包因此记下了**调用方 userd.o** 的实例。回复回来时 `operate_done`（= `notify_save_done` 链）
需要改投到函数的 home 实例（§5.L 的 `[cfv] owner-instance retarget`），
但那条路走的是 `OriginalVm::callFunctionPointer()`，它**没有**做 `callFunctionByName()` 里那段
「先把 `objVars_` 还原成自己的表再调用」的修正：
```cpp
// callFunctionByName（原来就有）
if (objVars_ != ownObjVars_) { savedVars = objVars_; objVars_ = ownObjVars_; ... }
```
于是 `notify_save_done` / `delete_content_to` / `do_operate` 整条链都在 userd.o 的表上跑。

#### 修复（`src/original_vm.cpp`，两处，同一个不变量：函数必须读自己程序的全局）

1. `OriginalVm::callFunctionPointer()` 补上和 `callFunctionByName()` 一样的变量表修正
   （`savedObjVarsForNextCall_` 也照抄，保留外层表以便恢复）。
2. `startInlineFunction()` 的自动切表加一条 else 分支：
   ```cpp
   } else if (!switchObjVars && objVars_ != ownObjVars_ &&
              objectRegistry().masterObj != &object()) {
       autoVars = ownObjVars_;          // callee 属于本 VM，却挂着别人的表
       autoStrings = &obj_.strings;
       autoFuncs  = &functionTable();
   }
   ```
   （master 程序被排除，和上面那条既有的跨对象分支保持一致。）

#### 验证方法

`_relogin_cycle.py` 连跑 ≥3 轮，每轮：
`do_stun_account` 与 `stop_stun_account` **成对出现**（各 1 次）、
出现 `[q-send] qid=26 ... fn=do_operate`、`cmd_delete_content`/`0x2405`、
`[fn] ENTER notify_delete_runtime_done`，登录不再出现 9026。


---

### O. `vm_cmdEcho` 的「坏 echo」与反加速判定按 IDA 还原

`vm_cmdEcho`（IDA 0x8053178）真正的判定顺序是：

```c
if (len <= 3) return 0;
v9 = 客户端 tick;  v5 = 可选的"回显的服务端 tick"（len > 4 时必须 len >= 8 才读）
if ( !vm_checkClientAccelerate(clientNo, v9) ) return 0;      // ← 先反加速
if ( cl.tickLastRecvLastEcho ) { v21 = v9 - cl.tickLastRecvLastEcho;
    if ( v21 <= 0x270F ) { printf("Got bad ECHO ... time period = %dms."); return 0; } }  // ≤9999ms 丢弃
cl.tickLastRecvLastEcho = v9;
time(&ti); if (!v5 || cl.tickServer == v5) cl.timeRecvEcho = ti;
... 回 0xF9C3（dtick = vm_getCurrentTick()）...
```

- 早先我们只判 `period == 0`（当时为了修「选线路后 10 多秒」而放宽）。**实测真实客户端**
  （`mv18/live_gs.txt`，client 75 连续 60 个 echo）间隔是 **10000/10010/10016 ms，从无 ≤9999**，
  所以原版的 `<= 0x270F` 丢弃规则对真实客户端**不会误伤**，那 10 多秒是 §H1 的 accept 等待，
  已在别处修掉。现按原版还原。
- `vm_checkClientAccelerate`（IDA 0x804ef50）**是驱动里唯一会强关游戏客户端的地方**
  （`cmd_close_client`），并且 `vm_cmdMoveTo`/`vm_cmdAccelerateMoveTo`/`vm_cmdMultiMoveTo` 也都会调它。
  已在 `src/game_service.cpp` 里按 IDA 逐条还原（首次同步 `tickLocalStartSync`/`tickClientStartSync`、
  客户端 tick 回退 = 异常 1、`tickLocalLost>0xBB7` 后 `localLost+5000 < clientLost*0.98` = 异常 2（客户端过快）、
  `clientLost+150000 < localLost` = 异常 3（客户端过慢）、累计 3 次才 `cmd_close_client`）。
  两侧都是**单调毫秒差**，起点会抵消（实测客户端 tick 与服务端 ms 同步走：10016ms vs 10016ms）。
  探针 `LPCVM_ECHO` 会打 `[echo] recv/reply/Got bad ECHO`、`[accelerate]`、`[echo-state]`。


---

### P. 「服务跑几个小时后登录不了」= DBA 的 filed 请求分发静默死掉，gs 永不重连

用户口径：「gs 又死了，等几个小时登录现在无法连接，中间没有登陆过」；
随后补充「普通账号提示数据不一致，gm 账号无任何提示」。

#### 取证（2026-09-16，mv22 已连续运行 9.6 小时）

| 证据 | 数值 |
|---|---|
| 登录现象 | 先 `0x1FE5: 通讯故障…（原因：9030）`（~20s），随后账号被卡临界区 `9026`；偶发一次 `9031 数据不一致` |
| gs 侧 | `[send-filed] cno=5 msgNo=0x3400 raw data(22)=3400…0772756E74696D650131000464617461`（path=runtime/name=1/branch=/data）**7 次请求 × 3 次重试全都在发** |
| gs 侧 | `[q-hook] qid=31 … "retry":0/1/2/3` → `clean_timeout_cookie` 重试 3 次后 `do_operate` 超时返回 **-2** |
| DBA 侧 | `mv22/live_dba.txt` **0 字节**（连 `[accept]`/`[recv-accept]` 都没有） |
| MySQL | `show processlist`：DBA 的 `dl_ddb_1_test` 连接 `Time=18499s` —— **最后一次 SQL 在 ~17:25**，之后一次查询都没执行 |
| 进程 | 四个服务都活着、端口 ESTABLISHED、CPU 空闲（不是死循环；`Wait/UserRequest`） |
| 线程数 | 健康启动 DBA=**7** 线程，挂死时=**6**（少了一个） |

⇒ **读请求根本没进到 DBA 的 SQL 层**：DBA 的 filed 请求分发在长时间运行后死掉；
而 gs 侧 `filed.c` 只在 `dba_client_no < 0` 时 `try_to_connect_dba()`，
`internat_comm_closed()` 只处理 `ldba_client_no`，**dba 链路断了没有任何重连路径** ⇒ 永久 9030。

关于「数据不一致 9031」：**不是数据坏了**。`create_check_sum` 被实现成固定返回 1，
所以 `1 ^ -1418120497 = -1418120498`，库里 4 行 `path='user'` 的 checksum 全是 `-1418120498`，
`path='runtime'` 行数为 0（§5.N 修好后登出会正常删掉它）。9031 是同一故障下的一次瞬时异常
（`read_runtime_info` 的 -3 分支）。**没有 runtime 行本身是正常的**：`try_recover_data` 的
`case -1` 就是「没有 runtime 信息、不恢复、继续登录」。已用 keepdb 重启实测确认可登录。

#### 本轮加的诊断（默认开，量很小；下次挂死直接看日志定位）

- `[hb] port=… t=…s pktsIn=…(+) pktsOut=…(+) filedIn=…(+) filedOut=…(+) schedIters=…(+)
  sendFail=…(+) clients=…` —— **每个服务每分钟一行**。挂死时哪个计数不再增长就指向哪一层：
  收不到包 → 链路/接收线程；收到不回 → LPC 处理层；`schedIters` 停了 → 调度器线程。
- `[send-fail] cno=… not-found|bad-fd|err=… len=…` —— `send_to_client` 的失败**以前完全静默**
  （LPC 调用方都忽略返回值），这是「链路死了但没人知道」的根源。
- `[recv-filed] cno=… cmd=0x… body=…` —— 接收侧的 0x3400/0x3411/0x4401/0x2403/0x2405/0x2407/0x236F，
  和既有的 `[send-filed]`（**一直是无条件打的**）配对，一眼分清「没发出」还是「收到没回」。
  开关：`LPCVM_DBGLINK=1` 或 `LPCVM_TAIL`。
- 实现：`src/original_vm.h` 的 `liveness()` 计数器 + 调度器主循环每轮 `schedIters+1`
  + `src/game_service.cpp` 的 `startGameService` 里起一个 60s 心跳线程。

#### 复现/恢复用的脚本

- `_run_probe_keepdb.py <dir> <秒> [探针]`：**不清库**起 4 个服务（`_run_probe.py` 必定先跑
  `_dbclear.py`）。服务挂了但不想毁角色数据时用它恢复。
- `_db_runtime2.py`：按 `path='runtime'` 查 DDB（注意 `read_content(path,name,branch)` 的
  `path` 才是 'runtime'，`branch` 是空串——按 `branch='runtime'` 查是查不到的）。
- `_gbkgrep.py <root> <中文串…>`：GBK 反编译源码检索（`grep` 工具按 UTF-8 读会全部漏掉）。

#### 现场结论 / 下一步

- 已用 keepdb 重启恢复（mv23，714s 开出 8160），`_relogin_cycle.py 1 1 fix1` →「RE-LOGIN OK 15.3s」，
  角色位置 (138,158) 原样保留。
- **还没找到 DBA 分发线程为什么会死**。下次挂死时对比：DBA 的 `[hb]` 里
  `filedIn` 是否停止增长、`schedIters` 是否停、`clients` 是否异常，以及线程数是否从 7 掉到 6。

---

---

### Q. 「再次上线任务面板空白 / 背包不显示 / 商店没货」— 进行中（已收窄到 is_function(funcs["log"])）

用户口径：再次上线后任务没了、药店/防具/杂货/武器都没有物品、背包物品不显示（但存在，换位一下就出来）。

**已确认的链路（全部有日志证据；脚本 `_bagtask_probe.py` / `_sender_probe.py`）**

1. 客户端收到的 `0x2603`（任务日志）首次/重登都是 **62 字节**（正常 284 字节）；
   结构 = `pad_buffer(9731, 1, task_name, len(desc), desc, len(log), log, len(tip), tip, ...)`
   （`comm_v37.c:986`），我们这里 **desc/log/tip 长度全是 0**。
2. **不是「没发包」**：`LPCVM_SEND` 追踪显示每个包的发送函数与次数首次/重登完全一致。
3. **不是「存档形状不对」**：`dl_ddb_1` 原版数据与我们的 `dl_ddb_1_test` 形状一致：
   `"task":([729:(["state":"S2","alias":38,"upgrade_type":0,]),])`。
4. **不是「键类型」问题**：`[keyt]` 实测我们 restore/save 后 `carry` 的键是 `keyType=2`（int，裸 `101:`），
   与 IDA 的 `vm_restoreMixedString`(0x8107ecc) / `vm_saveValueAscii`(0x8107ac4) 行为一致 ⇒ 忠实。
5. `.list` 配置读得到：`[listread]` 显示 `task_name.list bytes=60178`、`task_alias.list bytes=4041`。
6. 任务数据在对象里：`[fn] RETURN query_task ret=(["state":"S0","alias":"【新手】新的开始",
   "name":"【新手】1-9级新手任务","upgrade_type":0])`。
7. `get_task_class` 返回 `【新手】1-9级新手任务` —— **这是对的**：`etc/task_name.list:2859` 就是
   `class : 【新手】1-9级新手任务`（与 name 同名）。
8. 模板**确实注册了**：boot 期间 `register_task_template` 共 **1072** 次，其中一次
   `obj=/gs/daemons/tasks/instruction_v4/sub_tasks/instruction_1.o a0=t4:【新手】1-9级新手任务
   a1=t6:(["dest": <function …>, "tip": <function …>, "log": <function …>])`。
9. **但 `task_log()` / `task_tip()`（模板里的闭包）整个登录过程一次都没被进入**
   （`ENTER task_log: 0`），而 `query_task_log` 进了 2 次并返回空串。
   按 `taskd_v37.c:1612`：
   ```lpc
   task_log = funcs["log"];
   if (is_function(task_log)) task_log = (*task_log)(me, task_info);   // ← 没走到
   else if (!is_string(task_log)) task_log = task_info["log"];
   if (!is_string(task_log)) return "";                                // ← 在这里返回
   ```
   ⇒ **`is_function(funcs["log"])` 为假**：从 `all_templates` 映射里读回来的闭包不再是闭包（类型 != 8），
   或者这次 `funcs["log"]` 查找落空。

**下一步（只差一次重启）**

`is_function` **不在** `src/builtins.cpp` 的 `call_builtin` 里（`[isfn]` 探针 0 命中即证据），
而在 `src/original_vm.cpp` 的 efun 名字哈希分发链（`callEfunWithArgs`）里。
把 `[isfn]` 探针搬到那里的 `is_function` 分支，打 `caller=obj::fn argType=<type> val=<dump>`：

- `argType != 8` ⇒ 我们 VM 在**映射里存/取闭包**时丢了闭包类型（查 `MappingValue` 的
  拷贝/删除/闭包共享），修那里；
- `argType == 8` 却仍不进 `task_log` ⇒ 是闭包**调用**路径的问题
  （`call_fun_ptr`/`call_other` 处理 `(: call_other, obj, "fn" :)` 这类闭包），修那条。

**背包 / 商店**：`0x5103`（build_inventory）首次与重登**逐字节相同**（112B），数据没问题；
结合「商店也没货」，怀疑与任务同一根（同一套 `easy_task`/模板按需克隆机制），
任务这条修好后一起复验。

---

### R. 任务面板空白：`is_type` 探针的直接证据（进行中，下一步已明确）

给 §5.Q 加了一个新探针：**opcode 36 `is_type`**（`original_vm.cpp` case 36）在
`query_task_log` / `query_task_tip` / `query_task_desc` 帧内时，打印
`want=<期望类型> valueType=<实际类型> val=<值>`（运行时用 `_probe_on\LPCVM_ISFN` 打开，
因为 `is_mapping`/`is_string` 在**我们的 VM 里不是 efun 调用**，而是编译成 `is_type` 操作码 ——
这就是为什么在 `call_builtin` 里加的 `[isfn]`/`[ismp]` 探针 0 命中）。

**登录时抓到的关键三行：**

```
[istype] fn=query_task      obj=/gs/clone/char/user.o want=7 valueType=6
         val=(["state":"S0","alias":"【新手】新的开始","name":"【新手】1-9级新手任务","upgrade_type":0])
[istype] fn=get_task_class  obj=/gs/clone/char/user.o want=4 valueType=0 val=(undefined)   ×3
[istype] fn=query_task_log  obj=/gs/clone/char/user.o want=7 valueType=0 val=(undefined)
```

解读：

1. `query_task` 返回的是**正常的 mapping**（`want=7`=is_mapping，`valueType=6`=mapping）⇒ 任务数据没问题。
2. `get_task_class` 里连续三次 `is_string`（`want=4`）拿到的都是 **undefined**：
   - `task_info["class"]`（本来就没有，正常）
   - `task_name_list[task_info["sub_name"]]`
   - `task_name_list[task_info["name"]]` ← **按任务名查类名失败**
   ⇒ 落到最后一行 `return task_info["name"]`（名字当类名返回）。
3. `query_task_log` 里 `funcs = all_templates[task_class]`，`is_mapping` 也是 **undefined**
   ⇒ `if (!is_mapping(funcs)) return "";` ⇒ 0x2603 的 desc/log/tip 全空。

**同时确认了注册是好的**（`[fn] ENTER register_task_template` + `[fnargs]` 打十六进制）：

```
obj=/gs/daemons/tasks/instruction_v4/sub_tasks/instruction_1.o
  a0="【新手】1-9级新手任务" len=21 hex=A1BED0C2CAD6A1BF312D39BCB6D0C2CAD6C8CECEF1
  a1=(["dest": <function …>, "tip": <function …>, "log": <function …>, …])
```
（`etc/task_name.list:2858-2859` 里这个任务的 `class` 与 `name` **同名**，且 `class` 那行的值是
**不带引号**的：`class : 【新手】1-9级新手任务`，而别的条目是 `class : "新手指导"`。）

**现在剩两个必须二选一（或同时）的原因：**

- **(a) `#FORM2` 解析**：不带引号的中文值可能没被解析成字符串 ⇒ `task_name_list[name] = undefined`
  （键存在但值是 undefined，正好表现为 `is_string` 得到 `valueType=0`），于是 `get_task_class`
  只能回退成名字。
- **(b) `all_templates` 查不到**：即使回退成名字（而注册时用的就是这个名字、
  `etc` 里的 class 也同名），`all_templates[名字]` 仍是 undefined ⇒ 要么那个映射在查询帧里
  读到的是**另一个对象/另一个 taskd 实例**的全局，要么注册时 `register_task_template`
  在 `error()` 处提前退出（第 1676/1685 行的 dest / convert-restore 校验）而没执行到
  1688 行的 `all_templates[task_class] = task_template;`
  （`LPCVM_ERR_TRACE=1` 的 boot 里只有 3 条 `[lpc-error]`，都与任务无关，所以更像是 (a)+(b)
  里的"读到别的实例"）。

**下一步（一个探针就能定案）**：把 `[istype]` 探针扩展成——当待测值是 mapping 时，
顺手把它的 `"name"` 键的值打成 **len + hex**，和上面注册时的 21 字节 hex 对比：
- 若与 `A1BED0C2CAD6A1BF312D39BCB6D0C2CAD6C8CECEF1` **不同** ⇒ 编码/还原问题（回到 DB 读写或 pak 读的编码），修那里；
- 若**完全相同**却查不到 ⇒ `all_templates` 在那个帧里不是注册时那个映射 ⇒ 查 taskd 是否被二次
  初始化（`all_templates = ([ ])` 在全局初始化段）或存在两个 taskd 实例，修 var-block/实例解析。

同时已加进 `[filed-call]` 诊断名单（`filedFns[]`）的：`query_task_log` / `query_task_tip` /
`query_task_desc` / `query_task` / `get_task_class` / `send_task_log`，下次可以直接看
`vars/varsOwner/varsIsOwn/ownedFn`。
（注意：`varsOwner` 这一列在多实例共享变量块时会指到"最后一个匹配的注册键"，**不可单独作为证据**。）

另外本轮已经在 `src/original_vm.cpp` 的 `startInlineFunction` 里加了一条修正：
`switchObjVars` 传入的变量块如果不是**被调函数的 home 实例**的块，就用 home 的块。
这条修正本身是对的（同样的不变量），但**没有解决本问题**（实测 0x2603 仍是 62 字节），
说明这条链上的变量块并不是错的，问题更可能在 (a)/(b)。

---

### S. 任务面板空白的**根因**：同一对象有两个实例，变量块选到了**没初始化**的那个

§5.R 的探针再往前推一步就抓到了。`[istype]` 在 `query_task_log` 里打了三件事：
它测的值的类型、**这个帧正在读哪个变量块**、以及**该块里 taskd 的全局映射长什么样**。

```
[istype] fn=query_task_log obj=/gs/clone/char/user.o want=7 valueType=0 val=(undefined)
         vars=779393692 varsOwner=clone:gs/clone/char/user.o#15271 own=1331425156
         taskdBlk=779393692 this=clone:gs/clone/char/user.o#15271
         taskdMaps{ }                       ← 空！
```

对照同一进程里另一条路：

```
[treg] ret type=6
[treg] size=1110                            ← query_all_templates() 返回 1110 个模板
```

其中 `[treg]` 是通过 `callOriginalObjectFunction("/gs/daemons/taskd.c",
"query_all_templates", …)` 调的（走注册表按**路径**解析），而 `taskdMaps` 是直接读注册键
`pak:gs/daemons/taskd.o` 那个 VM 的变量块。

**结论：`/daemons/taskd.o` 在注册表里有两个实例**

- 一个**活着的、跑过全局初始化**的实例：`all_templates` 里有 1110 条注册（`register_task_template`
  实测调用 1072+ 次，且 `a0` 的十六进制与运行时 `task_info["name"]` **逐字节相同**：
  `A1BED0C2CAD6A1BF312D39BCB6D0C2CAD6C8CECEF1`，21 字节）；
- 一个 `pak:gs/daemons/taskd.o` 的**proto 实例**，它的全局变量块是空的（没有任何 ≥50 条的映射）。

而 `query_task_log` / `get_task_class` / `query_task` 这一串内联帧读的正是 **proto 那个空的块**
（`vars == taskdBlk`），于是：

- `task_name_list[…]` → undefined ⇒ `get_task_class` 走 `catch(error(...))` / 回退，类名解析不出来；
- `all_templates[task_class]` → undefined ⇒ `if (!is_mapping(funcs)) return "";` ⇒
  `0x2603` 的 desc/log/tip 全空 ⇒ 客户端任务面板空白。

这与 §5.N 的 9026 是**同一类** bug（同一个对象存在多个 VM 实例，选错了实例 ——
那次错的是闭包 owner 实例，这次错的是"函数 home 实例"）。§5.R 里那条
`switchObjVars` 修正本身是对的，但它把块换成了 `originalHomeVmOf(&fn)` 给出的实例，
而那个函数是**扫描注册表里第一个 ownsFunction(fn) 的 VM** —— proto 的键
`pak:gs/daemons/taskd.o` 按字典序在 clone 之前，于是永远选中空的那个。

**下一步（修）**：让"函数 home 实例"的解析与引擎其它地方一致 / 跳过未初始化的实例，候选做法：

1. `originalHomeVmOf()` 里优先选**跑过全局初始化**的实例（proto 的 `objVars_` 全是 undefined，
   可以用"是否存在至少一个非 undefined 的全局"或直接记录 `initialized_` 标志来判断）；
2. 或按**路径键**解析（与 `tryCallOtherObject` / `envVmForKey` 一样的规则），并优先
   `pak:<rel>` 之外的活实例；
3. 修完用**同一个判据**验收：重登后 `0x2603` 从 **62 字节回到 284 字节**
   （`pad_buffer(9731,1,task_name,len(desc),desc,len(log),log,len(tip),tip,…)` 三个长度非 0），
   并且 `[istype] fn=query_task_log want=7` 的 `valueType` 变成 **6**（mapping）。

**背包 / 商店**：`0x5103` 两次逐字节相同（112B）说明数据在；商店/任务都依赖这类
"按需克隆 + 全局注册表"的 daemon，怀疑同一根因，任务修好后一起复验。

---

### T. taskd 的**变量块只有 15 个全局**——`all_templates` 等靠后的全局根本不在块里

§5.S 的结论（"两个 taskd 实例、选到了空的 proto"）**被这一轮的数据推翻**。新探针把注册表里
所有 sourceName 含 "taskd" 的 VM 连同**变量块指针 / 块大小 / 块里最大的映射**一起打了出来：

```
taskdVms{
  [pak:gs/daemons/sort_taskd.o   blk=627511708 globals=3  maxMap=0]
  [pak:gs/daemons/couple_taskd.o blk=378595212 globals=6  maxMap=8]
  [pak:gs/daemons/taskd.o        blk=797122124 globals=15 maxMap=20]   ← 唯一一个 taskd.o
  [pak:gs/daemons/mobile_taskd.o blk=648330324 globals=4  maxMap=1]
  [pak:gs/daemons/easy_taskd.o   blk=626909724 globals=10 maxMap=32]
  [pak:gs/daemons/chain_taskd.o  blk=388363620 globals=1  maxMap=4]
}
```

要点：

1. **`/daemons/taskd.o` 只有一个实例**（不是两个），键就是 `pak:gs/daemons/taskd.o`，
   `vars == taskdBlk` 说明失败帧读的就是它 —— 所以 §5.S 的"选错实例"不成立。
2. 但它的**变量块只有 15 个全局**（`globals=15`），块里最大的映射只有 **20** 条 ——
   而 `register_task_template` 被调用 1072+ 次、`query_all_templates()` 返回 **1110** 条。
3. 反编译的 `taskd_v37.c` 里全局变量有几十个（`all_templates` 在第 21 行、
   `task_name_list` 44、`new_task_name_list` 37、`org_task_name_list` 39 …），
   **`all_templates` 不可能落在前 15 个之内**。

⇒ **根因（本轮最可能的结论）：`taskd.o` 的实例变量块尺寸小于对象的全局变量数
（`globalVarCount`），靠后的全局（`all_templates`/`task_name_list`/`new_task_name_list`/
`org_task_name_list`…）在块里根本不存在**，读出来就是 undefined ⇒
`is_mapping(all_templates[task_class])` = 0 ⇒ `query_task_log` 返回 "" ⇒ 任务面板空白。
这也解释了为什么 `task_name_list[…]` 三次 `is_string` 全是 undefined（§5.R）。

而 `[treg]` 那次 `query_all_templates()` 返回 1110 —— 说明**同一进程里另一个路径读到了完整的块**
（或那次调用落在别的 VM 上），这一点还需要下一次测量区分：**把 `object().globalVarCount`
和 `blk->size()` 并排打出来**。

**下一步（一次测量 + 一处修）**：

1. 探针加 `object().globalVarCount` 与 `blk->size()` 的对比（以及 OOB 读全局时的处理）；
2. 若确认 `size < globalVarCount`，修**变量块的分配/共享/拷贝**处，让块尺寸始终等于（或大于）
   `globalVarCount`（重点看 `shareObjVars`/`adoptObjVars`/`objVars()` 的 resize、以及
   `OriginalVm` 构造里 `obj.globalVarCount` 的来源）；
3. 验收判据不变：**重登后 `0x2603` 从 62 字节回到 284 字节**，且
   `[istype] fn=query_task_log want=7` 的 `valueType` 变成 6。

**本轮已试但无效**：让 `originalHomeVmOf()` 在多个 VM 都 ownsFunction(fn) 时优先选
"已初始化全局最多"的那个 —— 实测 0x2603 仍是 62 字节（因为只有一个 taskd 实例，
`vars==taskdBlk`，问题不在选实例而在块本身）。这条改动本身无害（单实例时行为不变），保留。

### U. 任务面板空白的**真正根因**：跨对象内联取到了别的对象的「活动变量块」（**已修复，已端到端验证**）

§5.T 的结论（"变量块分配小了"）**是错的**：块从来不是分配不足，而是**取块时取到了另一个对象
正在用的活动块**。上一轮新加的 `[vmsize]` 构造探针直接证明分配是对的：

```
[vmsize] ctor-ref obj=/gs/daemons/taskd.o blk=28 gvc=28 short=0 key=<this>
```

于是这一轮加了两个探针把"谁把块换掉了"钉死：
`[autovars]`（`LPCVM_AUTOVARS=1`，打印内联帧选中的 home VM 及其块）和
`[vs]`（每次 `objVars_` 交换都打印 from/to 大小、指针、frames、parked）。

**决定性证据（mv49，首次登录 + 重登）**

```
31851: [autovars] fn=send_this_task_log caller=/gs/daemons/taskd.o callerKey=pak:gs/daemons/taskd.o
         home=/gs/daemons/tasks/instruction_v4/sub_tasks/instruction_1.o homeKey=pak:...instruction_1.o
         homeBlk=793516460 size=15 homeGvc=15
31856: [autovars] fn=get_task_class caller=/gs/clone/char/user.o
         home=/gs/daemons/taskd.o homeKey=pak:gs/daemons/taskd.o
         homeBlk=793516460 size=15 homeGvc=28      ← 同一个 15 槽块，被当成 taskd 的块用了
31781: [autovars] fn=get_task_class caller=/gs/daemons/userd.o
         home=/gs/daemons/taskd.o homeBlk=380689172 size=28 homeGvc=28   ← 正常时是 28
```

`[istype]` 那行同时给出 `globals=15 gvc=28`（注册表 dump 里 `pak:gs/daemons/taskd.o`
`ownSize` 一直是 28，`blk` 是 15）。

**机制**（与原版驱动的差异）

1. `taskd` 的注册表 VM（键 `pak:gs/daemons/taskd.o`，`globalVarCount=28`）执行**任务模板的闭包**
   （`taskd_v37.c:1628` `task_log = funcs["log"]; (*task_log)(me, task_info)`）时，
   我们的内联帧把它的**活动块**切成模板 `instruction_1.o`（15 个全局）；该进程挂起(park)期间
   这个活动块一直保留。
2. 上线的 `comm_v37.c:958-961` 走 `"/gs/daemons/taskd.c"->query_task_log(...)` /
   `->get_task_class(...)`。我们的 `call_in_other_fun_ex` / `call_other` 内联分支用的是
   **`targetVm->objVarsShared()`（活动块）**，而不是那个 VM 的**身份块** `ownObjVars_`。
   于是 taskd 的函数读到模板的 15 槽块 ⇒ `all_templates` / `task_name_list` 全 undefined
   ⇒ `is_mapping(funcs)` 为假 ⇒ `query_task_log` 返回 "" ⇒ 0x2603 只有 62 字节。

原版驱动里每个对象**只有一份变量实例**（`current_object` 切的是对象，不是块），
所以"另一个函数正在跑时的临时块"永远不会被当成某对象的全局空间 —— 我们的内联实现必须
显式区分「身份块」和「活动块」。

**修复（`src/original_vm.cpp`）**：凡是要"按对象身份取变量块"的地方一律改用 `ownObjVars_`
（新增访问器 `ownObjVarsShared()`）：

- `startInlineFunction` 的自动切块与 `switchObjVars` 修正 → `home->ownObjVarsShared()`；
- `call_other` / 操作码 8 内联路径的 6 处 `targetVm->objVarsShared()`；
- 闭包 home 内联的 `fnHome` / `h` 两处；
- `shareObjVars()`（线程必须共享属主的**身份块**，而不是属主当时的活动块）；
  `drainPendingOriginalThreads()` 的 `adoptObjVars` 同理；
- `originalHomeVmOf()` 的"已初始化全局最多"计数。

**验证（mv50，keepdb 真机登录 + 重登，脚本 `_bagtask_probe.py 1 1 fix1 18`）**

```
[autovars] fn=get_task_class ... home=/gs/daemons/taskd.o homeBlk=384007196 size=28 homeGvc=28
[istype] fn=query_task_log want=7 valueType=6 val=(["dest": <function ...>, "tip": <func...])
[istype] fn=query_task_log want=9 valueType=8 val=<function call_other("...instruction_1.c","task_log")>
[istype] fn=query_task_log want=4 valueType=4 val=…#Z揽仙镇外(108,156)#Z…  len=49
=== A: 0x2603 x1 lens=[284]
=== B: 0x2603 x1 lens=[284]        ← 首次与重登都是 284（原来两次都是 62）
```

验收判据（重登后 0x2603 回到 284 字节 + `[istype] want=7 → valueType=6`）**达成**。

**背包 / 商店**：`0x5103`（build_inventory）首次与重登仍**逐字节相同**（112B，带物品名与数量），
说明服务端数据没问题；这两项是客户端可见症状，而它们的上游链
（`dealer_v37.c:list_goods → pad_goods_buffer → me->send(8563, …)`、`0x2173` 商品列表，
基础对象/模板的按需克隆）与任务面板**共用同一套跨对象内联机制**，
本轮修复对这一类是直接生效的 —— 需要真实客户端复测确认。

---

### V. 「背包不显示」+「商店没有物品」——两个根因（**已修复，服务端已端到端验证**）

§5.Q 里"背包/商店怀疑与任务同根"的猜测**只对了一半**：商店确实是同一条跨对象链，
但真正让客户端看到空背包/空商店的是另外两个独立缺陷。

#### V.1 背包：`0x5103` 的物品数量字段一直是 0

`buildmsgd_v37.c:5781 build_inventory()`：

```lpc
buf = make16_0_buf;                       // 2 字节数量占位
... buf += (make8(i) + build_item(item)); // 逐个物品
set_bits(&buf, 0, 16, ret);               // 把数量写回前 2 字节
```

我们实际发出的包（`_bagtask_probe.py`，112 字节）开头是 `0000 6500 …`：
**数量字段是 0**，后面挂着完整的物品条目 —— 客户端按 count=0 渲染，背包就是空的；
换位/使用物品走的是另一条"单格更新"包，所以物品又会冒出来，与用户描述完全一致。

两个缺陷：

1. `OriginalVm::callEfun` 对每个实参取 `pop().value`。左值实参 `&buf` 是 REF 栈项，
   `.value` 是 **undefined**（这正是 §5.Q 里 `popArgs` 注释写过的现象），
   于是 `set_bits` 拿到的是 undefined，写回自然失败。
2. `set_bits` 只实现了"整数位运算"形式，**没有 buffer 写回**形式。
   原版 `vm_f_set_bits` 的语义是 `set_bits(&buffer, bitno, nbits, value)`：把字段按
   **MSB 优先（网络序）** 写进 buffer 原地。

修复（`src/original_vm.cpp` + `src/builtins.cpp`）：

- `callEfun` 对 `isRef` 的实参改用 `deref(it)`（容器是 shared_ptr 语义，原地写能回到调用方变量）；
- `set_bits` 增加 buffer 分支（原地按位写、返回该 buffer），`get_bits` 同样支持 buffer。

验证：`0x5103` 首字节由 `0000 6500 …` 变成 **`0001 6500 …`**（count=1，与包内 1 个物品条目一致），
首次登录与重登一致。

#### V.2 商店：缺省实参没有生效 → 商品 `goods_amount` 是 undefined

`dealer_v37.c:680 pad_goods_buffer()` 的过滤器是：

```lpc
if ((!is_object(ob = goods[items[i]][1])) || (!ob->query_temp("goods_amount"))) items[i] = 0;
items -= ({ 0 });
```

所以只要有一样商品的 `goods_amount` 取不到，它就被丢掉；**全部丢掉 → 客户端看到空商店**。
新增探针把注册表里各商店 VM 的 `goods` 映射和"商品对象的 goods_amount"打出来：

```
# 修复前（mv52/mv53）
shopItems{ [clone:…/yaodian-laoban.o#5257  item=…/medicine/basic.o#5269 stock=12 goods_amount=(undefined)]
           [clone:…/buzhuang-laoban.o#4994 item=…/belt/basic.o#5017     stock=23 goods_amount=(undefined)]
           [clone:…/wuqidian-laoban.o#5019 item=…/weapon/basic.o#5059   stock=40 goods_amount=(undefined)] }
[ga] set_temp obj=…/medicine/basic.o this=…medicine/basic.o#5269 own=1 blk=7 arg1=(undefined)
```

即货架**建好了**（12/23/40 件，对象都在），但每件商品的 `goods_amount` 是 undefined。
`add_goods`（`dealer.o` 索引 39）末尾就是 `obj->set_temp("goods_amount", amount);`，
而 `amount` 的缺省值 **32767 写在原型上**：

```lpc
// dealer_v37.c:45（原型）
mixed add_goods(mixed the_goods = 0, int amount = 32767, int cost_type = 1, int renew = 0, int max = 32767);
// 定义里没有缺省值 —— 由被调函数的 inline 缺省块补齐
mixed add_goods(mixed the_goods, int amount, int cost_type, int renew, int max) { ... }
```

我们的 `check_args_num`（操作码 15）**把操作数当成了实参个数**：

```cpp
int maxArgNo = skipAll;                       // ← 错
if (maxArgNo < declared) maxArgNo = declared; // → add_goods 得到 6
if (passed >= maxArgNo) ip_ += skipAll;
```

新探针 `LPCVM_ARGDBG=add_goods` 实测（mv54）：

```
[argdbg] check_args_num fn=add_goods passed=1 declared=5 operand=6 padArgs=4
         next8= 0 32767 1 0 32767 …        ← 5 个缺省值 push（含 the_goods=0）
[argdbg] resort_stack  fn=add_goods defaultPushed=5 argNum=1 maxArgNo=6
```

操作数 **6 = 缺省块长度**（5 个 push + 1 个 resort），不是实参个数。
把 6 当 maxArgNo 使 `missing = maxArgNo - argNum` 多算 1，`resort_stack` 于是多留一个缺省值，
**所有带缺省值的形参整体错位一格** → `amount` 变成 undefined（多出来的那个槽是 undefined pad）。

修复（`src/original_vm.cpp`）：

- 操作码 15：`maxArgNo = fn->argCount`（Vm_Fun_T 的 maxArgNo，来自 .o 头），
  只有"所有声明形参都传了"才 `ip_ += 操作数`（操作数 = 缺省块长度）；
- 帧清理：缺省块 `resort_stack` 跑过之后，我们预填的 zero pad 仍留在重排后的形参**下面**，
  所以 `padEff` 不再按 `usedArgs == passedArgs` 判断，直接减 `padArgs`（否则会漏到调用方的栈帧里）。

验证（mv55）：

```
[argdbg] resort_stack fn=add_goods defaultPushed=5 argNum=1 maxArgNo=5      ← 正确
[ga] set_temp … arg1=32767  ×1474（修复前：32767 只有 31 次，undefined 1444 次；现在 0 次 undefined）
shopItems{ [yaodian-laoban.o#5257  stock=12 goods_amount=32767]
           [buzhuang-laoban.o#4994 stock=23 goods_amount=32767]
           [wuqidian-laoban.o#5019 stock=40 goods_amount=32767] }
```

⇒ 药店/布庄（防具）/武器店的商品都能通过 `pad_goods_buffer` 的过滤，`0x2173`（cmd 8563）
商品列表不再为空（**待真实客户端复测**）。杂货铺同理。

#### V.3 本轮新增探针（都在 §6 里）

| 开关 | 作用 |
| --- | --- |
| `LPCVM_AUTOVARS=1` | 内联帧选中的 home VM 及其变量块（`[autovars]`） |
| `[vs]`（无开关，跟随 `[vmsize]` 采样） | 每次 `objVars_` 交换的 from/to |
| `LPCVM_ARGDBG=<函数名>` | 操作码 15/71 的 passed/declared/操作数/缺省块中的值 |
| `_probe_on\LPCVM_GA`（运行时） | 所有触碰 `goods_amount` 的 set_temp/query_temp |
| `_probe_on\LPCVM_ISFN` | `is_type` 探针；其中新增 `taskdVms{} / shopVms{} / shopItems{}` 注册表转储 |


#### V.4 四家商店的实测货架（同一构建、真实登录）

`[istype]` 的 `shopItems{}` 转储（登录时打印，`_probe_on\LPCVM_ISFN`）——四家用户点名的商店
货架都建好、且每件商品都带 `goods_amount=32767`（`pad_goods_buffer` 的过滤条件）：

| 商店 | NPC | 货架条数 | 商品对象 `goods_amount` |
| --- | --- | --- | --- |
| 药店（万全药铺 map 1003） | `lanxianzhen/yaodian-laoban` | **12** | 32767 |
| 防具（天锦布庄 map 1001） | `lanxianzhen/budian-laoban` | **24** | 32767 |
| 防具（布庄 wumingxiaozhen） | `wumingxiaozhen/buzhuang-laoban` | **23** | 32767 |
| 杂货（永合杂货铺 map 1005） | `lanxianzhen/zahuodian-laoban` | **20** | 32767 |
| 杂货（wumingxiaozhen / donghai-yucun） | `zahuodian-laoban` | 21 / 20 | 32767 |
| 武器（威扬武器店 map 1002） | `wumingxiaozhen/wuqidian-laoban` | **40** | 32767 |

⇒ 过滤后 `items` 不再为空，`build_goods_list` 会给 `0x2173`（cmd 8563）写出真实商品条目，
客户端商店窗口不再空白（**最终以真实客户端为准**）。

**本轮同时记下一个新观察（未修，与本目标无关）**：用脚本把角色坐标直接写进存档、落在室内房间
（如 `万全药铺` map 1003）时，登录后**看不到任何 NPC**（无 `0xF05D`），因此脚本无法在店内
打开菜单；而 `揽仙镇外`（map 2000）同样的方式登录就能看到 3 个 NPC。`揽仙镇`（map 1000）
本身最近 NPC 距离 25 > 21（`is_near` 门限），所以空旷是正常的；室内这条更像是
"big-map / `idle_template` 房间实例"的 NPC 摆放只在正常走门进入时才发生。用户客户端能看到
商店 NPC（能打开商店窗口），说明正常路径没问题 —— 这里只作为"脚本复现商店窗口"的限制记录下来。


### W. 「重上后剧情不触发」的**根因**：`replace_owner()` 是返回 int 0 的空桩（**已修复，已实测**）

用户口径：*在线时剧情能一路触发；中途在任意位置下线、再上线后，走到剧情触发点没有任何反应。*

**链路**（都在 LPC 里，已从反编译源 + 字节码确认）：

```
客户端走进门口 -> cmd_enter_room(0x1026, s8(房间名))
  -> process_command("enter_room", ({name})) -> /gs/cmds/normal/enter_room.c::main
  -> "/gs/daemons/moved.c"->enter_room(me, env_dest, x, y, dir)
  -> me->trap_into(SF_ENTER_ROOM, env)            // shadow.c
  -> all_shadow_list["sf_enter_room"] 里注册的闭包 (*f)(env)
  -> instruction_1::sf_enter_room(me, room)      // 状态机，例如 S2 -> 万全药铺
  -> raise_event_ex("求疗伤药", me, ...)          // 剧本/剧情包 0x20D1
```

注册侧（`instruction_1::try_create_task` / `task_init`，也就是重上时的恢复路径）：

```lpc
me->register_interface(this_object(), me,
    ([ SF_ENTER_ROOM: replace_owner(0, (: call_other, ".../instruction_1.c", "sf_enter_room", me :)) ]));
```

**根因**：`replace_owner` 在我们引擎里是 `if (name == "replace_owner") return make_int(0);`
（`src/builtins.cpp`）。于是过滤映射的**值**变成 int 0，`shadow.c::register_interface`
把它原样塞进 `all_shadow_list`：

```
[istype] iface1[g17]{ sf_enter_room=arr1[n3 t7 t7 t2] ...}   ← 第三个元素 t2 = int（应该是 t8 闭包）
[istype] iface1[g17]{ sf_kill=arr1[n3 t7 t4 t8:sf_kill] }     ← 直接写闭包的注册是正常的
```

`trap_into()` 里 `if (is_function(f = cinfo[2])) ret = (*f)(para); else 删除该条目;`
⇒ **事件永远不触发**，条目还会被清掉。这就是"重上后剧情触发点没用"，
同时也解释了其它走 SF_* 接口的功能（部分菜单/剧情/战斗钩子）不动。

**修复**（`src/builtins.cpp`）：真正实现 `replace_owner(ob, closure)`——复制闭包，
`ob` 是对象时把 `owner/ownerKey` 指到它，否则清空 owner（efun 闭包如
`(: call_other, ... :)` 本来 owner 为空，原样返回）。非闭包参数按原版返回该值。

**实测**（mv61，账号 2 / 角色 `waesrdt`，任务状态 S2 = "找王老板"）：

```
# 触发前：状态 S2，人在揽仙镇 (197,186)
--- sent cmd_enter_room(万全药铺)
   *** DRAMA 0x20D1 len=499  <<< 剧情触发
日志：
  [fn] ENTER trap_into     obj=/gs/clone/char/user.o args=2
  [fn] ENTER sf_enter_room obj=/gs/clone/char/user.o args=2
  [fn] ENTER raise_event_ex obj=/gs/clone/char/user.o args=3
```

**重上线复现**：先让角色在揽仙镇下线（干净 0x3E2A），再上线 →
`cmd_enter_room(万全药铺)` → **同样收到 `0x20D1 len=499`** ⇒ 用户报的"重上不触发"已修好。
（修复前同样的操作：只有 `0xE003` exits 包，没有任何剧情包。）

同时回归验证 §5.U/§5.V 的两项：`0x2603` 首次/重登都是 **284 字节**、`0x5103` 首两字节
`0001`（1 件物品）—— 都没被这次改动影响。

**新增探针**：`[istype]` 行里的 `iface0/iface1{}`，把角色 VM 里 `all_shadow_list` /
`all_interface_list` 中含 `sf_*` 键的映射连同注册项的形状一起打印
（`arr{n}[t7 t7 t8:闭包名]`），一眼就能看出"闭包没注册进去"这类问题。


### X. 两个数值错误：守护神「守护时长」与天书属性（**已修，见验证说明**）

#### X.1 守护神守护时长 = 20713天3小时26分钟

`statued.c::init_menu_content` 的菜单文案：

```lpc
menu_content = sprintf("……%s守护神！……（守护时长：#R%s#n，……）…",
                       map_name, time_length_to_ch(time() - (int)npc->query("last_statue_time"), 3));
```

`last_statue_time` 在整个游戏里**只有** `statued::do_set_statue_time()` 会写
（`statue_template.c:144`，即"有队伍把雕像打下来"时）；清库后的新世界这个字段根本不存在，
于是 `time() - 0` = 整个 Unix 纪元 = **20713 天**（已从字节码确认：`push_i(3) / time() /
npc->query("last_statue_time") / sub / call_global_fn(#771=time_length_to_ch, nargs=2)`，
`#771` 就是 `time_length_to_ch(秒, 3)`）。

用户期望的语义是"没有守护者时从服务器启动开始算"。修复（`src/original_vm.cpp`）：
对象构造完成时（`runInit()` 之后 / 协作构造 phase 3 里 `markObjectInitialized()` 之后）
如果是雕像程序 `/gs/clone/npc/misc/statue.o` 且 `query("last_statue_time")` 为空，
就 `set("last_statue_time", time())`，并打一行
`[statue] seeded last_statue_time=<epoch> on /gs/clone/npc/misc/statue.o`。
之后任何一次守护者更替都会被 `do_set_statue_time()` 覆盖，语义与原版一致。

实测（mv62 启动日志，清库后）：`[statue] seeded last_statue_time=1789621313`；
雕像基础对象（`find_basic_object_by_import_name("misc/statue.c")` 在
`do_init_after_loaded` 的循环里创建）先被播种，之后 `clone_object` 出来的各**地图雕像**
从其 dbase 继承该值，所以客户端看到的是"几分钟"而不是 5.6 万年。

**说明（未验证到的部分）**：本轮清库后的测试世界里，脚本站在雕像坐标 (29,147)
（`etc/statue.list`：揽仙镇外 coordinate 27,147）**收不到任何 NPC 广播（0xF05D）**，
所以没能像剧情那样直接截到菜单里的"守护时长"文本 —— 这与 §5.V.4 记录的
"脚本落在室内/动态房间里看不到 NPC"是同一类现象（玩家正常登录进去时 NPC 是在的，
用户能看到守护神就说明如此）。这一条最终仍需真实客户端点开守护神菜单确认。

#### X.2 天书属性数值错误、属性被固定

`god_bookd.c::init_god_book(book)` 生成金色天书：

```lpc
attribs = random_get_basic_attrib(5, ({}), 1);          // 5 条基础属性（加权随机）
loop (attrib in attribs) prop[attrib] = random_attrib_value(attrib);
book->lite_set("prop_basic", prop);
attrib = random_get_extra_attrib(book->get_name());      // 额外属性
book->lite_set("prop_extra", ([ attrib: random_attrib_value(attrib) ]));
```

`random_attrib_value()`（god_bookd.c:2296）对基础属性返回

```lpc
random_attrib(arr[0], arr[1], minimum(arr[2], 80))      // arr = get_attrib_range() = (最小, 标称, 最大)
```

`get_attrib_range()` 直接取 `god_book_attrib.list` 的第 6/7/8 列
（例：max_life, basic, 1, 1, 180, **15%, 30%, 100%**, 400, 气血）。

**根因**：我们的 `random_attrib` 是

```cpp
if (name == "random_attrib") return make_int(sharedRand31());   // 参数全被忽略
```

—— 直接把一个 31 位随机数当属性值返回，**完全无视 min/标称/max 区间**。
所以天书（以及内丹 `seald.c`、宠物装备 `child_equipd.c`、`itemd.c` 的属性 roll）
属性值是十亿级的乱数、也不落在配置区间里。

**修复**（`src/builtins.cpp`）：实现 `random_attrib(min, nominal, max)`——
以 `nominal` 为众数的三角分布，取值范围严格落在 `[min, max]`，
参数顺序颠倒时自动交换，退化区间（min==max、max<min）也安全。

实测（`lpcvm run _t_randattrib.c`，500 次采样）：

```
max_life roll: min=15 max=80 avg=42   (期望 [15,80]，均值 ~40)
seal roll    : min=443 max=1896 avg=1139 (期望 [400,2000]，均值 ~1133)
degenerate   : 5 8 1                  (random_attrib(5,5,5)=5 / (10,3,7)=8 / (0,0,1)=1)
```

⇒ 天书属性值现在是"在配置区间内、围绕标称值的随机数"，
`prop_to_max_value()`/`get_attrib_value_max()` 给出的上限也重新对得上
（上限来自同一张表的第 8 列）。


## 6. 探针（全部 env 开关，**不要用 `LPCVM_TRACE`：它会让 gs 启动中途访问违例崩溃**）

### 6.0 运行时开关（**新**：不用再为重开探针等 11 分钟重启）

`LPCVM_*` 里那些在热路径上被读的开关都是"进程启动读一次并缓存"，所以探针没法在**已经跑起来**的
服务器上临时打开。现在多了一条**文件开关**，每秒重读一次：

```powershell
New-Item -ItemType Directory -Force -Path _probe_on | Out-Null
Set-Content -Path _probe_on\LPCVM_ECHO -Value x -NoNewline   # 打开
Remove-Item _probe_on\LPCVM_ECHO                             # 关闭
```
判据是「env 变量存在 **或** `_probe_on\<名字>` 文件存在」（路径相对于 lpcvm 的工作目录＝工程根）。
已接到文件开关上的探针：`LPCVM_NEWBIE`、`LPCVM_EVDBG`、`LPCVM_SVC_TRACE`、`LPCVM_QUEUE_TRACE`、
`LPCVM_ECHO`。实现见 `lpc::dynProbeFlag()`（`src/original_vm.h`/`original_vm.cpp`）：
一个 watch 线程每秒刷新，调用点只付一次 relaxed atomic load。

**建议做法**：启动时**不要**给这些探针（日志小、启动快），跑测试前 1 秒再 `Set-Content` 打开。

| 探针 | 用途 / 输出 |
|---|---|
| `LPCVM_SAMPLE`（新增） | 每 4096 条指令采样，每秒打印 `[sample] n=… \| obj::fn=次数 …`（热点剖析） |
| `LPCVM_SCHED`（新增） | 每秒 `[sched-rate] iters/s=… hot=… ready=… parked=… timers=… minDt=… qwait=… evwait=… sock=… mapIns/s=…` |
| `LPCVM_TIMER`（新增） | `[timer] schedule <回调> in <n>s obj=…` |
| `LPCVM_MENU`（新增） | `[menu-call]/[menu-ret]` 闭包调用链 + `[temp-op] <obj>::lite_set_temp/lite_query_temp/set_temp args=[…]` |
| `LPCVM_CFV`（新增） | op 23 `create_function_value` 解析：旧行 `[cfv] obj=… cur=… a=… funOffset=… tbl[a]=… tbl[a+off]=… mstr=…`（每个闭包建点）+ 新行 `[cfv-loc] … -> <解析到的函数名>`（仅 `loff!=0`）。**查"闭包绑错函数"首选这个**（一次启动 1636 行，代价约 +190s） |
| `LPCVM_MALL`（新增） | 在线商城/元宝扣费全链路：`[mall] ENTER/RETURN`（对象+函数+参数+返回值，按 (对象,函数) 每个 60 行上限）、`[mall-send]`（只记 0xFD27/0xFD67/0x5109/0x6372/0x6373/0x6376/0x6377/0x1166/0x3E24/0x3393）、`[mall-rv]`（`restore_value` 的输入长度/是否解析成功/结果类型/前 90 字节） |
| `LPCVM_MALL2`（新增） | 对 `buy_from_mall*`/`decrypt`/`precost_coin`/`do_when_got_precost_coin`/`send_result` **逐指令**打印 `[mall-op] fn=… ip=… op=… a=… b=… sp=…`（开关必须 static 缓存，否则拖慢启动 2 倍以上） |
| `LPCVM_MOVE` / `LPCVM_NEAR` | `[move] check_steps …`、`[near-env] q=… env=…`、`[near-off] (x,y)->(x,y)=…`、`[near-gob] id=… -> obj` |
| `LPCVM_SLOW_OP` | `[slow-op] <µs>us op=… obj=… fn=… ip=…`（单条指令 >阈值，含阻塞等待判定） |
| `LPCVM_OP_PROF` / `LPCVM_EFUN_PROF` / `LPCVM_LOAD_PROF` / `LPCVM_CLONE_PROF` / `LPCVM_STEP_PROF` | 各类聚合剖析 |
| `LPCVM_ERR_TRACE` | LPC 错误回溯（**当前无报错**） |
| 强制开启 | `_run_keep2.py` 强制 `LPCVM_TAIL=1`、`LPCVM_MEM2=1`（日志会较大） |

**启动模板**
```powershell
cd C:\Users\Administrator\Desktop\lpc
Get-Process lpcvm | Stop-Process -Force -ErrorAction SilentlyContinue; Start-Sleep 3
Remove-Item regressXXX -Recurse -Force -ErrorAction SilentlyContinue
$env:EXTRA_LPCVM='LPCVM_SCHED,LPCVM_SAMPLE'   # 按需追加
python -X utf8 _run_keep2.py regressXXX > _bootXXX.txt 2>&1   # 后台跑，~15 分钟
# 轮询端口：$s=New-Object Net.Sockets.TcpClient; $s.Connect('192.168.200.1',8160)
```

---

## 7. 测试脚本（登录/进游戏/NPC/购买）

| 脚本 | 作用 |
|---|---|
| `_enter_probe.py <ip> <port> <acct> <pwd> <char> <秒>` | 登录→建角/选角→进游戏，统计窗口包（含 0x211B/F0B1 等世界包） |
| `_createdlg.py` | 建角并解码弹窗（判据 `F061 count=1`） |
| `_talk_test.py`（新） | **不走路**，出生点直接对视野内每个 NPC 发 `0x7012`，打印回包并解 0x2037 文本（NPC 菜单验收） |
| `_npc_verbose.py`（新） | 同上但**解码每个回包**（0x3393 对话正文 / 0x2037 菜单正文），并区分"完全没回包"与"回了点别的"。用法 `python -X utf8 _npc_verbose.py 1 1 awesrd` |
| `_npcfix_run.py`（新） | **一条命令跑完整验收**：起服务（探针 `LPCVM_CFV,LPCVM_TALK`）→ 等 8160 → `_login_full2` 建角 `awesrd` → `_npc_verbose` 点 NPC → 抓 gs 日志做取证 → 杀服务。~13 分钟 |
| `_run_probe.py <dir> <秒> [K=V,…]`（新） | 只按给定探针集启动（不像 `_run_keep2.py` 强制 TAIL/MEM2），用于"不开日志"计时与单探针成本测量 |
| `_npc_buy_test.py` | 走到指定坐标 → `0x7012` → 商城购买 `0x1166`（条码参数可换） |
| `_move_test.py` / `_action_test.py` / `_newbie_test.py` | 移动 / 动作 / 新手流程探针 |
| `_dbclear.py` / `_chk45.py` / `_delach.py` / `_insach2.py` | 清库 / 查数据 / 删角色档 / 插角色档 |

**关键协议（实测）**
- 登录：`0x20D2` echo → `0xF9C3`；`0x4124` → `0x211B`(ask_fields) → `0xF061`(角色列表 count+buf)；建角 `0x205C`；选角 `0x1060`（含角色名）
- 进游戏成功判据：选角后收到大量世界包（如 `0x211B` 18190B、`0xF0B1`、`0x39F1` 等）
- NPC：`0x7012`(u32 id) → 期望 `0x2037`(menu_v37 `send(8247, buf)`)
- 商城：`0xFD64`(含内嵌 `0x3E24` 子帧: 账号+32位大写hex) 请求商城；购买 `0x1166` = `s8(barcode) u16(amount) s8(pwd) s8("gold_coin") s8(rebate)`（用户实测条码 `L00000000`）；回执 **`0xFD64`**，body `00 13 00 00`

---

## 8. 地图/坐标备忘

- 揽仙镇：客户端 mapId=1000，服务端 mapIndex=245；出生 (61,175)；NPC 多闻道人 (81,182)、渺渺道人 (55,173)、药店王老板等
- NPC 对象键形如 `clone:gs/clone/npc/lanxianzhen/duowen-daoren.o#<seq>`；容器（env）键为 `pak:gs/zone/lanxianzhen/lanxianzhen.o`
- `is_near()` 要求 `env_object(me) == env_object(ob)` 且 `calc_offset(...) <= talk_distance`（默认 21）；`calc_offset` 返回 -1 表示越界（仍算通过）

---

## 9. 交接时的工作区状态

- exe：`build/Release/lpcvm.exe`，已含：第 4 节全部修复 + 第 6 节探针 + 注册表逆索引 / efun 哈希跳表 / env 开关记忆化（§5.A）
  + **op 23 `baseFunOffset` 修复（§5.A3，NPC 对话）** + **商城 3 个根因的修复（§5.E：express_set 返回值 / set_timer 参数与引用 / 跨对象局部下标表）**
  + **forEach 循环变量相对化修复（§5.F，新角色剧情任务 + 传送揽仙镇外）**。
  **实测：8160 在 533s 打开**（安静模式，跑三遍一致），安静模式 1003–1022MB；带 CFV+MALL 探针时约 700–740s；
  带 `LPCVM_NEWBIE` 探针时约 930s（探针本身的 stderr 开销）。
- **功能回归**：已跑过完整"登录 → 建角 → 选角 → 进游戏 → 点 NPC"链路（`npcfix/`，PASS：`F061 count=1` / 重名 `9113` / 0x211B 18190B 世界包 / NPC 回 0x2037 菜单）
  与"开商城 → 买 4 件商品 → 每次 `0xFD27 result=1` + 弹购买成功"链路（`mall6/`、`mall7/`）；
  **新角色剧情链路已端到端验证**（`nb7/`：227/227 个钩子各执行一次、`try_create_task`→1、`send_task_log` 下发
  `【新手】1-9级新手任务|【新手】新的开始`、`raise_event_ex("初进游戏")`→1、`moved::enter_room(me, lanxianzhenwai.o, 86,158,4)`
  把角色送进揽仙镇外）。**还没验证**：`_move_test.py`（跨房间）、商城退款的 `onlinemall_rebated` 分支。
  风险点：`case 8` 的目标对象解析、`cloneObjectInRegistry` 的模板查找现在走 `GsRegIndex`（`vmsByKey`/`objBySuffix`），
  如果某个对象只在 `reg.objects` 里而没有对应 VM，或模板键的拼写不在归一化规则内，行为会和旧的线性扫描不同。
- **已知同类隐患（未修）**：`runDetached` 挂起合并把 callee 栈整体上移时只修正了 `frames_.bp`，栈上**值**里存的绝对下标
  （`StackItem::slotIndex` 的引用项、`CrossVmRef::slotIndex`）不会跟着平移。本次只修了 forEach 循环变量（§5.F）。
  若以后出现"挂起恢复后某个 &引用/局部引用指向错值"的现象，优先怀疑这里。
- **本轮（C/D 两节）新增探针**：`LPCVM_ROOM`（`[room]` 场景/过图调用链 + `[room-blk]` 只打印 `is_blocked -> -1` 的失败）、
  `LPCVM_SEND`（`[send] t=<ms> cno= cmd= body= by=obj::fn hex=`，出站包全审计，大包打 96 字节）、
  `LPCVM_LIFE`（`[life]` 角色生命周期：disconnected/reconnected/time_expired/enter_world/select_char/destruct_object…）。
  `[send]`/`[recv-accept]` 都带 `t=<ms>`：**可以直接量"是服务端慢还是客户端慢"**。
- **`destruct_object` 现在是真实现**（§5.C）：任何依赖"对象会被销毁"的老代码路径在本轮之后行为都会变
  （启动时就有 757 次临时道具析构）。如果出现"某个克隆对象消失得太早"的问题，先怀疑这里。
- **`build_map` 幂等**（§5.D）：同名地图返回既有 mapIndex。若某个克隆房间需要重建地图，必须先 `destruct_map()`（`clone_room.c` 已经这么做）。
- **`src/original_vm.cpp` 必须 CRLF**（MSVC 否则报 C3254/C2838）；`src/value.h`/`value.cpp` 是 LF，别混着转。
  本轮所有生成/改写用 `_apply_all.py`（从 `_bak_original_vm.cpp` 一次性重放全部补丁），改坏了可以重放。
  **注意**：`_apply_all.py` 只重放 §5.A 那 4 个补丁，会丢掉 npc/mall 等后续修复；当前已验证源码的完整快照在
  `_bak_original_vm_npcfix.cpp`（NPC 修复后）、`_bak_original_vm_mall6.cpp`（商城核对时）、`_bak_builtins_premall.cpp`（商城改动前）。
  恢复方式：`Copy-Item _bak_original_vm_mall6.cpp src\original_vm.cpp -Force` 之后必须重编（只换源码不会重新链接）。
- `src/value.h` 本轮被一次错误的"回退脚本"覆盖过，已从 `git cat-file HEAD:src/value.h` 恢复（LF、366 行、含 `liveHeapSnapshot`），
  当前工作树 `git diff src/value.h` 应只有 `g_mapInsertCount` + `g_liveMapEntries`/`g_liveMapBuckets` 三行新增。
- 本轮新增探针（env 开关，不影响默认行为）：`LPCVM_MEMSTAT`、`LPCVM_LOADPROF`、`LPCVM_CFV`、`LPCVM_MALL`、`LPCVM_MALL2`。`LPCVM_TRIM` 已删除（见 §5.A2 第 4 条）。
- 若编译报 C3254/C2838：把 `src/original_vm.cpp` 转回 CRLF 再编。
- 若 `_run_keep2.py` 报 `_dbclear.py` 失败：先确认 `192.168.200.128:3306` 可达（`charset='utf8'`，这台 MariaDB 没有 `utf8mb4`）。
- 大批临时脚本/日志在工作区根目录（`_*.py`、`regress*/`、`bench*/`），可随时清理，不影响构建。
- 复跑测量命令（本轮用的）：`python -X utf8 _run_bench.py <logdir> <秒数>`，它自己清库、起 4 个服务、每 15s 打一次 `8160=` 和 gs 内存。


> IDA MCP 的用法见 `docs/ida_mcp_使用.md`（连接 127.0.0.1:13337、`_idamcp.py`、efun 地址表 `_efun_table.txt`、反编译流程与示例）。


## ⛔ 禁止写入 / 添加 comm_desc.txt（重要，先读这条）

**不要在 `linux/gs/**`（或任何服务目录）里创建、复制、打包 `comm_desc.txt`，也不要往数据库写它。**

* 原版 gs **没有**这个文件，**也不需要**它；
* 接口/指令表内容的解析在 **gs 内部**（`gs/start_gs.c` 的 `load_comm_desc` 等）自行完成，
  即 gs 侧自己解析，不依赖该 txt 文件；
* 原版 gs 日志里出现 `Etc file comm_desc.txt not found.` 是**正常**输出，
  出现它说明“文件不存在”这一事实被正确处理，**不是**要你去补文件；
* 因此：遇到“接口没注册 / `[pkt] analyse cmd=0x2473 NO INTERFACE`”这类现象，
  只能去修 VM/协议的解析与派发（例如宏展开、命令表按名反查 id、回执派发），
  **绝对不允许**通过新增/复制 `comm_desc.txt` 来绕过（已犯过一次，务必不要再犯）。


## ✅ 每次改动都必须验证「能启动 + 能进游戏」（强制）

* 任何改动（VM / 协议 / LPC / 脚本 / 数据）都必须真机验证到：
  **服务端能启动（8160 监听）+ 客户端能登录进游戏**，才算改完；
* 判定责任归属的规则：**如果上一个版本能启动并进入游戏，而当前版本不能，
  那“起不来”的根源就是当前这次改动** —— 必须定位并修掉它，
  不允许用“环境问题 / 数据问题 / 偶发 / 启动本来就慢”搪塞过去；
* 操作方式（二分）：先 `git stash` 或 checkout 到**上一个能启动进游戏的版本**，
  启动一次确认基线正常（8160 开、客户端能进），
  然后**一次只加一个改动**重启验证；能进游戏才保留，不能就改到能为止；
* 验证记录写清楚：用哪个提交/工作区、启动日志目录、`8160` 是否监听、
  客户端是否真的进了游戏（截图或标题栏 `[一线] [角色名]`）。


## 🖱️ 客户端必经弹窗的正确点法（人机/自动化都照这个来）

1. **选服 / 区组界面** → 点 **取消**（不要点“确定”，否则又回到区组列表）；
2. **「重新登录游戏，需要再次进行安全验证」** → 点 **确定**，即可回到登录界面；
3. 回到登录界面后：账号框输入账号 → `Tab` → 密码框输入密码 → **回车**（回车等同“进入游戏/确定”）；
4. 建角界面：点“创建角色” → 选门派 → 点“角色名字”输入框 → 输入名字（该输入框**只接受 WM_CHAR**，
   SendKeys/SendInput 会被客户端丢弃）→ 点“创建角色” → 点“进入游戏”；
5. 剧情右下角 **跳过**；任务追踪（右上角）点一下会自动寻路到目标 NPC。

对应脚本（`_rdp.py`）：`BTN_SERVER_CANCEL` 用于第 1 条，`BTN_VERIFY_OK` 用于第 2 条，
`fastlogin` 走第 3 条，`makechar` 走第 4 条，`auto` 把这些串成一条命令。


## 🔁 登录验证的顺序：先操作真实客户端，确定无法操作才用模拟脚本

**必须先对客户端操作**（RDP 已连接、客户端窗口存在时，一律先用真实客户端跑登录/游戏内验证）：

1. 判断 RDP 是否可用：`qwinsta` 能看到 `rdp-tcp#` 会话、且 `_rdp.py rect` 能找到客户端窗口；
2. **可用 → 走客户端**：`python -X utf8 _rdp.py auto <账号> <密码> <角色名>`（起客户端→登录→建角→进游戏），
   截图用 `python -X utf8 _shot.py out.jpg 800 60 client`（压缩后再看）；
3. **只有 RDP 没连接、或反复确认客户端操作必定做不到时**（例如客户端窗口不存在、
   反外挂弹窗无法越过、注入输入被拒），才退回**模拟脚本**验证登录：

```
python -X utf8 _login_full.py 192.168.200.1 8001 <账号> <密码>      # 建号/登录/进图
python -X utf8 _autotest.py <账号> <密码> <角色> selfid|dump         # 协议级用例
python -X utf8 _relogin_test.py 192.168.200.1 <账号> <密码> <角色>   # 上下线 + 过图回归
python -X utf8 _fire_special.py <账号> <密码> <角色>                 # 剧情触发点
python -X utf8 _bagtask_probe.py <账号> <密码> <角色> <秒>           # 任务面板 + 背包
```

约定：
* **不允许跳过客户端操作直接上模拟脚本**；模拟脚本是“客户端确实无法操作”时的替代手段；
* 用模拟脚本时选**独立账号**，别和客户端占同一个号（否则客户端会弹
  「重新登录游戏，需要再次进行安全验证」）；
* 判定：客户端能进游戏（标题栏 `[一线] [角色名]`）或模拟脚本走到“进入地图/收到角色数据包”即视为登录通过。
