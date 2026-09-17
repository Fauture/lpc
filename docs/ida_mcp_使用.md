# IDA MCP 使用手册（原版语义核对）

> 用途：任何 efun / 操作码 / 驱动行为不清楚时，**先在这里查原版 `magic_Linux32x`**，禁止猜测实现。
> 规则本体见各文档开头的「⛔ IDA 语义核对（强制）」。

## 1. 连接

* IDA MCP 服务：`http://127.0.0.1:13337/mcp`（streamable HTTP，MCP 协议 `2025-06-18`，工具 60+ 个）。
* 当前 IDB：`C:\Users\Administrator\Desktop\magic_Linux32x.i64`，`imagebase = 0x8048000`，Hex-Rays 可用（`server_health` 可确认）。
* 命令行客户端：仓库里的 `_idamcp.py`（只用标准库，无需额外依赖）：

```powershell
python -X utf8 _idamcp.py list                       # 列出所有工具
python -X utf8 _idamcp.py schema decompile disasm     # 看某工具的参数结构
python -X utf8 _idamcp.py look vm_f_mapping_array     # 按名字/地址查函数
python -X utf8 _idamcp.py dec 0x80d876c               # 反编译 -> _ida_out/addr_80d876c.c
python -X utf8 _idamcp.py dis 0x80d8600               # 反汇编
python -X utf8 _idamcp.py call find '@_mcp_args.json' # 任意工具，参数放 JSON 文件（PowerShell 引号会吃掉内联 JSON）
```

* PowerShell 注意：内联 JSON 的双引号会被吞掉，**参数一律写进文件**再 `'@文件'` 传（脚本支持 `@file` 前缀）。

## 2. 找原版函数的三步法

1. **efun → 处理函数地址**：仓库里已有现成表，不用猜：
   * `_efun_table.txt`：`名字 地址 处理函数名`（例：`mapping_array 0x80d876c vm_f_mapping_array`）
   * `_efun_registered.txt`：`名字 地址 参数个数 处理函数`
   * `tools/efun_names_full.json` / `tools/efun_table.json`：efun 索引 ↔ 名字
2. **反编译**：`_idamcp.py dec <地址>`，结果落到 `_ida_out/`，重点看参数检查、返回值类型、错误分支。
3. **追调用链/表**：`xrefs_to`（谁引用它）、`find type=data_ref`（名字字符串 → 分发表项）、
   `callees`（它调用了谁，例如 `vm_f_mapping_array` → `vm_f_mapping_array1` → `vm_f_mapping_array2`）。

## 3. 已核对案例（照着做）

**`mapping_array`（新手上任务 S11 张老板发武器失败）**

* `mapping_array` = `vm_f_mapping_array` @ `0x80d876c`：参数 array + func，非空则压入"与源同长度"的结果数组 + index=0，
  调 `vm_f_mapping_array1`；空数组直接返回空数组。
* `vm_f_mapping_array1` @ `0x80d8600`：`index++`，校验 ret/src 长度相等，`index >= size` 收尾；
  否则压 `src[index]` + 函数并调用，再用 `vm_hookCallbackWhenNextAction` / `vm_hookCallbackWhenReturnAt`
  挂 `vm_f_mapping_array2`。
* `vm_f_mapping_array2` @ `0x80d8870`：把返回值写进 `ret[index]`，再回 `vm_f_mapping_array1`。
  → 结论：**原版是"返回钩子驱动的蹦床"，每个元素只占一次 VM 调用，不占 C++ 递归栈**；
    结果数组与源**同长同序**，不过滤。（我们的实现必须照此，否则大数组映射会撑爆 C 栈。）
* 相关：`vm_f_load_object` @ `0x80f8cbc` —— 已加载对象直接返回缓存（`flags != 2048`）；
  构造中的对象用 `flags(0x800/0x1000/6144)` + `initStep` 标记并继续推进，**不会重新跑 create**。
  这是"装载链重入"的判据。

## 4. 记录要求

* 核对完把地址、关键伪代码结论写进对应文档（如 `_ida_missing/<efun>.txt` 或 HANDOVER），
  并在代码注释里引用地址，方便下一个人复核。
* 没查到/查不动时：写清"未核对 + 待核对项"，不要用近似实现顶上去。
