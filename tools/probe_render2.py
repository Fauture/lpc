# -*- coding: utf-8 -*-
"""Probe ORIGINAL: %s / sprintf rendering of undefined and void."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('%s missing key', r'mapping ms1 = ([ ]); write(sprintf("[%s]\n", ms1["nope"]))'),
    ('(string) undef kw', r'write(sprintf("[%s]\n", (string)undefined))'),
    ('(string) void fn', r'void f1() { } write(sprintf("[%s]\n", (string)f1()))'),
    ('%O undef kw', r'write(sprintf("[%O]\n", undefined))'),
    ('%d missing key', r'mapping md1 = ([ ]); write(sprintf("[%d]\n", md1["nope"]))'),
    ('int missing key', r'mapping mi1 = ([ ]); write(sprintf("[%d]\n", (int)mi1["nope"]))'),
    ('var_type uninit', r'mixed vt1; write(sprintf("vt=%s\n", var_type(vt1)))'),
    ('var_raw_type uninit', r'mixed vr1; write(sprintf("vrt=%d\n", var_raw_type(vr1)))'),
    ('var_raw_type void', r'void f2() { } write(sprintf("vrt=%d\n", var_raw_type(f2())))'),
    ('var_type void', r'void f3() { } write(sprintf("vt=%s\n", var_type(f3())))'),
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
