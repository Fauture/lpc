# 9101 剩余问题：本轮（第 1 轮）的结论

## 已经确定的事实（都有日志/代码证据）

1. **重叠是真的**（基线 `88ba918`）：同一连接 `cno=22` 的两条
   `cmd_create_new_char` 链（`key=28538`/`28539`），第二条 BEGIN 时第一条仍 `live=1`，
   且第一条停在 `create_new_char<-check_valid_name<-get_gid<-do_operate`。
2. **一次 TCP 读会带多个包**，`feedClient()` 在一次调用内把整块里的包全派发完，
   所以门必须落在 `dispatchCommand`（每包唯一入口），按块加门无效（9101 反而 5→10）。
3. **驱动链结束 ≠ 命令结束**：`cmd_create_new_char` 链 250–340ms 就 END，
   LPC 的 create 继续在别的链上跑（threadd/daemon 的 `do_operate`）。
4. 加了"按包的门 + 续链归属"之后：**OVERLAP=0、门超时=0、`basic_char_info` 写出、
   客户端稳定进游戏**（基线是 0 行、停在建角页）。
5. **9101 的确切时序**（chain9，毫秒级）：

```
1116316  ev=BEGIN  cno=15 key=27163 entry=cmd_create_new_char
1116571  ev=PARK   cno=-1 key=154  obj=/gs/clone/char/user.o   （filed 往返）
1116574  ev=END    key=27162 entry=msg_got_catalog
1116575  ev=BEGIN  cno=15 key=27163   ← 下一个包已被派发
         [lg] process_command create_new_char  （读 lite_query_temp → op=1）
1116577  [send] 0x1fe5 9101 「正在处理中」
1116578  ev=END    cno=15 key=27163   （该命令本身耗 262ms）
```

即：**上一条命令的续跑正在被回复唤醒的同一瞬间，下一条命令已经进了
`_process_command` 去读重入标记**。这是"续跑 → 标记清零"与"下一个包 → 读标记"
之间的竞态窗口。

## 本轮试过并证伪的收尾方案（别再重复）

| 方案 | 结果 |
|---|---|
| `liteTempTruthy("operating")` 读对象 temp 映射 | 实现存在但恒 false：该标记不在 `objVars_` 的映射里（或其键不是 `operating`） |
| 观测 `lite_query_temp("operating")` 的返回值（`lpcGuardBusy`） | 采样点只在 `[login-flow]` 打印处，覆盖不到竞态瞬间 |
| 按 VM 归属续跑链（`noteVmClient`/`vmClientOf`，park 时认领） | **回归**：门把登录包也拦死（真机停在 `login(5)`、`0xF061 body=4`） |
| 150ms settle 窗口（park/resume 时 touch，门等静默期） | **回归**：启动卡死——DBA 的 `threadd.q:5` 等到不再被唤醒、GS 27 分钟没 listen（`dba_comm wait=socket`） |

## 下一步（按可信度排序）

1. **查清 `lite_set_temp` 的真实落点**（最直接）：
   * 起服时设 `LPCVM_TEMPKEY=1`（该探针用 `std::getenv`，不能用 `_probe_on` 运行时开），
     抓 `[tempkey] IN/OUT` 的 `objName`/`key`/返回值，确认 `operating` 存在哪个对象、
     哪个存储；
   * 或给 `liteTempTruthy` 加诊断：把登录 VM 的 `objVars_` 里所有 **映射** 的键名与
     `operating` 命中值打进日志（只在 `LPCVM_CHAINTRACE` 打开时）。
   然后门的 `clientLpcBusy` 指到正确存储——这是唯一能在"LPC 自己认为忙"时精确拦包的判据。
2. **epoch（世代号）方案**：门放行时给连接打 epoch，`filed` 往返/续跑链认领时带 epoch，
   门只等"同一 epoch 内未结束的链"。比 VM 归属精确（不会误伤登录/无关长命链），
   比 settle 窗口安全（不用盲等）。
3. settle 窗口若要重做，必须**限定范围**：只对"创建/登录类命令"生效，
   或只在该连接**已有**在飞命令时生效；本轮的全量版本会饿死排队等待者。

## 铁律

* 任何 gate/settle 改动只要让**启动不到 listening** 或**登录失败**，一律立刻回退
  （本轮 settle 窗口就是这样被否掉的）。
* 每轮改动都要跑：起服 → `_rapid2.py 100 <name> 20` → `_acc.py <dir> <name>`，
  并且检查 `ev=OVERLAP` 计数与 `[chain-gate]` 行。
