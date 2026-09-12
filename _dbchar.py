import pymysql, sys

conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                       password='123456', charset='utf8',
                       database='dl_ddb_1_test')
cur = conn.cursor()
tabs = ['basic_char_info', 'data', 'data1', 'gid_info', 'online_update']
for t in tabs:
    try:
        cur.execute("SHOW COLUMNS FROM `%s`" % t)
        cols = [r[0] for r in cur.fetchall()]
        cur.execute("SELECT COUNT(*) FROM `%s`" % t)
        n = cur.fetchone()[0]
        print('%-20s rows=%-6d cols=%s' % (t, n, cols))
    except Exception as e:
        print(t, 'ERR', e)

name = sys.argv[1] if len(sys.argv) > 1 else 'awesrd'
for t in tabs:
    try:
        cur.execute("SHOW COLUMNS FROM `%s`" % t)
        cols = [r[0] for r in cur.fetchall()]
        for c in cols:
            try:
                cur.execute("SELECT COUNT(*) FROM `%s` WHERE `%s` LIKE %%s" % (t, c),
                            ('%' + name + '%',))
                n = cur.fetchone()[0]
                if n:
                    print('HIT %s.%s -> %d rows' % (t, c, n))
            except Exception:
                pass
    except Exception:
        pass
cur.execute("SHOW COLUMNS FROM `basic_char_info`")
print([r[0] for r in cur.fetchall()])
cur.execute("SELECT * FROM `basic_char_info` LIMIT 5")
for r in cur.fetchall():
    print(str(r)[:400])
conn.close()
