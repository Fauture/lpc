# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 250 * 1024 * 1024
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
pat = re.compile(r'\[sched-park\] __master__ kind=(\d) key=(\S*)')
objpat = re.compile(r'key=obj-init:(\S+)')
last_master = None
for i, l in enumerate(lines):
    m = pat.search(l)
    if m:
        last_master = (i, m.group(1), m.group(2))
print('last master park:', last_master, lines[last_master[0]][:190] if last_master else '')
# collect all obj-init parks of master
hits = []
for i, l in enumerate(lines):
    if 'sched-park] __master__' in l and 'obj-init' in l:
        m = objpat.search(l)
        hits.append((i, m.group(1) if m else '?'))
print('master obj-init parks in window: %d' % len(hits))
for i, v in hits[-6:]:
    print('  L%d: %s' % (i, v))
    print('     ctx: %s' % lines[i][:200])
# last construct-done in window
cd = None
for i, l in enumerate(lines):
    if 'construct-done]' in l:
        cd = i
print('last construct-done in window at %d: %s' % (cd, lines[cd][:200] if cd is not None else '-'))
