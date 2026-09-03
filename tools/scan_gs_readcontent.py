# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
nbytes = 400 * 1024 * 1024
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
rc = [(i, l) for i, l in enumerate(lines) if '[readcontent]' in l]
print('readcontent replies in window:', len(rc))
for i, l in rc[-10:]:
    print('  L%d %s' % (i, l[:170]))
# gs cookie usage: find read_content / do_operate queue sends with cookies
qc = [(i, l) for i, l in enumerate(lines) if 'queue] send' in l and 'cookie' in l]
print('queue sends with cookie:', len(qc))
for i, l in qc[-6:]:
    print('  L%d %s' % (i, l[:170]))
