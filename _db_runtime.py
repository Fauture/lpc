# -*- coding: utf-8 -*-
"""Dump the DDB rows that matter for the login-time runtime recovery:
path='user' rows, especially branch='runtime' (the record read_runtime_info
loads and restore_value() must be able to parse)."""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    db='dl_ddb_1_test', charset='gbk')
cur = c.cursor()
cur.execute("select path, name, branch, length(content), time, checksum, left(content, 48) "
            "from data where branch='runtime'")
rows = cur.fetchall()
print('runtime rows: %d' % len(rows))
for r in rows:
    print('  path=%r name=%r branch=%r len=%s time=%s cksum=%s head=%r' % r)

cur.execute("select distinct path from data")
print('paths:', [r[0] for r in cur.fetchall()])
cur.execute("select distinct branch from data")
print('branches:', [r[0] for r in cur.fetchall()])

cur.execute("select name, branch, length(content), time from data where path='user' "
            "order by name, branch")
print('--- user rows ---')
for r in cur.fetchall():
    print('   %s  %-10s len=%-6s %s' % r)
c.close()
