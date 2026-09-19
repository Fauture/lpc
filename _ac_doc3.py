# -*- coding: utf-8 -*-
"""Append the real-client verification result to HANDOVER.md."""
import io

TEXT = '''
### 真机验收（2026-09-20，日志 `acfix4`，账号 `1` / 角色 `awesrd`）

* 进游戏后 **+137.5 秒**客户端发来 `0x5108`（`[recv] t=1851679ms cno=91 cmd=0x5108 len=484`），
  服务端流水线全程命中：
  ```
  [ac] cmd_check_server returned (undefined)      <- LPC 动态对象路径仍未跑通
  [ac] challenge script 788 bytes head=206F76657272696465 (" override ")
  [ac] x=7454 y=963 answer=80017216
  [ac] sent 0x21B1 cno=91 x=7454 y=963 answer=80017216
  ```
* `python -X utf8 _ac_live.py acfix4` 用**独立的 Python 参考实现**复算同一条真机脚本，
  得到 `7454 963 80017216`，与服务端发出的回答**逐位一致**（1/1 MATCH）。
* **客户端不再重发**：历史会话（`audit2`）客户端在 +137.8 / +152.8 / +167.8 / +182.8 秒
  连发 4 次、全部无回答，并在 **+212.9 秒**自登出；本次客户端在 +137.5 秒收到回答后
  **只发一次**，`0x3e2a`（客户端登出）条数 **0**。
* **挂机 8.9 分钟（533.6 秒）全程在游戏内**（`_soak2.py acfix4 9 awesrd` 每 10 秒采样，
  标题始终 `问道(1.60.0905) [一线] [awesrd]`），远超用户报告的 3-5 分钟掉线窗口；
  服务端 4 个 `lpcvm` 进程全程存活。截图：`_soak_final.jpg`。

**结论：3-5 分钟掉线的直接原因就是 `0x5108` 挑战没有回 `0x21B1`（或回答值不对）；
现在回包与答案均已对齐真机。**
'''

p = 'HANDOVER.md'
s = io.open(p, encoding='utf-8', errors='replace').read()
if '真机验收（2026-09-20，日志 `acfix4`' in s:
    print('already appended')
else:
    io.open(p, 'w', encoding='utf-8', newline='\n').write(s + TEXT)
    print('appended')
