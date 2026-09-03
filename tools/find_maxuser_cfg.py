# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
# where does aaa get my_setting for gs? check config table for max_user / my_setting-ish keys
for db in ['dl_adb_all_test', 'dl_adb_all', 'adb']:
    try:
        cur.execute("SELECT name, value, LENGTH(value) FROM %s.config WHERE name LIKE '%%max_user%%' OR name LIKE '%%my_setting%%' OR value LIKE '%%1200%%' LIMIT 10" % db)
        rows = cur.fetchall()
        if rows:
            print('=== %s ===' % db)
            for r in rows:
                print('  name=%r value=%r len=%s' % (r[0], str(r[1])[:60], r[2]))
    except Exception as e:
        print(db, 'err', e)
# search all config values containing 1200
cur.execute("SELECT name, value FROM dl_adb_all_test.config WHERE value LIKE '%1200%' LIMIT 10")
print('=== dl_adb_all_test values with 1200 ===')
for r in cur.fetchall():
    print('  name=%r value=%r' % (r[0], str(r[1])[:80]))
c.close()
