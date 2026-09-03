# -*- coding: utf-8 -*-
import pymysql, sys
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', database='dl_ddb_1_test', charset='latin1')
cur = c.cursor()
cur.execute('SELECT COUNT(*) FROM data')
print('dl_ddb_1_test.data rows:', cur.fetchone()[0])
cur.execute("SELECT path, COUNT(*) FROM data GROUP BY path ORDER BY COUNT(*) DESC LIMIT 30")
for r in cur.fetchall():
    print(r)
c.close()
