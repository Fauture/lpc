# -*- coding: utf-8 -*-
"""Grep one LPC source file (GBK) for substrings, printing matched lines."""
import io
import sys

path = sys.argv[1]
keys = sys.argv[2:]
L = io.open(path, encoding='gbk', errors='replace').read().split('\n')
for i, l in enumerate(L):
    if any(k in l for k in keys):
        print('%5d %s' % (i + 1, l.strip()[:170]))
