# -*- coding: utf-8 -*-
"""Decisive check for the 9031 「数据不一致」 path.

read_content("runtime", account) issues
    select path,name,branch,content,checksum from data
      where path='runtime' and name=<account> and branch=''
and cmd_read_content returns -3 when the stored checksum does not match
create_check_sum(1,path,name,branch,content) ^ -1418120497, or when the row
payload cannot be re-parsed.  -3 becomes the 9031 dialog; a missing row must
answer -1 (no dialog).
"""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    db='dl_ddb_1_test', charset='gbk')
cur = c.cursor()

cur.execute("select count(*) from data where path='runtime'")
print("path='runtime' rows:", cur.fetchone()[0])
cur.execute("select path,name,branch,length(content),time,checksum,left(content,60) "
            "from data where path='runtime'")
for r in cur.fetchall():
    print('   path=%r name=%r branch=%r len=%s time=%s cksum=%s head=%r' % r)

cur.execute("select count(*) from data where path='user'")
print("path='user' rows:", cur.fetchone()[0])
cur.execute("select name,branch,length(content),time,checksum from data where path='user'")
for r in cur.fetchall():
    print('   name=%s branch=%-8s len=%-6s time=%s cksum=%s' % r)

cur.execute("select distinct path from data where path like '%%runt%%'")
print('paths matching *runt*:', [r[0] for r in cur.fetchall()])
c.close()
