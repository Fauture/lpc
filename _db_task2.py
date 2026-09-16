# -*- coding: utf-8 -*-
"""Dump the `task/...` part of the saved character row."""
import re

import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    db='dl_ddb_1_test', charset='gbk')
cur = c.cursor()
cur.execute("select content from data where path='user' and name='0000000000000046' and branch=''")
row = cur.fetchone()[0]
b = row.encode('gbk', 'replace') if isinstance(row, str) else row
s = b.decode('gbk', 'replace')
print('row chars = %d' % len(s))
print('has "task/" ? %s' % ('task/' in s))
idx = [m.start() for m in re.finditer('task', s)]
print('"task" offsets: %s' % idx[:12])
for i, off in enumerate(idx[:4]):
    a = max(0, off - 40)
    print('--- around %d ---' % off)
    print(s[a:off + 500])
c.close()
