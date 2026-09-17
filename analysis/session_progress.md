# 会话进展存档（原版 gs + 重写 aaa/dba/ccs 联调）

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



## 已完成并验证的引擎修复（src/original_vm.cpp / src/builtins.cpp / src/value.h）
1. exquisited 建号(构造)类问题 → 跨对象 case-8 引用参数写回/函数表：实际直接修复点是
   **call_other / call_other_arg 跨对象 inline 缺 callee 函数表切换**（只切 objVars/strings）。
   修复：startInlineFunction 增加 `&targetVm->functionTable()`，inline 失败回退 tryCallOtherObject。
   验证：cmd_create_gid.o::main 内部私有 create_gid 从此能执行（此前被静默跳过，gs 30s 超时 9106）。
2. create_gid 回包 gid 全 0 → atoi64/itoa64 语义错。
   原版(vm_f_atoi64 0x80e0854 / vm_f_itoa64 0x80e09ac, IDA)：atoi64 返回 ({高32,低32})；
   itoa64 接收同型数组输出十进制串。已按此重写。验证：gid_info 正常落库、gid 串正确下发。
3. **商城"元宝不足"（已完成 2026-09-05）→ call_fun_ptr(case 5) 闭包引用参数扁平化丢失。**
   - 现象：账号 a 余额 1987万仍提示"元宝不足"，不扣款不发东西。
   - 根因：aaa `cmd_l_precost_coin._main` 构造闭包
     `(: call_other, ".../cmd_l_precost_coin.c", "precost_coin", <11 args> :)` 传给
     `account_transactiond.do_account_transaction`，后者执行 `(*proc_func)(&m, info, fields)`。
     case 5 收集 apply 参数用 `pop().value`，把 `&m`（slot 引用）**扁平化成 undefined**；
     precost_coin 内 `result = (["result": ret])`（disasm `[220] push_local_ptr(result);
     [221] assign`）写不回调用方局部 m → do_account_transaction 返回非 mapping → _main 发 result=0。
   - gs 侧判定：coind.do_after_precost_coin **只认 result==1 成功**；result==0/2/4/5 → 弹"元宝不足/繁忙/金元宝不足/银元宝不足"。
   - 修复：case 5 收集参数时，若被调闭包是 efun-closure 且函数名是
     call_other/call_other_arg，遇到 isRef 的 slot/obj 参数则包成 `CrossVmRef`
     （复用 writeThrough/set_local_var 的 index-11 跨 VM 写回路径），而非扁平化。
   - 验证：重启 aaa/dba/ccs（清 dl_ddb_1_test，账号 a 建新角色 gid ...06），
     连续 3 次商城购买循环全部成功：0x6377 回包 result=1 → gs 发 0x6374 cost_coin →
     aaa 扣款 affected=1（gold 19870465→19870357 等逐次递减，DB 确认）→ 回包 0x6375。
     用户游戏内确认：扣款成功并拿到物品。
4. 环境/脚本：
   - `_dbclear.py` 已改为**只清 dl_ddb_1_test**（用户要求，勿恢复旧版全清）；
   - `_restore_adb.py` 已按用户要求删除；
   - 验证流程：每次验证前清 dl_ddb_1_test，aaa/ccs 普通或按需带 LPCVM_SQL_TRACE/ERR_TRACE/TRACE。

## 遗留观察（非阻塞）
- start_account_transaction 的锁行 update 拼出 `update (undefined) set account = account where account = 'a'`
  → SQL 1146 报错但被忽略（事务未真正锁行；单测无碍，并发扣款有理论竞态）。
  来源：do_account_transaction 调 start_account_transaction(cno, account) 用默认参数 table="account"，
  默认参数未生效（可能同 case 5 / 默认值填充问题）。暂未修，记录待查。

## 复现口令（供下一轮）
- 服务：aaa 8001 / dba 8120 / ccs 8110（build\Debug\lpcvm.exe run linux/<svc>/pack_data/lib_<svc>32.pak）
- dba 追踪：LPCVM_SQL_TRACE/LPCVM_ERR_TRACE（create_gid/流程）；aaa 追踪同理；
  需要看全局混合 mysql 时加 LPCVM_TRACE=1。
- 每次验证先清 dl_ddb_1_test（python _dbclear.py）。
- 商城购买：账号 a 建角色，在线商城金元宝支付；成功链路 = 0x6377 result=1 → 0x6374 → 0x6375。
