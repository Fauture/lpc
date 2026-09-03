import pymysql
import sys
sys.stdout.reconfigure(encoding='utf-8')

conn = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                       database='dl_ddb_1', charset='utf8')
cur = conn.cursor()
for path in ['lgxy', 'douchong_dahui', 'time_info_list', 'zhengdaodian', 'herod', 'master']:
    cur.execute('select name, branch, left(content,120) from data where path=%s limit 2', (path,))
    print(f'--- {path} ---')
    for r in cur.fetchall():
        print('  ', r)
conn.close()
