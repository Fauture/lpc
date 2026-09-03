# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# decode all 4470 bodies: extract ascii name from hex recv-accept lines
for i, l in enumerate(lines):
    if '0x4470' in l and 'recv-accept' in l:
        hexpart = l.split('n=')[-1] if 'n=' in l else ''
        # the hex body is after 4D5A header; find printable ascii
        m = re.search(r'(4D5A[0-9A-F]+)', l)
        if m:
            h = m.group(1)
            try:
                raw = bytes.fromhex(h)
                # skip 10-byte frame header, then parse value-typed
                # extract all printable segments
                parts = re.findall(rb'[\x20-\x7e]{3,}', raw[10:])
                print('L%d: %s' % (i+1, [p.decode('ascii', 'replace') for p in parts]))
            except Exception as e:
                print('L%d: decode err' % i)
