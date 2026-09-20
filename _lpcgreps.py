# -*- coding: utf-8 -*-
"""Find LPC source lines that touch a given table/field name (GBK-decoded).

usage: python -X utf8 _lpcgreps.py <needle> [max_hits]
"""
import io
import os
import sys

needle = sys.argv[1] if len(sys.argv) > 1 else 'runtime'
limit = int(sys.argv[2]) if len(sys.argv) > 2 else 25
pat = '"%s"' % needle
hits = []
for root, dirs, files in os.walk('linux'):
    for f in files:
        if not f.endswith('.c'):
            continue
        p = os.path.join(root, f)
        try:
            t = io.open(p, 'rb').read().decode('gbk', 'replace')
        except Exception:
            continue
        if pat not in t:
            continue
        lines = t.split('\n')
        for i, l in enumerate(lines):
            if pat in l:
                hits.append((p, i + 1, l.strip()[:150]))
                if len(hits) >= limit:
                    break
        if len(hits) >= limit:
            break
    if len(hits) >= limit:
        break
for h in hits:
    print('%s:%d  %s' % h)
print('shown %d' % len(hits))
