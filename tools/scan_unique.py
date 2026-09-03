# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
RX = {
    'fn-post_init': re.compile(r'\[gfn\] inline global fn=post_init'),
    'Driver-load-ok': re.compile(r'Driver load ok'),
    'Driver-startup': re.compile(r'Driver startup'),
    'start_service_on': re.compile(r'start_service_on'),
    'Service-started': re.compile(r'Service started on port'),
    'My-config': re.compile(r'My configuration'),
    'login-ready': re.compile(r'login'),
    'server-reg': re.compile(r'register_entry.*gs'),
    'broadcast': re.compile(r'broadcast\("'),
    'Done-init-module': re.compile(r'Done of init_service_module'),
}
RANGES = [(0, 200_000_000), (200_000_000, 400_000_000), (400_000_000, 600_000_000),
          (600_000_000, 800_000_000), (800_000_000, 1_000_000_000)]
for lo, hi in RANGES:
    try:
        f = open(path, 'rb')
        f.seek(lo)
        raw = f.read(hi - lo)
        f.close()
    except Exception as e:
        print(lo, 'ERR', e)
        continue
    if not raw:
        continue
    if raw[:2] == b'\xff\xfe':
        raw = raw[2:]
    txt = raw.decode('utf-16-le', errors='replace')
    lines = txt.splitlines()
    for n, r in RX.items():
        hits = [i for i, l in enumerate(lines) if r.search(l)]
        if hits:
            print('%9d..%-9d %-18s x%-5d firstL %d: %s' % (lo, hi, n, len(hits), hits[0],
                  lines[hits[0]][:150]))
