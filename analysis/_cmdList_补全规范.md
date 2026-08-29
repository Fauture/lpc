# cmdList 补全规范（flash 执行版）

目标：把原驱动 `magic_Linux32x` 里的 `cmdList` 表（924 项）逐条还原成 lpcvm 的
`kCmdList` + 对应 C handler，使 gs/ccs/dba（无 comm_desc.txt 的项目）能靠
driver 级 cmd 处理消息。**禁止自动推导 / 驼峰转下划线猜测 LPC 函数名**，
每个命令都要在 IDA 里 decompile 对应 `vm_cmdXXX` 确认 `vm_apply` 的函数名与
参数类型序列。

---

## 1. 数据来源与格式

- 原始表导出：`analysis/_cmdList.txt`（930 行，IDA `.data:083C8E60` 导出）。
  每行格式：
  ```
  Vm_Command_List_T <cmdNo(hex), type, offset vm_cmdXXX>
  ```
  有两条特殊形式：
  - `2 dup(<cmdNo, type, offset fn>)` → 该条目重复 2 次（同 cmdNo 连两个函数）。
  - 跨行续行 `\`（`offset vm_cmd_XXX` 被折到下一行）。
- 已解析结果：`analysis/_cmdList_parsed.json`，结构为数组
  `[ [cmdNo(int), type(int), funcName(str)], ... ]`，共 917 项。
  其中 `type` 分布：`1`=751 项，`2`=164 项，`3`=1 项，`7`=1 项。

`type` 字段就是 attrib 掩码（原 driver 用 u16，lpcvm 用 uint32）：
| type | 含义 | lpcvm 常量 |
|---|---|---|
| 1 | 客户端命令 CLIENT_USER | `ATTRIB_CLIENT_USER` (1) |
| 2 | 服务器间消息 INTERNAL_COMM | `ATTRIB_INTERNAL_COMM` (2) |
| 4 | 来自服务器（serverCmdList 3 项） | `ATTRIB_FROM_SERVER` (4) |
| 3 / 7 | echo 类特殊（0xF9C3 msgReplyEcho / 0x20D2 cmdEcho） | 按位写 |

> serverCmdList 是独立的 3 项表（`.data:083C8E40`）：`0x5351 msg_l_auth`、
> `0xF9D3 msg_login_done`，外加 `0xF9C3 msgReplyEcho`（type=3 在 cmdList 里）。

---

## 2. 每个命令的确认流程（逐个 decompile，不猜）

对 `_cmdList_parsed.json` 里每一项 `funcName`（如 `vm_cmdAddFriend`）：

1. 在 IDA 定位函数：`mcp__ida__lookup_funcs` 传函数名（或直接从
   `_cmdList.txt` 的 `offset vm_cmdXXX` 得到地址，IDA 里 `offset` 是符号，
   lookup 即可）。
2. `decompile` 该函数，找到 **`vm_apply(...)` 调用**（可能被优化成直接调
   `vm_invokeScriptFunction` 或内联，但绝大多数是 `vm_apply`）。
3. 从该调用提取三样东西：
   - **LPC 函数名**（`vm_apply` 第 6 个实参，字符串字面量，如 `"msg_cfg_my_info"`）。
   - **nArgs**（第 7 个实参，`vm_apply` 的 `a7`）。
   - **type/value 对序列**（第 8 个实参开始，`type1, value1, type2, value2, ...`）。

一个典型 decompile 形态（x86 cdecl 反编译后通常只有 `vm_apply(0,0,0,0,0, "...", n, ...)`）：

```c
vm_apply(0, 0, 0, 0, 0, "msg_cfg_spa_info", 4,
         0x58u, clientNo,    // 88  = int
         0x165u, ip,         // 357 = string
         0x58u, port,        // 88  = int
         0x165u, name);      // 357 = string
```

> 注意：`vm_apply` 的前 5 个实参是 `(pProgram, inFd, outFd, pReturnNumber,
> pending)`，绝大多数命令传 `0,0,0,0,0`。**若某命令的这 5 个里有非 0**（例如
> 要拿返回值的 `pReturnNumber`），要单独记录，这属于特殊命令，先跳过留给人工。

---

## 3. vm_apply 签名与 type 编码表（已从 IDA 确认）

`vm_apply` @ `0x809a328`：

```c
Vm_Status vm_apply(Vm_Program_T *pProgram, int inFd, int outFd,
                   int *pReturnNumber, int pending,
                   const char *funName, int nArgs, ...);
```

`vm_insertArgsIntoFunction` @ `0x8099678` 是 type 的权威解释器，type→LPC 类型：

| type 值 | 十六进制 | LPC 类型 | lpcvm Value | 值布局（va_list 里） |
|---|---|---|---|---|
| 88  | 0x58  | number (int) | `make_int` | 1 个 int |
| 109 | 0x6D  | number（取指针指向值） | `make_int` | 1 个 `int*` |
| 228 | 0xE4  | buffer | `make_buffer` | ptr + len（占 12 字节） |
| 357 | 0x165 | string | `make_string` | 1 个 `char*` |
| 394 | 0x18A | string（带长度） | `make_string` | ptr + len（12 字节） |
| 453 | 0x1C5 | mapping | `make_mapping` | 1 个指针 |
| 936 | 0x3A8 | array | `make_array` | 1 个指针 |
| 975 | 0x3CF | 任意值（复制） | 视情况 | 1 个 `Vm_Value*` |

**cmdList 里实际出现的几乎只有 88 / 357 / 453 / 936 四种**，偶有 228(buffer)、
394(string+len)。看到 109/975 先记录跳过。

---

## 4. body 的 value-type 编码（vm_padVariable / vm_pktPadVariable 线格式）

body 是命令的负载，handler 要按此规则逐字段读：

- **string**：`u8 长度` + 字节（不含 NUL）。
- **int / int16**：`u16` 大端（`(p[0]<<8)|p[1]`）。
- **mapping**：`u16 元素个数` + 每项 `key(string) + value(string)`（具体键值
  类型以 decompile 为准，msg_cfg_my_info 里就是 string→string）。
- **array**：通常 `u16 元素个数` + 元素，元素字段布局按该命令的 vm_apply 参数
  类型逐一读（string→u8len+str，int→u16）。**读法必须以 decompile 的 push 顺序
  为准，不要猜字段数。**
- buffer / 其它：以对应 vm_cmdXXX 的 decompile 为准。

现有 `VtReader`（`src/game_service.cpp:185`）已封装 `str()`（u8len+str）与
`u16()`（u16 BE），够用；缺 `u32()` 时按 `rd32be` 补。

---

## 5. 输出：kCmdList 条目 + handler

对每个确认过的命令，产出两块：

### 5.1 kCmdList 条目（`src/game_service.cpp:98` `const CmdEntry kCmdList[]`）

```c
{ 0x<cmdNo>, <attrib掩码>, "<lpc函数名>", 0 },  // vm_cmdXXX
```

- `attrib掩码`：直接把 `_cmdList_parsed.json` 的 `type` 值写成常量
  （1→`ATTRIB_CLIENT_USER`，2→`ATTRIB_INTERNAL_COMM`；serverCmdList 三项按
  `ATTRIB_CLIENT_USER | ATTRIB_INTERNAL_COMM | ATTRIB_FROM_SERVER` 写）。
- `lpc函数名`：从 vm_apply 第 6 个实参原样抄（**不要驼峰转下划线改写**）。
- `argSpec` 字段暂保留 0，handler 里解析。

### 5.2 handler 函数 + dispatch switch 分支

```c
void handleXxx(GameClient& cl, const uint8_t* body, size_t len) {
    VtReader r(body, len);
    // 按第 4 节的线格式 + decompile 的顺序逐字段读
    std::string a = r.str();
    int b = r.u16();
    ...
    applyLpc("<lpc函数名>", {make_int(cl.clientNo), make_string(a), make_int(b), ...});
}
```

dispatch switch（`src/game_service.cpp:340`）加：
```c
case 0x<cmdNo>: handleXxx(cl, body, len); break;
```

---

## 6. handler 可复用的 API（均已存在，勿重复造）

- `applyLpc(const char* fun, const std::vector<Value>& args)` → 调 master 程序
  的 LPC 函数。**第一个参数永远 `make_int(cl.clientNo)`**（除非 decompile 明确
  第一个不是 clientNo，但 751 项游戏命令 + msg_cfg_* 都是）。
- 值构造（`src/value.h`）：`make_int(int64)`、`make_string(std::string)`、
  `make_buffer(std::string)`、`make_array()`、`make_mapping()`。
- 数组：`auto arr = std::make_shared<ArrayValue>(); arr->items.push_back(...);`
  mapping：`auto m = std::make_shared<MappingValue>(); m->insert(key, val);`
- 参考已实现的 6 个 msg_cfg_* handler（`src/game_service.cpp:206-308`）——
  **先读它们再写**，保持风格一致。

---

## 7. 关键坑（务必遵守）

1. **body 读取顺序 ≠ vm_apply 参数顺序（有时不同）**。例：
   `msg_cfg_my_info`（vm_msgCfgMyInfo 0x8065b30）：body 是
   `name, port, dist, id, distId, [settings mapping]`，但 apply 是
   `(clientNo, dist, name, port, id, distId, settings)`。**两个顺序都从同一份
   decompile 各自确认，不要想当然对应。**
2. **数组命令的 vm_apply 是压入单个扁平 array**，不是二维数组。LPC 端是
   `list[i], list[i+1], ...` 连续读（见 msg_cfg_game_server_list / dba_list /
   cta_list）。所以 handler 里 `arr->items` 直接按元素平铺 push。
3. **dup 条目 cmdNo 重复**（如 `vm_cmdAnswerSurvey` 出现 2 次），kCmdList 里
   只保留一份 cmdNo 即可（`findCmd` 是线性查第一个）。
4. 看到 vm_apply 前 5 个实参非 0、或 type 值不是 88/357/453/936/228/394 的，
   **记录到单独清单**，跳过，留给人工。
5. 反编译里 type 常量通常以十六进制出现：`0x58`=88、`0x165`=357、
   `0x1C5`=453、`0x3A8`=936、`0xE4`=228、`0x18A`=394。
6. 不要改 `dispatchCommand` 的 attrib 检查逻辑（`(cl.attrib & e->type & 7)`）；
   只需按上面写 type 掩码。

---

## 8. 分批优先级（先跑通 ccs ↔ dba，再补游戏命令）

1. **第一批（最高优先）**：type=2 的 164 项里，filed↔dba 数据协议，直接决定
   ccs 的 share_datad 能否过。已知关键项：
   - `0x4401 msgReadContent`、`0x2403 msgWrittenContent`、`0x3411 msgCreatedGid`、
     `0x3415 msgGotGid`（filed 读写，dba 端处理）。
   - 其余 164 项 type=2 按 `_cmdList_parsed.json` 顺序逐个补。
2. **第二批**：type=2 里剩余的 msg_cfg_*（已补 6 个，核对未补的：msgCfgAaaList
   0x2479、msgCfgCcsInfo 0x2367、msgCfgDdbList 0x2363、msgCfgDistList 0x235B、
   msgCfgEtcPageData 0x2471、msgCfgMyDist 0x135D、msgCfgReadEtc 0x2473、
   msgCfgWriteEtc 0x247B、msgCfgCsaInfo 0x247D、msgCfgTtsInfo 0x247F、
   msgCfgAHServerInfo 0x2475、msgInternalAuth 0x3A02 等）。
3. **第三批**：type=1 的 751 项游戏命令（aaa 全量）。

---

## 9. 每批交付物

- 更新 `kCmdList`（追加条目）。
- 新增/更新 handler 函数 + dispatch switch 分支。
- 一份 `analysis/_cmdList_progress.md`（或 JSON）：已补命令清单
  `{cmdNo, funcName, lpcFun, nArgs, [type序列]}`，以及"跳过/异常"清单。
