# -*- coding: utf-8 -*-
"""Append section 5.K to HANDOVER.md (LF file)."""
import io

P = 'HANDOVER.md'
d = io.open(P, 'rb').read()
assert d.count(b'\n\n---\n\n## 6. ') == 1

NEW = u"""

### K. NPC 对话没头像/没名字（头像 FF FF FF FF、名字长度 00）— 省略实参被填成 int 0 而不是 undefined

用户口径：「npc还是没有头像,没有名字,头像变成了FF FF FF FF 名字变成了00长度」。

链路：`cmds/normal/open_menu.c::main(me, id)` → `me->start_talk_to(ob)`（menu.c:245）→
```lpc
set_menu(ob, menu, menu_content);          // 只传 3 个参数
```
而 `set_menu(ob, menu, content, portrait, pic_no, allow_close, mobile_use_flag)` 声明 7 个形参，后 4 个**省略**；
`build_menu_buffer` 里：
```lpc
if (is_string(portrait)) { ... }
else if (is_undefined(portrait)) { actor_name = ob->get_name(); portrait_used = (int)ob->query("portrait"); }
else { portrait_used = ((portrait == 0) ? -1 : portrait); actor_name = ""; }
```
**原版驱动把帧的形参区清零，而全零值在本 VM 里就是 type 0 = "undefined"**（`is_undefined()` 为真、`is_int()` 为假）。
我们的补位用的是 `pushValue(make_int(0))`（**int 0**）→ `is_undefined(portrait)` 为假、`is_int()` 为真 → 走了 else 分支：
`portrait_used = -1 (0xFFFFFFFF)`、`actor_name = ""` ⇒ 客户端「没头像、名字长度 0」。
（旧注释担心 `childd.update_child` 的 `is_int()` 检查——那是当年另一个 bug 的表象，现在按原版语义清零即可。）

**修**：`src/original_vm.cpp` 五处省略形参补位 `pushValue(make_int(0))` → `pushValue(Value{})`（= type 0 undefined）。

**验证**（`_npc_menu_check.py`，新增：点击视野内每个 NPC 并解码 `0x2037` 的
`id/portrait/pic_no/content/actor_name`）：
```
修复前: portrait=0xFFFFFFFF(-1)   name="" len=0
修复后: 0x549E MENU id=0x549E portrait=0x00000000(0) pic_no=1 content=31B name="宝箱" len=4   OK
        0x5492 MENU id=0x5492 portrait=0x00000000(0) pic_no=1 content=31B name="宝箱" len=4   OK
        0x548A MENU ... name="宝箱" len=4 OK        0x5499 MENU ... name="宝箱" len=4 OK
```
（这一轮点的是剧情临时 NPC「宝箱」，它们 `query("portrait")` 本来就是 0 —— 名字回来了就说明分支走对了；
真实 NPC 的 portrait 值需要走到揽仙镇里再点，见下轮待办。）
同轮回归：`0x20D1` 剧本照常、`0x2603` 的 `log(49)/tip(45)` 依旧、`[crash]` 0 次。

**顺带发现（未修，用户口径 2/3 的线索）**：
- 过图后服务端只回 `0x1155`，**不再广播新房间的对象列表**（`0xF05D`）：`_npc_menu_check.py` 里
  `0x1026 → 0x1155` 成功但 5s 内 `npcs=[]`。真实客户端是靠 `0x1B01`(apex) 再申请一次；要复现用户的
  「重上后物品/任务不显示、要动一下才出来」得对齐这条链路。
- 用户三个角色都被存在 `position.room=/lanxianzhenwai/lanxianzhenwai.c`（揽仙镇外）——
  过图卡死要按"当前房间的 `exits` 查不到目标"来查（`enter_room.c::main` 第 66/69 行两处 `break` 都是静默返回）。
""" .replace('\n', '\n')

d = d.replace(b'\n\n---\n\n## 6. ', NEW.encode('utf-8') + b'\n\n---\n\n## 6. ')
io.open(P, 'wb').write(d)
print('written, bytes', len(d))
