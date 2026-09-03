# -*- coding: utf-8 -*-
"""Query original gs telnet: lib_pak_path + pak-prefixed get_dir."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

def connect(port=1024, tries=8):
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

print('=== expand_macro("__BIN_DIR__") ===')
print(q('expand_macro("__BIN_DIR__")'))
print('=== lib_pak_path ===')
print(q('lib_pak_path'))
print('=== get_dir(lib_pak_path + "/gs/daemons/tasks/*.o") ===')
print(q('get_dir((lib_pak_path + "/gs/daemons/tasks/*.o"))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/daemons/tasks/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/daemons/tasks/*.o")))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/zone/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/zone/*.o")))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/daemons/*.o")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/daemons/*.o")))'))
print('=== sizeof(get_dir(lib_pak_path + "/gs/daemons/tasks/")) ===')
print(q('sizeof(get_dir((lib_pak_path + "/gs/daemons/tasks/")))'))
try:
    c.close()
except Exception:
    pass
