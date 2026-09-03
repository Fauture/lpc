# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 160 * 1024 * 1024
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
t = None
for i, l in enumerate(lines):
    if 'Total 348 rooms' in l:
        t = i
        break
if t is None:
    print('no printf found')
    sys.exit(0)
print('printf at %d; showing %d..%d' % (t, max(0, t - 55), t + 8))
for i in range(max(0, t - 55), min(len(lines), t + 8)):
    print('%6d %s' % (i, lines[i][:195]))
