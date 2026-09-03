# -*- coding: utf-8 -*-
import sys
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
lines = raw.decode('utf-16-le', errors='replace').splitlines()
t = None
for i in range(len(lines) - 1, -1, -1):
    if 'create dist start time failed' in lines[i]:
        t = i
        break
print('last print at %d of %d' % (t, len(lines)))
lo = max(0, t - 60)
for i in range(lo, min(len(lines), t + 6)):
    print('%6d %s' % (i, lines[i][:200]))
