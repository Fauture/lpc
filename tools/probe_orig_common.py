# -*- coding: utf-8 -*-
"""Probe original GS: get_dir_ex / get_dir semantics for /feature/common."""
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

c = GameShellRpcClient(host='192.168.200.128', port=1027, password='123456', timeout=10)
b, w = c.connect()

def q(expr, wait=12):
    try:
        return c.eval(expr, wait=wait)
    except Exception as e:
        return 'ERROR: %s' % str(e)[:150]

print('=== sizeof(get_dir_ex("/feature/common/")) ===')
print(q('sizeof(get_dir_ex("/feature/common/"))'))
print('=== get_dir_ex("/feature/common/")[0..6] ===')
print(q('get_dir_ex("/feature/common/")[0..6]'))
print('=== get_dir(lib_pak_path + "/feature/common/*.o") ===')
print(q('get_dir((lib_pak_path + "/feature/common/*.o"))'))
print('=== get_dir("/feature/common/*.c") ===')
print(q('get_dir("/feature/common/*.c")'))
print('=== get_dir_ex("/feature/") ===')
print(q('get_dir_ex("/feature/")'))
print('=== get_sub_dirs_ex("/feature/") ===')
print(q('get_sub_dirs_ex("/feature/")'))
c.close()
