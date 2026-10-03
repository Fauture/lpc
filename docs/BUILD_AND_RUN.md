# 编译与启动（lpcvm 四服务）

面向本机 Windows 开发：**怎么编译** `lpcvm.exe`、**怎么起 aaa/dba/ccs/gs 四个服务**、
怎么判断起好了、怎么查数据、以及这套脚本的坑。

---

## 0. 环境要求

| 项 | 值 |
|---|---|
| 编译工具 | Visual Studio 2022 Community 的 MSBuild：`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe` |
| 工程 | `build/lpcvm.sln`，配置 **Release / Win32**（Release 带 `/Zi /DEBUG /OPT:REF /OPT:ICF`，出 PDB 供崩溃转储与采样） |
| 产物 | `build\Release\lpcvm.exe` |
| Python | 3.11（脚本统一用 `python -X utf8` 跑，避免中文/编码问题），需要 `pymysql` |
| MySQL | `192.168.200.128:3306`，`root/123456`；测试库 `dl_ddb_1_test`，参照库 `dl_ddb_1`（只读对照） |
| 服务端口 | 绑定在 **192.168.200.1**：aaa `1024`、dba `8120`、gs `8160`（单实例守卫还看 `8001`、`8110`） |
| 数据包 | `linux/aaa/pack_data/lib_aaa32.pak`、`linux/dba/...lib_dba32.pak`、`linux/ccs/...`、`linux/gs/pack_data/lib_gs32.pak` |

> 注意：**不要扫描 `192.168.200.128` 的端口**（那是原始服务端所在机器，只按需 MySQL / telnet 使用）。

---

## 1. 编译

```powershell
cd C:\Users\Administrator\Desktop\lpc
python -X utf8 _build.py
```

`_build.py` 做四件事，避免"看起来成功其实没编"：

1. **杀掉正在跑的 lpcvm**（否则 exe 被占用，链接报 `LNK1104`）；
2. `touch` 关键源码（`src/original_vm.cpp`、`builtins.cpp`、`value.cpp/.h`、`pak_loader.cpp`、`main.cpp`），
   防止 MSBuild 因为时间戳判断"无需重编"而跳过；
3. **删掉旧的 `build\Release\lpcvm.exe`**；
4. 调 MSBuild（`build/lpcvm.sln -p:Configuration=Release -p:Platform=Win32 -m -v:m -nologo`），
   最后必须看到**新 exe 存在**才算成功：

```
BUILD OK: C:\Users\Administrator\Desktop\lpc\build\Release\lpcvm.exe  3427328 bytes  00:15:11
```

失败时打印 `BUILD ERRORS:`（前 10 条 `: error` / `LNK`）并 `!! BUILD FAILED`，退出码 1。

手动等价命令：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build/lpcvm.sln -p:Configuration=Release -p:Platform=Win32 -m -v:m -nologo
```

---

## 2. 启动四个服务

```powershell
cd C:\Users\Administrator\Desktop\lpc
$env:EXTRA_LPCVM='LPCVM_QUIET=1'
python -X utf8 _run_keep2.py gs_run          # gs_run = 日志目录名，任意取
```

`_run_keep2.py <LOG_DIR>` 的流程：

1. **单实例守卫**：先 `Stop-Process lpcvm`，再检查 `8001/8110/8120/8160`；只要有一个还被占，
   就打印 `!! ports still held by another process` 并**拒绝启动**（避免两套服务共用一个库）；
2. **清库**：调 `_dbclear.py` 清空 `dl_ddb_1_test`（要保留数据就设 `KEEP_DB=1`）；
   > 按本项目约定：**服务一律带清库启动**，除非明确要保留现场。
3. `rmtree` 日志目录并重建，然后按 `aaa → dba → ccs → gs` 顺序启动（每个间隔 3 秒）：
   ```
   build/Release/lpcvm.exe run linux/<name>/pack_data/lib_<name>32.pak
   ```
   日志分别写到 `<LOG_DIR>/live_<name>.txt`；
4. 每 15 秒轮询一次，打印 `[  345s] 8160=False/True`；有服务退出会打印
   `service exited: [(名称, 退出码)]`；**守护进程不自动退出**（服务会一直跑）。

环境变量注入：`EXTRA_LPCVM` 用逗号分隔，`KEY=VALUE` 设值、裸 `KEY` 视为 `=1`：

```powershell
$env:EXTRA_LPCVM='LPCVM_QUIET=1,LPCVM_TEMPKEY=1,LPCVM_INSN_FN=get_user_carry_data'
```

- `LPCVM_QUIET=1`：不设 `LPCVM_TAIL`/`LPCVM_MEM2`，启动日志小很多（**平时建议都开**）。
- 其它开关见 `docs/FUNCTIONAL_SWITCHES.md`（功能类 11 个 vs 日志类 117 个）。

---

## 3. 判断"起好了"

```powershell
Test-NetConnection -ComputerName 192.168.200.1 -Port 8160 -InformationLevel Quiet   # 期望 True
Get-Process lpcvm | Measure-Object | % { $_.Count }                                 # 期望 4
Get-Content gs_run\live_gs.txt -Tail 5
```

**启动耗时按版本差异很大，必须等 8160 开口，别中途杀掉**：

| 版本线 | 8160 打开大致时间 |
|---|---|
| `ef4da68`（安静启动优化） | ~310 s |
| `a3f7a00` / 船夫线 `9aca824` | ~400–500 s |
| `22a15ad` 线（未含安静启动优化） | ~640–720 s |

`live_gs.txt` 里出现 `Driver startup` 表示 gs 已进入驱动初始化；加载完成会看到全局命令加载等日志。

停止服务：

```powershell
Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 4. 数据检查（起好之后）

```powershell
python -X utf8 _verify_bag.py     # 角色存档 carry 分支：206+ 字节且有物品 = 正常；17 字节 = 丢背包
python -X utf8 _dbclear.py        # 手动清 dl_ddb_1_test（KEEP_DB=1 跳过）
```

常用 SQL（测试库 `dl_ddb_1_test`，表 `data(path,name,branch,content,time,checksum)`）：

```sql
SELECT name, branch, LENGTH(content), time
FROM dl_ddb_1_test.data WHERE path='user' ORDER BY name, branch;

SELECT content FROM dl_ddb_1_test.data
WHERE path='user' AND branch='carry' ORDER BY time DESC LIMIT 1;   -- 背包
```

---

## 5. 这套脚本的坑（都踩过）

1. **被跟踪的脚手架会被 git 回退删掉**：`_run_keep2.py` / `_dbclear.py` 只在较新提交里被跟踪，
   `git reset --hard <旧提交>` 会删除它们（服务因此起不来）。对策：仓库里另存**未被跟踪**的副本
   （`_run4.py`、`_run_keep2_untracked.py`），回退不受影响；`_dbclear.py` 丢了就按
   "清空 `dl_ddb_1_test` 全部表、`KEEP_DB=1` 跳过"的语义重建。
2. **运行中的服务会占用 exe**：不先停服务就编译 → `LNK1104`，而且旧 exe 还在，容易被误当成"新编译的"。
   `_build.py` 已内置杀进程，手工编译时务必自己先停。
3. **编译完必须重启服务**：`_build.py` 只产出 exe，不会自动拉起服务。
4. **启动耗时差异大**：不要用固定的 60 秒判断失败；以 8160 是否 True 为准（见上表）。
5. **不要在服务运行中改 `_probe_on\` 以外的环境变量**：`LPCVM_GETENV` 类开关只在进程启动时读一次；
   `dynProbeFlag` 类（`_probe_on\<名字>` 空文件）可以运行中热加载，1 秒生效、不用重启。

---

## 6. 一页速查

```powershell
# 1) 编译
python -X utf8 _build.py

# 2) 起服务（带清库；日志目录 gs_run）
$env:EXTRA_LPCVM='LPCVM_QUIET=1'
python -X utf8 _run_keep2.py gs_run

# 3) 等 8160 为 True（约 5–12 分钟，看版本线）
Test-NetConnection -ComputerName 192.168.200.1 -Port 8160 -InformationLevel Quiet

# 4) 停服务
Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force
```
