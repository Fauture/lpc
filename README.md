# LPC VM for Windows (C++17)

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



This project is a from-scratch Windows rewrite of the LPC virtual machine found in
the IDA-loaded `magic_Linux32x` binary. It is designed to be built with Visual
Studio 2022 / CMake.

## Features

- LPC source interpreter for the pseudo-LPC dialect recovered from the original
  game driver (`v2.1` style, plus a useful subset of `v37` style).
- Stack-based LPC virtual machine core (`src/interpreter.cpp`).
- Original compiled `.o` container parser and disassembler (`src/o_loader.cpp`):
  reads the binary format used by the original VM, including the header, string
  pool, identifier table, hidden functions, DLT function table, and code segments.
- Original `.o`/`.pak` execution (`src/original_vm.cpp`): a partial action-bytecode
  VM that can run global initializers and user functions from real `magic_Linux32`
  `.o` files.
- Efun index table extracted from IDA (`src/efun_table.h`): includes both efun
  names and return-type bytes. `call_in_efun` is connected to the implemented
  builtins, so `write/printf/sprintf` etc. can produce real output during `.pak`
  startup.
- `call_other` object loading with a global object registry: loaded `.o` objects are
  cached and reused, so later `call_other` calls can see the same object state.
- `create()` call and a real `while` main-loop/timer stub: if the object defines
  `heart_beat`, the VM runs it repeatedly with a timed interval.
- LZSS decompression (`src/lzss.cpp`) and `.pak` unpacking (`src/pak_loader.cpp`)
  for the `lib_aaa32.pak` / `etc.pak` archives used by the original driver.
- Builtin efuns (stubs plus common helpers) so simple scripts can run.

## Build

Using Visual Studio 2022:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Or use the open folder / CMake integration in VS2022.

## Usage

```bat
lpcvm run script.c
lpcvm run script.o
lpcvm run lib_aaa32.pak
lpcvm server examples\server.c 4000
lpcvm disasm script.o
lpcvm unpack lib_aaa32.pak outdir
```

`run` accepts an LPC `.c` source file, a compiled `.o` file, or a `.pak` archive.
For `.pak`, it extracts the archive, executes `start_aaa.o` through the original
action-bytecode VM, loads the pak's real `.o` objects, prints the startup log, and
then starts a real TCP service on port `8001`.

When running `start_aaa_v37.c`, the LPC interpreter executes the v37 source and
`safe_load_object` actually loads the corresponding `.o` files through the original
bytecode VM (`runOriginalObjectInit`), so the log lines correspond to real object
initialization.
`server` starts a simple telnet-like LPC server (Winsock TCP).
`disasm` prints the parsed original `.o` structure and action nodes.  
`unpack` decompresses a `.pak` archive into a directory.

Try the live server:

```bat
build\Release\lpcvm.exe server examples\server.c 4000
```

Then connect with telnet/PowerShell to `127.0.0.1:4000` and send:
`hello`, `who`, `time`, `dbdemo`, `help`.

## Layout

- `src/value.*` – LPC values (int/float/string/array/mapping/object/closure)
- `src/lexer.*` – tokenizer
- `src/ast.*` – AST nodes
- `src/parser.*` – LPC parser
- `src/interpreter.*` – tree-walking interpreter / VM
- `src/builtins.*` – builtin efuns
- `src/o_loader.*` – original `.o` binary parser

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


## 🖱️ 客户端操作补充（实测）

* **ESC = 关闭当前窗口**（商城、背包、礼包等）。**不要连按 ESC**，连按会弹出「结束游戏」菜单，容易误退；
* 商城（在线商城）：点客户区**左上角**（约窗口相对 (6,32)，即客户区 (6,6)）打开；
  切到 **练功道具** 页 → 点道具格 → 右侧显示名称与说明 → 点「购买」→ 弹确认框点「确定」；
* 背包：**Alt+E** 打开；商城买到的道具会进背包；
* 使用道具：**鼠标右键**点道具格；若弹确认框，点「确定」或按回车；
* **使用提示显示在左下角聊天区**（例如“你使用了XXX”）。


## ⚠️ 「3–5 分钟被踢下线」的真正原因（用户确认）

**不是 ESC/客户端操作造成的**，而是**服务端缺少部分回包**：

* `anticheaterd` 每 `force_challenge_interval = 300` 秒下发一次反外挂挑战，
  客户端执行后回答案，服务端比对失败即 `close_client` 断线；
* 挑战脚本用 `load_object("/challenge.dynamic", 0, <程序内容>)` 装载动态对象 ——
  我们的 `load_object` 早先丢掉了第三个参数，对象建不出来、挑战必然失败；
* 同一族的还有 `read_etc` 回包（`MSG_CFG_READ_ETC = 0x2473`）与接口注册
  （`comm_desc.txt` 由 gs 内部 `start_gs.c` 解析，缺接口会导致回包被当“无接口”丢弃）；
* 因此排查掉线要查**服务端有没有把该回的回包发出去**（filed/aaa/ccs 的应答、
  动态对象装载、接口注册），不要归因到客户端按键操作。
