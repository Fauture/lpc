# -*- coding: utf-8 -*-
"""Probe ORIGINAL aaa: buffer conversions, sizeof(buffer), pad_buffer wire."""
import sys
sys.path.insert(0, r'C:\Users\Administrator\Desktop\lpc\tools')
import gameshell_rpc

rpc = gameshell_rpc.GameShellRpcClient(host='192.168.200.128', port=1024,
                                       password='123456', timeout=15.0)
rpc.connect()
print('CONNECTED')

tests = [
    ('sizeof string', r'write(sprintf("sizeof=%d\n", sizeof("abc")))'),
    ('(buffer)string hex', r'buffer b = (buffer)"({[]})"; write(sprintf("%O\n", b))'),
    ('(buffer)string len', r'buffer b = (buffer)"({[]})"; write(sprintf("len=%d\n", sizeof(b)))'),
    ('pad_buffer str', r'write(sprintf("%O\n", pad_buffer("ab")))'),
    ('pad_buffer buf', r'buffer b = (buffer)"({[]})"; write(sprintf("%O\n", pad_buffer(b)))'),
    ('pad_buffer int', r'write(sprintf("%O\n", pad_buffer(0x1122)))'),
    ('pad_buffer int+str', r'write(sprintf("%O\n", pad_buffer(0x1122, "ab")))'),
    ('pad_buffer len+str', r'write(sprintf("%O\n", pad_buffer(make32(sizeof("({[]})")), (buffer)"({[]})")))'),
    ('save_string undef?', r'mixed u; write(sprintf("%O\n", save_string(u)))'),
    ('save_string empty arr', r'write(sprintf("%O\n", save_string(({}))))'),
    ('sizeof empty arr', r'write(sprintf("sizeof(({}))=%d\n", sizeof(({}))))'),
    ('string->buffer->string', r'buffer b = (buffer)"({[]})"; write(sprintf("back=%s\n", (string)b))'),
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
