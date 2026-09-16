# -*- coding: utf-8 -*-
"""Follow one (event, mask) pair through the [ev] trace.

usage: python -X utf8 _dbg_ev.py <log> <event> <mask-hex> [first-line] [last-line]
"""
import sys

path = sys.argv[1]
ev = sys.argv[2]
mask = sys.argv[3].lower().replace('0x', '')
lo = int(sys.argv[4]) if len(sys.argv) > 4 else 0
hi = int(sys.argv[5]) if len(sys.argv) > 5 else 1 << 30
L = [l.decode('gbk', 'replace').rstrip() for l in open(path, 'rb').read().split(b'\n')]
pat = '%s mask=0x%s' % (ev, mask)
pat2 = '%s raised=0x%s' % (ev, mask)
wdog = []
for i, l in enumerate(L):
    if not (lo <= i + 1 <= hi):
        continue
    if 'construct-wdog' in l:
        wdog.append(i)
    if pat in l or pat2 in l or ('[ev] wake %s' % ev) in l:
        tag = 'WDOG' if False else ''
        # mark the nearest preceding watchdog
        near = ''
        for w in reversed(wdog[-3:]):
            near = ' (after wdog@%d)' % (w + 1)
            break
        print('%7d %s%s' % (i + 1, l[:150], near))
