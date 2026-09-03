# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 90000000
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
# find last find_object with /gs/zone/... while in=do_scan
target = None
for i in range(len(lines) - 1, -1, -1):
    if 'find_object' in lines[i] and 'do_scan' in lines[i] and '/gs/zone/' in lines[i]:
        target = i
        break
if target is None:
    print('no do_scan yunxiaogong in window')
    sys.exit(0)
print('do_scan start at winline %d; showing from %d' % (target, max(0, target - 45)))
for i in range(max(0, target - 45), min(len(lines), target + 40)):
    print('%6d %s' % (i, lines[i][:190]))
