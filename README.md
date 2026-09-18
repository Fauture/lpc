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
