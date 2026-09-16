# -*- coding: utf-8 -*-
"""Original server data vs ours: is the `task` mapping key a STRING or an INT?

The user pointed at dl_ddb_1 (the ORIGINAL dataset) table `data`,
path='user', name='0000000000000001'.  Print the exact bytes around "task" for
both the original DB and our test DB, so the key quoting is unambiguous:
vm_saveValueAscii (0x8107ac4) writes int keys bare and string keys quoted.
"""
import re

import pymysql

for db in ('dl_ddb_1', 'dl_ddb_1_test'):
    try:
        c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                            db=db, charset='gbk')
    except Exception as e:
        print('== %s: connect failed: %s' % (db, e))
        continue
    cur = c.cursor()
    cur.execute("select name, branch, length(content) from data where path='user' "
                "order by name, branch")
    rows = cur.fetchall()
    print('== %s: %d row(s)' % (db, len(rows)))
    for name, branch, ln in rows[:8]:
        cur.execute("select content from data where path='user' and name=%s and branch=%s",
                    (name, branch))
        raw = cur.fetchone()[0]
        b = raw.encode('gbk', 'replace') if isinstance(raw, str) else raw
        s = b.decode('gbk', 'replace')
        idx = [m.start() for m in re.finditer('"task"', s)]
        print('   name=%s branch=%-8s len=%-6s  "task" at %s' % (name, branch or "''", ln, idx))
        for off in idx[:3]:
            print('      %s' % s[off:off + 150])
        # also show every mapping key that looks numeric, for the whole row
        numkeys = re.findall(r'[\(\[](\d{1,6}):', s)
        qkeys = re.findall(r'"(\d{1,6})":', s)
        print('      bare numeric keys: %s   (first 12)' % numkeys[:12])
        print('      quoted numeric keys: %s (first 12)' % qkeys[:12])
    c.close()
