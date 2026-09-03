# -*- coding: utf-8 -*-
"""Query original GS telnet (port 1027): get_dir_ex semantics for tasks."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

def connect(port=1027, tries=8):
    for i in range(tries):
        try:
            c = GameShellRpcClient(host='192.168.200.128', port=port, password='123456', timeout=15)
            b, w = c.connect()
            print('connected try %d' % i)
            return c
        except Exception as e:
            print('try %d fail: %s' % (i, str(e)[:100]))
            time.sleep(5)
    return None

c = connect()
if not c:
    sys.exit(1)

def q(expr, wait=10):
    try:
        r = c.eval(expr, wait=wait)
        return r
    except Exception as e:
        return 'ERROR: %s' % e

print('=== lib_pak_path ===')
print(q('lib_pak_path'))
print('=== sizeof(get_dir_ex("/gs/daemons/tasks/")) ===')
print(q('sizeof(get_dir_ex("/gs/daemons/tasks/"))'))
print('=== sizeof(get_dir_ex("/gs/zone/")) ===')
print(q('sizeof(get_dir_ex("/gs/zone/"))'))
print('=== sizeof(get_dir_ex("/gs/daemons/")) ===')
print(q('sizeof(get_dir_ex("/gs/daemons/"))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/daemons/tasks/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/daemons/tasks/*.o")))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/zone/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/zone/*.o")))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/daemons/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/daemons/*.o")))'))
print('=== get_dir_ex("/gs/daemons/tasks/") 前几个 ===')
print(q('get_dir_ex("/gs/daemons/tasks/")[0..5]'))
try:
    c.close()
except Exception:
    pass
