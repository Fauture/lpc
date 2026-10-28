# 工作区规则（agent 必读）

仓库：`C:\Users\Administrator\Desktop\lpc`（LPC VM for Windows，C++17；服务端 `lpcvm` 四服务 aaa/dba/ccs/gs）。
面向人的说明见 [README.md](README.md)，编译与启动见 [docs/BUILD_AND_RUN.md](docs/BUILD_AND_RUN.md)。

## 1. 禁止往仓库根目录写任何临时文件（硬性）

- **根目录只放仓库自身的东西**：`src/`、`build/`、`tools/`、`docs/`、`linux/`、`third_party/`、`analysis/`、`out/`、`README.md`、`CMakeLists.txt`、`AGENTS.md`、`.gitignore`、`.gitattributes`。
- **禁止在根目录新建或遗留**：`.py` 脚本、图片/截图（`.jpg/.png`）、日志（`.txt/.log`）、归档（`.zip/.tar`）、dump、消息草稿（`*.msg.txt`）等一次性文件。
- **临时文件一律写 `tmp\`**（已被 `.gitignore` 忽略）：临时脚本 `tmp\`、临时数据 `tmp\`、截图/导出 `tmp\`、服务日志 `tmp\<名字>\`（例如 `tmp\gs_run`）。
- **长期工具放 `tools\`**：编译 `tools\_build.py`、启动 `tools\_run_keep2.py`、清库 `tools\_dbclear.py`、存档验证 `tools\_verify_save.py`、登录/探针/反编译工具等。运行方式（当前目录仍是仓库根）：

  ```powershell
  python -X utf8 tools\_build.py
  $env:EXTRA_LPCVM='LPCVM_QUIET=1'
  python -X utf8 tools\_run_keep2.py tmp\gs_run
  ```

- 需要一次性产物时直接放 `tmp\`，不要放根目录；任务结束顺手清掉。

## 2. 数据库红线（摘自 README，必须遵守）

- 启动服务端**之前先清空测试库 `dl_ddb_1_test`**；除此之外**只读**，只允许 `SELECT`。
- `dl_adb_all_test` 禁止清；参考库（`dl_ddb_1`、`dl_adb_all`、`dl_mdb_1`、`dl_mdb_all`、`dl_ldb_*`、`dl_dmdb_1`、`dl_tdb_1`）**永不写入、永不清理**，只用于比对。
- 禁止手工造/改账号、角色、存档、坐标、任务、物品、商店、经验等数据来绕过问题；测试数据必须由服务端自己产生。
- 详见 [README.md](README.md) 的「数据库红线」。

## 3. 其它强制项

- efun / 操作码 / 字节码语义不清时**必须查 IDA**（见 `docs/ida_mcp_使用.md`），禁止猜测、禁止「能用就行」的近似实现。
- **禁止创建/复制 `comm_desc.txt`**：原版 gs 没有也不需要，接口解析在 gs 内部完成（见 README）。
- 运行中的服务会占用 `build\Release\lpcvm.exe`，编译前必须先停（`tools\_build.py` 已内置杀进程）。
- 服务启动耗时按版本线差异很大，以 `8160` 端口是否打开为准，不要中途杀掉（见 `docs/BUILD_AND_RUN.md` §3）。
