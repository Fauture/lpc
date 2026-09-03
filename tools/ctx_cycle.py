# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 450 * 1024 * 1024
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
# pick the fail print at ~3780862 relative, print +/- 40
t = 3780862 - 0
lo = max(0, t - 45)
for i in range(lo, min(len(lines), t + 12)):
    print('%6d %s' % (i, lines[i][:190]))
