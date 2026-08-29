# -*- coding: utf-8 -*-
"""Probe ORIGINAL: %s rendering of undefined via explicit string cast."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('(string) missing key', r'mapping mz1 = ([ ]); write(sprintf("[%s]\n", (string)mz1["nope"]))'),
    ('(string) uninit var', r'mixed uz1; write(sprintf("[%s]\n", (string)uz1))'),
    ('sprintf %O uninit', r'mixed uo1; write(sprintf("[%O]\n", uo1))'),
    ('save uninit var', r'mixed us1; write(sprintf("save=%O\n", save_string(us1)))'),
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
