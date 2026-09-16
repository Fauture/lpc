# -*- coding: utf-8 -*-
"""Decode the 0x2603 (task log) bodies captured by _bagtask_probe.py.

First login  : 284 bytes
Re-login     :  62 bytes
Same prefix, so some per-task fields are simply missing on the re-login.
"""
import sys

A = bytes.fromhex(
    '000126a1bed0c2cad6a1bf312d39bcb6d0c2cad6c8cecef17ca1bed0c2cad6a1bfd0c2b5c4bfaacabc'
    '0046d0c2b5c4cecab5c0a3acd0c2b5c4bfaacabcbfc9d2d4c8c3c4e3bfeccbd9c1cbbde2d3cecfb7'
    'b5c4bbb9bef3a3acb5c8c4e3b5bd34bcb6b5c4cab1baf2bfc9d2d4c8a5b4f3b3c7d5d2ccc6c0cfcaa6'
    'd1a7cfb0bcbc4fd2e')
B = bytes.fromhex(
    '000126a1bed0c2cad6a1bf312d39bcb6d0c2cad6c8cecef17ca1bed0c2cad6a1bfd0c2b5c4bfaacabc'
    '000000000000000000010000000000000000000000')


def dump(tag, b):
    print('=== %s (%d bytes) ===' % (tag, len(b)))
    # print GBK runs
    cur = bytearray()
    i = 0
    while i < len(b):
        c = b[i]
        if 0x81 <= c <= 0xFE and i + 1 < len(b) and 0x40 <= b[i + 1] <= 0xFE:
            cur += b[i:i + 2]
            i += 2
            continue
        if cur:
            print('    str@%d: %r' % (i - len(cur), cur.decode('gbk', 'replace')))
            cur = bytearray()
        i += 1
    if cur:
        print('    str@%d: %r' % (len(b) - len(cur), cur.decode('gbk', 'replace')))
    print('    hex: %s' % b.hex())


dump('A first login', A)
dump('B re-login', B)
print()
print('A tail after the common prefix:')
n = 0
while n < min(len(A), len(B)) and A[n] == B[n]:
    n += 1
print('  common prefix = %d bytes' % n)
print('  A[%d:] = %s' % (n, A[n:].hex()))
print('  B[%d:] = %s' % (n, B[n:].hex()))
