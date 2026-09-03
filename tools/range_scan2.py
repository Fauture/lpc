# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
RANGES = [(0, 150_000_000), (150_000_000, 300_000_000)]
RX = [
    ('postsync', re.compile(r'postinit-sync\] do_init on (\S+)')),
    ('ownerDONE', re.compile(r'owner-init DONE do_init on (\S+)')),
    ('callInit', re.compile(r'cfptr\] calling do_init on (\S+)')),
    ('ownInit', re.compile(r'owner-init do_init on (\S+)')),
    ('guardd', re.compile(r'guardd')),
    ('zoned', re.compile(r'zoned\.o')),
    ('loadzone', re.compile(r'load_all_zone')),
    ('douchong', re.compile(r'douchong')),
    ('lgxy', re.compile(r'longgong|lgxy')),
    ('inpost', re.compile(r'inPostInit=1')),
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
                found[n].append((i, m.group(1) if m.groups() else l[:90]))
    print('=== range %d..%d (%d lines) ===' % (lo, hi, len(lines)))
    for n, r in RX:
        hits = found[n]
        seen = []
        for _, v in hits:
            if v not in seen:
                seen.append(v)
        print('  %-10s x%-5d last3: %s' % (n, len(hits), str(seen[-3:])[:180]))
