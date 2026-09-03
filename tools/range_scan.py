# -*- coding: utf-8 -*-
# Read fixed byte ranges of the UTF-16LE log and report the LAST occurrences of
# post_init phase markers in each range (fast, no full scan).
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]

RANGES = [
    (300_000_000, 480_000_000),
    (480_000_000, 620_000_000),
    (620_000_000, 760_000_000),
    (760_000_000, 830_000_000),
]
RX = [
    ('postsync', re.compile(r'\[postinit-sync\] do_init on (\S+)')),
    ('ownerDONE', re.compile(r'owner-init DONE do_init on (\S+)')),
    ('callInit', re.compile(r'\[cfptr\] calling do_init on (\S+)')),
    ('masterpark', re.compile(r'\[sched-park\] __master__ kind=(\d)')),
    ('mastercall', re.compile(r'sched-call\] (\S+) args')),
]
for lo, hi in RANGES:
    f = open(path, 'rb')
    f.seek(lo)
    raw = f.read(hi - lo)
    f.close()
    if raw[:2] == b'\xff\xfe':
        raw = raw[2:]
    txt = raw.decode('utf-16-le', errors='replace')
    lines = txt.splitlines()
    found = {n: [] for n, _ in RX}
    for i, l in enumerate(lines):
        for n, r in RX:
            m = r.search(l)
            if m:
                found[n].append((lo + i * 0, m.group(1) if m.groups() else l[:80]))
    print('=== range %d..%d (%d lines) ===' % (lo, hi, len(lines)))
    for n, r in RX:
        hits = found[n]
        if hits:
            # unique last few
            seen = []
            for _, v in hits:
                if v not in seen:
                    seen.append(v)
            print('  %-10s x%d last: %s' % (n, len(hits), seen[-3:]))
