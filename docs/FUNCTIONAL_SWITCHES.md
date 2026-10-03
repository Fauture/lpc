# 运行时开关清单：功能类 vs 日志类

扫描范围：`src/*.cpp`、`src/*.h`、`src/*.inc` 里所有 `dynProbeFlag("…")` / `LPCVM_GETENV("…")` /
`getenv("…")`，共 **128 个名字**。下面只列**会改变行为（语义/分支/返回值）**的；
其余全部是日志/探针/性能采样类（只打印，不影响逻辑），末尾给名单。

约定：`dynProbeFlag` 的开关可以运行时热加载（`_probe_on\<名字>` 空文件，1 秒生效，不用重启）；
`LPCVM_GETENV` 的开关只能在**启动时**用环境变量给（`EXTRA_LPCVM=名字=1`）。

---

## 一、功能开关（会改变行为）

| 开关 | 位置 | 默认 | 作用 | 备注 |
|---|---|---|---|---|
| `LPCVM_OBJVAR_VALUENO` | `original_vm.cpp:268`（`objVarValueNoRule()`） | **关** = 旧语义 | 开 = 对象变量**按名取址遍历全部 constitution 并用 `valueNo`**（原版 `vm_getObjVar` 规则）；关 = 旧的 `progVars` 下标语义 | 实测开启后商店叠加失效（买 100 给 2 个 `amount=1`），故默认关 |
| `LPCVM_FO_LEGACY` | `builtins.cpp:3701` | **关** = IDA 恒等 | 开 = `find_object(object)` 退回"字符串化后按名字查表"；关 = **对象实参恒等返回**（船夫修复） | 关掉它船夫菜单就没了（`[initscript]` 616→2812）,已修复 |
| `LPCVM_CHARRANGE` | `original_vm.cpp:15806` | **关** = 按字节 | 开 = 字符串范围切片按 **GBK 字符**数（旧行为）；关 = 按**字节**（原版 `vm_queryRange` 语义） | 影响 `name[i..j-1]` 这类取子串 |
| `LPCVM_MAXFRAMES` | `original_vm.cpp:7907` | **400** | LPC 调用深度上限；超过就触发深度守卫（转储 + 异常），防止爆栈 | 值本身就是语义（设小可提前暴露递归） |
| `LPCVM_NO_HB` | `original_vm.cpp:19249` | **未设 = 心跳开** | 设了 = `postHeartBeatAll()` 直接返回 0（关掉心跳回调） | `LPCVM_HB` 只是心跳的**打印**开关 |
| `LPCVM_NO_DLTFIX` | `original_vm.cpp:4249` | **未设 = 修复生效** | 设了 = 恢复"零字节 dlt 占位条目遮蔽真实实现"的旧 bug（王老板买 100 只到 99 就是它） | A/B 用 |
| `LPCVM_NO_RATESTR` | `builtins.cpp:6472` | **未设 = 新行为** | 设了 = 恢复"数组里的字符串元素按 rate 0 计"的旧行为 | 影响抽取/掉落概率计算 |
| `LPCVM_NO_CONST` | `original_vm.cpp:7228` | **未设 = 用常量表** | 设了 = `curTbl = nullptr`，查名字时不走该帧的字符串常量表 | 会影响"常量名能不能解析" |
| `LPCVM_NAMEBYPASS` | `original_vm.cpp:12293` | **未设 = 不绕过** | 设了 = 内联 `check_name_length_legal` 时整数直接返回 0（绕过名字长度校验） | 起名相关 |
| `LPCVM_NO_HOMEFIX` | `original_vm.cpp:1476` | **未设 = 用索引** | 设了 = 恢复未做记忆化的后缀线性扫描 | **性能类 A/B**，不改语义 |
| `LPCVM_NO_OBJIDX` | `original_vm.cpp:1467` | **未设 = 用索引** | 设了 = 恢复未记忆化的后缀扫描 | **性能类 A/B**，不改语义 |

> 说明：把 `LPCVM_NO_HOMEFIX` / `LPCVM_NO_OBJIDX` 也列进来，是因为它们会切换**代码路径**（只是结果应当一致）。

---

## 二、日志 / 探针 / 性能采样类（只打印，不影响逻辑）

其余 117 个名字都属于这一类，按用途分组：

- **调用/指令追踪**：`LPCVM_TRACE*`（TRACE、TRACE_C8、TRACE_CASE、TRACE_CREATE、TRACE_EXPLODE、TRACE_FN、TRACE_MEMBER、TRACE_OBJ、TRACE_OPERMAP）、`LPCVM_INSN`/`LPCVM_INSN_FN`、`LPCVM_CALLTRACE`/`LPCVM_CALLFILTER`、`LPCVM_CALLMISS`、`LPCVM_FNTRACE`/`LPCVM_FNARGS`/`LPCVM_FNOBJ`、`LPCVM_CFUDBG`、`LPCVM_CFV`、`LPCVM_CREATETRACE`/`LPCVM_CREATELIST`/`LPCVM_CREATEFILTER`、`LPCVM_ENTRYDBG`、`LPCVM_FRAMEINFO`、`LPCVM_STACKDBG`、`LPCVM_DEPTH`、`LPCVM_MAPDBG`、`LPCVM_OPFN`/`LPCVM_OPOBJ`/`LPCVM_OPMAX`、`LPCVM_RETVAL`、`LPCVM_RST`、`LPCVM_CVREF`、`LPCVM_FUNPTR`、`LPCVM_MENUCLOSURE`/`LPCVM_MENUDBG`/`LPCVM_MENUDECIDE`/`LPCVM_OPENMENU`、`LPCVM_GA`、`LPCVM_INITDBG`、`LPCVM_C8INIT`、`LPCVM_NEWBIE`、`LPCVM_AUTOVARS`、`LPCVM_ARGDBG`、`LPCVM_KEYT`、`LPCVM_LIFE`、`LPCVM_NAMECHAIN`/`LPCVM_NAMERULE`/`LPCVM_NAMERULE2`、`LPCVM_TALK`、`LPCVM_NEAR`、`LPCVM_ROOM`、`LPCVM_MOVE`/`LPCVM_MOVEPKT`、`LPCVM_MSG*`（MSGIO 等）
- **temp/数据库/事件**：`LPCVM_TEMPKEY`、`LPCVM_TEMPVAL`、`LPCVM_TEMPIO2`、`LPCVM_EVDBG`、`LPCVM_EVENT_TRACE`、`LPCVM_QUEUE_TRACE`、`LPCVM_IDDBG`、`LPCVM_IIDDBG`、`LPCVM_IIDQ`、`LPCVM_LOADDBG`、`LPCVM_SQL_TRACE`、`LPCVM_SVC_TRACE`、`LPCVM_ECHO`
- **网络包**：`LPCVM_RECV`/`LPCVM_RECV2`/`LPCVM_RECV_MAX`、`LPCVM_SEND`/`LPCVM_SEND_MAX`、`LPCVM_DBGLINK`、`LPCVM_AC`、`LPCVM_PWD_DIAG`
- **性能/统计**：`LPCVM_CPUPROF`/`LPCVM_CPUPROF_MS`、`LPCVM_EFUN_PROF`、`LPCVM_OP_PROF`、`LPCVM_STEP_PROF`、`LPCVM_LOADPROF`/`LPCVM_LOAD_PROF`、`LPCVM_CLONE_PROF`、`LPCVM_REG_PROF`、`LPCVM_SLOW_OP`/`LPCVM_SLOW_OP_US`/`LPCVM_SLOW_OP_NOLIMIT`、`LPCVM_MEMSTAT`/`LPCVM_MEM2`、`LPCVM_CENSUS`、`LPCVM_SAMPLE`、`LPCVM_IDLE_DUMP`、`LPCVM_LONGPARK`、`LPCVM_SCHED`、`LPCVM_TIMER`、`LPCVM_LOCK_TRACE`、`LPCVM_STUCK_TRACE`、`LPCVM_HB`
- **自检/杂项**：`LPCVM_FMT_CHECK`、`LPCVM_CHAINTRACE`、`LPCVM_FLOW`、`LPCVM_CHAT`/`LPCVM_CHATVAR`、`LPCVM_TAIL`、`LPCVM_BAN`、`LPCVM_MALL`/`LPCVM_MALL2`、`LPCVM_IMPW`、`LPCVM_ISFN`、`LPCVM_LIFE`、`LPCVM_STACKPROBE`、`LPCVM_SERVE_TASKD`、`LPCVM_RANGE2`、`LPCVM_NO_*`（见上，除已列出的功能性项）

> 复现方法：`python -X utf8 _switchnames.py`（列名字）、`python -X utf8 _switches3.py`（按代码块粗分类）。
> 粗分类会把"块内声明计数器"误判成功能类，所以最终以上面的人工判定为准。


