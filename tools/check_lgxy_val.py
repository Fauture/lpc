# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
for db in ['adb', 'dl_adb_all', 'dl_adb_all_test']:
    print('=== %s ===' % db)
    cur.execute("SELECT dist, section, name, value FROM %s.config WHERE name IN ('lgxy_server','douchong_dahui','CITY_WAR_SERVER') OR section='global' AND name IN ('lgxy_server','douchong_dahui')" % db)
    for r in cur.fetchall():
        print('  dist=%r section=%r name=%r value=%r' % (r[0], r[1], r[2], str(r[3])[:60]))
c.close()
