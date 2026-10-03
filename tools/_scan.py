# -*- coding: utf-8 -*-
"""Scan the extracted LPC tree for a literal byte pattern."""
import os
import sys

root = sys.argv[1]
needle = sys.argv[2].encode('gbk', 'replace')
ext = sys.argv[3] if len(sys.argv) > 3 else '.c'
hits = []
for dp, dn, fn in os.walk(root):
    for f in fn:
        if ext and not f.endswith(ext):
            continue
        p = os.path.join(dp, f)
        try:
            d = open(p, 'rb').read()
        except Exception:
            continue
        if needle in d:
            hits.append(p)
print('files scanned, hits=%d' % len(hits))
for h in hits[:60]:
    print(' ', h)
