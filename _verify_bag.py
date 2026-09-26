# -*- coding: utf-8 -*-
"""验证：当前版本重上后背包（carry 分支）是否有物品。"""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8')
cur = c.cursor()
cur.execute("SELECT name, branch, LENGTH(content), time FROM data WHERE path='user' "
            "ORDER BY name, branch")
rows = cur.fetchall()
print('user 行数: %d' % len(rows))
carries = [r for r in rows if r[1] == 'carry']
for n, b, ln, tm in rows:
    flag = '  <== 背包' if b == 'carry' else ''
    print('  %-18s %-8s %-6s %s%s' % (n, b, ln, tm, flag))
print('')
for n, b, ln, tm in carries:
    cur.execute("SELECT content FROM data WHERE path='user' AND name=%s AND branch='carry'", (n,))
    ct = cur.fetchone()[0]
    s = ct if isinstance(ct, str) else ct.decode('utf-8', 'replace')
    head = s[:260].replace('\n', ' ')
    print('carry(%s) %d 字节: %s' % (n, ln, head))
    print('  -> %s' % ('✅ 有物品（正常）' if ln > 20 else '❌ 空表（17 字节=丢物品）'))
