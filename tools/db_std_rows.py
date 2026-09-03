# -*- coding: utf-8 -*-
import pymysql, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
c = pymysql.connect(host='192.168.200.128', user='root', password='123456', database='dl_ddb_1', charset='latin1')
cur = c.cursor()
for path in ['lgxy', 'douchong_dahui', 'time_info_list', 'zhengdaodian', 'herod', 'master', 'daemons']:
    cur.execute("SELECT name, branch, CHAR_LENGTH(content) FROM data WHERE path=%s LIMIT 3", (path,))
    rows = cur.fetchall()
    print(f'=== {path} ({len(rows)} shown) ===')
    for r in rows:
        name = r[0].decode('latin1', 'replace') if isinstance(r[0], bytes) else r[0]
        branch = r[1].decode('latin1', 'replace') if isinstance(r[1], bytes) else r[1]
        print(f'  name={name!r} branch={branch!r} len={r[2]}')
        cur.execute("SELECT content FROM data WHERE path=%s AND name=%s LIMIT 1", (path, r[0]))
        content = cur.fetchone()[0]
        if isinstance(content, bytes):
            content = content.decode('latin1', 'replace')
        print('    content[:160]:', repr(content[:160]))
c.close()
