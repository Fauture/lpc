# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', connect_timeout=5)
cur = c.cursor()
cur.execute('SHOW DATABASES')
dbs = [r[0] for r in cur.fetchall()]
print('databases:', dbs)
for db in dbs:
    try:
        cur.execute('SHOW TABLES FROM %s' % db)
        tabs = [r[0] for r in cur.fetchall()]
        if 'config' in tabs:
            print('=== %s.config ===' % db)
            cur.execute('SELECT COUNT(*) FROM %s.config' % db)
            print('rows:', cur.fetchone()[0])
            cur.execute("SELECT dist, section, name, version, LENGTH(value) FROM %s.config WHERE name='lgxy_server' OR name LIKE '%%lgxy%%' OR name='douchong_dahui' LIMIT 10" % db)
            for r in cur.fetchall():
                print('  ', r)
            # sample all names in global section
            cur.execute("SELECT DISTINCT name FROM %s.config WHERE section='global' LIMIT 40" % db)
            print('global section keys:')
            for r in cur.fetchall():
                print('   ', r[0])
    except Exception as e:
        pass
c.close()
