# -*- coding: utf-8 -*-
"""Grep the decompiled LPC tree for GBK Chinese strings (the plain grep tool
reads UTF-8 and misses them).

usage: python -X utf8 _gbkgrep.py <root> <needle> [needle2 ...]
"""
import io
import os
import sys

root = sys.argv[1] if len(sys.argv) > 1 else 'linux/gs/pack_data/lib_gs32-c'
needles = sys.argv[2:]
hits = 0
for dp, dn, fn in os.walk(root):
    for f in fn:
        p = os.path.join(dp, f)
        try:
            d = io.open(p, encoding='gbk', errors='replace').read()
        except Exception:
            continue
        if not any(n in d for n in needles):
            continue
        for i, line in enumerate(d.replace('\r', '').split('\n')):
            if any(n in line for n in needles):
                print('%s:%d: %s' % (p.replace(root, ''), i + 1, line.strip()[:170]))
                hits += 1
                if hits > 120:
                    print('... (truncated)')
                    sys.exit(0)
