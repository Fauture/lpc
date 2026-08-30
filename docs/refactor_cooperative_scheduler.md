# lpcvm 单线程协作式调度重构设计

## 目标

将 lpcvm 从"多线程 + 互斥锁"模型重构为忠实还原原版 magic_Linux32x 的
"单线程事件循环 + 协作式进程调度"模型,消除 execMutex_/shareObjVars/
跨线程 queue/accept 忙循环等反复出现的问题。

## 原版调度模型(magic_Linux32x)

### 全局结构
- `vm_mainLoop`(0x80adea8):单线程主循环,持有 vmMutex:
  1. `vm_scheduleProcessInternal()` 调度进程
  2. `vm_scanSocketInMainLoop()` 扫描 socket
  3. `vm_scanTelnet()` 扫描 telnet
  4. 无活动时 `vm_waitEventByTime(relinquishTick)` 休眠
- `vm_scheduleProcessInternal`(0x80ad1d4):核心调度:
  1. `vm_processAllRespondingInQueue()` 处理响应队列
  2. 处理 timer 列表(`vm_pTimerList`),到期执行 `vm_executeAndFreeTimer`
  3. 轮转 active 进程列表(`vm_pActiveProcess` 双向循环链表)
  4. 每个进程执行 `runTicks` 时间片内的 `vm_doActions`
  5. 进程让出(relinquish)或挂起(pend)时切到下一个

### 进程(PCB)模型
- `vm_allPCB[]` 进程池,每个 PCB 492 字节,有独立 stack/funContext/catchContext
- flags:bit0=in use, bit1=active, bit2=suspended, bit5=?, bit7=in timer
- `vm_freePCBList` 空闲链表,`vm_pActiveProcess` 就绪循环链表
- `vm_pTimerList` 定时器链表(按 timeoutTicks 排序)
- `vm_pInvokeList` 立即调用链表

### 挂起/恢复
- `vm_pendCurrentProcess`(0x80a9784):ctrlFlags |= 4 → vm_doActions 返回 -8104
- `vm_pendCurrentProcessOnObject`(0x80a9a64):挂到 pend object 上
- `vm_resumeProcess`(0x80aac10):清 suspend flag,重新插入 active 链表
- `vm_resumeFromObject`(0x80aaab8):从对象等待队列恢复
- `vm_resumePCBFromService`(0x80becd8):服务线程完成回调恢复

### 阻塞 efun 机制
- sleep → 挂到 timer 列表,到期恢复
- 等待队列/事件 → 挂到 pend object,raise/send 时恢复
- 网络 recv/send → 挂到 socket 事件,select 可读写时恢复
- 跨服务器 RPC → 挂起,收到响应后恢复

## 当前 lpcvm 多线程问题清单

1. 每个对象一个 OriginalVm,`execMutex_` 串行化,`execDepth_`/`runDetached` 复杂
2. `create_thread` → 真实 OS 线程 + objVarsSnapshot 拷贝,状态分裂
3. `set_timer` → 每个 timer 一个 OS 线程
4. queue/event → 跨线程 mutex/cv + g_pendingServices
5. 网络 accept/recv → 每连接一个线程
6. MySQL/xdbc → 独立线程 + 回调
7. 大量 `std::atomic`/`sleep_for` 竞态修正(accept 忙循环、serviceQueueIdle 等)

## 重构方案:单线程事件循环 + 协作式进程

### 新架构组件

1. **Scheduler(调度器)**:单线程事件循环,取代 vm_mainLoop
   - 就绪进程队列(ready list)
   - 定时器堆(timer heap,按到期时间)
   - invoke 列表(立即执行)
   - 挂起表(pending by object/queue/event/socket)
   - select/poll 网络事件(必要时 Windows IOCP 或 select 循环)

2. **Process(进程/PCB)**:一次 LPC 执行上下文
   - 保存 ip/stack/frames/catch 上下文(可挂起恢复)
   - 持有 objVars 引用(共享,不再拷贝)
   - 持有 searchRoot/thisObjectName
   - 状态:ready/running/pending/dead

3. **execute 协程化**:execute() 改为可挂起
   - 阻塞 efun(sleep/wait/queue/recv/mysql)挂起当前进程,注册恢复条件
   - 恢复时从挂起点继续(保存/恢复 ip、stack、frames)

4. **create_thread → spawnProcess**:创建协作式进程,共享 objVars

5. **set_timer → timer heap**:注册定时器,到期投递进程恢复

6. **queue/event → 调度器内队列**:send/raise 唤醒挂起进程

7. **网络层**:accept/recv 事件驱动,数据到达唤醒进程

8. **MySQL**:保持阻塞但独立连接池线程,查询完成回调恢复进程

### 关键实验结论(2026-08-30)

### Windows Fiber 方案不可行
实现并验证了 Fiber 方案(每个进程一个 Fiber,阻塞 efun 内
SwitchToFiber 挂起),结论:

1. **调度器核心可行**:单线程事件循环 + Fiber 进程池 + timer/挂起表
   (sched_test 验证:两个进程协作切换 + 定时器恢复正常)。
2. **Fiber + C++ 异常不兼容**:execute() 的主循环用 try/catch 包裹整个
   switch(VM 的 VmError/catch 语义),而阻塞 efun(sleep/wait/queue)在
   try 块内触发 SwitchToFiber。Fiber 切换破坏 SEH(结构化异常处理)链,
   深递归 execute 场景(daemon 加载 → logd sql_select)下确定性崩溃
   (access violation,回溯帧指向 vcruntime 的 __CxxFrameHandler)。
3. 简单 fiber_test(try 内 yield + 后续 throw/catch)通过,但真实
   execute 的深层 try/catch + 跨 VM 调用触发已知的 Fiber+EH 兼容问题。
4. 共享 VM 执行状态(stack_/frames_)在多进程下会被交错破坏:
   execMutex_ 是线程级锁,在单线程 Fiber 协作式下失效(同一线程的
   不同 Fiber 都能"拿到"锁)。

### 正确路径:显式栈机(原版 vm_doActions 的确切模型)
- execute() 改为**非递归**:函数调用 = push Frame(保存返回 ip/code/meta),
  切换 code/meta/ip,继续同一循环;函数返回 = pop Frame 恢复。
- ip/code/meta 提升为 VM 成员(可保存/恢复)。
- 阻塞 efun:保存执行状态到进程,返回调度器;恢复时从保存的 ip 继续。
- 不依赖 Fiber,不依赖 C++ 异常跨栈(VM 的 catch 用显式 catches_ 数组)。
- 每个调度器进程持有独立的执行状态;对象(objVars)共享。


## 阶段 A 完成情况(2026-08-30)

**execute 显式栈机已完成并提交**(1308646),全部服务验证通过:
- aaa 8001(游戏服务)+8101(telnet)、dba 8120+9120、ccs 8110+9110 完整启动
- 跨服务器连接正常:ccs/dba 通过 aaa 获取配置,server.o clone 处理远程消息
- LPC 表达式求值正确,MySQL 查询正常

关键实现:
1. ip/code/meta 提升为 VM 成员,execute() 非递归化(runLoop 单循环)
2. startInlineFunction:函数调用压 Frame(存 retCode/retMeta/retIp)切换流
3. case 72/73 返回三分支:上下文帧(fn==nullptr)不弹直接结束 init 段;
   顶层帧(retCode==nullptr)退出 runLoop;普通帧恢复调用者继续
4. executeFunctionOnStack 保存/恢复外层 code/meta/ip(efun 内嵌调用安全)

踩坑:
- Fiber+C++ EH 不兼容已确认,显式栈机是唯一正确路径
- register.o 的 global init 以 return_from_fake_fun 结束,上下文帧必须特殊处理
- TRACE 路径多处 frames_.back().fn 可能为 nullptr(上下文帧),需判空

**下一步(阶段 B/C)**:调度器核心 + 阻塞 efun 挂起化

## 阶段 B 完成情况(2026-08-30,5 个提交)

**单线程协作式调度器已实现并验证**,aaa 完整运行:
- 8001 游戏服务 + 8101 telnet 监听,daemon 进程挂起/恢复正常
- sleep(timer)/receive_from_queue(queue)/wait_event(event)挂起化
- send_to_queue/raise_event 唤醒挂起进程,从保存的 ip_ 恢复执行
- 无崩溃、无 OS 线程竞争(execMutex_ 仅剩路径未清,阶段 E)

关键实现:
1. **Scheduler 类**(original_vm.cpp 匿名 namespace):ready 队列、
   timers_(wakeAtMs 排序)、queueWaiters_/eventWaiters_(按 key)、
   sockWaiters_(kind-5 fd 等待,idle 时 select 轮询)、parked_ 表
2. **SuspendSignal 机制**:阻塞 efun 抛 SuspendSignal(非 std::exception,
   避开 catch(VmError)/catch(std::exception)),携带 parkedVm+ResumeCond;
   runProcess 捕获后注册唤醒条件,park 进程;唤醒后 resumeProcess 从
   保存的 ip_ 继续同一 runLoop(显式栈机状态全在 VM 成员)
3. **objKeepAlive**:spawn 进程 VM 持有 shared_ptr<const OriginalObject>,
   修复 obj_ 引用悬垂(create_thread 局部 objPtr 析构后调度器访问崩溃)
4. **set_timer 迁移**:detach OS 线程 → scheduler().spawnDelayed(timerEntry
   取消由 delete_timer 经 cancelled 标志处理)
5. **post_init 进程化**:do_init 闭包 spawn 为调度器进程(顺序语义,
   阻塞挂起恢复),替代同步 for 循环(SuspendSignal 逃逸 0xe06d7363)
6. **connect/recv 挂起化**:xdbcConnectAsync(非阻塞,EINPROGRESS→park)、
   xdbcRecvAsync(select 探测,无数据→park)、xdbcPoll(调度器 socket 轮询)
7. **parkedVm**:跨对象 call_other 在目标 registry VM 上阻塞时,进程挂到
   cond 所属 VM 并 resume 该 VM(修复跨 VM 挂起条件丢失)

已知问题(待阶段 C 后处理):
- case 8 跨对象内联禁用:启用后 aaa express_recharged time_desc 递归
  (全局函数 return 后 ip 回 0,快路径),call_other_arg 内联安全可用
- dba:logd init → sql_get_table → sql stub(sql_query/sql_step 占位返回
  假值)→ time_desc 后 0xe06d7363(仅快路径;TRACE 慢执行时不崩)

## 迁移阶段

- 阶段 A:execute 显式栈机(非递归化,call/return 用显式 frame 栈)
- 阶段 B:调度器核心(就绪队列/timer/invoke/挂起表,不依赖 Fiber)
- 阶段 C:阻塞 efun(sleep/wait/queue/recv/mysql)挂起化
- 阶段 D:create_thread/set_timer/closure 改为调度器进程
- 阶段 E:移除 execMutex_/runDetached/objVarsSnapshot 等全部多线程机制
- 阶段 F:网络层事件驱动化
- 阶段 G:验证 aaa/dba/ccs(+gs) 跨服务器链路与 lottery 保存一致

### 兼容性要求

- 重构后 aaa/dba/ccs(+gs) 跨服务器链路行为与修复后的多线程版一致
- lottery_info 5 分钟保存、share_data、ad_shop、读/写 DB 正常
- 保留原版 wire 协议、pkt_protocol、xdbc 语义

