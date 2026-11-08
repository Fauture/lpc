# IDA MCP 使用手册（原版语义核对）

> 用途：任何 efun / 操作码 / 驱动行为不清楚时，**先在这里查原版 `magic_Linux32x`**，禁止猜测实现。
> 规则本体见各文档开头的「⛔ IDA 语义核对（强制）」。

## 1. 连接

* IDA MCP 服务：`http://127.0.0.1:13337/mcp`（streamable HTTP，MCP 协议 `2025-06-18`，工具 60+ 个）。
* 当前 IDB：`C:\Users\Administrator\Desktop\magic_Linux32x.i64`，`imagebase = 0x8048000`，Hex-Rays 可用（`server_health` 可确认）。
* 命令行客户端：仓库里的 `_idamcp.py`（只用标准库，无需额外依赖）：

```powershell
python -X utf8 _idamcp.py list                       # 列出所有工具
python -X utf8 _idamcp.py schema decompile disasm     # 看某工具的参数结构
python -X utf8 _idamcp.py look vm_f_mapping_array     # 按名字/地址查函数
python -X utf8 _idamcp.py dec 0x80d876c               # 反编译 -> _ida_out/addr_80d876c.c
python -X utf8 _idamcp.py dis 0x80d8600               # 反汇编
python -X utf8 _idamcp.py call find '@_mcp_args.json' # 任意工具，参数放 JSON 文件（PowerShell 引号会吃掉内联 JSON）
```

* PowerShell 注意：内联 JSON 的双引号会被吞掉，**参数一律写进文件**再 `'@文件'` 传（脚本支持 `@file` 前缀）。

## 2. 找原版函数的三步法

1. **efun → 处理函数地址**：仓库里已有现成表，不用猜：
   * `_efun_table.txt`：`名字 地址 处理函数名`（例：`mapping_array 0x80d876c vm_f_mapping_array`）
   * `_efun_registered.txt`：`名字 地址 参数个数 处理函数`
   * `tools/efun_names_full.json` / `tools/efun_table.json`：efun 索引 ↔ 名字
2. **反编译**：`_idamcp.py dec <地址>`，结果落到 `_ida_out/`，重点看参数检查、返回值类型、错误分支。
3. **追调用链/表**：`xrefs_to`（谁引用它）、`find type=data_ref`（名字字符串 → 分发表项）、
   `callees`（它调用了谁，例如 `vm_f_mapping_array` → `vm_f_mapping_array1` → `vm_f_mapping_array2`）。


## 3. 记录要求

* 核对完把地址、关键伪代码结论写进对应文档（如 `_ida_missing/<efun>.txt` 或 HANDOVER），
  并在代码注释里引用地址，方便下一个人复核。
* IDA 原始反编译采集（勿重复采集，直接引用）：
  * `docs/ida_ops/opNN_<名称>_<处理函数>.c` — 全部 85 个操作码处理函数的 Hex-Rays 原始伪代码。
  * `docs/ida_ops/vm_mainLoop.c`、`vm_scheduleProcessInternal.c`、`vm_doActions.c`、
    `vm_regularProcessor.c`、`vm_run.c`、`vm_startProcess/resumeProcess/pendCurrentProcess/
    bringProcessToNext.c`、`vm_callIn*.c`、`vm_returnFromFun.c`、`vm_initForLoop.c`、
    `vm_cmpAndJmpInLoop.c` 等 — 核心处理链原始伪代码。
  * 辅助归纳（可读版，如需速查）：`docs/原版VM核心处理.md`、`docs/操作码语义参考.md`。
* 没查到/查不动时：写清"未核对 + 待核对项"，不要用近似实现顶上去。
