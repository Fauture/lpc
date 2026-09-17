# cmdList 补全进度

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



方法见 `analysis/_cmdList_补全规范.md`。所有 LPC 函数名 / nArgs / type 序列都从 IDA
decompile 的 `vm_apply` 调用逐项确认，**非驼峰转下划线猜测**（个别例外如
`vm_cmdCancelAuthVipSms` 的 lpcFun 原样是 `vm_cmdCancelAuthVipSms`、`cmd_view_achieve_overriew`
的拼写错误，均从二进制字符串字面量原样抄录）。

## 一、完成情况

| 批次 | 命令数 | 说明 | 状态 |
|---|---|---|---|
| 首批 filed↔dba（msg_read_content 等） | 6 | 0x4401/2403/3411/3415/4419/4417 | ✅ 已写入 handler |
| 二批 filed↔dba content/gid 协议 | 15 | 0x2405…0x4402 | ✅ |
| 既有 msg_cfg_\* | 6 | 0x3365/3361/3363/435F/40F5/236F | ✅（会话前已存在） |
| 既有 serverCmdList | 3 | 0x5351/0xF9D3/0x20D2 | ✅（0xF9D3 msg_login_done 无 handler，见下） |
| **type=1 游戏命令** | **749** | `_cmdList_parsed.json` 全部 type==1 | ✅ |
| **type=2 剩余消息** | **137** | 含全部剩余 msg_cfg_\* / msgL\* / \*Oper / ccs/gs 族 | ✅ |
| **合计新增（本次会话 fan-out）** | **886** | chunk1-5 | ✅ |

生成结果已并入 `src/game_service.cpp`：
- `kCmdList` 共 **916** 条（`cmdNo` 均无重复，已用 `_cmdList_parsed.json` 权威 cmdNo 复核修正）。
- 新增 handler **886** 个（`handle_<vm函数名>`）+ 首批 6 个手写 handler，共 904 个。
- dispatch switch 共 **915** 个 `case`。
- 已用 MSBuild 编译通过（`build/Release/lpcvm.exe`）。

## 二、各命令确认数据（每批一张表）

批量结果原始 JSON 在 `analysis/gen/`：`chunk1_batch_*.json` … `chunk5_batch_*.json`，
每项含 `{cmdNo, fun, addr, lpcFun, nArgs, typeSeq, bodyFields, handlerName, handlerCode, kcmdEntry, status, note}`。
- chunk1-4 = type=1（749 项），chunk5 = type=2（137 项）。
- 全部 886 项 status=ok，**无 skip**（无 vm_apply 前 5 实参非 0、type 码均在
  {88,357,453,936,228,394} 内）。

## 三、跳过 / 异常 / 遗留

1. **`0xF9D3 msg_login_done`**：在 `kCmdList` 但无 handler / dispatch（会话前就缺）。未补。
2. **type=3 / type=7（echo 族）**：`0x20D2 cmd_echo` 已有 handler（回复 `0xF9C3 msgReplyEcho`）。
   收发关系按原样保留，无需额外 handler。
3. **body 读取顺序 ≠ vm_apply 参数顺序** 的典型项（agent 已按 decompile 逐项确认，见各 note）：
   - `cmd_got_gid_info` / `msg_got_gid_info`：body 为 cookie/result/type/name，apply 同序。
   - `cmd_c_do_action`（0/1/2）、`cmd_open_menu`（0/1/2）、`cmd_select_menu_item`、EnterRoom0/1/2：
     首参是硬编码的 cmdGroup/菜单索引，**非 clientNo**，handler 里已处理。
   - `cmd_get_image`：body 读 (md5, gid)，apply 传 (gid, md5)。
   - `msg_cfg_csa_info` 等：跳过未用的头部字节，apply 顺序与 body 不同。
4. **编译期修复的生成问题**：`chunk3_batch_6` 的 10 个 `kCmdEntry` 字符串 cmdNo 写错
   （已按权威 cmdNo 修正，dispatch 用的是正确的 cmdNo 字段，故不冲突）；`ATTRIB_*` 被
   个别 agent 写成字符串（已去引号）；补了 `rd8`、`rd16be(ptr,off)`、`rd32be(ptr,off)`、
   `make_string(ptr,len)`、`make_buffer(ptr,len)` 重载。

## 四、运行时注意事项

- handler 用 `applyLpc(lpcFun, {make_int(cl.clientNo), ...})`，第一参 clientNo；少数组
  （cmdGroup 类）例外已注明。
- 建议用 `LPCVM_TRACE=1` 起服，抓 `[game] ...` 日志核对各命令实际触发时 LPC 侧参数是否对齐。
- `game_service.cpp` 现约 1.3 万行，全部在 `lpc::<anonymous>` 命名空间内。

## 五、生成产物（analysis/gen/）

- `chunk{1,2,3,4,5}_batch_*.json`：886 条分析结果。
- `_todo_cmds.json` / `_todo_cmds.tsv`：本次要处理的 886 条清单（已排除会话前已补的）。
- `_chunk1_handlers.txt` / `_batch1_handlers.txt` 等：整合用的中间文本。

> IDA MCP 的用法见 `docs/ida_mcp_使用.md`（连接 127.0.0.1:13337、`_idamcp.py`、efun 地址表 `_efun_table.txt`、反编译流程与示例）。
