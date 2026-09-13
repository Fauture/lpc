# -*- coding: utf-8 -*-
"""Append section 5.J to HANDOVER.md (LF file)."""
import io

P = 'HANDOVER.md'
d = io.open(P, 'rb').read()
assert d.count(b'\n\n---\n\n## 6. ') == 1

NEW = u"""

### J. 「玩家数据不保存 / 重上每次重跑剧情 / 礼包打开变成新建礼包」— 三个根因（已修，已端到端验证）

用户口径：①「玩家数据并没有保存下来，重上每次都触发剧情剧本，角色数据也恢复到了1级，购买的物品也不见了」；
②「打开新手礼包错误，正常是弹出一个列表选择取出物品，现在的是打开后生成一个新的新手礼包」。

**先定位数据面**：`dl_ddb_1_test.data` 里角色自己的 dbase 只在**创建那一刻**写过一次，之后再没更新过：
```
修复前:  user/<gid> = 1520 B @创建时刻；carry=17 B；achieve=4 B          ← 之后永远是这条
修复后:  user/<gid> = 3023 B @注销时刻；carry=124 B；achieve=488 B        ← 注销时真正回写
```
（同一时间 `purchase_infos`/`lgxy` 等其它路径一直在写 → dba/`write_content` 本身没问题，问题在**没人触发角色存盘**。）
②就是①的后果：礼包的 `gift_info` 没有恢复 ⇒ `newbie_gift::is_empty()` 为真 ⇒
`open_newgift_item` 走 `rebuild_newbie_gift`（发下一个礼包）而不是 `build_item_info`（弹列表）。

#### J1. 客户端断开时驱动没有通知 LPC → 角色永不析构、永不存盘

原版驱动在 socket 关闭时会 `vm_apply("cmd_disconnected", cno, where)`；
`start_gs::_cmd_disconnected(37938)` → `user.o::disconnected(425)` → （非 logout）`set_timer(180, "time_expired")`
→ `destruct` → `userd::destroy_user(2645)` → `me->save(0, cb)`（**唯一**把角色 dbase 写回 ddb 的路径）。
我们的 `src/game_service.cpp` 收完包就直接 `closesocket` + 从 `gameClients()` 里删掉，**从不通知 LPC**。

**修**：客户端线程在注销 client slot 之前调用 `cmd_disconnected(cno, 2)`（2 = `stat_disconnect` 的 "driver_callback"），
且必须在 `gameClients().erase()` 之前（LPC 用 `find_user_by_client(cno)` 找对象）。

#### J2. `callDestructors` 先跑继承来的析构 → `carry.c::destruct` 把 `destructing` 置位 → `user.o::save` 直接 return 1

`user.o` 继承 `/gs/feature/char/carry.c`（user_v37.c:17），而
```lpc
// carry.c:428
private void destruct() { if (!is_clone() || !is_mapping(inv = query_inventory())) return;
                          lite_set_temp("destructing", 1); loop (item in values(inv)) destruct_object(item); }
// user.o:633
override int save(mixed as_create, mixed f) {
    if ((!lite_query_temp("entered_world")) && (!lite_query_temp("permit_save"))) return 1;
    if ((lite_query_temp("destructing") || lite_query_temp("destructing_store"))) return 1;   // ← 命中
```
`LPCVM_FNTRACE=sav` 实录（注销时）：
```
[fn] ENTER get_save_done_func obj=userd.o args=2
[fn] RETURN get_save_done_func ret=<function call_other("...userd.c","notify_save_done","1","user/","0000000000000031","",(undefined))>
[fn] ENTER save obj=/gs/clone/char/user.o args=2
[fn] RETURN save ret=1          ← 立刻返回，没有 ::save / save_to_text / 任何 filed 写入
```
即：`destruct_object(user)` 时 `callDestructors` 按 `funcsByIndex_` 的**下标顺序**跑所有 `destruct`，
继承来的 `carry.c::destruct` 先跑并把 `destructing` 置位，随后 `user.o::destruct` → `destroy_user` → `save` 被自己人拦掉。

**修**（`src/original_vm.cpp` `callDestructors`）：按 `funOffset`（对象自身程序 = 0，继承程序依次递增）**升序**稳定排序，
即**先自身、后继承**——这也正是引擎的析构链顺序（本对象 `destruct` 是主析构）。
`stable_sort` by `(funOffset, index)`。

#### J3. `destructObjectByKey` 先摘注册表再跑析构 → 析构里的回调全成空操作

```cpp
reg.vms.erase(key); reg.objects.erase(key); reg.initialized.erase(key);   // 旧：先摘
vm->callDestructors(lifeOn);                                             // 后跑
```
析构链要**回调本对象**（`user.o::destruct` → `destroy_user` → ... → `me->save(0, cb)`、`carry.c` 里逐件 `destruct_object(item)`），
而 `call_other` 是通过 `reg.vms` 解析目标对象的——先摘掉就让这些调用变成静默 no-op。

**修**：先 `callDestructors`（对象仍在注册表里），**再**摘注册表/`regIndexUnregister`/park；
析构链里再出现 `destruct_object(<同一 key>)` 时直接返回（外层负责拆除），避免把注册表提前摘掉。

#### 验证（`_verify_save.py`，一轮里做两次登录）

```
session 1: 登录→建角→进世界 (PASS-ENTERED)
等 210s（注销定时器 180s）
ddb: ('user','...0031','',        3023 B, 18:14:44)   ← 主 dbase 回写
     ('user','...0031','achieve',  488 B, 18:14:44)
     ('user','...0031','carry',    124 B, 18:15:01)
     ('user','...0031','patch',    161 B, 18:15:01)
session 2: 再次登录 → **没有 0x20D1（剧情不再重跑）**、**0x2603 任务还在**、85 个世界包 PASS-ENTERED
```
（`_verify_save.py` 里 session 2 紧跟在存盘回调之后，正好落在"临界区未释放"窗口里被 9026/9029 拒了一次；
`stop_stun_account` 在 `notify_save_done` → `delete_content_to("runtime")` → `notify_delete_runtime_done` 之后才执行，
之后再登就正常了——原版同样是「注销 180s 定时器 + 帐号临界区」两段式。）

#### 顺带修好的：`destruct_object` 期间对象不可用

J3 让"析构过程中对象仍可被调用"成立——`carry.c::destruct` 逐件 `destruct_object(item)`、
`user.o::destruct` → `destroy_user` → `me->save()`、`me->remove_link_of_runtime_info()` 等全部恢复正常。
""" .replace('\n', '\n')

d = d.replace(b'\n\n---\n\n## 6. ', NEW.encode('utf-8') + b'\n\n---\n\n## 6. ')
io.open(P, 'wb').write(d)
print('written, bytes', len(d))
