# dba cmd_create_gid 卡死诊断（gs 建号 9106）

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



## 症状
原版 gs(.128:8160) → 重写 aaa/dba/ccs(.200.1)。客户端建号约 30s 后报
"无法使用这个名字创建角色，请重新尝试。（原因：9106）"。
gs 侧 = login_v37.c：`char_gid = filed->create_gid("user", name)` 返回非字符串。

## 已排除
- 包确实到达重写 dba(8120)（dba 日志含完整帧，client4/5 peer=192.168.200.128）。
- 前置检查均正常回包：
  - 0x440C cmd_get_gid：mysql `select gid from gid_info ...` → 0 行 → 回 0x3415 FFFFFFFF00(-1/不存在)。
  - 0x4408 读 change_name → 0 行 → 回 0x2409 00000000。
- gid_info 从未被 INSERT（表始终 0 行）。

## 卡点（dba 日志铁证，LPCVM_TRACE 全程）
0x440A = cmd_create_gid：
```
[pkt] analyse cmd=0x440a iface=cmd_create_gid
[pkt] calling closure process_command
[call_other] /daemons/threadd.c -> dispatch_ddb_service args=2
[q-send] qid=5 ok=1 ... fn=dispatch_ddb_service ... qv=5   // 提交给 ddb 服务队列
[wake] daemon cond.kind=2 key=q:5
[cfv] calling process_command on /dba/daemons/dbaserverd.o args=3
[cfu] fwd query_entry idx=11 -> impl idx=2
[vm] efun call_other_arg ... in=process_command obj=/dba/daemons/dbaserverd.o
[coa-inline] /dba/daemons/dbaserverd.o -> /dba/daemons/dbaserver/cmd_create_gid.o::main argc=4
-- 之后该执行流再无任何输出：没有 assure_connection/mysql、没有 [fn] create_gid、没有回包 --
```
gs 重试同 tid 多次，均无应答 → 30s 超时。

对照：0x440C cmd_get_gid 不经过 threadd qid5，直接在 dbaserverd 执行流里完成 mysql 并回包 —— 正常。
即 **cmd_create_gid 走 threadd DDB 服务派发（dispatch_ddb_service → invoke_before_timeout → dbaserverd process_command → inline main）后，main 内部执行状态丢失**：私有 create_gid 从未被进入。

## 待查（引擎层）
1. main 在 startInlineFunction 之后停在哪个指令 —— 下一轮用 LPCVM_TRACE=1 + LPCVM_TRACE_FN=main 复现一次，抓 main 的逐指令流末端。
2. 关注点：
   - startInlineFunction 对跨对象(threadd 服务 VM→dbaserverd VM→cmd_create_gid.o)的 funcLookup/objVars 切换；
   - main 内 try{create_gid}catch{} 的 op44/26/14（new_thread_catch 等，引擎 catch 未实现）与 call_user_fn create_gid 的执行；
   - create_gid 内第一个跨对象调用 assure_connection("/daemons/dbd.o") 若发生 SuspendSignal park，resume 路径是否存在于该 runDetached/inline 上下文。

## 复现环境
aaa/ccs 普通启动；dba 带 LPCVM_SQL_TRACE=1 LPCVM_ERR_TRACE=1（另按需 LPCVM_TRACE=1 LPCVM_TRACE_FN=main）。
gs 建号动作在 .128 原版 gs 上执行。数据库：先 _dbclear.py（仅 dl_ddb_1_test，脚本已改）。
