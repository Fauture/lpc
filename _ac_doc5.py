# -*- coding: utf-8 -*-
"""Append the reproduced 9030 failure + MySQL socket-timeout fix to HANDOVER.md."""
import io

TEXT = '''
---

## 2026-09-20 已复现「gs 长时间运行 → 真机选线路提示通讯错误(9030)」并定位到 DBA 停摆

### 复现（真机，`_login10.py` 自动抓到的就是这一刻）

```
[12:28:05] attempt 3 uptime=529.2min -> FAIL ingame=False enter=60.1s title=问道(1.60.0905) [试剑内测]
=== FAILURE: login did not reach the game ===
```
客户端界面：选区/线路界面 + 弹窗「**通讯故障，暂时无法登录，请稍候重试。（原因：9030）**」（`_now_err.jpg`）。
即：客户端连上了 gs、能登录界面、能收发 echo，但**进不了游戏**。

### 证据链（`errfix1` 日志）

1. 客户端 cno=3107 在 t=31574074ms 连上 8160，发 `0x4124` 登录包（203 字节），
   gs 派发到 `clone:gs/clone/char/login.o::process_command` ✓；
2. 之后客户端只发 echo，**58 秒内服务端没有任何回包** → 客户端 9030 断开（`n=-1`）；
3. gs 随后 `[send-filed] cno=5 msgNo=0x3400 raw data(22)=3400 0001FC46 07"runtime" 01"a" 00 04"data"`
   —— 登录流程要读 DBA 的 `runtime` 记录；DBA 侧确实收到了**三次**（GS 重试），
   也派发了（`communicated.o -> mixed_agent.o#6::process_command` →
   `dbaserverd.c::process_command`），但**一条回复都没有**（既无 -1/-2 也无内容）；
4. DBA 最后一次成功回包：给 gs(cno=4) 是 **337.6 分钟**，给 ccs(cno=2) 是 **342.1 分钟**；
   此后 DBA 一直在收包、一直在派发，就是不再回包 —— 而 342 分钟那次是
   `0xF501 active_stat_log` 写入之后；
5. DBA 进程活着：`[hb]` 每秒心跳、echo 正常应答、10 秒 CPU 仅 0.016s（**阻塞而非自旋**），
   `schedIters=293828(+78)`；
6. gs 侧对应卡在 `[idle-wait] event=ev_filed1 entry=vendue_daemon …`，即等一个永远不来的 filed 回复。

### 根因：MySQL 连接半死 → DBA 的 ddb 服务线程永久阻塞

`MysqlClient` 的连接是**阻塞 socket，且从未设置 SO_RCVTIMEO/SO_SNDTIMEO**
（只有 connect 有 5 秒超时）。长时间运行后连接会变成半死状态
（MySQL 自身 `net_read_timeout=30`/`net_write_timeout=60` 在语句中途掐断会话、
NAT/防火墙静默丢包、服务器重启等），此时 `recv()` **永不返回**：
DBA 那条唯一的 ddb 服务线程就永久卡住，之后所有 filed 命令「收到 + 派发 + 永不执行/回复」
—— 与观测完全一致。原版驱动有 `assure_connection` 每 60 秒 ping + 重连兜底，我们没有。

### 修复（本轮，已提交）

1. `src/mysql_client.cpp`：连接建立后设置 `SO_RCVTIMEO`/`SO_SNDTIMEO` = 20 秒；
   `readPacket`/`writePacket` 在超时/出错时**关闭 socket 并把错误写清楚**
   （`read timeout/error during packet body (wsa=10060)`），
   于是 `isConnected()` 变 false，不再无限阻塞。
2. `src/builtins.cpp`（上一提交 ebdd0e0）：`MysqlHandle` 记住连接参数与使用时间；
   空闲 >120 秒先 `SELECT 1` 保活；连接不可用（含启动时未连上）时自动重连；
   语句打死 socket 时重连重试一次；打点 `[mysql] reconnected cno=… (reconnects=N)`。
   两条合起来 = 原版 `assure_connection` 的 60 秒 ping + 重连语义，外加「绝不无限阻塞」。

### 复现/验证工具

* `_login10.py`：每 10 分钟真机登录一次，失败即打印 gs/客户端日志尾并退出（本轮就是它抓到的）；
* `_fail_dump.py <logdir>`：`[hb]` 趋势 / `[accept] wait` / send-fail / 关键词 / 四服务日志尾；
* `_filed_trace.py <logdir>` / `_filed_balance.py <logdir>`：gs→dba 的 filed 请求与回复时间线，
  用来确认「DBA 从某一刻起不再回包」；
* `_dbhealth.py`：MySQL 只读体检（wait_timeout、各连接空闲时长）。
'''

p = 'HANDOVER.md'
s = io.open(p, encoding='utf-8', errors='replace').read()
if '已复现「gs 长时间运行' in s:
    print('already appended')
else:
    io.open(p, 'w', encoding='utf-8', newline='\n').write(s + TEXT)
    print('appended %d chars' % len(TEXT))
