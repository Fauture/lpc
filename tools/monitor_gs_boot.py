# -*- coding: utf-8 -*-
# Streaming tail monitor for a growing trace log (UTF-16LE w/ BOM or plain
# bytes). Incremental decode so no partial-char splitting. Appends one summary
# line to <out> each interval.
import sys, os, time, re, codecs

log_path = sys.argv[1]
out_path = sys.argv[2]
interval = float(sys.argv[3]) if len(sys.argv) > 3 else 5.0

PATTERNS = [
    (r'Driver: 一线|DRIVER|driver', 'driver'),
    (r'MAX_USER', 'maxuser'),
    (r'My configuration', 'mycfg'),
    (r'qianyuanshan', 'qys'),
    (r'obj-init', 'objinit'),
    (r'load_all_zone', 'loadzone'),
    (r'construct-done|create error|construct fail', 'construct'),
    (r'__init__ failed|not a integer|is not a integer|It is not a integer', 'initfail'),
    (r'douchong', 'douchong'),
    (r'lgxy|longgong', 'lgxy'),
    (r'init_all_npc', 'initnpc'),
    (r'guardd', 'guardd'),
    (r'owner-init DONE', 'ownerDONE'),
    (r'post_init', 'postinit'),
    (r'Done of init', 'doneinit'),
    (r'start_service_on|Service started on port', 'svcport'),
    (r'Driver load ok', 'loadok'),
    (r'login', 'login'),
    (r'Bad message', 'badmsg'),
]
rx = [(re.compile(p), name) for p, name in PATTERNS]

size = 0
counts = {name: 0 for _, name in PATTERNS}
recent = []
lines_total = 0
buf = ''
try:
    size = os.path.getsize(log_path)   # start from the current tail
    size -= size % 2                   # UTF-16LE: stay on even boundaries
except OSError:
    size = 0
dec = codecs.getincrementaldecoder('utf-16-le')(errors='replace')

def flush():
    with open(out_path, 'a', encoding='utf-8') as f:
        f.write('t=%.0fs size=%d lines=%d %s\n' % (
            time.time() % 100000, size, lines_total,
            ' '.join('%s=%d' % (n, counts[n]) for _, n in PATTERNS if counts[n])))
        for l in recent[-10:]:
            f.write('  | ' + l[:220] + '\n')
        recent.clear()

last = time.time()
while True:
    try:
        cur = os.path.getsize(log_path)
    except OSError:
        cur = size
    if cur > size:
        with open(log_path, 'rb') as f:
            f.seek(size)
            chunk = f.read(cur - size)
        buf += dec.decode(chunk)
        # reset if we see a BOM mid-stream (shouldn't happen, but safe)
        while True:
            nl = buf.find('\n')
            if nl < 0:
                break
            line = buf[:nl]
            buf = buf[nl + 1:]
            lines_total += 1
            for rx_, name in rx:
                if rx_.search(line):
                    counts[name] += 1
                    if len(recent) < 60:
                        recent.append(line)
                    break
        size = cur
    now = time.time()
    if now - last >= interval:
        flush()
        last = now
    time.sleep(0.4)
