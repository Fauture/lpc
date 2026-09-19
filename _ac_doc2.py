# -*- coding: utf-8 -*-
"""Append the 0x5108/0x21B1 section to HANDOVER.md (UTF-8, LF preserved)."""
import io

TEXT = '''

---

## 2026-09-20 掉线根因：客户端安全验证 `0x5108` → 必须回 `0x21B1`

### 现象与证据链

* 真机客户端每 ~15 秒发一次 `0x5108 (CMD_CHECK_SERVER)`，包体是**加密的 LPC 源码**
  （`lzss_decompress` + 滚动 xor：`code=102; if(b!=code) b^=code; code=((code*2)+7)^71 & 255`）。
* 服务端 `/gs/cmds/normal/check_server.c`（原版反编译：`linux/gs/pack_data/lib_gs32-c/gs/cmds/normal/check_server_v37.c`）
  必须做：`_fob = load_object("/challenge.c", 0, content_buf); x=_fob->get_x(); y=_fob->get_y();
  answer=_fob->challenge_server(x,y); me->send(8625, make32(x), make32(y), make32(answer));`
  即 **`0x21B1`**。
* **8 个历史日志目录里 `0x21B1` 条数 = 0** —— 客户端一个回答都没收到，
  于是弹「重新登录游戏，需要再次进行安全验证，请点击确认继续。」并自己登出
  （真机截图已确认该弹窗；服务端全程没有踢人）。
* 真机会话时长 160~213 秒，与「每 15 秒一次挑战、连续 N 次无回答后自登出」完全吻合。

### 本轮修复（`src/original_vm.cpp` / `src/game_service.cpp`）

缺的包已经在回：`[ac] sent 0x21B1 cno=… x=… y=… answer=…`。
但**光回包还不够**——客户端会校验 answer，所以解释器语义必须和原版逐位一致。
本轮修掉三个语义错误（此前 answer 全错）：

1. **循环体跑在环境副本上**：`runLoopBody` 把 `env_` 拷一份给内层 `Exec`，循环体的写入
   全部丢弃（`a += ({...})` 永远不增长）。现在循环体直接在调用者环境里执行
   （`Exec` 持有 `std::map&`，`{...}` 块也不再隔离作用域）。
2. **`a[i] = …` 被当成 mapping 写**：旧代码 `if (dst.kind != 'm') dst = VMAP();` 把数组
   整个换成空 mapping，于是生成脚本里的 `a[i] = (a[i] - …) & mask;` 直接失效。
   现在按数组下标写入元素（越界尾部按 push 处理）。
3. **循环变量语义**：按 IDA 原版 `vm_loop.c` 的 `init_for_loop 35` /
   `cond_jmp_for_loop 18` / `step_jmp_for_loop 80` + `vm_cmpAndJmpInLoop`
   （见 `original_vm.cpp::loopStep`）：循环自带计数器，**每一步都 `setVar(计数器)`**，
   即循环体内对 `i` 的修改（生成器故意插的 `i++`）会被下一步覆盖；
   边界是**闭区间**（原版 lib `pwd_gen_1980.c` 里 `loop (i = 0 upto sizeof(buf) - 1)` 可证），
   退出后变量留在 `B + 1`（原版先 setVar 再判越界）。
4. `count_total` = 求和（`builtins.cpp`，来自 IDA `vm_f_count_total`）、`sort` 升序、
   `keys/values` 取数组、`(y - x + i + 24) & mask` 负数按补码取低位 —— 均已对齐。

### 验证

* `python -X utf8 _ac_cxx_check.py`：**21/21 全部一致**（C++ 生产实现 vs `_ac_eval.py` 参考实现）。
* 两次独立手算校核（人工推导，不依赖任何解释器）：
  * `#12`（`acz`）：`x=6565 y=1010` → **106**；
  * `#17`（`acv`）：`x=9170 y=8140` → **89**（含 `sort(({x,2x,3x,4x}), (: sprintf, "%d" :))[2]`
    与 `this_object()->fm(...)` 嵌套调用）。
  两者与实现输出一致。
* 复现工具：`python -X utf8 _ac_dump.py <i>`（导出第 i 条脚本）、
  `python -X utf8 _ac_min.py`（最小语义用例）、`python -X utf8 _ac_lhs.py`（语句形状普查）。

### 账号口令（仅供真机登录驱动使用，不写库）

`calc_password_new_md5(account, pwd) = save_string(md5(account + MD5hex_upper(pwd) + "20070201"))[1..<2]`
（`linux/aaa/pack_data/lib_aaa32-c/aaa/daemons/accountd_v37.c:791`）。
据此核对测试库：`a/a`、`1/1`、`11/1`、`b/b`、`12/1`、`23/1`。
真机登录：`python -X utf8 _rdp.py auto a a awesrd`。
'''

p = 'HANDOVER.md'
s = io.open(p, encoding='utf-8', errors='replace').read()
if '0x5108' in s and '本轮修复（`src/original_vm.cpp` / `src/game_service.cpp`）' in s:
    print('already appended')
else:
    io.open(p, 'w', encoding='utf-8', newline='\n').write(s + TEXT)
    print('appended %d chars' % len(TEXT))
