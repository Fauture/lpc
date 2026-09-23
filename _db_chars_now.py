# -*- coding: utf-8 -*-
"""Which characters exist in the test ddb right now (and which account owns them)?

Used to answer "the server said 9113 (name exists) but the account's character
list came back empty" - i.e. was the character written and linked?
"""
import io
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
import pymysql  # noqa: E402

HOST = '192.168.200.128'
USER = 'root'
PWD = '123456'
DB = sys.argv[1] if len(sys.argv) > 1 else 'dl_ddb_1_test'

conn = pymysql.connect(host=HOST, user=USER, password=PWD, database=DB, charset='utf8')
cur = conn.cursor()
cur.execute('SHOW TABLES')
tabs = [r[0] for r in cur.fetchall()]
print('tables:', len(tabs))
for t in tabs:
    cur.execute('SELECT COUNT(*) FROM `%s`' % t)
    n = cur.fetchone()[0]
    if n:
        print('  %-40s rows=%d' % (t, n))
# character-ish tables: dump a few identifying columns
for t in tabs:
    low = t.lower()
    if not any(k in low for k in ('char', 'role', 'user', 'player')):
        continue
    cur.execute('SHOW COLUMNS FROM `%s`' % t)
    cols = [c[0] for c in cur.fetchall()]
    pick = [c for c in cols if c.lower() in
            ('id', 'gid', 'name', 'cname', 'account', 'acct', 'acc', 'loginname',
             'accountname', 'userid', 'uid', 'create_time', 'createtime')]
    if not pick:
        pick = cols[:5]
    try:
        cur.execute('SELECT %s FROM `%s` LIMIT 12' % (','.join('`%s`' % c for c in pick), t))
        rows = cur.fetchall()
        print('-- %s (%s)' % (t, ','.join(pick)))
        for r in rows:
            print('     ', r)
    except Exception as e:
        print('-- %s query failed: %s' % (t, e))
cur.close()
conn.close()
