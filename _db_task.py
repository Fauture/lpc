# -*- coding: utf-8 -*-
"""Is the character's task data (`task/<name>`, which taskd::query_task reads)
actually present in the saved rows?

taskd_v37.c:1292  query_task(me,name) = me->query("task/"+name,1) || me->query_temp(...)
comm_v37.c:960    task_log = taskd->query_task_log(me,name)  -> "" when task_info
                  is not a mapping, which is what makes the re-login 0x2603 body
                  carry empty desc/log/tip.
"""
import pymysql

c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                    db='dl_ddb_1_test', charset='gbk')
cur = c.cursor()
cur.execute("select path,name,branch,content,length(content) from data where path='user'")
for path, name, branch, content, ln in cur.fetchall():
    b = content.encode('gbk', 'replace') if isinstance(content, str) else content
    keys = [k for k in ('task/', 'task', 'newbie', 'log', 'tip', 'achieve') if
            (k.encode('gbk') in b)]
    print('%-6s name=%-18s len=%-6s contains=%s' % (branch or "''", name, ln, keys))
    # print the first 200 decoded chars of the main row to eyeball the schema
    if not branch:
        try:
            s = b.decode('gbk', 'replace')
        except Exception:
            s = repr(b)
        print('     head: %s' % s[:220].replace('\n', ' '))
cur.execute("select path,name,branch,length(content) from data where path like '%%achieve%%'")
print('--- achieve-ish rows ---')
for r in cur.fetchall():
    print('   ', r)
c.close()
