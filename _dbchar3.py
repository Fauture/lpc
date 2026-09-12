import pymysql, sys

conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                       password='123456', charset='utf8',
                       database='dl_ddb_1_test')
cur = conn.cursor()
name = sys.argv[1] if len(sys.argv) > 1 else 'awesrd'
# 1) content match
cur.execute("SELECT path,`name`,branch,LENGTH(content),memo FROM `data` WHERE content LIKE %s",
            ('%' + name + '%',))
print('--- content hits ---')
for r in cur.fetchall():
    print(r)
# 2) path match
cur.execute("SELECT path,`name`,branch,LENGTH(content),memo FROM `data` WHERE path LIKE %s",
            ('%' + name + '%',))
print('--- path hits ---')
for r in cur.fetchall():
    print(r)
# 3) name match loose
cur.execute("SELECT path,`name`,branch,LENGTH(content) FROM `data` WHERE `name` LIKE %s",
            ('%' + name + '%',))
print('--- name hits ---')
for r in cur.fetchall():
    print(r)
# 4) where do char saves live?
cur.execute("SELECT path,COUNT(*) FROM `data` GROUP BY path ORDER BY COUNT(*) DESC LIMIT 25")
print('--- top paths ---')
for r in cur.fetchall():
    print(r)
conn.close()
