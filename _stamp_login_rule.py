# -*- coding: utf-8 -*-
"""Stamp the "test the login flow after every start" rule into every document,
and create docs/ida_mcp_使用.md (IDA MCP workflow) + reference it everywhere.
Idempotent."""
import io
import os

LOGIN_MARK = '启动后必须测试登录（强制'
LOGIN_BLOCK = """## ⛔ 启动后必须测试登录（强制，报告"已启动"之前必做）

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
"""

DOCS = [
    'README.md', 'run.md', 'HANDOVER.md', '_自动化测试方法.md', '_efun_scan_summary.md',
    '登录流程包.txt',
    os.path.join('docs', 'refactor_cooperative_scheduler.md'),
    os.path.join('analysis', '_cmdList_补全规范.md'),
    os.path.join('analysis', '_cmdList_progress.md'),
    os.path.join('analysis', '_cmdList_filed_dba_首批样例.md'),
    os.path.join('analysis', 'create_gid_dba_dispatch_bug.md'),
    os.path.join('analysis', 'session_progress.md'),
]

IDA_DOC_REF = ('> IDA MCP 的用法见 `docs/ida_mcp_使用.md`（连接 127.0.0.1:13337、`_idamcp.py`、'
               'efun 地址表 `_efun_table.txt`、反编译流程与示例）。\n')

for path in DOCS:
    if not os.path.exists(path):
        print('SKIP  %s (missing)' % path)
        continue
    lines = io.open(path, encoding='utf-8', errors='ignore').read().splitlines()
    changed = False
    if not any(LOGIN_MARK in l for l in lines):
        # insert after the IDA rule block (which ends with the VM-inline lesson)
        anchor = None
        for i, l in enumerate(lines):
            if 'VM 内的实现必须在**当前 VM 上内联执行**' in l:
                anchor = i
        if anchor is None:
            for i, l in enumerate(lines):
                if l.strip() == '**宁可不测，也不许写库。**':
                    anchor = i
        if anchor is None:
            lines = lines[:1] + [''] + LOGIN_BLOCK.splitlines() + lines[1:]
        else:
            lines = lines[:anchor + 1] + [''] + LOGIN_BLOCK.splitlines() + lines[anchor + 1:]
        changed = True
    if not any('docs/ida_mcp_使用.md' in l for l in lines):
        lines = lines + ['', IDA_DOC_REF.rstrip()]
        changed = True
    if changed:
        io.open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
        print('STAMP %s' % path)
    else:
        print('OK    %s' % path)

# ---------------------------------------------------------------- IDA MCP doc
os.makedirs('docs', exist_ok=True)
p = os.path.join('docs', 'ida_mcp_使用.md')
io.open(p, 'w', encoding='utf-8', newline='\n').write("""# IDA MCP 使用手册（原版语义核对）

> 用途：任何 efun / 操作码 / 驱动行为不清楚时，**先在这里查原版 `magic_Linux32x`**，禁止猜测实现。
> 规则本体见各文档开头的「⛔ IDA 语义核对（强制）」。

## 1. 连接

* IDA MCP 服务：`http://127.0.0.1:13337/mcp`（streamable HTTP，MCP 协议 `2025-06-18`，工具 60+ 个）。
* 当前 IDB：`C:\\Users\\Administrator\\Desktop\\magic_Linux32x.i64`，`imagebase = 0x8048000`，Hex-Rays 可用（`server_health` 可确认）。
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

## 3. 已核对案例（照着做）

**`mapping_array`（新手上任务 S11 张老板发武器失败）**

* `mapping_array` = `vm_f_mapping_array` @ `0x80d876c`：参数 array + func，非空则压入"与源同长度"的结果数组 + index=0，
  调 `vm_f_mapping_array1`；空数组直接返回空数组。
* `vm_f_mapping_array1` @ `0x80d8600`：`index++`，校验 ret/src 长度相等，`index >= size` 收尾；
  否则压 `src[index]` + 函数并调用，再用 `vm_hookCallbackWhenNextAction` / `vm_hookCallbackWhenReturnAt`
  挂 `vm_f_mapping_array2`。
* `vm_f_mapping_array2` @ `0x80d8870`：把返回值写进 `ret[index]`，再回 `vm_f_mapping_array1`。
  → 结论：**原版是"返回钩子驱动的蹦床"，每个元素只占一次 VM 调用，不占 C++ 递归栈**；
    结果数组与源**同长同序**，不过滤。（我们的实现必须照此，否则大数组映射会撑爆 C 栈。）
* 相关：`vm_f_load_object` @ `0x80f8cbc` —— 已加载对象直接返回缓存（`flags != 2048`）；
  构造中的对象用 `flags(0x800/0x1000/6144)` + `initStep` 标记并继续推进，**不会重新跑 create**。
  这是"装载链重入"的判据。

## 4. 记录要求

* 核对完把地址、关键伪代码结论写进对应文档（如 `_ida_missing/<efun>.txt` 或 HANDOVER），
  并在代码注释里引用地址，方便下一个人复核。
* 没查到/查不动时：写清"未核对 + 待核对项"，不要用近似实现顶上去。
""")
print('WROTE %s' % p)
