# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
cur.execute("SELECT name, branch, LEFT(content,90), LENGTH(content) FROM dl_ddb_1.data WHERE path='lgxy' LIMIT 5")
for r in cur.fetchall():
    print('lgxy name=%r branch=%r content=%r len=%s' % (str(r[0])[:35], str(r[1])[:30], str(r[2]), r[3]))
cur.execute("SELECT name, branch, LEFT(content,90) FROM dl_ddb_1.data WHERE path='douchong_dahui' LIMIT 3")
for r in cur.fetchall():
    print('douchong name=%r branch=%r content=%r' % (str(r[0])[:40], str(r[1])[:30], str(r[2])))
cur.execute("SELECT name, branch, LEFT(content,90) FROM dl_ddb_1.data WHERE path='time_info_list' LIMIT 3")
for r in cur.fetchall():
    print('time_info name=%r branch=%r content=%r' % (str(r[0])[:40], str(r[1])[:30], str(r[2])))
c.close()
