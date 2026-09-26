# -*- coding: utf-8 -*-
"""清空测试库 dl_ddb_1_test 的所有表（KEEP_DB=1 时跳过）。
   本文件是旧仓库里被 git 回退删掉的 _dbclear.py 的重建版：_run_keep2.py 会调用它。"""
import os
import sys

if os.environ.get('KEEP_DB'):
    print('[dbclear] KEEP_DB=1 -> 跳过清库')
    sys.exit(0)

try:
    import pymysql
except ImportError:
    print('[dbclear] 缺少 pymysql', file=sys.stderr)
    sys.exit(2)

DB = os.environ.get('DB_NAME', 'dl_ddb_1_test')
conn = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                       charset='utf8', autocommit=True)
cur = conn.cursor()
cur.execute('SHOW TABLES FROM `%s`' % DB)
tables = [r[0] for r in cur.fetchall()]
total = 0
for t in tables:
    cur.execute('SELECT COUNT(*) FROM `%s`.`%s`' % (DB, t))
    n = cur.fetchone()[0]
    cur.execute('DELETE FROM `%s`.`%s`' % (DB, t))
    total += n
    print('[dbclear] %-24s 删除 %d 行' % (t, n))
print('[dbclear] %s 共清 %d 行，%d 张表' % (DB, total, len(tables)))
conn.close()
