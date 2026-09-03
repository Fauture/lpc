# -*- coding: utf-8 -*-
"""Query original GS: full get_dir_ex tasks listing + names, and get_sub_dirs_ex."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

def connect(port=1027, tries=8):
    for i in range(tries):
        try:
            c = GameShellRpcClient(host='192.168.200.128', port=port, password='123456', timeout=15)
            b, w = c.connect()
            return c
        except Exception as e:
            print('try %d fail: %s' % (i, str(e)[:100]))
            time.sleep(5)
    return None

c = connect()
if not c:
    sys.exit(1)

def q(expr, wait=15):
    try:
        r = c.eval(expr, wait=wait)
        return r
    except Exception as e:
        return 'ERROR: %s' % e

print('=== get_dir_ex("/gs/daemons/tasks/") 全部(转字符串) ===')
print(q('implode(get_dir_ex("/gs/daemons/tasks/"), ",")'))
print('=== sizeof(get_sub_dirs_ex("/gs/daemons/tasks/")) ===')
print(q('sizeof(get_sub_dirs_ex("/gs/daemons/tasks/"))'))
print('=== get_sub_dirs_ex("/gs/daemons/tasks/") 前 20 ===')
print(q('implode(get_sub_dirs_ex("/gs/daemons/tasks/")[0..19], ",")'))
try:
    c.close()
except Exception:
    pass
