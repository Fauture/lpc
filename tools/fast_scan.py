# -*- coding: utf-8 -*-
# Fast single-pass marker scan of a UTF-16LE trace (32MB chunks, no progress
# prints). Records FIRST/LAST line index of each marker + the last few lines.
import sys, re, codecs

path = sys.argv[1]
out = sys.argv[2]

MARKERS = [
    ('DRIVER', r'Driver: |DRIVER:'),
    ('MAXUSER', r'MAX_USER'),
    ('MYCFG', r'My configuration'),
    ('INVOKE_REG', r'Invoke registered'),
    ('LOADOK', r'Driver load ok'),
    ('SVC', r'Service started on port|start_service_on'),
    ('POSTSYNC', r'postinit-sync\] do_init on (\S+)'),
    ('OWNERDONE', r'owner-init DONE do_init on (\S+)'),
    ('GUARDD', r'guardd\.o'),
    ('GUARDD_DONE', r'owner-init DONE do_init on /gs/daemons/guardd\.o'),
    ('ZONED', r'zoned\.o'),
    ('ZONED_DONE', r'owner-init DONE do_init on /gs/daemons/zoned\.o'),
    ('LOADZONE', r'load_all_zone'),
    ('QYS', r'qianyuanshan'),
    ('QYS_FAIL', r'__init__ failed: It is not a integer|not a integer'),
    ('ROOMD', r'roomd\.o'),
    ('DOUCHONG', r'douchong'),
    ('DC_INIT', r'do_init on /gs/daemons/.*douchong'),
    ('DC_DONE', r'owner-init DONE do_init on /gs/daemons/.*douchong'),
    ('LGXY', r'longgong|lgxy'),
    ('LG_INIT', r'do_init on /gs/daemons/.*longgong'),
    ('LG_DONE', r'owner-init DONE do_init on /gs/daemons/.*longgong'),
    ('INITNPC', r'init_all_npc'),
    ('MASTERPARK', r'sched-park\] __master__'),
    ('MASTER_RUN', r'sched-run\] entry=__master__'),
]
rx = [(n, re.compile(p)) for n, p in MARKERS]

dec = codecs.getincrementaldecoder('utf-16-le')(errors='replace')
first = {}
last = {}
cnt = {}
lastln = {}
buf = ''
pos = 0
CH = 32 * 1024 * 1024
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
            pos += 1
            for n, r in rx:
                m = r.search(line)
                if m:
                    cnt[n] = cnt.get(n, 0) + 1
                    if n not in first:
                        first[n] = pos
                    last[n] = pos
                    if len(lastln) > 4000:
                        lastln.clear()
                    lastln[n] = (m.group(0) if m.groups() else line[:200])
                    break
with open(out, 'w', encoding='utf-8') as o:
    o.write('total lines scanned: %d\n' % pos)
    o.write('%-12s %6s %-10s %-10s %s\n' % ('marker', 'count', 'first', 'last', 'sample'))
    for n, _ in rx:
        o.write('%-12s %6d %-10s %-10s %s\n' % (n, cnt.get(n, 0), first.get(n, '-'), last.get(n, '-'),
                lastln.get(n, '')[:160]))
print('done lines=%d' % pos)
