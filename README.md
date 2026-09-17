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
