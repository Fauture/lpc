import pymysql

def get_paths(db):
    conn = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                           database=db, charset='utf8')
    cur = conn.cursor()
    cur.execute("select path, count(*) from data group by path order by count(*) desc")
    rows = cur.fetchall()
    conn.close()
    return dict(rows)

std = get_paths('dl_ddb_1')       # 标准参考库
tst = get_paths('dl_ddb_1_test')  # 我们启动写入的测试库

print(f"标准库 dl_ddb_1     总行数: {sum(std.values())}")
print(f"测试库 dl_ddb_1_test 总行数: {sum(tst.values())}")
print()
print("=== 标准库有、测试库完全没有的 path（缺失写入）===")
missing = {k: v for k, v in std.items() if k not in tst}
for k, v in sorted(missing.items(), key=lambda x: -x[1]):
    print(f"  {k}: 标准={v}  测试=0")

print()
print("=== 两边都有的 path（数量对比）===")
for k in sorted(set(std) & set(tst)):
    print(f"  {k}: 标准={std[k]}  测试={tst[k]}")
