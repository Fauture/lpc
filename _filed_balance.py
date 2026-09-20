# -*- coding: utf-8 -*-
"""Per-bucket balance of gs->dba filed requests vs dba replies, to find the
moment the round trips stop.

usage: python -X utf8 _filed_balance.py <logdir> [bucket_min]
"""
import io
import os
import re
import sys

D = sys.argv[1] if len(sys.argv) > 1 else 'errfix1'
BUCKET = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0

t = io.open(os.path.join(D, 'live_gs.txt'), 'rb').read().decode('utf-8', 'replace')
lines = t.splitlines()
ts = re.compile(r't=(\d+)ms')
req = re.compile(r'\[send-filed\] cno=\d+ msgNo=0x[0-9A-Fa-f]+')
rep = re.compile(r'\[recv\] t=\d+ms cno=\d+ cmd=0x(4401|5406|2403)')

buckets = {}
last_t = 0
for l in lines:
    m = ts.search(l)
    if m:
        last_t = int(m.group(1)) / 60000.0        # minutes
    if req.search(l):
        b = int(last_t // BUCKET)
        e = buckets.setdefault(b, [0, 0])
        e[0] += 1
    elif rep.search(l):
        b = int(last_t // BUCKET)
        e = buckets.setdefault(b, [0, 0])
        e[1] += 1

print('bucket(min)  filed_req  filed_reply   delta')
prev = None
for b in sorted(buckets):
    r, p = buckets[b]
    print('%6d-%6d  %9d  %11d  %+6d' % (b * BUCKET, (b + 1) * BUCKET, r, p, p - r))
