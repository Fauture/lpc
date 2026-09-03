# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
# find byte offset of the LAST '/gs/zone/yunxiaogong' occurrence by scanning from the tail window
nbytes = 60000000
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
target = None
for i in range(len(lines) - 1, -1, -1):
    if 'yunxiaogong' in lines[i] and ('setVar' in lines[i] or 'zone' in lines[i]):
        target = i
        break
if target is None:
    print('not found')
    sys.exit(0)
print('last zone-list-ish at winline %d' % target)
for i in range(target - 25, min(len(lines), target + 120)):
    l = lines[i]
    # trim very long lines
    print('%6d %s' % (i, l[:170]))
