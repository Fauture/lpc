# -*- coding: utf-8 -*-
"""Probe original gs telnet shell (ports 1024-1027): get_dir on tasks dirs."""
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, 'tools')
from gameshell_rpc import GameShellRpcClient

# find which port accepts password prompt
for port in (1024, 1025, 1026, 1027):
    try:
        c = GameShellRpcClient(host='192.168.200.128', port=port, password='123456', timeout=6)
        banner, welcome = c.connect()
        print('PORT %d CONNECTED' % port)
        print('banner:', banner[:80])
        print('welcome:', welcome[:80])
        # sanity: which driver
        try:
            r = c.eval('write(sprintf("DRIVER=%d\\n", 1))', wait=4)
            print('test:', r[:200])
        except Exception as e:
            print('test err:', e)
        c.close()
        break
    except Exception as e:
        print('PORT %d fail: %s' % (port, str(e)[:120]))
