# -*- coding: utf-8 -*-
"""Show the char rows and whether the bag (carry) has items after the gift fix."""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8')
cur = c.cursor()
cur.execute("SELECT name,branch,LENGTH(content),time FROM data WHERE path='user' ORDER BY name,branch")
print('--- user rows ---')
for r in cur.fetchall():
    print('   ', r)
cur.execute("SELECT gid,name FROM basic_char_info")
print('--- chars ---')
for r in cur.fetchall():
    print('   ', r)
cur.execute("SELECT name, content FROM data WHERE path='user' AND branch='carry'")
print('--- carry content ---')
for n, ct in cur.fetchall():
    print('   ', n, repr(ct[:400] if ct else ct))
