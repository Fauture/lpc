# -*- coding: utf-8 -*-
"""Stamp the "unclear efun/bytecode semantics must be verified against IDA" rule
into every project document (idempotent, same list as the DB rule)."""
import io
import os

MARK = 'IDA 语义核对（强制'
BLOCK = """## ⛔ IDA 语义核对（强制，任何对话 / 任何工具都必须遵守）

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

DB_END = '**宁可不测，也不许写库。**'

for path in DOCS:
    if not os.path.exists(path):
        print('SKIP  %s (missing)' % path)
        continue
    lines = io.open(path, encoding='utf-8', errors='ignore').read().splitlines()
    if any(MARK in l for l in lines):
        print('OK    %s (already stamped)' % path)
        continue
    try:
        end = next(i for i, l in enumerate(lines) if l.strip() == DB_END)
        lines = lines[:end + 1] + [''] + BLOCK.splitlines() + lines[end + 1:]
    except StopIteration:
        # no DB block: append at the top
        if lines and lines[0].startswith('#'):
            lines = [lines[0], ''] + BLOCK.splitlines() + [''] + lines[1:]
        else:
            lines = BLOCK.splitlines() + [''] + lines
    io.open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print('STAMP %s' % path)

# record the efun whose IDA semantics are still unverified
os.makedirs('_ida_missing', exist_ok=True)
p = os.path.join('_ida_missing', 'mapping_array.txt')
if not os.path.exists(p):
    io.open(p, 'w', encoding='utf-8', newline='\n').write(
        "efun: mapping_array (index 81, original vm_addMappingArray / vm_addMappingArray 0x809effc)\n"
        "status: 语义未用 IDA 核对（本会话无 IDA/MCP）。\n"
        "current implementation: original_vm.cpp callEfunWithArgs 内联实现 —— 对数组每个元素调用闭包，\n"
        "  返回结果数组；非闭包入参原样返回。\n"
        "evidence used: 调用点语义（scand::query_objects 用 (: find_basic_object_by_import_name :)\n"
        "  把 import_name 数组转成对象数组；importd 用 call_efun(mapping_array, nargs=2)）。\n"
        "to verify with IDA: 参数个数/类型检查、非函数入参的行为、返回数组是否保留空洞/顺序、\n"
        "  以及是否像 filter_array 一样跳过 0 结果。\n")
    print('WROTE %s' % p)
else:
    print('OK    %s exists' % p)
