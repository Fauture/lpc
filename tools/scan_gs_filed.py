# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 450 * 1024 * 1024
f = open(path, 'rb')
f.seek(0, 2)
size = f.tell()
start = max(0, size - nbytes)
f.seek(start)
raw = f.read(size - start)
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
lines = raw.decode('utf-16-le', errors='replace').splitlines()
print('window %d lines (%d..%d)' % (len(lines), start, size))
for patname, pat in [
    ('read_content-call', re.compile(r'filed\.c" -> read_content|-> read_content args')),
    ('create_content-call', re.compile(r'-> create_content args')),
    ('dist-fail', re.compile(r'load dist start time failed|create dist start time failed')),
    ('filed-inline', re.compile(r'cfu-filed|coa-inline.*filed')),
    ('try-to-connect-dba', re.compile(r'try_to_connect_dba')),
]:
    hits = [(i, l) for i, l in enumerate(lines) if pat.search(l)]
    print('%-20s x%d' % (patname, len(hits)))
    for i, l in hits[-4:]:
        print('   L%d %s' % (i, l[:180]))
