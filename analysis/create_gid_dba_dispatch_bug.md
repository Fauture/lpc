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

> IDA MCP 的用法见 `docs/ida_mcp_使用.md`（连接 127.0.0.1:13337、`_idamcp.py`、efun 地址表 `_efun_table.txt`、反编译流程与示例）。
