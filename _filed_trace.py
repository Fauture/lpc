# -*- coding: utf-8 -*-
"""Timeline of gs->dba filed requests with the reply that followed each.

usage: python -X utf8 _filed_trace.py <logdir> [last_n]
"""
import io
import os
import re
import sys

D = sys.argv[1] if len(sys.argv) > 1 else 'errfix1'
N = int(sys.argv[2]) if len(sys.argv) > 2 else 25

lines = io.open(os.path.join(D, 'live_gs.txt'), 'rb').read().decode('utf-8', 'replace').splitlines()
ts = re.compile(r't=(\d+)ms')
req = re.compile(r'\[send-filed\] cno=(\d+) msgNo=0x([0-9A-Fa-f]+) raw data\(\d+\)=([0-9A-Fa-f]*)')
rep = re.compile(r'\[recv\] t=\d+ms cno=\d+ cmd=0x([0-9A-Fa-f]+) len=(\d+)')

events = []
last_t = 0
for l in lines:
    m = ts.search(l)
    if m:
        last_t = int(m.group(1)) / 60000.0
    m = req.search(l)
    if m:
        events.append(('REQ', last_t, m.group(2), m.group(3)[:40]))
        continue
    m = rep.search(l)
    if m:
        events.append(('REP', last_t, m.group(1), m.group(2)))

# pair each REQ with the next REP mentioning the same msgNo in its body
for ev in events[-N:]:
    kind, t, a, b = ev
    print('%-4s %7.1fmin  %s  %s' % (kind, t, a, b[:60]))
print('total events', len(events))
