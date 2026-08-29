# -*- coding: utf-8 -*-
"""Probe ORIGINAL aaa: is_void / is_undefined / var_type on various values."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('is_void missing key', r'mapping mv1 = ([ ]); write(sprintf("isvoid=%d\n", is_void(mv1["nope"])))'),
    ('is_undefined missing key', r'mapping mu1 = ([ ]); write(sprintf("isundef=%d\n", is_undefined(mu1["nope"])))'),
    ('var_type missing key', r'mapping mt1 = ([ ]); write(sprintf("vartype=%s\n", var_type(mt1["nope"])))'),
    ('var_type 0', r'write(sprintf("vartype0=%s\n", var_type(0)))'),
    ('var_type "s"', r'write(sprintf("vartypes=%s\n", var_type("abc")))'),
    ('var_type array', r'write(sprintf("vartypea=%s\n", var_type(({}))))'),
    ('var_type mapping', r'write(sprintf("vartypem=%s\n", var_type(([ ]))))'),
    ('var_type buffer', r'write(sprintf("vartypeb=%s\n", var_type(allocate_buffer(3))))'),
    ('var_type float', r'write(sprintf("vartypef=%s\n", var_type(1.5)))'),
    ('void fn result', r'void f() { } write(sprintf("isvoidfn=%d save=%O\n", is_void(f()), save_string(f())))'),
    ('uninit var is_void', r'mixed uv1; write(sprintf("isvoid=%d isundef=%d\n", is_void(uv1), is_undefined(uv1)))'),
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
