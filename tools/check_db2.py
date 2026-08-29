# -*- coding: utf-8 -*-
"""Dump ALL rows from ddb data tables."""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', connect_timeout=5, charset='latin1')
cur = c.cursor()
for tbl in ('data', 'data1'):
    cur.execute('SELECT COUNT(*) FROM `%s`' % tbl)
    n = cur.fetchone()[0]
    print('table %s rows=%d' % (tbl, n))
    cur.execute('SELECT path,name,branch,HEX(content),LENGTH(content) FROM `%s` LIMIT 60' % tbl)
    for r in cur.fetchall():
        print('  path=%r name=%r branch=%r len=%d hex=%s' % (r[0], r[1], r[2], r[4], r[3]))
c.close()
