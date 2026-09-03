# -*- coding: utf-8 -*-
"""Probe original gs: taskd no_dest_tasks."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

c = GameShellRpcClient(host='192.168.200.128', port=1027, password='123456', timeout=15)
try:
    b, w = c.connect()
    print('connected')
except Exception as e:
    print('connect fail:', str(e)[:120]); sys.exit(1)

def q(expr, wait=12):
    try:
        return c.eval(expr, wait=wait)
    except Exception as e:
        return 'ERR: %s' % str(e)[:80]

print('=== taskd no_dest_tasks["居所活动"] ===')
print(q('"/gs/daemons/taskd.c"->query_daemon_data("no_dest_tasks", "居所活动")'))
print('=== taskd no_dest_tasks size ===')
print(q('sizeof("/gs/daemons/taskd.c"->query_daemon_data("no_dest_tasks"))'))
try:
    c.close()
except Exception:
    pass
