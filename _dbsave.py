import pymysql, sys, io

conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                       password='123456', charset='utf8',
                       database='dl_ddb_1_test')
cur = conn.cursor()
name = sys.argv[1] if len(sys.argv) > 1 else '0000000000000015'
cur.execute("SELECT content FROM `data` WHERE path='user' AND `name`=%s", (name,))
row = cur.fetchone()
if not row:
    print('no row')
    sys.exit(1)
c = row[0]
if isinstance(c, (bytes, bytearray)):
    s = bytes(c).decode('latin-1')
else:
    s = str(c)
out = sys.argv[2] if len(sys.argv) > 2 else '_charsave.txt'
io.open(out, 'w', encoding='utf-8', newline='\n').write(s)
print('wrote', out, len(s), 'chars')
print(s[:1500])
conn.close()
