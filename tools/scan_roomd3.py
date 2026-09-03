# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 120 * 1024 * 1024
f = open(path, 'rb')
f.seek(0, 2)
size = f.tell()
start = max(0, size - nbytes)
f.seek(start)
raw = f.read()
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
txt = raw.decode('utf-16-le', errors='replace')
lines = txt.splitlines()
print('window %d lines (%d..%d)' % (len(lines), start, size))
hits = []
for i, l in enumerate(lines):
    if 'Total ' in l and 'rooms' in l:
        hits.append((i, l[:220]))
    elif 'roomd' in l and ('[sched-run] entry=' in l or '[sched-park]' in l):
        hits.append((i, l[:140]))
for i, l in hits[-20:]:
    print('%d %s' % (i, l))
print('--- last roomd run/park line index:', hits[-1][0] if hits else None, 'of', len(lines))
