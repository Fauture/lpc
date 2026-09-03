# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
# find tables with max_user or server config
cur.execute("SHOW TABLES FROM dl_adb_all_test")
tabs = [r[0] for r in cur.fetchall()]
print('tables:', tabs)
for t in tabs:
    try:
        cur.execute("SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='dl_adb_all_test' AND TABLE_NAME='%s' AND COLUMN_NAME LIKE '%%max%%'" % t)
        cols = [r[0] for r in cur.fetchall()]
        if cols:
            print(t, 'has cols:', cols)
    except: pass
# check game server / gs tables
for t in tabs:
    if any(k in t for k in ['server', 'gs', 'dist', 'line']):
        try:
            cur.execute("SELECT * FROM dl_adb_all_test.%s LIMIT 3" % t)
            cols2 = [d[0] for d in cur.description]
            print('=== %s cols=%s ===' % (t, cols2))
            for r in cur.fetchall():
                print('  ', str(r)[:200])
        except Exception as e:
            print(t, 'err', str(e)[:80])
c.close()
