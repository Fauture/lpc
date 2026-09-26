# 下线丢背包物品：已修复并验证 ✅

**结论**：把 `lodger_object` 退回 `22a15ad` 的旧行为（**常量 0，不带任何开关**）后，
角色下线再上线**物品不再丢失**，背包存档结构与参照库一致。

- 修复提交：`ed479d3`（分支 `refactor/ldmud-vm-model`，船夫修复之上）
- 验证时间：2026-09-27 00:22（用户操作 + 服务端查库双确认）

---

## 1. 验证证据

用户操作："重上有物品了" ✅

服务端查库（`python -X utf8 _verify_bag.py`）：

```
dl_ddb_1_test, path='user', name=0000000000000015, time=20260927002218
  branch=''        3053 字节
  branch='achieve'  487 字节
  branch='carry'    206 字节   <== 背包有物品 ✅
  branch='patch'    161 字节

carry 内容：
(["carry":([101:"新手礼包（10级）:([255:36,233::6AB7F12C00010000237A:,232:101,226:1,
   "type":8,"gift_info":(["3":"功能卷轴·神行千里","2":"烟花筒","1":"驯兽诀",]),
   "imp_item":0,47:1,])",]),])
```

对照：

| 状态 | `carry` 分支 |
|---|---|
| 修复前（`ef4da68` 及之后所有含该项的版本） | `(["carry":([]),])` = **17 字节** ❌ |
| 修复后（本提交） | **206 字节**，含"新手礼包（10级）"+`233:<iid>`+`gift_info` ✅ |
| 参照库 `dl_ddb_1`（生产存档） | 206–317 字节，结构一致 ✅ |

---

## 2. 根因链（怎么定位到的）

1. `ef4da68`（"gs 安静启动 616s→312s；按 IDA 还原 select/recv/**lodger_object**/objects/child_objects 等 efun"）
   把 `lodger_object` 从**常量 0 桩**换成了"返回容器里的对象"。
2. 二分：**只把这一项**搬到可保存的 `22a15ad` 上（分支 `only1-lodger`，提交 `436660c`，166 行新增）
   就**复现**丢背包 ⇒ 这一项单独即可致丢。
3. 机制：新实现让"我身上有什么"的枚举语义变了，角色背包列表（temp `inventory`）保持为空
   → 存档链 `char/user.o::save → userd::save_user_carry_data → get_user_carry_data →
   carry.o::query_inventory → lite_query_temp("inventory")` 取到空表
   → `carry` 分支写空 → 下线即丢；同时合并/叠加找不到已有堆叠、`gift_info` 一并消失。
4. 两次修法都**没修好**（保留在单独分支，供以后按原版重做时参考）：
   - `fix-lodger-object`（`1d7a377`）：对象实参不再因"字符串键未登记"抛 `VmError`，查询键多键形尝试；
   - `only1-lodger`（`2ed3244`）：`envLodgerKeys` 索引未命中时按 env 键扫描注册表补齐。

---

## 3. 当前代码状态

`src/original_vm.cpp`（哈希分派 `h == 0xAF888C6C3C418282ull`）：

```cpp
if (h == 0xAF888C6C3C418282ull) {
    // lodger_object：保持 22a15ad 的旧行为（常量 0）。
    // ef4da68 把它改成"返回容器里的对象"之后，物品就进不了角色的背包列表
    // （temp "inventory" 为空）-> 存档 carry 分支写空 -> 角色下线丢物品；
    // 只把这一项搬到可保存的 22a15ad 上即可复现，两次修法都没能修好，所以这里退回旧行为。
    return make_int(0);
}
```

即：`lodger_object(x)` 恒为整数 0，所有 `loop (ob in lodger_object(x))` 回到"空转"状态
（这正是存档正常时的行为）。**没有任何开关**。

---

## 4. 遗留事项（未做，记录在案）

1. **`lodger_object` 的真实实现仍未落地**：原版 `vm_f_lodger_object`（IDA `0x80f41c8`）走
   `pObject->pLogderObject` 链表，与对象指针绑定；本引擎用"字符串键 + 索引"模拟时键形不一致
   （`pak:` 原型键 vs `clone:…#N` 实例键）会导致枚举为空。以后要重做，必须先解决"容器身份用实例键统一"。
2. `ef4da68` 的另外 4 项改动仍在船夫线里，未逐项验证是否有副作用：
   `objects`/`child_objects` 由 `({})` 变真集合、`save_object`/`restore_object`/`set_obj_var` 去 stub
   （其中 `restore_object` 的原子性/`clearUndefined` 尚未完工）、`set_obj_var` 4 参参数序改为
   `(obj|name, var, value, prog)`、以及性能/网络那部分。
3. 相关文档分布在不同分支：
   - 本文件（修复验证）—— `refactor/ldmud-vm-model`
   - `docs/LODGER_OBJECT_ORIGINAL_22A15AD.md`（22a15ad 原版实现提取 + 两次失败修法）—— `only1-lodger`
   - `docs/BACKPACK_SAVE_STATUS.md`（判定方法/17 字节判据）—— `save-backpack-doc`
   - `docs/EF4DA68_BACKPACK_ANALYSIS.md`（ef4da68 改动分析）—— bisect 线
   - `docs/FUNCTIONAL_SWITCHES.md`（功能开关清单）—— 本分支

---

## 5. 以后回归时怎么判定（30 秒）

```bash
python -X utf8 _verify_bag.py      # 有物品 -> ✅ 206+ 字节；空表 -> ❌ 17 字节
```
