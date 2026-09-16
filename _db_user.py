# -*- coding: utf-8 -*-
"""Show the user rows of the DDB with their timestamps."""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8')
cur = c.cursor()
cur.execute("SELECT path,name,branch,LENGTH(content),time FROM data WHERE path='user' ORDER BY branch")
print('--- user rows ---')
for r in cur.fetchall():
    print('   ', r)
cur.execute("SELECT gid,name,time FROM basic_char_info")
print('--- basic_char_info ---')
for r in cur.fetchall():
    print('   ', r)
cur.execute("SELECT name,LENGTH(content),time FROM data WHERE path='runtime'")
print('--- runtime ---')
for r in cur.fetchall():
    print('   ', r)
