# -*- coding: utf-8 -*-
"""Append section 5.H to HANDOVER.md (LF file)."""
import io

P = 'HANDOVER.md'
d = io.open(P, 'rb').read()
assert d.count(b'\n\n\n---\n\n## 6. ') == 1

NEW = u"""

### H. 「新连接后首个包要等约 3.6 秒」+「没有剧情 / 没有剧本演员」— 两个根因（已修，待真实客户端复测）

用户口径：①「新连接建立后首个包仍要等约 3.6 秒」（心跳是 10ms 一次，原版选线路后 ≤1ms 就该到选角界面）；
②「只有进入游戏被传送，然后什么都没：剧本没有、任务没有、只有新手礼包」，「剧情创建的 npc 也不存在」。

#### H1. 每个新连接白等 ~3.1 秒（accept 等错了注册表）

`src/game_service.cpp::startGameService` 的每连接线程，在读第一个包之前要等「客户端 agent 注册好」：

```cpp
for (int tries = 0; tries < 200; ++tries) {
    Value agent = callMasterProgramFunction("find_agent_by_client", {make_int(clientNo)}, root);
    if (agent.index() == 7) break;                  // object registered
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}
```

gs 里 `client_agents[]` **只**由 `feature/common/agent.c::comm_set_client_no` → `set_client_agent` 写入，
而它只能从 `comm::set_client_no` 到达（内部 comm agent，例如 configd 的 `config_agent`）。
**真实玩家连接**走的是另一张表：`logind::new_user(2434)` → `comm::set_client`(gs/feature/char/comm.c:751)
→ `set_client_user`(start_gs:39423) → `client_users[cno]`。

所以这个循环对玩家连接**一次都命中不了**：200 次 ×（5ms sleep + 每轮约 10ms 的 LPC 往返调度）≈ **3.1 秒**，
加在**每个新连接的第一个包之前**。

证据（lat4，未修版）：
```
[accept] t=1459253ms client=89 wait(无) agent-type=0 peer=192.168.200.1:60451   ← agent 永远是 nil
[recv-accept] t=1459253ms client=89 n=20 4D5A...                                 ← 读包时数据早已到达
```
客户端侧 `_conn_lat.py`（connect 后立刻发 echo，分离 connect/send/reply 三段）：
```
round 1: connect=9.0ms  send=9.0ms  reply=3132.5ms      ← 修前
round 1: connect=9.0ms  send=9.0ms  reply=12.0ms       ← 修后
```
`_conn_par.py`（两个连接相隔 200ms 打开）显示两者各自 ~3.1–3.6 秒返回 → 是**每连接固定开销**，不是串行临界区。
同一个 accept 循环在 aaa 上不慢（`agent-type=7`，configd 早就注册过 config_agent），所以只有 gs 受害。

**修**：等待条件改成「**user 或 agent 任一注册好**」，先查 `find_user_by_client`（玩家走这张表）：

```cpp
Value u = callMasterProgramFunction("find_user_by_client", {make_int(clientNo)}, root);
userType = (int)u.index();  if (u.index() == 7) break;
Value a = callMasterProgramFunction("find_agent_by_client", {make_int(clientNo)}, root);
agentType = (int)a.index(); if (a.index() == 7) break;
```
`[accept]` 日志现在带 `wait=` / `user-type=` / `agent-type=`：
```
[accept] t=672550ms client=5 wait=16ms user-type=7 agent-type=0
[accept] t=674633ms client=6 wait=0ms  user-type=7 agent-type=0
```
（drama1 一轮 8 次 accept：wait 最大 16ms、平均 2ms。）

#### H2. 剧本包 0x20D1 头部错位 → 客户端读到 list_count=0（没剧情、没剧本演员、剧情永不结束）

`gs/feature/char/menu_v37.c:197`：
```lpc
void set_talk_drama(object talk_npc, array actors, array content, mixed func = 1, string para = "")
```
`xlibd_v37.c:3310` 用 **4 个参数**调用它（`director->set_talk_drama(actor_npc, actors, scenario[0..<2], func)`），
因此 `para` 必须取默认值 `""`。我们的 VM 却把 **`func` 的默认值 `1`** 填给了 `para`。

证据（0x20D1 body 开头，修前 vs 修后）：
```
修前: 00 00 64 08 | 01 | 00 00 00 01 | 7C | 00 04 10 01 ...
        id(4B)    strlen  (buffer)para  ↑ 实际 list_count=0x7C=124 在这里
                  =1     = BE32(1)
修后: 00 00 64 69 | 00 | 7C | 00 04 10 01 ...
        id(4B)    strlen=0  list_count=124@offset5
```
`(buffer)para` = `00 00 00 01` 正是 BE32(1)，`strlen(para)` = 1（`as_string(1)` 是 "1"）——
两个字段都独立证明 `para` 拿到的是**整数 1**，而不是 `""`。客户端按原版布局解析
`id(4) | strlen(para) | para | u8 list_count | N×(u16 len + fragment)`：
它读到 **list_count = 0x00** → 一个片段都没有 → **剧本不播、剧本演员不生成**，
而且「初进游戏」剧本永远不结束（`func` 回调不触发）→ 任务停在 S0（log/tip 都是空串）。

**根因**：`src/original_vm.cpp` 的 `resort_stack`（op 71）。编译器为**所有带默认值的形参**生成一段内联默认值代码
（编译期不知道运行时实际传了几个），旧实现把 push 出来的**全部**默认值都当成"缺失形参"的默认值
（`f.argNum += defaultPushed;`）。5 形参传 4 个时 `defaultPushed=2`（`func=1`、`para=""`），
按正向顺序 push，于是 `para` 拿到第一个（`func` 的 `1`）。

**修**：`check_args_num`（op 15）把 `maxArgNo`（= max(skipAll, argCount)）记到 `Frame::maxArgNo`，
`resort_stack` 只保留**最后** `maxArgNo - argNum` 个默认值：

```cpp
int keep = defaultPushed;
if (f.maxArgNo > 0) {
    int missing = f.maxArgNo - f.argNum;          // 真正缺几个形参
    if (missing < 0) missing = 0;
    if (missing < defaultPushed) keep = missing;
}
for (int i = defaultPushed - keep; i < defaultPushed; ++i) defaults.push_back(...);
...
f.argNum += keep;
```
默认值块是**形参正向顺序** push，所以要保留的是**最后** keep 个（这一条已由 H2 的证据钉死：
第一个 push 的默认值属于 `func`，不是 `para`）。`keep == defaultPushed` 时与旧行为完全一致，
所以 `get_all_catalog(condition="", timeout=1800)` 那类调用不受影响。

**顺带修好的同类错位**：`send_task_log(task_name, tip=0, sync=0, upgrade_type=-1)`（gs/feature/char/comm.c:986）
用 2 个参数调用时 `upgrade_type` 曾拿到 `0`（应为 `-1`），使 `upgrade_type < 0` 的分支判断走错。

#### 验证

- `_conn_lat.py`：gs 首个 echo 回包 **3132ms → 12ms**（aaa 一直是 ~2ms）。
- `_enter_probe.py`（drama1/）：`PASS-ENTERED`，103 个世界包；世界包从 7.6s 提前到 **1.3s**（accept 不再空等）。
- `0x20D1`：`id=25705, strlen(para)=0, list_count=124@offset5`，`_dec_drama.py` 暴力对齐确认
  「片段数 × (u16 长度 + 内容)」恰好读完 3544 字节、无剩余。
- `0x2603`：`_dec_task.py` 逐字段解码，190 字节一字不差：
  `【新手】1-9级新手任务|【新手】新的开始` / desc 70B / refresh=1 / person=1人 / level=1~9级 /
  bonus=#I数值奖励|… / task_type=指引 / att32=0。
- **待真实客户端复测**：剧本是否播放、剧本演员是否出现、随剧情推进的任务是否显示。
""" .replace('\n', '\n')

d = d.replace(b'\n\n\n---\n\n## 6. ', NEW.encode('utf-8') + b'\n\n---\n\n## 6. ')
io.open(P, 'wb').write(d)
print('written, bytes', len(d))
