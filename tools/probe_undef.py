# -*- coding: utf-8 -*-
"""Probe ORIGINAL aaa: undefined vs nil in save_string / is_undefined."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('save mixed u', r'mixed u; write(sprintf("%O\n", save_string(u)))'),
    ('save missing key', r'mapping m = ([ ]); write(sprintf("%O\n", save_string(m["nope"])))'),
    ('save arr[99]', r'array a = ({}); write(sprintf("%O\n", save_string(a[99])))'),
    ('save undef kw', r'write(sprintf("%O\n", save_string(undefined)))'),
    ('is_undefined missing key', r'mapping m = ([ ]); write(sprintf("isundef=%d\n", is_undefined(m["nope"])))'),
    ('save nil', r'write(sprintf("%O\n", save_string(nil)))'),
]

for title, expr in tests:
    print('\n==== %s ====' % title)
    print('>>>', expr)
    try:
        out = rpc.call(expr, wait=6.0)
        print('=>', gameshell_rpc.fix_mojibake(out))
    except Exception as e:
        print('ERR:', e)

rpc.close()
print('\nDONE')
