# -*- coding: utf-8 -*-
"""Check what content dba actually stored for ad_shop in dl_ddb_1_test."""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', connect_timeout=5, charset='latin1')
cur = c.cursor()
cur.execute("SELECT path,name,branch,content,HEX(content),LENGTH(content) FROM data WHERE path=%s", ('ad_shop',))
rows = cur.fetchall()
print('rows:', len(rows))
for r in rows:
    path, name, branch, content, hexc, ln = r
    print('path=%r name=%r branch=%r len=%d hex=%s' % (path, name, branch, ln, hexc))
    print('  content=%r' % content)
cur.execute("SELECT path,name,branch,content,HEX(content),LENGTH(content) FROM data LIMIT 5")
for r in cur.fetchall():
    print('sample:', r[0], r[1], r[2], 'len=%d hex=%s' % (r[5], r[4]))
c.close()
