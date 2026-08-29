# -*- coding: utf-8 -*-
"""Probe ORIGINAL aaa: missing-key type via var_type + is_undefined (unique vars)."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('type missing key', r'mapping mm1 = ([ ]); write(sprintf("var_type=%d\n", var_type(mm1["nope"])))'),
    ('isundef missing key', r'mapping mm2 = ([ ]); write(sprintf("isundef=%d\n", is_undefined(mm2["nope"])))'),
    ('save missing key', r'mapping mm3 = ([ ]); write(sprintf("save=%O\n", save_string(mm3["nope"])))'),
    ('type uninit mixed', r'mixed uu1; write(sprintf("var_type=%d\n", var_type(uu1)))'),
    ('save uninit mixed', r'mixed uu2; write(sprintf("save=%O\n", save_string(uu2)))'),
    ('save undef keyword', r'write(sprintf("save=%O\n", save_string(undefined)))'),
    ('save nil keyword', r'write(sprintf("save=%O\n", save_string(nil)))'),
    ('save empty string', r'write(sprintf("save=%O\n", save_string("")))'),
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
