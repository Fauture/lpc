# -*- coding: utf-8 -*-
"""Dump the DDB (dl_ddb_1_test) paths/rows we care about."""
import sys
import pymysql

db = sys.argv[1] if len(sys.argv) > 1 else 'dl_ddb_1_test'
c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    charset='utf8', db=db)
cur = c.cursor()
cur.execute('select path, count(*), max(length(content)) from data group by path order by 2 desc')
print('--- path counts (table data) ---')
for r in cur.fetchall():
    print('  %-16s rows=%-6d maxlen=%s' % (r[0], r[1], r[2]))
for p in ('runtime', 'user', 'account', 'gid'):
    cur.execute('select path,name,branch,length(content) from data where path=%s order by name,branch', (p,))
    rows = cur.fetchall()
    print('--- %s: %d rows ---' % (p, len(rows)))
    for r in rows[:40]:
        print('   name=%-24s branch=%-12s len=%s' % (r[1], r[2], r[3]))
