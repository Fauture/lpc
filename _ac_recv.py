# -*- coding: utf-8 -*-
"""Histogram of [recv]/[send] commands per connection in one log dir.

usage: python -X utf8 _ac_recv.py <logdir>
"""
import collections
import io
import os
import re
import sys

d = sys.argv[1] if len(sys.argv) > 1 else 'audit2'
p = os.path.join(d, 'live_gs.txt')
lines = io.open(p, 'rb').read().decode('utf-8', 'replace').splitlines()
print('%s: %d lines' % (p, len(lines)))

rx = re.compile(r'\[recv\] t=(\d+) cno=(\d+) cmd=(0x[0-9a-fA-F]+) len=(\d+)')
sx = re.compile(r'\[send\] t=(\d+) cno=(\d+) cmd=(0x[0-9a-fA-F]+) len=(\d+)')
rx2 = re.compile(r'\[recv\] cno=(\d+) cmd=(0x[0-9a-fA-F]+)')
sx2 = re.compile(r'\[send\] cno=(\d+) cmd=(0x[0-9a-fA-F]+)')

recv = collections.Counter()
send = collections.Counter()
per_cno_recv = collections.defaultdict(collections.Counter)
per_cno_send = collections.defaultdict(collections.Counter)
for ln in lines:
    m = rx.search(ln) or rx2.search(ln)
    if m:
        g = m.groups()
        cno, cmd = g[-2], g[-1]
        recv[cmd] += 1
        per_cno_recv[cno][cmd] += 1
        continue
    m = sx.search(ln) or sx2.search(ln)
    if m:
        g = m.groups()
        cno, cmd = g[-2], g[-1]
        send[cmd] += 1
        per_cno_send[cno][cmd] += 1

print('--- recv total %d ---' % sum(recv.values()))
for c, n in recv.most_common(25):
    print('  %-10s x%d' % (c, n))
print('--- send total %d ---' % sum(send.values()))
for c, n in send.most_common(25):
    print('  %-10s x%d' % (c, n))
print('--- per cno (recv>0) ---')
for cno in sorted(per_cno_recv, key=lambda x: -sum(per_cno_recv[x].values()))[:6]:
    print('  cno=%s recv=%d send=%d' % (cno, sum(per_cno_recv[cno].values()),
                                        sum(per_cno_send[cno].values())))
    for c, n in per_cno_recv[cno].most_common(12):
        print('      recv %-10s x%d   (send same cmd x%d)' % (c, n, per_cno_send[cno][c]))
