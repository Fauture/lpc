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

### 迁移阶段

- 阶段 A:调度器核心(就绪队列/timer/invoke/挂起表)+ execute 挂起原语
- 阶段 B:create_thread/set_timer/closure 调用改为协作式进程
- 阶段 C:queue/event/sleep/wait 挂起化
- 阶段 D:网络 accept/recv/send 事件驱动
- 阶段 E:MySQL/xdbc 回调恢复
- 阶段 F:移除所有 mutex/atomic,验证 aaa/dba/ccs/gs

### 兼容性要求

- 重构后 aaa/dba/ccs(+gs) 跨服务器链路行为与修复后的多线程版一致
- lottery_info 5 分钟保存、share_data、ad_shop、读/写 DB 正常
- 保留原版 wire 协议、pkt_protocol、xdbc 语义
