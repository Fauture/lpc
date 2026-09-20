# -*- coding: utf-8 -*-
"""Append the long-uptime watch + MySQL hardening section to HANDOVER.md."""
import io

TEXT = '''
---

## 2026-09-20 gs 长时间运行观察 + 长跑健壮性加固

### 观察方法（用户要求：每 10 分钟用真实客户端登录一次）

`_login10.py`（后台任务，日志 `_login10.log`）：每 10 分钟
`关闭 asktao.mod → _rdp start 重开 → _rdp auto 登录(1/1, awesrd) → 判定是否进游戏`，
每轮记录 gs 运行时长、是否进游戏、进游戏耗时、客户端日志新增行、截图 `_login_LNN.jpg`；
首次失败即打印 gs/客户端日志尾并以 rc=3 退出（便于在故障状态下直接开始修）。

**第一轮 8 小时结论：48 次登录全部成功，0 次失败，gs 运行到 495 分钟仍健康。**
* `[hb] clients=6` 稳定、`schedIters` 稳定 ~8300/11min、`sendFail=6` 不增长；
* `[accept] wait` 中位数 0ms、p90 16ms（日志里两条 wait=3206591ms / 26520401548ms
  是**多线程同时写 std::cerr 造成的串行交错**，原文可见 `wait=265` + `20401548ms`
  被拼接，不是真实等待）；
* 无 `no live connection` / `gone away` / 连接拒绝记录。

### 为什么「选线路」这一步在本环境复现不到

真机客户端的「重选区组」要访问官方区组列表（`Login.ini` 的 `[URL] Address0=…gyyx.cn`），
本环境不可达；把 `data/Config.ini` 的 `Dist=试剑内测` 删掉后客户端**仍然直接进登录界面**
（区组名由服务端下发），即本环境没有独立的「选择线路」界面，
等价步骤就是**选角色→进游戏**（客户端连 8160），这一点每 10 分钟都被监控覆盖。

### 已定位并修好的长跑缺陷：MySQL 连接失效后不再重连（本轮加固，待部署）

* 事实：MySQL `wait_timeout=interactive_timeout=28800`（8 小时），空闲连接会被服务端关掉；
  原版驱动靠 `dbd::assure_connection` **每 60 秒 ping** 并在需要时重新 `mysql_connect`，
  而我们的 `mysqlExecReal` 在连接不可用时**只返回失败、从不重连** ——
  一旦 socket 死掉，此后所有查询永远返回 0（角色读取/存档静默变空 → 进不去游戏），
  完全符合「gs 长时间运行后无法进入游戏」。
* 修复（`src/builtins.cpp`）：
  1. `MysqlHandle` 记住 `host/port/user/pass/database` + 上次使用时间 + 重连计数；
  2. 空闲 >120 秒的连接先跑一次 `SELECT 1` 保活（顺便刷新 MySQL 侧计时器），失败即重连；
  3. 连接不可用（含启动时就没连上）时**自动重连**；语句把 socket 打死时**重连后重试一次**；
  4. 重连日志：`[mysql] reconnected cno=… (reconnects=N)`。
* 状态：**已编译通过、已提交，但尚未部署**（部署需要在下次重启服务端时带上；监控当前跑的
  仍是旧 exe，因此这条修复不影响正在进行的观察）。

### 下一步

监控继续（`_login10.py` 16 小时窗口，gs 运行时长从 8.6 小时继续累加）；
一旦出现「进不去游戏」，用 `_fail_dump.py <logdir>` 取证（[hb] 趋势 / [accept] wait /
send-fail / 关键词 / 四服务日志尾）后立即定位修复，并把 MySQL 重连加固一起部署验证。
'''

p = 'HANDOVER.md'
s = io.open(p, encoding='utf-8', errors='replace').read()
if 'gs 长时间运行观察 + 长跑健壮性加固' in s:
    print('already appended')
else:
    io.open(p, 'w', encoding='utf-8', newline='\n').write(s + TEXT)
    print('appended')
