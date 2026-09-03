# -*- coding: utf-8 -*-
"""Probe original aaa telnet shell: what does get_dir return for tasks dirs."""
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

c = GameShellRpcClient(host='192.168.200.128', port=1024, password='123456', timeout=8)
banner, welcome = c.connect()
print('banner:', banner[:120])
print('welcome:', welcome[:120])

queries = [
    ('get_dir("/gs/daemons/tasks/*.o")', 'tasks *.o'),
    ('get_dir("/gs/daemons/tasks/")', 'tasks /'),
    ('get_dir("/gs/daemons/*.o")', 'daemons *.o'),
    ('get_dir("/gs/zone/*.o")', 'zone *.o'),
]
for expr, label in queries:
    try:
        r = c.eval(expr, wait=6)
        print('=== %s ===' % label)
        print(r[:3000])
    except Exception as e:
        print('=== %s ERROR: %s ===' % (label, e))
c.close()
