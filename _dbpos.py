# -*- coding: utf-8 -*-
import io, re, sys
import pymysql

conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                       password='123456', charset='utf8', database='dl_ddb_1_test')
cur = conn.cursor()
cur.execute("SELECT `name`, LENGTH(content) FROM `data` WHERE path='user'")
rows = cur.fetchall()
print('user rows:', len(rows))
for name, ln in rows:
    cur.execute("SELECT content FROM `data` WHERE path='user' AND `name`=%s", (name,))
    c = cur.fetchone()[0]
    s = bytes(c).decode('latin-1') if isinstance(c, (bytes, bytearray)) else str(c)
    m = re.search(r'"position":\((.*?)\),', s)
    tasks = re.findall(r'([^"]*1-9[^"]*)', s)
    print('%-20s len=%-6d position=%s' % (name, ln, m.group(1) if m else '?'))
    print('    gbk-task-hits:', [t.encode('latin-1').decode('gbk', 'replace')[:40] for t in tasks][:4])
    for key in ('newbie', '"task"', 'lanxianzhenwai'):
        print('    %-16s %s' % (key, key in s))
conn.close()
