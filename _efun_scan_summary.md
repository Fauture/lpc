# Efun 覆盖扫描报告 (magic_Linux32x 对照)

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



## 1. 真相来源
参考二进制 `C:\Users\Administrator\Desktop\magic_Linux32x` (未 strip, 带完整 .symtab):

| 来源 | 数量 | 说明 |
|---|---|---|
| `vm_f_*` 符号 | 604 | 每个 efun 的 C 实现 |
| efun 注册表 (ELF .data, 28 字节/项: name@+0, fptr@+20, nargs@+24) | 636 条 | 4 张表是 VM 操作码伪 efun (add/if/goto/push_* 等) |
| **真实注册 efun** | **561** | 636 - 75 个操作码条目 |

扫描脚本: `_elf_scan.py` `_elf_efuns.py` `_elf_efuntable.py` `_elf_reg2.py` `_efun_final.py`
产物: `_efun_real.json` (561 个 efun + 地址 + nargs + 符号), `_efun_report.txt`

## 2. 本引擎覆盖情况 (每轮由 tools/gen_stubset.py 重新生成)
```
参考驱动注册 efun : 561
本引擎已实现     : 504
仍缺失           : 57  (全部会在被调用时打印 [efun-stub]，不会静默返回 0)
```
`src/stub_efuns.inc` = 缺失清单，启动时整表打印:
`[efun-stub-list] 57 unimplemented efuns: ...`

## 3. 本轮新实现 (对照 vm_f_* 反编译逐条核对)
* `src/efun_batch1.inc` (118 个): 调试/统计开关、编码与宽度 (get/set_internal_encoding,
  strwidth, to_unsigned_str)、hash/net8/16/32、break_utf8/combine_utf8、csv_parse、
  empty_value、get_first_data、atomac、fxn、is_pt_in_line/rect、is_rect_crossed、
  is_line_crossed、check_event、convert_mapping_to_string、explode_to_mapping、
  mysql_escape_string、uscounter、rpc_*/mysql_*/telnet_config_info 等。
* `src/efun_batch2.inc` (12 个): padd/psub/psum/pcmp (按原始字节序与饱和规则)、sig、
  rmdir、rename_file、set_internal_comm、mysql_get_connection_info/view_info、
  peek_io_buffer、var_info。
* `src/original_vm.cpp` (19 个): get_timer_id、previous_function、object_dir、
  leave_object、neighbour_object、get_obj_string、bind_value、find_object_function、
  get_global_function、get_function_info、size_program、get_function_report、
  orphan_values、seq_no_to_tid、get_thread_name/seq_no/priority/attr。
* `src/map_module.cpp`: get_in_polygens (多边形内对象 id 列表，按原始分块遍历)。

## 4. 仍缺失的 57 个 (全部会打印日志)
| 类别 | efun |
|---|---|
| 线程退出 | clear_env exit exit_thread execute_file |
| 信号量/信箱 | create_semaphore delete_semaphore take_semaphore give_semaphore list_mailbox peek_mailbox post_mailbox delete_mailbox pend_on_mailbox pend_on_event |
| 对象/上下文 | get_uid get_euid set_obj_var save_object restore_object previous_arguments previous_context set_previous_context_var |
| 内存调试 | dump_memory modify_memory |
| 套接字 | get_sock_opt ioctl_socket recv_from send_to |
| 打包/加解密 | pkt_pad_map pkt_pad_part_map pkt_analyse std_des |
| 索引树/表 | itable_* (6) itree_* (11) tixml_load_mapping tixml_save_mapping |
| 字体渲染 | verigen_add_noise_to_pic verigen_new_background verigen_scale_glyph verigen_scale_pic |

## 5. 关键结论
1. 之前 `stub_efuns.inc` 里混入了 29 个参考驱动并不注册的名字 (旧表来源)，并漏掉 7 个真实 efun
   (`enable_apex send_to_apex recv select sscanf find_reachable find_reachable_ex` — 后 5 个其实已实现，
   只是走 `names[efun] ==` 分支)。现在清单与二进制一一对应。
2. 现在**没有任何 efun 会静默返回 0**：缺失的 57 个每次触发都打印对象::函数。
3. `find_reachable/find_reachable_ex/sscanf/recv/select` 已确认实现；`new_view_point`、
   `telnet_new_client` 不再是 stub (后者并非注册 efun)。

> IDA MCP 的用法见 `docs/ida_mcp_使用.md`（连接 127.0.0.1:13337、`_idamcp.py`、efun 地址表 `_efun_table.txt`、反编译流程与示例）。
