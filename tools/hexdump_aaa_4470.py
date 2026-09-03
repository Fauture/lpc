# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if 'recv-accept' in l and '4470' in l:
        # line like: [recv-accept] client=2 n=54 4D5A...HEX
        idx = l.find('4D5A')
        if idx >= 0:
            h = l[idx:].strip()
            try:
                raw = bytes.fromhex(h)
                print('L%d len=%d:' % (i+1, len(raw)))
                # dump bytes with ascii
                for off in range(0, len(raw), 16):
                    chunk = raw[off:off+16]
                    hexs = ' '.join('%02X' % b for b in chunk)
                    ascs = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
                    print('  %04X  %-47s  %s' % (off, hexs, ascs))
            except Exception as e:
                print('L%d hex parse err: %s' % (i+1, e))
