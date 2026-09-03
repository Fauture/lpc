# -*- coding: utf-8 -*-
"""Query original gs telnet: get_dir / get_dir_ex / lib_pak_path semantics."""
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

c = GameShellRpcClient(host='192.168.200.128', port=1024, password='123456', timeout=10)

def q(expr, wait=8):
    try:
        r = c.eval(expr, wait=wait)
        return r
    except Exception as e:
        return 'ERROR: %s' % e

print('=== 1) get_dir("/gs/daemons/tasks/*.o") ===')
print(q('get_dir("/gs/daemons/tasks/*.o")'))
print('=== 2) get_dir("/gs/daemons/tasks/*") ===')
print(q('get_dir("/gs/daemons/tasks/*")'))
print('=== 3) sizeof(get_dir("/gs/daemons/tasks/*.o")) ===')
print(q('sizeof(get_dir("/gs/daemons/tasks/*.o"))'))
print('=== 4) get_dir_ex("/gs/daemons/tasks/") ===')
print(q('get_dir_ex("/gs/daemons/tasks/")'))
print('=== 5) sizeof(get_dir_ex("/gs/daemons/tasks/")) ===')
print(q('sizeof(get_dir_ex("/gs/daemons/tasks/"))'))
print('=== 6) sizeof(get_dir_ex("/gs/zone/")) ===')
print(q('sizeof(get_dir_ex("/gs/zone/"))'))
print('=== 7) sizeof(get_dir_ex("/gs/daemons/")) ===')
print(q('sizeof(get_dir_ex("/gs/daemons/"))'))
c.close()
