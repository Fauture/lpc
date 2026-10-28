# -*- coding: utf-8 -*-
"""Dump the raw `data` row content for a path/name, both as text and hex.

usage: python -X utf8 _dumprow.py <path> <name> [db]
"""
import io
import sys

import pymysql

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

path = sys.argv[1]
name = sys.argv[2]
db = sys.argv[3] if len(sys.argv) > 3 else 'dl_ddb_1_test'

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    database=db, charset='utf8')
cur = c.cursor()
cur.execute("SELECT branch, content FROM data WHERE path=%s AND name=%s", (path, name))
for branch, content in cur.fetchall():
    raw = content if isinstance(content, bytes) else str(content).encode('utf-8', 'replace')
    print('--- branch=%r len=%d' % (branch, len(raw)))
    print('text:', raw.decode('gbk', 'replace'))
    print('hex :', raw.hex())
c.close()
