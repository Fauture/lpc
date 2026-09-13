# -*- coding: utf-8 -*-
"""Append section 5.I to HANDOVER.md (LF file)."""
import io

P = 'HANDOVER.md'
d = io.open(P, 'rb').read()
assert d.count(b'\n\n---\n\n## 6. ') == 1

NEW = u"""

### I. 「用新手礼包 GS 崩溃」+「任务没有提示/不显示」— 两个根因（已修，待真实客户端复测）

用户口径：剧本正常了，但①还是没有任务；②「貌似 gs 崩了」。

#### I1. 开新手礼包 → GS 栈溢出崩溃（`0xC00000FD`）

日志（drama1/，用户真实客户端）：
```
[recv-accept] t=1728218 client=96 n=13 4D5A...0003 220C 65      ← 客户端点用背包里的新手礼包
[case8] user.o::process_command -> commandd.o::process_command -> cmds/normal/apply.o::main
[life] destructor destruct/delete (idx 2,9,7,66,20,202...) on clone:gs/clone/item/newbie_gift/basic.o#16473
[crash] code=0xc00000fd addr=0x004EC0D7 frames=4                  ← STACK_OVERFLOW
[crash] last LPC functions: query_entire_dbase move_to_bag_check get_module_status get_iid lite_query
        get_import_name query_inventory lite_query_temp check_limit_item is_limit_item lite_query
        move_to_dest ... refresh_item ... build_item build_item_ex ... get_item_basic_info ...
```
链路：`apply()` → `open_newgift_item` → （礼包为空）`rebuild_newbie_gift` → `named.c->new_item`
→ `move_to_bag` → `put_in_container`/`notify_carrier`/`refresh_item`/`build_item`/`build_item_ex`
→ `get_item_basic_info` → `is_important_item` → `get_raw_name` → `basic_object` → `query`
——**约 100 层嵌套 LPC 调用**。

**根因**：`CMakeLists.txt` 里 `target_link_options(... /STACK:0x100000)`（1MB）。
调度器跑在**主线程**（`main.cpp` → `lpc::schedulerRun()`），所有 LPC 调用都嵌套在主线程栈上；
注释里「deep LPC call chains run on explicitly-reserved big-stack threads (create_thread)」**已经过时**
（create_thread 现在也走 `scheduler().spawn()`，见 `src/original_vm.cpp` 的 `if (!isTelnetThread)` 分支）。
原版 Linux 默认栈 8MB，所以 100 层没事；我们 1MB 直接爆。
这也让 `checkCallDepth()`（400 帧）形同虚设：1MB ÷ 100 帧 ≈ 10KB/帧，400 帧要 4MB，永远到不了就先崩。

**修**：`/STACK:0x2000000`（32MB）。地址空间代价可忽略（32 位 LARGEADDRESSAWARE 有 4GB），
而 400 帧上限现在真正生效（≈4MB < 32MB）→ 万一再撞到深递归，会抛可被 LPC `catch` 的 VmError 而不是杀进程。

#### I2. 任务日志下发但 `log`/`tip` 全空 → 客户端任务面板没有提示（用户口径「没有任务」）

`0x2603`（`send_task_log`）里 `task_log`/`task_tip` 两个字段一直是**空串**（修前）：
```
log(0) =           tip(0) =              ← 修前
log(49) = 到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途吧。
tip(45) = 到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途      ← 修后
```
链路：`taskd::query_task_log/query_task_tip`：
```lpc
funcs = all_templates[task_class];              // = instruction_1 的模板 mapping ✓
task_tip = funcs["tip"];                        // = (: call_other, ".../instruction_1.c", "task_tip" :)
if (is_function(task_tip)) task_tip = (*task_tip)(me, task_info);
if (!is_string(task_tip)) return "";            // ← 在这里返回空
```
`LPCVM_OPFN=query_task_tip`（新增：逐指令 + 栈顶类型）实录：
```
[op] ip=18 op=3 a=0  top=t6:(["冥-...":...])                 ← push_ovar(all_templates) 是 mapping ✓
[op] ip=21 op=9 ...  top=t6:(["dest":<function task_dest>, "tip":<function call_other(...)>])  ← 模板 ✓
[op] ip=33 op=9 ...  top=t8:<function call_other(".../instruction_1.c","task_tip")>            ← 闭包 ✓
[op] ip=38 op=! a=8  top=t2:1                                ← is_function(闭包)=1 ✓
[op] ip=42 op=5 a=4 b=3  top=t8:<function call_other(...)>   ← call_ptr(nargs=3)
[op] ip=43 op=9 ...  top=t2:1                                ← 调用结果是 int 1 ✗（应为 string）
```
**根因**：op 23 `create_function_value` 在「全局表位 + 找不到 master 函数 + 命中 efun 名」时建的
**efun 闭包**（`(: call_other, path, "fn" :)`、`(: create_thread, ... :)`）会被写成
`func=nullptr, funcIndex=<该 efun 在全局表里的槽号>, owner=nullptr`
（注释原话：「owner = nullptr so invocation reaches the efun fallback (call_fun_ptr's final else)」）。
但 `call_fun_ptr`（case 5）在到达最后的 efun-by-name 兜底之前，先跑了通用兜底：
```cpp
} else if (cl->funcIndex >= 0 && !cl->func) {
    auto fit = funcsByIndex_.find(cl->funcIndex);      // ← 当前 VM 的合并函数表
    if (fit != funcsByIndex_.end()) fn = fit->second;  // ← 撞车：索引空间和 efun 槽号重叠
    else if (auto gfn = findMasterFunction(cl->funcIndex)) fn = gfn;
}
```
只要**当前 VM 的合并表在这个下标上恰好有一个函数**（taskd 就有），`fn` 就被解析成那个**无关函数**
并 inline 执行——`task_tip` 根本没跑（FNTRACE 里 `task_tip` 的 ENTER 完全缺失），返回值是那段
字节码残留的 `int 1`。于是 `is_string(1)=0` → `return ""` → 任务没提示。

**修**（`src/value.h` + `src/original_vm.cpp`）：
- `ClosureValue` 新增 `bool efunClosure = false;`；op 23 的 efun 闭包分支置 `cl->efunClosure = true;`
  （两处闭包拷贝 `bindClosure`/`bind` 也一并复制这个标志）。
- `call_fun_ptr` 的兜底加上 `&& !cl->efunClosure`，让 efun 闭包直接落到 efun-by-name 的
  `callEfunWithArgs` 分支。
**影响面**：所有 `(: call_other, path, "fn" :)` 形式的回调（任务模板的 log/tip、剧本 `func`、
菜单 closure…）此前都可能被静默换成另一个函数的返回值。

**验证**（fix1/）：
- `[fn] ENTER task_tip` / `RETURN task_tip ret=到#Z揽仙镇外(108,156)#Z领取任务，开始新的旅途` ✓
- `0x2603` body 192 → **284** 字节，`log(49)`/`tip(45)` 非空，且逐字段解码仍恰好读完 ✓
- `0xF9FB`（任务追踪）带任务条目 ✓
- `0x20D1` 剧本不受影响：`strlen(para)=0, list_count=124@5`，124 片段恰好读完 3544 字节 ✓
- 全程 `[crash]`/`[deep]` 0 次，4 个服务存活 ✓

**新增探针**：`LPCVM_FNTRACE=<substr>`（匹配函数的 ENTER/RETURN + 返回值）、
`LPCVM_OPFN=<substr>`（匹配函数逐指令 `ip/op/a/b/m0/m1/sp/top(类型:值)`，按函数名封顶 400 行）。
""" .replace('\n', '\n')

d = d.replace(b'\n\n---\n\n## 6. ', NEW.encode('utf-8') + b'\n\n---\n\n## 6. ')
io.open(P, 'wb').write(d)
print('written, bytes', len(d))
