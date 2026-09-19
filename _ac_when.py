# -*- coding: utf-8 -*-
"""Timeline of the anti-cheat challenge packets in one historical log.

usage: python -X utf8 _ac_when.py <logdir> [cno]
"""
import io
import os
import re
import sys

D = sys.argv[1]
p = os.path.join(D, 'live_gs.txt')
lines = io.open(p, 'rb').read().decode('utf-8', 'replace').splitlines()
pat = re.compile(r'\[(recv|send)\] t=(\d+)ms cno=(\d+) cmd=0x([0-9a-fA-F]+)')
rows = []
for ln in lines:
    m = pat.search(ln)
    if m:
        rows.append((int(m.group(2)), m.group(3), m.group(4).lower(), ln))
# find the connection that sent 0x5108
cnos = sorted({c for _, c, cmd, _ in rows if cmd == '5108'})
print('%s: 0x5108 from cnos %s' % (D, cnos))
for cno in cnos:
    sub = [(t, cmd) for t, c, cmd, _ in rows if c == cno]
    if not sub:
        continue
    t0 = sub[0][0]
    print('--- cno=%s first=%dms last=%dms span=%.1fs pkts=%d' % (cno, t0, sub[-1][0], (sub[-1][0] - t0) / 1000.0, len(sub)))
    for t, cmd in sub:
        if cmd in ('5108', '3e2a', '21b1', '20d2'):
            print('    +%7.1fs cmd=0x%s' % ((t - t0) / 1000.0, cmd))
    for t, c, cmd, ln in rows:
        if c == cno and cmd == '3e2a':
            print('    LOGOUT at +%.1fs' % ((t - t0) / 1000.0))
