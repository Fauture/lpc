# -*- coding: utf-8 -*-
"""Dump the ddb rows for characters (gid-keyed paths) and the user rows."""
import io, sys, pymysql
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8')
cur = c.cursor()
cur.execute("SELECT path, name, branch, LENGTH(content), time FROM data "
            "WHERE path='user' OR name REGEXP '^[0-9A-F]{16}$' ORDER BY time, path, branch")
print('--- rows ---')
for r in cur.fetchall():
    print(r)
print('--- counts ---')
for t in ('basic_char_info', 'gid_info', 'data', 'data1'):
    cur.execute('SELECT COUNT(*) FROM `%s`' % t)
    print(t, cur.fetchone()[0])
cur.execute('SELECT gid, name, time FROM basic_char_info')
for r in cur.fetchall():
    print('char', r)
