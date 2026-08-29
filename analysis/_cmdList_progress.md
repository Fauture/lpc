# cmdList 补全进度

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
