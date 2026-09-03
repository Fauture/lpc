# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 300 * 1024 * 1024
f = open(path, 'rb')
f.seek(0, 2)
size = f.tell()
start = max(0, size - nbytes)
f.seek(start)
raw = f.read(size - start)
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
txt = raw.decode('utf-16-le', errors='replace')
lines = txt.splitlines()
print('window %d lines (%d..%d)' % (len(lines), start, size))
pats = {
    'dist-fail-print': re.compile(r'load dist start time failed|create dist start time failed'),
    'dist-read': re.compile(r'dist_start_time'),
    'master-run': re.compile(r'sched-run\] entry=__master__'),
    'master-sched': re.compile(r'sched-(call|park)\] (__master__|\S+)'),
    'readcontent': re.compile(r'\[readcontent\]'),
    'load_dist': re.compile(r'load_dist_start_time'),
}
last = {}
for i, l in enumerate(lines):
    for n, r in pats.items():
        if r.search(l):
            last[n] = (i, l[:220])
for n in pats:
    if n in last:
        i, l = last[n]
        print('%-16s last@%d: %s' % (n, i, l))
# count dist fail prints
cnt = 0
first = None
for i, l in enumerate(lines):
    if pats['dist-fail-print'].search(l):
        cnt += 1
        if first is None:
            first = i
print('dist-fail prints: %d (first at %s)' % (cnt, first))
