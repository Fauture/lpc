import pymysql

conn = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                       database='dl_ddb_1_test', charset='utf8')
cur = conn.cursor()
cur.execute('select count(*) from data')
print('data rows:', cur.fetchone()[0])
cur.execute("select path, count(*) from data group by path order by count(*) desc limit 15")
for r in cur.fetchall():
    print(r)
conn.close()
