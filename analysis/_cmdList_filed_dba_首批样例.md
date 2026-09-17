# filed↔dba 数据协议：首批 6 个命令（已逐个 decompile 确认）

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



这是给 flash 的第一批样例：**已从 IDA 逐个 decompile 出 vm_apply 函数名与参数类型
序列**，直接照抄即可。方法见 `analysis/_cmdList_补全规范.md`。

## 0. 本批命令的统一特征（重要）

- 这批命令的 body 是 **raw big-endian u32 + 长度前缀字符串**，**不是** value-type
  编码（u8len+str / u16）。不要用 `VtReader`，用现成的 `rd32be`（`game_service.cpp:71`）。
- 长度前缀有两种，**每个命令不同，必须按 decompile 区分**：
  - `u32 BE` 长度前缀：msg_read_content / msg_got_gid_arr
  - `u8` 长度前缀：msg_created_gid / msg_got_gid / msg_got_gid_info（type、name）
- LPC 函数第一个参数都是 `clientNo`（int，type=88）。
- handler 统一签名：`void handleXxx(GameClient& cl, const uint8_t* body, size_t len)`。
- 全部 type=2 → kCmdList 用 `ATTRIB_INTERNAL_COMM`。

---

## 1. msg_read_content  (cmdNo 0x4401)

- vm 函数：`vm_msgReadContent` @ `0x80674ec`（size 0xa1）
- body（大端）：`[u32 a][u32 b][u32 strLen][str 字节]`，共 12 + strLen
- 校验：`len >= 12` 且 `strLen <= len - 12`
- LPC：`msg_read_content(clientNo, a, b, str)` —— **4 参数，全 int/int/int/string**
  - decompile：`vm_apply(0,0,0,0,0, "msg_read_content", 4, 88, clientNo, 88, v6, 88, v10, 357, v11)`
  - 字段语义推断：a=gid，b=结果码，str=内容（待 LPC 源码确认，实现只按布局）

```c
// vm_msgReadContent (0x80674ec): body = [u32 BE a][u32 BE b][u32 BE strLen][str]
// -> msg_read_content(clientNo, a, b, str)
void handleMsgReadContent(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 12) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    uint32_t slen = rd32be(body + 8);
    if (slen > len - 12) return;
    std::string s(reinterpret_cast<const char*>(body + 12), slen);
    applyLpc("msg_read_content",
             {make_int(cl.clientNo), make_int(a), make_int(b), make_string(s)});
}
```

---

## 2. msg_written_content  (cmdNo 0x2403)

- vm 函数：`vm_msgWrittenContent` @ `0x8067634`（size 0x68）
- body（大端）：`[u32 a][u32 b]`，共 8 字节
- 校验：`len >= 8`
- LPC：`msg_written_content(clientNo, a, b)` —— **3 参数，全 int**
  - decompile：`vm_apply(a1,0,0,0,0, "msg_written_content", 3, 88, clientNo, 88, v7, 88, v9)`

```c
// vm_msgWrittenContent (0x8067634): body = [u32 BE a][u32 BE b]
// -> msg_written_content(clientNo, a, b)
void handleMsgWrittenContent(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 8) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    applyLpc("msg_written_content",
             {make_int(cl.clientNo), make_int(a), make_int(b)});
}
```

---

## 3. msg_created_gid  (cmdNo 0x3411)

- vm 函数：`vm_msgCreatedGid` @ `0x8067b80`（size 0xe8）
- body（大端）：`[u32 cookie][u32 result][u8 gidLen][gid 字节]`
- 校验：`len >= 9` 且 `gidLen <= len - 9`
- LPC：`msg_created_gid(clientNo, cookie, result, gid)` —— **4 参数 int/int/int/string**
  - decompile：`vm_apply(..., "msg_created_gid", 4, 88, clientNo, 88, cookie, 88, result, 357, gid)`

```c
// vm_msgCreatedGid (0x8067b80): body = [u32 BE cookie][u32 BE result][u8 gidLen][gid]
// -> msg_created_gid(clientNo, cookie, result, gid)
void handleMsgCreatedGid(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    uint32_t result = rd32be(body + 4);
    uint32_t glen = body[8];
    if (glen > len - 9) return;
    std::string gid(reinterpret_cast<const char*>(body + 9), glen);
    applyLpc("msg_created_gid",
             {make_int(cl.clientNo), make_int(cookie), make_int(result), make_string(gid)});
}
```

---

## 4. msg_got_gid  (cmdNo 0x3415)

- vm 函数：`vm_msgGotGid` @ `0x8067d38`（size 0xe8）
- body / 校验 / LPC 签名与 msg_created_gid **完全相同**：
  `[u32 cookie][u32 result][u8 gidLen][gid]` → `msg_got_gid(clientNo, cookie, result, gid)`
  - decompile：`vm_apply(..., "msg_got_gid", 4, 88, clientNo, 88, cookie, 88, result, 357, gid)`

```c
// vm_msgGotGid (0x8067d38): body = [u32 BE cookie][u32 BE result][u8 gidLen][gid]
// -> msg_got_gid(clientNo, cookie, result, gid)
void handleMsgGotGid(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    uint32_t result = rd32be(body + 4);
    uint32_t glen = body[8];
    if (glen > len - 9) return;
    std::string gid(reinterpret_cast<const char*>(body + 9), glen);
    applyLpc("msg_got_gid",
             {make_int(cl.clientNo), make_int(cookie), make_int(result), make_string(gid)});
}
```

---

## 5. msg_got_gid_arr  (cmdNo 0x4419)

- vm 函数：`vm_msgGotGidArr` @ `0x8067e20`（size 0xa1）
- body：`[u32 a][u32 b][u32 strLen][str]`，与 msg_read_content **完全相同**
- LPC：`msg_got_gid_arr(clientNo, a, b, str)` —— **4 参数 int/int/int/string**
  - decompile：`vm_apply(0,0,0,0,0, "msg_got_gid_arr", 4, 88, clientNo, 88, v6, 88, v10, 357, v11)`

```c
// vm_msgGotGidArr (0x8067e20): body = [u32 BE a][u32 BE b][u32 BE strLen][str]
// -> msg_got_gid_arr(clientNo, a, b, str)
void handleMsgGotGidArr(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 12) return;
    uint32_t a = rd32be(body);
    uint32_t b = rd32be(body + 4);
    uint32_t slen = rd32be(body + 8);
    if (slen > len - 12) return;
    std::string s(reinterpret_cast<const char*>(body + 12), slen);
    applyLpc("msg_got_gid_arr",
             {make_int(cl.clientNo), make_int(a), make_int(b), make_string(s)});
}
```

---

## 6. msg_got_gid_info  (cmdNo 0x4417)

- vm 函数：`vm_msgGotGidInfo` @ `0x8067ec4`（size 0x13c）
- body（大端）：`[u32 cookie][u32 result][u8 typeLen][type 字节][u8 nameLen][name 字节]`
- 校验：`len >= 9`，`typeLen <= len - 9`，随后 `nameLen <= len - 9 - typeLen - 1`
- LPC：`msg_got_gid_info(clientNo, cookie, result, type, name)` —— **5 参数 int/int/int/string/string**
  - decompile：`vm_apply(..., "msg_got_gid_info", 5, 88, clientNo, 88, cookie, 88, result, 357, type, 357, name)`

```c
// vm_msgGotGidInfo (0x8067ec4): body = [u32 BE cookie][u32 BE result]
//   [u8 typeLen][type][u8 nameLen][name]
// -> msg_got_gid_info(clientNo, cookie, result, type, name)
void handleMsgGotGidInfo(GameClient& cl, const uint8_t* body, size_t len) {
    if (len < 9) return;
    uint32_t cookie = rd32be(body);
    uint32_t result = rd32be(body + 4);
    size_t off = 8;
    uint32_t tlen = body[off++];
    if (tlen > len - off) return;
    std::string type(reinterpret_cast<const char*>(body + off), tlen);
    off += tlen;
    if (off >= len) return;
    uint32_t nlen = body[off++];
    if (nlen > len - off) return;
    std::string name(reinterpret_cast<const char*>(body + off), nlen);
    applyLpc("msg_got_gid_info",
             {make_int(cl.clientNo), make_int(cookie), make_int(result),
              make_string(type), make_string(name)});
}
```

---

## 7. 本批 kCmdList 追加条目（`game_service.cpp:98`）

```c
{ 0x4401, ATTRIB_INTERNAL_COMM, "msg_read_content", 0 },    // vm_msgReadContent
{ 0x2403, ATTRIB_INTERNAL_COMM, "msg_written_content", 0 }, // vm_msgWrittenContent
{ 0x3411, ATTRIB_INTERNAL_COMM, "msg_created_gid", 0 },     // vm_msgCreatedGid
{ 0x3415, ATTRIB_INTERNAL_COMM, "msg_got_gid", 0 },         // vm_msgGotGid
{ 0x4419, ATTRIB_INTERNAL_COMM, "msg_got_gid_arr", 0 },     // vm_msgGotGidArr
{ 0x4417, ATTRIB_INTERNAL_COMM, "msg_got_gid_info", 0 },    // vm_msgGotGidInfo
```

dispatch switch（`game_service.cpp:340`）追加：

```c
case 0x4401: handleMsgReadContent(cl, body, len); break;
case 0x2403: handleMsgWrittenContent(cl, body, len); break;
case 0x3411: handleMsgCreatedGid(cl, body, len); break;
case 0x3415: handleMsgGotGid(cl, body, len); break;
case 0x4419: handleMsgGotGidArr(cl, body, len); break;
case 0x4417: handleMsgGotGidInfo(cl, body, len); break;
```

---

## 8. 第二批：filed↔dba content/gid 协议（15 项，已逐个 decompile 确认 ✅）

这批 15 项已逐个 decompile 出 `vm_apply` 函数名与参数序列，handler 已写入
`src/game_service.cpp`。全部 type=2 → `ATTRIB_INTERNAL_COMM`；body 是 raw 大端
u32 + 长度前缀字符串（**不用 `VtReader`**，用 `rd32be`）。

### 8.1 确认结果总表（decompile 逐一核实，非推测）

| cmdNo | vm 函数 (地址) | LPC 函数 | nArgs | type 序列 | body 布局 |
|---|---|---|---|---|---|
| 0x2405 | vm_msgDeletedContent (0x806769c) | msg_deleted_content | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x2407 | vm_msgCreatedContent (0x8067704) | msg_created_content | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x54A1 | vm_msgMovedContent (0x8068000) | msg_moved_content | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x2409 | vm_msgGotCatalog (0x80678e0) | msg_got_catalog | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x240F | vm_msgGotCatalogEx (0x8067948) | msg_got_catalog_ex | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x241B | vm_msgReadLog (0x80679b0) | msg_read_log | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x0F51F | vm_msgUpdateLog (0x8067a18) | msg_update_log | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x240B | vm_msgGotPageData (0x8067a80) | msg_got_page_data | 3 | 88,88,**357** | [u32 gid][u32 strLen][str] |
| 0x240D | vm_msgGotPageDataEx (0x8067b00) | msg_got_page_data_ex | 3 | 88,88,**228** | [u32 gid][u32 blen][buffer] |
| 0x2411 | vm_msgDeletedGid (0x8067c68) | msg_deleted_gid | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x2413 | vm_msgUpdatedGid (0x8067cd0) | msg_updated_gid | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x5409 | vm_msgCreatePetDeposit (0x806776c) | msg_create_pet_deposit | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x5401 | vm_msgReadPetDeposit (0x806783c) | msg_read_pet_deposit | 4 | 88,88,88,**357** | [u32 a][u32 b][u32 strLen][str] |
| 0x5405 | vm_msgWritePetDeposit (0x80677d4) | msg_write_pet_deposit | 3 | 88,88,88 | [u32 a][u32 b] |
| 0x4402 | vm_msgGetCounts (0x8066ed4) | msg_get_counts | 4 | 88,88,88,**357** | [u32 cookie][u32 result][u8 count][count bytes] |

> 原「推测 LPC」表 15 项**全部与 decompile 结果一致**（驼峰转下划线这次碰巧全对），
> 已逐项用 `vm_apply` 第 6 个实参核实，nArgs/type 序列以 decompile 为准。

要点：

- **11 项「两 u32」**（deleted/created/moved content、got/read/update catalog/log、
  deleted/updated gid、create/write pet_deposit）body 恒为 8 字节 `[u32 a][u32 b]`，
  LPC 3 参数全 int，共用一个 handler。
- **msg_got_page_data / msg_got_page_data_ex**：body `[u32 gid][u32 strLen][str/buffer]`。
  前者 type=357(string)，原驱动就地 `body[strLen]=0` 补 NUL；后者 type=228(buffer)，
  带长度、**不**补 NUL。handler 分别用 `make_string` / `make_buffer`。
- **msg_read_pet_deposit**：body `[u32 a][u32 b][u32 strLen][str]`，LPC 4 参数
  (int,int,int,string)，与 msg_read_content 同构。
- **msg_get_counts**：body `[u32 cookie][u32 result][u8 count][count bytes]`，count 是
  **u8**（原驱动 `char counts[280]` 栈缓冲），LPC 4 参数 (int,int,int,string)。
- 15 项 vm_apply 前 5 个实参均 `(pProgram,0,0,0,0)`、type 只在 {88,357,228}，**无跳过项**。

### 8.2 handler（`game_service.cpp:326` 起，已写入）

```c
// 11 项「两 u32」共用：
void handleMsgIntInt(GameClient& cl, const uint8_t* body, size_t len, const char* fun) {
    if (len < 8) return;
    uint32_t a = rd32be(body), b = rd32be(body + 4);
    applyLpc(fun, {make_int(cl.clientNo), make_int(a), make_int(b)});
}
// handleMsgReadPetDeposit   : len>=12; [u32 a][u32 b][u32 strLen][str] -> (int,int,int,str)
// handleMsgGotPageData      : len>=8;  [u32 gid][u32 strLen][str]        -> (int,int,str)
// handleMsgGotPageDataEx    : len>=8;  [u32 gid][u32 blen][buffer]       -> (int,int,buffer)
// handleMsgGetCounts        : len>=9;  [u32 cookie][u32 result][u8 count][count bytes]
//                                       -> (int,int,int,str)
```

### 8.3 kCmdList 追加条目 + dispatch 分支（已写入）

```c
{ 0x2405, ATTRIB_INTERNAL_COMM, "msg_deleted_content", 0 },    // vm_msgDeletedContent
{ 0x2407, ATTRIB_INTERNAL_COMM, "msg_created_content", 0 },    // vm_msgCreatedContent
{ 0x54A1, ATTRIB_INTERNAL_COMM, "msg_moved_content", 0 },      // vm_msgMovedContent
{ 0x2409, ATTRIB_INTERNAL_COMM, "msg_got_catalog", 0 },        // vm_msgGotCatalog
{ 0x240F, ATTRIB_INTERNAL_COMM, "msg_got_catalog_ex", 0 },     // vm_msgGotCatalogEx
{ 0x241B, ATTRIB_INTERNAL_COMM, "msg_read_log", 0 },           // vm_msgReadLog
{ 0x0F51F, ATTRIB_INTERNAL_COMM, "msg_update_log", 0 },        // vm_msgUpdateLog
{ 0x240B, ATTRIB_INTERNAL_COMM, "msg_got_page_data", 0 },      // vm_msgGotPageData
{ 0x240D, ATTRIB_INTERNAL_COMM, "msg_got_page_data_ex", 0 },   // vm_msgGotPageDataEx
{ 0x2411, ATTRIB_INTERNAL_COMM, "msg_deleted_gid", 0 },        // vm_msgDeletedGid
{ 0x2413, ATTRIB_INTERNAL_COMM, "msg_updated_gid", 0 },        // vm_msgUpdatedGid
{ 0x5409, ATTRIB_INTERNAL_COMM, "msg_create_pet_deposit", 0 }, // vm_msgCreatePetDeposit
{ 0x5401, ATTRIB_INTERNAL_COMM, "msg_read_pet_deposit", 0 },   // vm_msgReadPetDeposit
{ 0x5405, ATTRIB_INTERNAL_COMM, "msg_write_pet_deposit", 0 },  // vm_msgWritePetDeposit
{ 0x4402, ATTRIB_INTERNAL_COMM, "msg_get_counts", 0 },         // vm_msgGetCounts
```

dispatch switch（`game_service.cpp:443`）已追加 15 个 `case 0x....:` 分支。

---

## 9. 剩余批次（已全部完成 ✅）

> 本批（msg_cfg_* 剩余 + type=1 的 749 项 + type=2 剩余 137 项）已在后续会话全部
> decompile 确认并写入 `src/game_service.cpp`，共 886 项，见 `analysis/_cmdList_progress.md`。
> 以下为当时规划的清单，现均已落地。

### 原 msg_cfg_*（type=2，`_cmdList.txt` 525-541 行）
`0x2479 msgCfgAaaList, 0x2367 msgCfgCcsInfo, 0x2363 msgCfgDdbList, 0x235B msgCfgDistList,
0x2471 msgCfgEtcPageData, 0x135D msgCfgMyDist, 0x2473 msgCfgReadEtc, 0x247B msgCfgWriteEtc,
0x247D msgCfgCsaInfo, 0x247F msgCfgTtsInfo, 0x2475 msgCfgAHServerInfo, 0x3A02 msgInternalAuth`
（连同其余 type=2 消息，共 137 项，全部在 chunk5）。

### 原 type=1 的 751 项游戏命令（aaa 全量，`_cmdList_parsed.json` 里 type==1）
749 项全部在 chunk1-4。

### 每批交付物
1. 更新 `kCmdList`（追加条目，type 按 cmdList type 映射成 attrib 常量）。
2. 新增/更新 handler + dispatch switch 分支。
3. `analysis/_cmdList_progress.md`：已补清单 `{cmdNo, funcName, lpcFun, nArgs, [type序列]}` +
   跳过/异常清单（前 5 个 vm_apply 实参非 0、或 type 值非 88/357/453/936/228/394 的）。

## 10. 关键提醒（重复一遍，避免返工）
- body 读取顺序 ≠ vm_apply 参数顺序，两者都从同一份 decompile 各自确认。
- 数组命令压入单个扁平 array，不是二维。
- 反编译里 type 常量：`0x58`=88(int)、`0x165`=357(string)、`0x1C5`=453(mapping)、
  `0x3A8`=936(array)、`0xE4`=228(buffer)、`0x18A`=394(string+len)。
- 看到 `109`/`975` 或前 5 个实参非 0 → 记录跳过，留人工。
