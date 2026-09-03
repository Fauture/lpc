# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
RANGES = [(100_000_000, 300_000_000), (300_000_000, 500_000_000), (500_000_000, 700_000_000)]
RX = {
    'connect-dba': re.compile(r'connect.*8120|try_to_connect_dba|connect\] fd=\S+ 192\.168\.200\.1:8120'),
    'send-to-dba': re.compile(r'send_to_client.*8120|\[send\] cno=(\d+).*sent'),
    'read_content': re.compile(r'read_content'),
    'create_content': re.compile(r'create_content'),
    'queue-send': re.compile(r'queue\] send qid=(\d+)'),
    'dba-comm': re.compile(r'dba_comm'),
    'filed': re.compile(r'filed\.o'),
}
for lo, hi in RANGES:
    f = open(path, 'rb')
    f.seek(lo)
    raw = f.read(hi - lo)
    f.close()
    if raw[:2] == b'\xff\xfe':
        raw = raw[2:]
    lines = raw.decode('utf-16-le', errors='replace').splitlines()
    print('=== range %d..%d (%d lines) ===' % (lo, hi, len(lines)))
    for n, r in RX.items():
        hits = [(i, l) for i, l in enumerate(lines) if r.search(l)]
        print('  %-14s x%d' % (n, len(hits)))
        if hits:
            i, l = hits[-1]
            print('     last L%d: %s' % (i, l[:170]))
