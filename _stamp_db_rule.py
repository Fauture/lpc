# -*- coding: utf-8 -*-
"""Stamp the "never write to the database" standard into every project document.

Idempotent: a file that already contains the marker line is left alone.
"""
import io
import os

MARK = '数据库红线（最高优先级'
BLOCK = """## ⛔ 数据库红线（最高优先级，任何对话 / 任何工具 / 任何脚本都必须遵守）

**禁止向数据库写入任何数据。默认只读：只允许 `SELECT`。**

* 严禁 `INSERT` / `UPDATE` / `DELETE` / `REPLACE` / `CREATE` / `DROP` / `TRUNCATE` / `ALTER`。
* 严禁跨库复制数据 —— **尤其禁止把参考库（`dl_ddb_1`、`dl_adb_all`、`dl_mdb_1` 等）的任何行复制进测试库**，
  也禁止把测试库的数据写回参考库。参考库是比对基准，只能读。
* 严禁手工造 / 改账号、角色、存档、坐标、任务、物品、商店、经验等数据来"绕过"问题；
  **测试数据必须由服务端自己产生**（真实客户端，或 `_login_lib.py` / `_enter_probe.py` 走正常登录、建号、进游戏流程）。
* 唯一例外：**人工明确要求"清除数据"** 时，只允许清空测试库 `dl_ddb_1_test`（`_dbclear.py` / `_run_probe.py`），
  绝不允许触碰其它任何库；执行前必须先告知用户"这会删掉测试库里的角色存档、且无法恢复"。
* 仓库里的 `_db*.py`、`_fromorig.py`、`_dbrestore*.py` 之类脚本中的写库语句一律按 **禁用代码** 对待，
  只有在用户明确授权时才可执行；新增此类脚本必须先把用途和影响写清楚。
* 违反后果（真实事故，2026-09-17）：清库后为了"造出一个能登录的角色"，把参考库的角色数据复制进测试库，
  并往 `dl_mdb_1.acc_info` 写入账号行 —— 环境被污染、原测试角色存档无法恢复，且参考数据的副本混进测试库。
  **宁可不测，也不许写库。**

"""

DOCS = [
    'README.md',
    'run.md',
    'HANDOVER.md',
    '_自动化测试方法.md',
    '_efun_scan_summary.md',
    '登录流程包.txt',
    os.path.join('docs', 'refactor_cooperative_scheduler.md'),
    os.path.join('analysis', '_cmdList_补全规范.md'),
    os.path.join('analysis', '_cmdList_progress.md'),
    os.path.join('analysis', '_cmdList_filed_dba_首批样例.md'),
    os.path.join('analysis', 'create_gid_dba_dispatch_bug.md'),
    os.path.join('analysis', 'session_progress.md'),
]

for path in DOCS:
    if not os.path.exists(path):
        print('SKIP  %s (missing)' % path)
        continue
    raw = io.open(path, encoding='utf-8', errors='ignore').read()
    if MARK in raw:
        print('OK    %s (already stamped)' % path)
        continue
    lines = raw.splitlines()
    out = []
    if lines and lines[0].startswith('#'):
        out.append(lines[0])
        out.append('')
        out.append(BLOCK)
        out.extend(lines[1:])
    else:
        out.append(BLOCK)
        out.extend(lines)
    io.open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    print('STAMP %s' % path)
