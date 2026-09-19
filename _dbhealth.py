# -*- coding: utf-8 -*-
"""Read-only MySQL health snapshot: timeouts + our services' connections."""
import sys

sys.path.insert(0, '.')
import pymysql  # noqa: E402

c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='utf8')
cur = c.cursor()
cur.execute("show variables like '%timeout%'")
print('--- timeouts ---')
for k, v in cur.fetchall():
    if k in ('wait_timeout', 'interactive_timeout', 'net_read_timeout', 'net_write_timeout'):
        print('  %-22s %s' % (k, v))
cur.execute('select id, user, host, db, command, time, state from information_schema.processlist order by time desc')
print('--- processlist ---')
for row in cur.fetchall():
    print('  id=%-6s user=%-10s host=%-22s db=%-18s cmd=%-8s time=%-6s state=%s'
          % (row[0], row[1], row[2], row[3], row[4], row[5], (row[6] or '')[:40]))
cur.execute('select now(), @@version')
print('server:', cur.fetchone())
c.close()
