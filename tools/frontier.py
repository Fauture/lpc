# -*- coding: utf-8 -*-
# Read the LAST N MB of a UTF-16LE trace and report the LAST occurrences of
# phase markers (progress frontier) with their byte-offsets, plus the run of
# the last few hundred thousand lines' phase mix.
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = int(sys.argv[2]) if len(sys.argv) > 2 else 60 * 1024 * 1024
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
print('window %d..%d (%d lines)' % (start, size, len(lines)))

patterns = {
    'cfptr-owner-init': re.compile(r'owner-init do_init on (\S+)'),
    'ownerDONE': re.compile(r'owner-init DONE do_init on (\S+)'),
    'postinit-sync': re.compile(r'postinit-sync\] do_init on (\S+)'),
    'construct-done': re.compile(r'construct-done\] (\S+)'),
    'obj-init-park': re.compile(r'sched-park\] (\S+) kind=3 key=obj-init:(\S+)'),
    'loadzone': re.compile(r'load_all_zone'),
    'zone-list': re.compile(r'/gs/zone/'),
    'guardd': re.compile(r'guardd\.o'),
    'douchong': re.compile(r'douchong'),
    'longgong': re.compile(r'longgong|lgxy'),
    'sched-call': re.compile(r'sched-call\] (\S+)'),
    'master-run': re.compile(r'sched-run\] entry=(__master__|__init__)'),
}
last = {}
for i, l in enumerate(lines):
    for n, r in patterns.items():
        m = r.search(l)
        if m:
            last[n] = (i, m.group(0)[:130], l[:200])
for n in sorted(last):
    i, grp, l = last[n]
    print('%-16s @winline %d (abs %d): %s' % (n, i, start, l[:200]))
# current executing mix in last 300k lines
from collections import Counter
c = Counter()
for l in lines[-300000:]:
    m = re.search(r'\[sched-run\] entry=(\S+)', l)
    if m:
        c[m.group(1)] += 1
    m2 = re.search(r'obj=(\S+\.o)', l)
print('--- sched-run entry mix last 300k ---')
for k, v in c.most_common(12):
    print('  %-24s %d' % (k, v))
