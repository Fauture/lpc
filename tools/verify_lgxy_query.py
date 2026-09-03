# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
# exact query aaa runs (latin1, gbk bytes for dist)
dist = 'ÊÔ½£ÄÚ²â'  # 试剑内测 in latin1 (same as GBK bytes)
cur.execute("SELECT dist, section, name, version, LENGTH(value), value FROM dl_adb_all_test.config WHERE dist=%s AND section='global' AND name='lgxy_server' AND version=''", (dist,))
rows = cur.fetchall()
print('lgxy_server rows:', len(rows))
for r in rows:
    print('  dist=%r value=%r' % (r[0], r[5]))
# also check what dist values look like raw
cur.execute("SELECT DISTINCT dist, HEX(dist) FROM dl_adb_all_test.config")
for r in cur.fetchall():
    print('dist=%r hex=%s' % (r[0], r[1]))
c.close()
