# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
name = '试剑内测'.encode('gbk')
sql = "select path, name, branch, content, checksum from data where path='dist_start_time' and name='" + name.decode('latin1') + "'"
print('sql:', sql)
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', charset='latin1', db='dl_ddb_1_test')
cur = c.cursor()
try:
    cur.execute(sql)
    print('rows:', cur.fetchall())
except Exception as e:
    print('ERR:', e)
cur.execute("SHOW TABLES LIKE 'data'")
print('tables like data:', cur.fetchall())
cur.execute("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='dl_ddb_1_test' AND table_name='data'")
print('info_schema count:', cur.fetchone())
c.close()
