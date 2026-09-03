# -*- coding: utf-8 -*-
# Read the LAST N MB of a UTF-16LE trace and report the earliest interesting
# marker inside it (i.e. the last progress region), plus tail summary.
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')

path = sys.argv[1]
nbytes = int(sys.argv[2]) if len(sys.argv) > 2 else 30 * 1024 * 1024

f = open(path, 'rb')
f.seek(0, 2)
size = f.tell()
f.seek(max(0, size - nbytes))
raw = f.read()
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
txt = raw.decode('utf-16-le', errors='replace')
lines = txt.splitlines()
print('window: %d bytes, %d lines (offset %d)' % (len(raw), len(lines), max(0, size - nbytes)))

rx = {
    'construct-done': re.compile(r'construct-done'),
    'Load:': re.compile(r'^Load: '),
    'obj-init-park': re.compile(r'sched-park\].*obj-init'),
    'master-park': re.compile(r'sched-park\] __master__'),
    'qianyuanshan': re.compile(r'qianyuanshan'),
    'guardd': re.compile(r'guardd'),
    'douchong': re.compile(r'douchong'),
    'longgong': re.compile(r'longgong|lgxy'),
    'ownerDONE': re.compile(r'owner-init DONE'),
    'initfail': re.compile(r'__init__ failed|not a integer'),
    'svc': re.compile(r'start_service_on|Service started'),
    'driverloadok': re.compile(r'Driver load ok'),
    'invoke-reg': re.compile(r'Invoke registered'),
}
first = {}
lastl = {}
for i, l in enumerate(lines):
    for n, r in rx.items():
        if r.search(l):
            if n not in first:
                first[n] = i
            lastl[n] = l[:200]
print('--- first occurrence line-in-window per marker (smallest = most recent progress) ---')
for n, r in rx.items():
    if n in first:
        print('%-16s first@%d last@%d  %s' % (n, first[n], lastl and n in lastl and lines.__len__() or -1, ''))
        if n in lastl:
            pass
# print in order of first occurrence
order = sorted(first.items(), key=lambda kv: kv[1])
for n, i in order:
    print('  [%s] line %d: %s' % (n, i, lines[i][:220]))
print('--- last 6 lines ---')
for l in lines[-6:]:
    print('T:', l[:200])
