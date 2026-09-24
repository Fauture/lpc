# 真机「创建角色 → 进游戏 → 触发剧本」延迟：根因与修复（2026-09-24）

## 0. 结论（一句话）

**不是某个 efun 没实现，也不是 DB 慢，而是 `tryCallOtherObject()` 找不到目标时退化成
「拿 `hasSuffix` 扫全注册表」——一次登录期间每秒 ~1000 万次 `std::string` 比较。
它把 `userd::start_game` 里 `refresh_magic_pennant_skills()` 的一次调用拖到 853 ms，
而这一秒正好卡在「客户端已进房间」和「剧本/传送到揽仙镇外」之间。**

同类问题还有两处（都已在本次修复）：
`findFunctionAny()` 每次按名字查找都要遍历该对象**整张函数表两遍**（第二遍不提前退出），
`originalHomeVmOf()` 未命中缓存时要**线性扫全部 ~23k 个 VM** 并对每个 VM 再走一遍它的函数表。

## 1. 真机测量（RDP + 真实客户端，非模拟脚本）

```
python -X utf8 _run_probe.py selfprof 7200 "LPCVM_SEND=1;LPCVM_SEND_MAX=200000;LPCVM_CPUPROF_MS=1000"
python -X utf8 _rdp.py auto 1 1 sp1 sp1
```

### 1.1 最终交付（默认配置，真机 `_rdp.py auto 1 1 hf1 hf1` 新建角色进游戏）

| 指标 | 修复前 | 修复后（默认） |
|---|---|---|
| **进房间(`0x1155`) → 剧本(`0x20D1`)** | **2871 ms** | **2213 ms（-23%）** |
| 进房间 → 传送到揽仙镇外 | 2680 ms | 2018 ms |
| `build_magic_pennant_skills`（`0x7007`→`0x21C1`） | **853 ms** | **363 ms（-57%）** |
| `callOther` self（1s 窗口） | 664 ms | **30 ms** |
| `callOther.suffixScan` 迭代数（1s） | 10,258,680 | 270,024 → 之后 ~1（记忆化生效后） |
| `findFunctionAny` self（1s） | 205 ms | 114 ms（哈希预筛） |
| gs 工作集 | ~1080 MB | 943 MB（无内存回归） |

截图 `hf1_2_ingame.jpg`：`[一线] [hf1]`，坐标 102,157（揽仙镇外），新手任务在跟踪。

### 1.2 实测过的几种实现（选型依据）

| `findFunctionAny` 实现 | 1s 窗口 self | gs 内存 | 结论 |
|---|---|---|---|
| 原线性扫描 | 205 ms（43 µs/次） | 1080 MB | 基线 |
| 每 VM 一个 `unordered_map<string,fn*>` 名字索引 | 22 ms（6 µs/次） | **1500 MB** | 快但 +400MB，**弃用** |
| 函数上存 64 位名字哈希 + 整数预筛（**采用**） | 114 ms（25 µs/次） | 943 MB | 省一半，无内存代价 |

（每条查找平均仍要遍历 ~1160 个函数，5.3M 次循环/秒——要再快就得有索引，
但索引的内存代价见上；下一步可考虑"只对 `obj_.functions` 建共享的 per-OriginalObject 索引"
或给 `findFunctionAny` 加一个小 LRU。）

### 定位过程（关键的一次测量）

用 1 秒窗口的 per-site profiler 抓到的、包含那 853 ms 的那一秒：

```
[cpuprof] insns=127042
  op8:            incl 10508ms / SELF 519ms over 4024 calls
  callOther:      incl  9944ms / SELF 664ms over 3988 calls   ← 全部在这里
  execOutside:    incl  9775ms / SELF  65ms
  callByName:     incl  9285ms / SELF  15ms
  findFunctionAny incl   205ms / SELF 205ms over 4776 calls
  efun:           incl   149ms / SELF 144ms over 3665 calls
  homeVm:         incl    67ms / SELF  67ms over  478 calls
  callOther.suffixScan: extra=10,258,680      ← 10.2M 次 hasSuffix
  findFunctionAny.scan: extra= 5,514,735      ← 5.5M 次函数表比较
```

`callOther` 的 **self 664 ms ÷ 10.26M 次 ≈ 65 ns/次**，正好就是 `hasSuffix` 一次
`std::string` 比较的代价 —— 10.2M 次字符串比较 ≈ 615 ms，占那 853 ms 的三分之二。

## 2. 根因代码

### 2.1 `tryCallOtherObject()` 的后缀兜底（主因）

`src/original_vm.cpp`（`tryCallOtherObject`，精确候选键全部未命中后）：

```cpp
    {
        std::shared_ptr<OriginalVm> hit;
        {
            std::lock_guard<std::recursive_mutex> rl(registryMutex());
            for (auto& kv : reg.vms) {              // ← 全注册表 ≈ 23k 条
                if (hasSuffix(kv.first, rel)) {     // ← 每条一次 std::string 比较
                    hit = kv.second; break;
                }
            }
        }
```

为什么走到这里：前面的精确键只有两个候选
`searchRoot + "/" + rel` 与 `"pak:" + rel`（`reg.vms` 实际注册形如 `pak:gs/...`），
登录期间大量 `call_other` 两个都没命中 → 每个调用都要扫注册表。

### 2.2 `findFunctionAny()` 每次名字查找扫两遍函数表

```cpp
    const OriginalFunction* findFunctionAny(const std::string& name) const {
        for (const auto& fn : obj_.functions)                    // 第一遍：全表
            if (fn.name == name && fn.index >= 0) return &fn;
        const OriginalFunction* impl = nullptr;
        for (const auto& kv : funcsByIndex_)                     // 第二遍：全表，且不提前退出
            if (kv.second->name == name && kv.second->index >= 0 && !kv.second->code.empty())
                impl = kv.second;                                // 保留最后一个
        return impl;
    }
```
每个 `callByName`（`call_other` 按名字解析）都走一次。

### 2.3 `originalHomeVmOf()` 未命中缓存 = 扫全 VM + 每 VM 再扫函数表

```cpp
        for (auto& kv : objectRegistry().vms) {                  // ≈23k 个 VM
            if (!kv.second || !kv.second->ownsFunction(fn)) continue;   // ownsFunction 内部又是线性
            ... 数 ≤33 个全局取"最已初始化" ...
        }
```
`ownsFunction()` 本身是对该 VM 整个 `obj_.functions` 的线性遍历。
实测一次未命中 ≈ **6.5 ms**，登录期间 10~46 次未命中。

## 3. 修复（已实现，真机验证）

`src/original_vm.cpp`：

1. **后缀兜底按 `rel` 记忆化（默认开启，本次的主要收益）**
   —— 见 §3.1：扫描本身**一字不改**（同样遍历 `reg.vms`、同样 `hasSuffix`、同样取第一个命中），
   只把结果缓存起来；每次注册/注销 `regGeneration()` 自增，代次不符即整体失效，
   所以永远不会用到过期结果。`LPCVM_NO_OBJIDX` 可切回未记忆化的扫描。
2. `OriginalVm::ownsFunctionFast()` —— `OriginalObject::functions` 是连续 `vector`，
   把"这个函数是不是我的"从线性遍历改成指针区间判断（+ 原有 map 兜底）。
3. `originalHomeVmOf()` 未命中后的扫描改用上面的 O(1) 判定。
4. `findFunctionAny()` 的**哈希预筛** —— 加载器给每个函数算一次 64 位 FNV-1a
   (`OriginalFunction::nameHash`)，两条扫描在 `std::string` 比较前先用整数比较排除；
   **扫描顺序、first-match / last-match 规则、字符串比较作为最终判据全部不变**。
   （先用的是"每 VM 一个名字索引"，实测 +400MB 内存，已弃用——见 §1.2。）

2/3/4 由 `LPCVM_NO_HOMEFIX` 关闭（默认开）；1 由 `LPCVM_NO_OBJIDX` 关闭。

### 3.1 一次走错的路（务必别再走）

第一版把兜底换成"用现成的有序键索引 `GsRegIndex::objBySuffix` + `vmsByKey`"，
**开机默认打开后启动卡死在 `还没有从 CONFIG_D 取得服务器所在区组名`**。
原因：`rel = gs/daemons/configd.o` 这类后缀会同时命中
`/gs/daemons/configd.o` 与 `pak:gs/daemons/configd.o`（**活实例 vs proto 实例**），
而 `vmsByKey` / `objBySuffix` 的键是**归一化键**，两者归一化后同键 → 索引里只剩一个，
于是选中了错的实例。原扫描按 `reg.vms` 的迭代顺序取第一个命中，
**顺序本身就是语义的一部分**，不能换成"归一化键最小"。
（这与 HANDOVER §5.S 的 "proto vs 活实例" 是同一类坑。）

## 4. 新增探针（默认关闭，运行时可按需打开）

| 开关 | 作用 |
|---|---|
| `LPCVM_CPUPROF` | per-site **inclusive + self** CPU 剖析；每 `LPCVM_CPUPROF_MS`（默认 5000）毫秒打印一次，含该窗口执行的 LPC 指令数 `insns=` |
| `LPCVM_SLOW_OP` | **改成 dyn 探针**（可用 `_probe_on\LPCVM_SLOW_OP` 在跑着的服务上临时打开）；`LPCVM_SLOW_OP_US` 设阈值（µs）、`LPCVM_SLOW_OP_NOLIMIT` 去掉 100ms 限流 |

> 注意：`LPCVM_SLOW_OP` 会在每条指令上取一次时钟，**开启后整体慢 4 倍以上**
> （实测 `build_magic_pennant_skills` 887 ms → 3888 ms，且产生 78 万行日志），
> 只适合看"哪些指令慢"，不能用来量时间。

## 5. 仍待处理（按实测 self 时间排序，1 秒窗口）

| self | 站点 | 说明 / 建议 |
|---|---|---|
| 148 ms | `efun`（3513 次，42 µs/次） | `call_builtin` 里仍是 **355 个顺序 `if (name == "...")`** 字符串比较（`original_vm.cpp` 的哈希跳表只覆盖 31 个 efun）。建议给 `call_builtin` 做一张 `name → 处理函数` 的表（或把哈希判据扩到全部 efun） |
| 114 ms | `findFunctionAny`（4572 次） | 哈希预筛后仍要遍历 ~1160 个函数/次（5.3M 次循环/秒）。可考虑给 `obj_.functions` 建**共享的 per-OriginalObject 索引**，或加一个小 LRU |
| 47 ms | `homeVm`（430 次，7 次未命中） | 未命中时要扫全注册表 ~23k 个 VM（实测 ~6.7 ms/次）。可做 `OriginalObject* → VM 列表` 索引（clone 共享同一 object，条目很少） |
| 30 ms | `callOther` self | 已达标 |
| — | 精确键为何未命中 | 兜底已记忆化，但仍是症状：`rel` 常见拼写是 `gs/daemons/xxx.o`，注册表里 daemon 用**带前导斜杠**的 `/gs/daemons/xxx.o`。把 `"/" + rel` 作为**追加**候选键可以基本消除兜底——但要先确认追加候选不会改变 proto/活实例的选择（见 §3.1 的教训） |
| — | 驱动层 | `src/game_service.cpp::waitFlowSettled()` 每包最多等 1 s（`waitClientSendIdle(cno,50,1000)`）；`dispatchCommand()` 的 20 s gate 超时会丢包 |
| — | efun 层 | `save_object` / `restore_object` / `set_obj_var` 在 `src/stub_efuns.inc` 里被当成未实现（`call_builtin` 开头直接 `return make_int(0)`），但 `builtins.cpp:7005/7021/7026` 已有实现 → 死代码。且 `tools/gen_stubset.py` 因哈希分发已失效，重跑会把 17 个**已实现**的 efun 误判为未实现（本次已复现并回滚） |

## 6. 本次改动的文件

| 文件 | 内容 |
|---|---|
| `src/original_vm.cpp` | 后缀兜底记忆化 + `ownsFunctionFast` + `findFunctionAny` 哈希预筛（全部默认开）+ `LPCVM_CPUPROF`/`LPCVM_SLOW_OP` 探针改造 |
| `src/o_loader.h` / `src/o_loader.cpp` | `OriginalFunction::nameHash` + `lpcNameHash()`，加载时一次算好 |
| `_patch_cpuprof.py` … `_patch_cpuprof8.py` | 上述改动的可重放补丁（带锚点校验，保持 CRLF） |
| `_bak_original_vm_*.cpp` | 各阶段源码快照（`precpuprof` / `cpuprof1` / `cpuprof2` / `memofix` / `allfix`） |
| `_t_stubobj.c` | 验证 `save_object`/`restore_object`/`set_obj_var` 被 stub 拦截（`lpcvm run` 可直接跑） |
| `analysis/enter_game_drama_delay.md` | 本文 |

> 仍未提交 git（工作区里本来就有大量与本轮无关的改动）；需要我提交请说一声。
