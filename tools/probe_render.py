# -*- coding: utf-8 -*-
"""Probe ORIGINAL aaa: %O and (string) rendering of undefined/void/missing."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('%O missing key', r'mapping mo1 = ([ ]); write(sprintf("[%O]\n", mo1["nope"]))'),
    ('(string) missing key', r'mapping ms1 = ([ ]); write(sprintf("[%s]\n", (string)ms1["nope"]))'),
    ('+ missing key', r'mapping mp1 = ([ ]); write(sprintf("[%s]\n", "x" + mp1["nope"]))'),
    ('%O void fn', r'void f1() { } write(sprintf("[%O]\n", f1()))'),
    ('(string) void fn', r'void f2() { } write(sprintf("[%s]\n", (string)f2()))'),
    ('save void fn', r'void f3() { } write(sprintf("save=%O\n", save_string(f3())))'),
    ('if(missing) t', r'mapping mi1 = ([ ]); write(sprintf("t=%d\n", (mi1["nope"] ? 1 : 0)))'),
    ('missing==0', r'mapping me1 = ([ ]); write(sprintf("eq0=%d\n", (me1["nope"] == 0 ? 1 : 0)))'),
    ('%O undefined kw', r'write(sprintf("[%O]\n", undefined))'),
    ('to_string undef', r'write(sprintf("ts=%s\n", to_string(undefined)))'),
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
