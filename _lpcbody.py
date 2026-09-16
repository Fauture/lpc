# -*- coding: utf-8 -*-
"""Print the body of an LPC function from a GBK source file.

usage: python -X utf8 _lpcbody.py <file> <funcname> [extra_lines]
"""
import io
import sys

path = sys.argv[1]
fn = sys.argv[2]
extra = int(sys.argv[3]) if len(sys.argv) > 3 else 45
L = io.open(path, encoding='gbk', errors='replace').read().split('\n')
for i, l in enumerate(L):
    s = l.strip()
    if s.startswith(fn + '(') and s.endswith('{'):
        print('=== %s @%d ===' % (fn, i + 1))
        for j in range(i, min(len(L), i + extra)):
            print(j + 1, L[j])
        print()
