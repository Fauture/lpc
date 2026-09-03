# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 260 * 1024 * 1024
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
# find last construct-done index
cd = None
for i, l in enumerate(lines):
    if 'construct-done]' in l:
        cd = i
print('last construct-done at %d of %d' % (cd, len(lines)))
for i in range(cd, min(len(lines), cd + 70)):
    print('%6d %s' % (i, lines[i][:200]))
