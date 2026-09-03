# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 100 * 1024 * 1024
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
print('window %d lines, %d..%d' % (len(lines), start, size))
t = None
for i in range(len(lines) - 1, -1, -1):
    if 'setVar slot=5 val=/gs/zone/yunxiaogong' in lines[i] or 'val=/gs/zone/yunxiaogong' in lines[i]:
        t = i
        break
if t is None:
    print('no yunxiaogong setVar in window; last zone setVars:')
    hits = [(i, lines[i][:150]) for i in range(len(lines)) if 'setVar slot=5 val=/gs/zone/' in lines[i]]
    for i, l in hits[-6:]:
        print('  %d: %s' % (i, l))
    sys.exit(0)
print('last yunxiaogong setVar at %d' % t)
for i in range(t, min(len(lines), t + 35)):
    print('%6d %s' % (i, lines[i][:190]))
