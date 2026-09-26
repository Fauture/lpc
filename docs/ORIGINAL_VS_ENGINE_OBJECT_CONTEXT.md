# 原版 vs 本引擎：函数调用上下文与对象变量（底层差异清单）

> 结论先行：**原版把"函数局部变量"放在栈上（每帧一个 `bp`），对象变量放在"每个对象唯一的一份数组"里，
> 由每帧的 `thisObject + consititution` 决定槽位；本引擎把两者合并进同一个值数组，并用
> `objVars_`（活动块）/`ownObjVars_`（本 VM 自己的块）两个指针来区分。**
> 双块不是"多余的缓存"，而是在补一个结构性缺口 —— 所以直接删掉/强行同步都会把启动搞死
> （实测：删掉后 DBA 起不来、四个服务全退；强行同步后同样起不来）。
>
> 正确的重构方向不是"让两块永不失步"，而是**按原版把局部变量搬到帧上**，届时双块自然消失。

IDA 证据（全部来自本仓库 `lpcvm.exe` 对应 IDA 数据库）：

| 函数 | 地址 | 作用 |
|---|---|---|
| `vm_callInOtherFun` | `0x8094cd8` | 跨对象调用（`ob->fn()` / `"path"->fn()`） |
| `vm_callInUserFunBody` | `0x8093eb0` | 所有函数调用的公共入口：建帧、定 `thisObject`、选 constitution |
| `vm_getObjVar` | `0x80bc5c0` | 对象变量按名字取址：`&pObject->pObjectValues[valueNo + baseVarOffset]` |
| `vm_f_object_template` | `0x80f4b9c` | `object_template()`：返回**指针**（`pTemplateObject`），模板被析构才返回 0 |
| `vm_findObjectByString` | `0x80f4150` | `find_object()`：**只接受字符串**，其它类型直接 `vm_error("Bad type of value, expected string, got %s")` |
| `vm_compareValue` | `0x80bbcb4` | 相等比较：对象是**指针比较** |
| `vm_cloneObject` | `0x80b1e4c` | 克隆 |

---

## 1. 原版的调用模型（逐条来自 IDA 反编译）

### 1.1 `vm_callInOtherFun(callFlags, argn)` —— 跨对象调用

```c
v3 = &vm_stack[vm_sp];               // 栈顶 = 第一个参数（接收者）
if (v3->type == 4) {                 // 字符串路径：按名字解析
    info = vm_lookupObjectByName(str);
    if (!info || info->flags == 2048) vm_error("Object is not found.\n");
} else {
    if (v3->type != 10) vm_error("Object is not found.\n");
    info = v3->v.info;               // ★ 对象值 → 直接用【指针】，不做名字解析
    if (*((char*)info + 13) < 0) vm_error("Object is not found (destructed).\n");
}
if (v5->type != 4) vm_error("Function is not found.\n");
// ★ 在【目标对象自己的程序】里按名字找函数（含它的 constitution 链）
if (vm_findProgramFunction(*((Vm_Program_T**)info + 8), ...)) {
    vm_popNStack(2);
    vm_callInUserFunBody(0, info /*目标对象*/, ..., funOffset);   // ★ 用目标对象建帧
    return 1;
}
if (vm_findProgramFunctionByCStr(..., "default_interface")) { ... }   // default_interface 回退
vm_popNStack(2);
if (!(callFlags & 4)) vm_error("Function is not found.\n");
vm_popNStack(argn); vm_pushVoid(); return 0;                          // 找不到 → push void
```

要点：
1. **对象参数是身份（指针）**；只有字符串才 `vm_lookupObjectByName`。
2. 函数查找发生在**目标对象的程序**上，因此随后变量槽位也是目标对象的。
3. 找不到函数时：**弹掉参数并 `pushVoid()`**（返回"空"，不报错，除非标志位要求）。

### 1.2 `vm_callInUserFunBody(isGlobal, targetObject, funIdx, fun, argn)` —— 建帧与上下文

```c
CurrentPCB = vm_getCurrentPCB();
vm_funCp ++;                             // 函数上下文栈 +1
vm_updateCurrentFunCallContext();
v9 = vm_currentContext;
v9->bp   = vm_sp;                        // ★ 帧基址 = 当前栈指针：局部变量就在【栈】上
v9->flags = 4; v9->argNum = argn; v9->f.pEfun = fun;

if (isGlobal) {                          // 全局函数
    vm_currentContext->thisObject = 0;   // ★ thisObject = NULL
    v9->flags |= 0x10;
} else {
    if (targetObject) vm_currentContext->thisObject = targetObject;          // ★ 目标对象
    else              vm_currentContext->thisObject = vm_currentContext[-1].thisObject; // ★ 继承调用者
    thisObject = v10->thisObject;
    if (thisObject) {
        // ★ constitution 从【目标对象自己的程序】按函数的 consititutionNo 取
        v10->consititution = &thisObject->pProgram->pConsititutions[
                                thisObject->pProgram->pFunctions[funIdx].consititutionNo];
        thisObject->execRef ++;
    }
}
v10->ip = 0; v10->cs = (Vm_Code_Segment_T*)(fun + 4);
```

要点：
4. **局部变量的家是栈**（`bp` 相对），与对象变量数组无关。
5. **对象变量只有一个来源：`vm_currentContext->thisObject` 指向的那一份 `pObjectValues`**（配合 `consititution->baseVarOffset`）。
6. 同一帧里 `thisObject` 可以是被调对象（`ob->fn()`）、也可以是调用者的（隐式 `this`）、全局函数则是 NULL。

### 1.3 `vm_getObjVar(object, name, progName, &no)` —— 对象变量的唯一寻址方式

```c
for (i = 0; i < pProgram->maxConsititutionNo; i ++) {         // ★ 遍历全部 constitution
    for (pProgVars = pConsititutions[i].pProgram->pProgVars; pProgVars; pProgVars = pProgVars->pNext) {
        if (匹配名字) {
            v8 = pProgVars->valueNo + pConsititutions[i].baseVarOffset;   // ★ 绝对槽位
            return &pObject->pObjectValues[v8];                           // ★ 每个对象【一份】数组
        }
    }
}
```

要点：7. **一个对象一份值数组**；祖先程序的变量靠 `baseVarOffset` 落进同一份数组。

### 1.4 `vm_allocateObject` —— 数组大小 = 程序的全部变量（IDA 0x80B1D08）

```c
if ( a1->maxVariableNo <= 0 ) Memory->pObjectValues = 0;
else { Values = vm_allocateValues(...); Memory->pObjectValues = Values; }   // 恰好 maxVariableNo 槽
```

⇒ 引擎的 `obj_.globalVarCount` 就是原版的 `maxVariableNo`，构造时按它分配数组是对的
（`ctor-ref/ctor-shared/ctor-clone` 三处都是 `obj_.globalVarCount`）。
**任何 `slot >= 数组长度` 的写都说明"当前活动数组不是这个对象的"** —— 现在这两种情况都会
打印 `[objw-oob]`（`set_obj_var` 与 `writeThrough` 的对象引用两条路径）。

### 1.5 变量名 → 槽位：`valueNo`，不是 progVars 下标（2026-09-18 实测）

`Vm_Prog_Var_T`（IDA）：`pVarName 0x0, pProgName 0x4, type 0x8, attrib 0x9, valueNo 0xa, pNext 0xc`。
`/gs/clone/npc/basic.o` 的真实槽位表（`python -X utf8 _ovars.py <o文件>` 可复现）：

| 槽 | 变量 | 声明程序 |
|---|---|---|
| 0 | `_id` | `feature/common/id.o` |
| 1,2 | dbase 的两个变量 | `feature/common/dbase.o` |
| **3** | **`my_import_name`** | **`inherit/misc/basic_object.o`** |
| 4..16 | `my_dir/my_x/my_y/current_map_index/...` | `feature/common/move.o` |
| 17,18 | `all_interface_list/all_shadow_list` | `feature/common/shadow.o` |
| ... | ... | ... |
| 30 | `bind_task_list` | `feature/npc/task.o` |

（`basic_object.o` 自己：gvc=4，`my_import_name` 的 **valueNo=3**、progVars 下标=0 ——
旧代码返回下标，所以连"自己的变量"都可能取错槽；祖先变量更是直接返回 -1。）

---

## 1.9 更正：局部变量**不在**对象值数组里（2026-09-18 实测，推翻 D1 的前提）

原版：`vm_stack[bp + a1]`。
  - a1 >= 0 是**参数**（调用方压入的槽，a1=0 就是 bp）；
  - a1 < 0 是**本帧自己保留的局部槽**（函数入口的 `enter_function`/`to_reserve_stack_value`
    往帧里压的槽在 a1=-1,-2,…）。
  - `vm_pushLocalVptr(a1)`（0x809134C）IDA 原文：
    `if (v2->argNum <= a1) vm_error("No such argument."); v3 = v2->bp + a1;`
    —— 所以“局部变量”是帧相对的栈槽，**与对象值数组完全无关**。

本引擎：**同一个模型**。
  - `slotOf(a1) = frame.bp - a1`（栈向上增长，故为镜像）；参数 a1>=0 落在 bp、bp-1…；
  - `case 27 enter_function` 里 `for (i < f.localCount) pushValue(Value{})` 就是原版
    “帧内保留局部槽”的等价物，压出的槽正是 a1=-1,-2,… 的地址。

实测证据（本轮）：
  1. `[fi]` 探针：aaa/dba/ccs 启动 27516 个帧中 `passedArgs < localCount` 有 5524 个 —— 这是
     **正常**的（多出来的槽由 `enter_function` 在帧内压出），不是“局部变量丢了”。
  2. 指令级对照（`LPCVM_INSN_FN=_config_daemon` / `update_today_zero_time`，同一二进制
     仅差 Step 1 补丁）：`enter_function` 压了 1 个槽（sp 1→2），编译器随后用
     `push_local_vptr a=-1` 写它、`push_local_value a=-1` 读它。把这次压栈删掉后，
     `bp+1` 变成 efun 返回值的槽 → 紧跟的 `pop_stack` 把“变量”本身弹掉 →
     `sizeof(config)==0` 永远成立 → `_config_daemon` 死循环、三个服务全部卡在
     “Driver startup”。⇒ 局部变量槽**必须**留在栈上。
  3. 结论：Step 1（把局部变量搬进 `Frame::locals`）前提错误、且破坏启动；真正的缺陷是
     对象变量 / thisObject（D2/D3/D4/D7）。

## 2. 本引擎的模型

- 每个 `OriginalVm` 有两个指针：
  - `objVars_`：**当前活动**的值数组（谁在跑就是谁的）；
  - `ownObjVars_`：**这个 VM 自己对象**的值数组（外部入口必须跑在自己块上时用）。
- 局部变量：帧相对的**栈槽**（见 1.9），`slotOf(a1) = bp - a1`，负下标是帧内保留槽。
- 对象变量：`operand + currentVarOffset()`（= 当前帧所属 constitution 的 `baseVarOffset`）
  索引到 `objVars_`（**当前活动块**）——问题就在“当前活动块”这一层（D2）。
- 内联跨对象调用用 `startInlineFunction(fn, …, block, …)` 指定要用哪一块：
  `original_vm.cpp:5704`、`5814` 传的是 `targetVm->ownObjVarsShared()`。
- 运行期的块切换诊断：`noteVars("inline"/"own"/"pop-f"/"saved")`，即日志里的
  `[vs] inline obj=… gvc=35 from=35@A to=1@B`。

---

## 3. 差异清单（每条都对应一个实测现象）

| # | 原版 | 本引擎 | 实测现象 |
|---|---|---|---|
| D1 | 局部变量在**栈**上（帧 `bp` 相对） | 局部变量在**对象值数组**里 | 不同对象的帧交替执行时，必须切换"谁的数组"，于是有了双块；一旦切错就是"写一块、读另一块" |
| D2 | 对象变量只有**一份数组**，由帧的 `thisObject` 决定 | 有 `objVars_` + `ownObjVars_` 两份指针 | `[vs] inline … to=1@…`：执行 configd(35 槽) 的代码却切到 1 槽块（调用方的块）⇒ `import()` 写 `my_import_name` 落在那块里，对象之后读自己的 35 槽块 ⇒ 永远 0 |
| D3 | 跨对象调用：对象值按**指针**解析，字符串才查名字 | 多处按名字解析（`clone:<key>`、程序名回退） | `[clonesrc] src=/gs/clone/npc/basic.c`（拿到的是类而非实例）；`object_template()` 返回类；`basic_object()` 返回 0 |
| D4 | `find_object()` **只收字符串**，否则 `vm_error` | 收对象时按名字解析（宽容） | 曾经把对象当路径解析，模板解析飘到别的实例 |
| D5 | 找不到函数：弹参数 + `pushVoid()` | 分支较多（detach/服务化/回退） | 返回值语义不一致（同一调用两条路径不同结果） |
| D6 | 对象相等 = 指针比较 | 曾经按名字比较 | "传送后点不开 NPC"（已修：`6a715f8`） |
| D7 | 对象变量按名取址要**遍历全部 constitution** | `objVarSlotByName` 只查 `object().progVars` | 祖先变量（如 `my_import_name` 来自 `basic_object.c`）取不到槽位（实测 `noslot`） |

**由 D1+D2 推出的根因链（船夫没菜单）：**
`clone/npc/basic.c::create` → `basic_object()`（= `global_basic_object[my_import_name]`）→
`my_import_name` 在 `import()` 里被写进**调用方的块**⇒ 对象自己读不到 ⇒ 返回 0 ⇒
`if (!is_object(basic_ob)) return;` 直接返回 ⇒ `call_init_script` 永不执行 ⇒
`/gs/init/**` 的 `set_dynamic_menu` 不执行 ⇒ 方丈岛/东海渔村船夫**没有菜单**。

---

## 4. 重构方案（按原版构造，分步可验证）

**目标形态**：每个对象一份值数组（对象变量）；函数局部变量随帧分配；跨对象调用的 `thisObject`
按 `vm_callInUserFunBody` 的规则设置（目标对象 / 继承调用者 / NULL=全局）。

**步骤（每步都必须能通过"不带 gs 启动 aaa/dba/ccs 两分钟 + 数据库有写入"这一关，
再上 gs 验证船夫菜单）：**

0. ~~给帧加局部变量区~~ **【已作废，见 1.9】**：局部变量本来就在帧相对的栈槽上，
   与对象值数组不共用；把它搬进 `Frame::locals` 会删掉 `enter_function` 的保留槽，
   导致 `_config_daemon` 死循环、三个服务卡在 “Driver startup”（实测）。
1. **每帧记录它要用的对象值数组（= 该帧 `thisObject` 的那一份）**，op61/62/78 与
   `deref`/`writeThrough` 的对象引用一律经帧解析，而不是读可被内联调用改写的
   `objVars_` 全局指针。这是 D2 的正解，也是"写一块读另一块"的直接修复。
2. **`objVarSlotByName` 按原版遍历全部 constitution + `baseVarOffset`**（D7），
   供 efun（`set_obj_var`/`get_obj_var`/`save_object`）与诊断使用。
   （IDA `vm_getObjVar` 0x80BC5C0：`maxConsititutionNo` 循环 + `valueNo + baseVarOffset`。）
3. **`find_object` / `object_template` / `clone_object` 的对象参数一律按身份处理**（D3/D4），
   不存在的名字解析路径删掉。
4. 只有在 1–3 稳定后，才考虑动 `ownObjVars_`（删双块）——它是第 1 步的收尾，不是前置。

**风险与护栏**：
- 第 1 步的风险点是"帧的 thisObject 到底是谁"（内联跨对象调用时 `switchObjVars/autoVars`
  的启发式）。判据用原版规则：目标对象（`funOffset` 非 0）→ 目标对象；否则继承调用者；
  全局函数 → NULL。对应 `vm_callInUserFunBody` 0x8093EB0。
- 任何一步若让 aaa/dba/ccs 起不来或数据库无写入，立即回退该步
  （`git checkout -- src/` + `python -X utf8 _build.py`）。

---

## 5. 复现与验证入口（本仓库现成）

- 启动：`python -X utf8 _run_keep2.py <dir>`（四个服务 aaa/dba/ccs/gs；gs 就绪约 300 s，8160）。
- 运行期探针：`_probe_on\LPCVM_*` 空文件即开（`LPCVM_INITDBG` 覆盖 create/init 链）。
- 关键日志行：`[vs] inline|own|pop-f`（块切换）、`[exactkey]`（派发返回值）、
  `[initscript]`（`call_init_script` 实参：`init_script=` / `<absent>`）、`[isguard]`（两道 `is_type(10)` 守卫）。
- 原版对照：`tools/orig_probe.py` + `tools/gm_probe.c`（上传到原版 gs 的 `/gm_probe.c` 再 `call_other`），
  原版 gs 控制台在 `192.168.200.128:1025`（aaa 是 1024），密码 `123456`。
  实测样本：`/gs/clone/npc/basic.c#10446`，`tpl=/gs/clone/npc/basic.c#2548`，
  `basic=#2548`（**实例**，不是类），`imp/key` 均为 NPC 名。


---

## 6. 船夫点击实测（2026-09-18，运行时探针）

探针：`_probe_on\LPCVM_MENUDBG`、`LPCVM_MENUCLOSURE`、`LPCVM_TEMPKEY`、`LPCVM_FUNPTR`
（`dynProbeFlag` 1 秒轮询热加载，不用重启）+ `LPCVM_INITDBG`。

**方丈岛（id 14752）**
```
[menudbg] get_object_by_id id=14752 -> clone:gs/clone/npc/basic.o#4751
[menudbg] open_menu: is_in_combat / is_deny / is_addicted / is_near / is_deny 全部通过
[menudbg] -> user::start_talk_to            (菜单确实打开了)
[initguard] receiver=#4751 import_name=[船夫10] basic_object=#2647
            dbase{init_script=npc/fangzhangdao-chuanfu sub_type=raw key_name=船夫10}
[tplid] clone=#4751  template=  tplName=clone:gs/clone/npc/basic.o#2647
[initsite] #4751 create -> #2647::get_import_name / #2647::query_entire_dbase
[menuclo] call_fun_ptr funcName="create_menu_content" owner=/gs/clone/npc/basic.o
          func=create_menu_content bound=1 obj=/gs/clone/char/user.o
```

**东海渔村（id 15113）**：同样全部守卫通过、`start_talk_to` 执行、`create_menu_content` 被调用
（`#5109`）。**服务端两次都开了菜单** ⇒ "点不开/错误对话"发生在**菜单内容**这一层：
`dynamic_menu.c::create_menu_content` 调 `(*menu_func[i][0])(&menu_content, me, ob)`，
被调者（`init_myself_menu_content` 等）**按引用往 `&menu_content` 里追加**条目；
引用一旦在闭包调用边界被压平，追加就丢了 ⇒ 菜单为空（客户端什么都不显示）
或只剩闭包自带的默认文本（显示成"错误的对话"）。

引擎现有实现（`case 5 call_fun_ptr`）：闭包调用时把 ref 参数转成 `CrossVmRef`，
但条件只覆盖 `objIndex >= 0 || slotIndex >= 0`；`writeThrough`/`set_local_var` 再写穿。
下一次点击用 `LPCVM_FUNPTR`（已热加载）抓这一层的 `REF:` 标记与闭包链，判断
`&menu_content` 到底有没有以引用形式到达被调者、以及写穿是否落到调用者的槽。

**已知同源缺陷**：克隆身份 —— `#4751` 的模板是另一个实例 `#2647`（clone-of-a-clone），
`get_import_name`/`query_entire_dbase` 都读 `#2647`，`tmp_dbase`（`menu_content`/
`menu_func`/`current_menu` 都在里面）因此是兄弟实例**共用**的（D3）。


### 6.1 决定性证据：**房间里的船夫实例从未执行自己的 `call_init_script`**

同一次会话里两类实例的对比（`[initscript]` 探针 = `create` 走到 `call_init_script`）：

| 实例 | 身份 | `call_init_script` | 菜单 |
|---|---|---|---|
| `#2528` | 船夫的地基/basic object（启动时创建） | ✅ `init_script=npc/donghai-yucun-chuanfu key_name=船夫1` | 注册了 |
| `#2647` | 方丈岛 basic object | ✅ `init_script=npc/fangzhangdao-chuanfu key_name=船夫10` | 注册了 |
| **`#5109`** | **房间里被点到的东海船夫（`#2528` 的克隆）** | **❌ 一条都没有** | **空 → 客户端收不到（"点不开"）** |
| **`#4751`** | **房间里被点到的方丈岛船夫（`#2647` 的克隆）** | **❌ 一条都没有** | 只有全局任务钩子 → "错误对话" |

并且：
```
[initguard] receiver=#5109 import_name=[船夫1] basic_object=t7:#2528
            dbase{init_script=npc/donghai-yucun-chuanfu sub_type=raw key_name=船夫1}
[exactkey]  #5109 -> basic_object ret=t7:#2528 alive=yes
[tplid]     clone=#5109 template= tplName=clone:gs/clone/npc/basic.o#2528
[initsite]  #5109 create -> #2528::get_import_name
            #5109 create -> #5109::basic_object
            #5109 create -> #5109::query_entire_dbase / #2528::query_entire_dbase
[tail]      named.o::register_npc_ob args=[#5109 | 船夫1]
```
即：`basic_object()` 返回的是**兄弟实例**（`#2528`）而不是"basic object"，
`create` 走到 `query_entire_dbase()` 之后**没有** `call_init_script` 就返回了 ——
`basic_object()` 通过 ✅、dbase 也拿到了 ✅，所以提前返回点在这两句之间（原版
`clone/npc/basic.c::create` 的守卫链，见 `original_vm.cpp:12300-12313` 的注释重建）。

`vm_f_object_template`（IDA 0x80F4B9C）已确认：返回的是 **object**（`vm_pushObject`）
或 `0`，不是字符串 ⇒ `find_object(object_template(...))` 这一跳在原版是合法的，
不能再按"find_object 只收字符串"处理（文档 D4 需据此修正）。

**下一步（唯一未闭合点）**：在 `clone/npc/basic.c::create` 的每一个 `return` 点打点，
看 `#5109` 这类"实例的克隆"到底在哪一句返回（预计是 `find_object(#2528)` 这一跳：
引擎把 object 实参当路径解析/返回 0 ⇒ `!is_object(basic_ob)` ⇒ 直接 return）。
修掉这一点，房间里的船夫就会跑自己的 `call_init_script`，菜单随之出现。
