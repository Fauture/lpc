# -*- coding: utf-8 -*-
import sqlite3, sys, os
for p in ['linux/gs/log/filed_backup.sql', 'linux/ccs/log/filed_backup.sql', 'linux/dba/log/log_dba_daili.sql']:
    print('=== %s ===' % p, flush=True)
    if not os.path.exists(p):
        print('  MISSING', flush=True)
        continue
    db = sqlite3.connect(p)
    cur = db.cursor()
    cur.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = [r[0] for r in cur.fetchall()]
    print('  tables:', tables, flush=True)
    for t in tables[:8]:
        try:
            cur.execute('SELECT COUNT(*) FROM "%s"' % t)
            n = cur.fetchone()[0]
            cur.execute('SELECT * FROM "%s" LIMIT 2' % t)
            rows = cur.fetchall()
            cols = [d[0] for d in cur.description]
            print('  %s: rows=%d cols=%s' % (t, n, cols), flush=True)
            for r in rows:
                print('    ', str(r)[:200], flush=True)
        except Exception as e:
            print('  %s err %s' % (t, e), flush=True)
    db.close()
