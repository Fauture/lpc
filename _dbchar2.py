import pymysql, sys

conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                       password='123456', charset='utf8',
                       database='dl_ddb_1_test')
cur = conn.cursor()
name = sys.argv[1] if len(sys.argv) > 1 else 'awesrd'
cur.execute("SELECT path,`name`,branch,LENGTH(content),memo FROM `data` WHERE `name`=%s", (name,))
rows = cur.fetchall()
print('rows for', name, len(rows))
for r in rows:
    print(r)
cur.execute("SELECT `name`,path,LENGTH(content) FROM `data` WHERE path LIKE %s ORDER BY LENGTH(content) DESC LIMIT 6",
            ('%awesrd%',))
print('--- by path ---')
for r in cur.fetchall():
    print(r)
conn.close()
