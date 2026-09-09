import pymysql
c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8', binary_prefix=True)
cur = c.cursor()
cur.execute("select name, left(content,24), length(content) from data where path='party' order by name")
for r in cur.fetchall():
    print(r)
cur.close()
c.close()
