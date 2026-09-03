# -*- coding: utf-8 -*-
import re, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
fn = sys.argv[1]
d = open(fn, encoding='utf-8', errors='replace').read()
strs = re.findall(r'\[\s*(\d+)\] (".*?")\n', d)
byidx = {}
for idx, s in strs:
    try:
        byidx[int(idx)] = s
    except Exception:
        pass
for i in range(0, 40):
    print(i, repr(byidx.get(i, '?')))
