# -*- coding: utf-8 -*-
"""Rewrite the stamped DB rule (v2) in every document.

Corrected per the user:
  * dl_ddb_1_test : delete ALL rows before every server start (_dbclear.py /
                    _run_probe.py does it automatically) - 必做步骤
  * dl_adb_all_test : 禁止删除 (never delete anything there)
  * reference DBs : never cleared, never written
  * everything else: read-only (SELECT only)
"""
import io
import os

OLD_START = '## ⛔ 数据库红线（最高优先级'
OLD_END = '**宁可不测，也不许写库。**'

NEW = """## ⛔ 数据库红线（最高优先级，任何对话 / 任何工具 / 任何脚本都必须遵守）

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

for path in DOCS:
    if not os.path.exists(path):
        print('SKIP  %s (missing)' % path)
        continue
    lines = io.open(path, encoding='utf-8', errors='ignore').read().splitlines()
    try:
        i0 = next(i for i, l in enumerate(lines) if l.startswith(OLD_START))
        i1 = next(i for i, l in enumerate(lines) if l.strip() == OLD_END)
    except StopIteration:
        print('WARN  %s (no stamped block - stamping fresh)' % path)
        if lines and lines[0].startswith('#'):
            lines = [lines[0], ''] + NEW.splitlines() + lines[1:]
        else:
            lines = NEW.splitlines() + lines
    else:
        lines = lines[:i0] + NEW.splitlines() + lines[i1 + 1:]
    io.open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print('UPDATED %s' % path)

# the old instruction inside run.md's own body must not contradict the rule
p = 'run.md'
t = io.open(p, encoding='utf-8', errors='ignore').read()
old = '每次启动都要删除dl_adb_all_test库中所有表的数据或只清除data数据,注意不要删除表,以保证每次改动后启动都是正常的'
new = ('每次启动都要清空 dl_ddb_1_test（删除所有行，不要删表）；dl_adb_all_test 禁止删除任何数据 '
       '（旧文本"删除dl_adb_all_test数据"已作废），以保证每次改动后启动都是正常的')
if old in t:
    io.open(p, 'w', encoding='utf-8', newline='\n').write(t.replace(old, new))
    print('FIXED  run.md line about dl_adb_all_test')
else:
    print('NOTE   run.md old line not found (already changed?)')
