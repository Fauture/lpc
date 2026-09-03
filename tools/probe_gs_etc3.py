# -*- coding: utf-8 -*-
"""Probe original gs: read_file on etc.pak paths."""
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

print('=== read_file("/gs/pack_data/etc.pak/valid_chinese.list") ===')
print(q('strlen(read_file("/gs/pack_data/etc.pak/valid_chinese.list"))'))
print('=== read_file("/gs/pack_data/etc.pak/ban_words.list") ===')
print(q('strlen(read_file("/gs/pack_data/etc.pak/ban_words.list"))'))
print('=== get_dir("/gs/pack_data/etc.pak/*.list") count ===')
print(q('sizeof(get_dir("/gs/pack_data/etc.pak/*.list"))'))
try:
    c.close()
except Exception:
    pass
