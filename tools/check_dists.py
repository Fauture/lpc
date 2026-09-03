# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
# all dists in dl_adb_all_test config
for db in ['dl_adb_all_test', 'dl_adb_all', 'adb']:
    try:
        cur.execute("SELECT DISTINCT dist FROM %s.config" % db)
        print('%s dists: %r' % (db, [r[0] for r in cur.fetchall()]))
    except Exception as e:
        print(db, 'err', e)
# what config does the aaa need for my_dist? check game_server / dist tables
for db in ['dl_adb_all_test', 'dl_adb_all', 'adb']:
    try:
        cur.execute('SHOW TABLES FROM %s' % db)
        print('%s tables: %s' % (db, [r[0] for r in cur.fetchall()][:20]))
    except Exception as e:
        pass
c.close()
