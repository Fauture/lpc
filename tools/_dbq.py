# -*- coding: utf-8 -*-
"""Read-only query helper for the test DB (SELECT only).

usage: python -X utf8 _dbq.py "<sql select>"
"""
import io
import sys

import pymysql

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

sql = sys.argv[1] if len(sys.argv) > 1 else 'show tables'
if not sql.lstrip().lower().startswith(('select', 'show', 'desc', 'describe')):
    print('refusing non-read-only statement')
    sys.exit(2)

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database='dl_ddb_1_test', charset='utf8')
cur = c.cursor()
cur.execute(sql)
for row in cur.fetchall():
    print(row)
c.close()
