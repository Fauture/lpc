# -*- coding: utf-8 -*-
# Stream-scan a UTF-16LE trace once, reporting marker line numbers + tail.
# Never holds the whole file in memory.
import sys, codecs

path = sys.argv[1]
out = sys.argv[2]

MARKERS = [
    ('DRIVER', r'Driver: |DRIVER'),
    ('MAXUSER', r'MAX_USER'),
    ('QYS', r'qianyuanshan'),
    ('QYS_DONE', r'construct-done.*qianyuanshan'),
    ('INITFAIL', r'__init__ failed|not a integer|It is not a integer'),
    ('OBJINITPARK', r'sched-park\] .*obj-init'),
    ('GUARDD', r'guardd'),
    ('GUARDD_DONE', r'owner-init DONE do_init on /gs/daemons/guardd\.o'),
    ('LOADZONE', r'load_all_zone'),
    ('ZONED_DONE', r'owner-init DONE.*zoned'),
    ('DOUCHONG', r'douchong'),
    ('DOUCHONG_INIT', r'do_init on /gs/daemons/.*douchong'),
    ('LGXY', r'longgong|lgxy'),
    ('LGXY_INIT', r'do_init on /gs/daemons/.*longgong'),
    ('INITNPC', r'init_all_npc'),
    ('OWNERDONE', r'owner-init DONE'),
    ('POSTINIT', r'post_init'),
    ('DONEINIT', r'Done of init|Driver load ok'),
    ('SVC', r'Service started on port|start_service_on'),
]
import re
rx = [(n, re.compile(p)) for n, p in MARKERS]

dec = codecs.getincrementaldecoder('utf-16-le')(errors='replace')
first = {}
last = {}
count = {}
tail = []      # last 60 lines
lastk = {}     # last line per marker
pos = 0
buf = ''
CH = 8 * 1024 * 1024
with open(path, 'rb') as f:
    while True:
        chunk = f.read(CH)
        if not chunk:
            break
        buf += dec.decode(chunk)
        while True:
            nl = buf.find('\n')
            if nl < 0:
                break
            line = buf[:nl]
            buf = buf[nl + 1:]
            tail.append(line)
            if len(tail) > 60:
                tail.pop(0)
            for name, r in rx:
                if r.search(line):
                    count[name] = count.get(name, 0) + 1
                    if name not in first:
                        first[name] = pos
                    last[name] = pos
                    lastk[name] = line[:240]
                    break
            pos += 1
        sys.stdout.write('\rlines=%d' % pos)
        sys.stdout.flush()
# flush decoder tail (if the file ends mid-line)
if buf:
    for name, r in rx:
        if r.search(buf):
            count[name] = count.get(name, 0) + 1
            if name not in first:
                first[name] = pos
            last[name] = pos
            lastk[name] = buf[:240]
with open(out, 'w', encoding='utf-8') as o:
    o.write('total lines: %d\n' % pos)
    for name, _ in rx:
        o.write('%-14s count=%-6d first=%-10s last=%s\n' % (name, count.get(name, 0), first.get(name, '-'), last.get(name, '-')))
    o.write('\n--- last line per marker ---\n')
    for name, _ in rx:
        if name in lastk:
            o.write('[%s] %s\n' % (name, lastk[name]))
    o.write('\n--- tail 60 ---\n')
    for l in tail:
        o.write(l[:250] + '\n')
print('\ndone')
