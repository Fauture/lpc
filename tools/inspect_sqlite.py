# -*- coding: utf-8 -*-
"""Inspect the GS sqlite databases: filed_backup.sql row counts + tables."""
import sqlite3
import sys

path = sys.argv[1] if len(sys.argv) > 1 else r'linux/gs/log/filed_backup.sql'
try:
    c = sqlite3.connect(path)
    tables = c.execute("select name from sqlite_master where type='table'").fetchall()
    print('file:', path)
    print('tables:', tables)
    for (t,) in tables:
        try:
            n = c.execute('select count(*) from "%s"' % t).fetchone()[0]
            print('  table %s rows=%d' % (t, n))
        except Exception as e:
            print('  table %s error %s' % (t, e))
    c.close()
except Exception as e:
    print('ERR:', e)
