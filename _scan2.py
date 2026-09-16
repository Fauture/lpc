# -*- coding: utf-8 -*-
"""Search every file under a root for one of several byte patterns."""
import os
import sys

root = sys.argv[1]
pats = []
for spec in sys.argv[2:]:
    pats.append((spec, bytes.fromhex(spec)))
maxsz = 400 * 1024 * 1024
found = 0
for dp, dn, fn in os.walk(root):
    for f in fn:
        p = os.path.join(dp, f)
        try:
            if os.path.getsize(p) > maxsz:
                continue
            d = open(p, 'rb').read()
        except Exception:
            continue
        for name, pat in pats:
            i = d.find(pat)
            if i >= 0:
                print('%s  off=%d  %s' % (name, i, p))
                found += 1
print('done, hits=%d' % found)
