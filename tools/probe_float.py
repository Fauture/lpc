# -*- coding: utf-8 -*-
"""Probe ORIGINAL dba(1025): float save_string formats."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1025,
                                       password='123456', timeout=10.0)
rpc.connect()
print('CONNECTED DBA')
tests = [
    ('save 1.5', r'write(sprintf("save=%O\n", save_string(1.5)))'),
    ('save 1.0', r'write(sprintf("save=%O\n", save_string(1.0)))'),
    ('save 0.1', r'write(sprintf("save=%O\n", save_string(0.1)))'),
    ('save 123.456', r'write(sprintf("save=%O\n", save_string(123.456)))'),
    ('save -2.5', r'write(sprintf("save=%O\n", save_string(-2.5)))'),
    ('save 1e10', r'write(sprintf("save=%O\n", save_string(10000000000.0)))'),
    ('save 0.0001', r'write(sprintf("save=%O\n", save_string(0.0001)))'),
    ('save arr of f', r'write(sprintf("save=%O\n", save_string(({1.5, 2.25}))))'),
    ('save map f', r'write(sprintf("save=%O\n", save_string(([ "x": 1.5 ]))))'),
]
for title, expr in tests:
    print('\n== %s ==' % title)
    print('>>>', expr)
    try:
        out = rpc.call(expr, wait=6.0)
        print('=>', gameshell_rpc.fix_mojibake(out))
    except Exception as e:
        print('ERR:', e)
rpc.close()
print('\nDONE')
