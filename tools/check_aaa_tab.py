# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
cur.execute("DESCRIBE dl_adb_all_test.aaa")
cols = [r[0] for r in cur.fetchall()]
print('aaa cols:', cols)
cur.execute("SELECT * FROM dl_adb_all_test.aaa LIMIT 10")
for r in cur.fetchall():
    print(str(r)[:200])
cur.execute("DESCRIBE dl_adb_all_test.ccs")
cols2 = [r[0] for r in cur.fetchall()]
print('ccs cols:', cols2)
cur.execute("SELECT * FROM dl_adb_all_test.ccs LIMIT 5")
for r in cur.fetchall():
    print(str(r)[:200])
c.close()
