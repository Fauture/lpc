# -*- coding: utf-8 -*-
"""Find which port is gs: check lib_pak_path on each port."""
import sys, io, time
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

for port in (1024, 1025, 1026, 1027):
    try:
        c = GameShellRpcClient(host='192.168.200.128', port=port, password='123456', timeout=10)
        b, w = c.connect()
        r = c.eval('lib_pak_path', wait=6)
        print('PORT %d -> lib_pak_path = %r' % (port, r.strip()))
        c.close()
    except Exception as e:
        print('PORT %d fail: %s' % (port, str(e)[:100]))
    time.sleep(1)
