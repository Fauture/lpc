# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find recv lines with 0x2473 with attrib, and any dispatch/denied/game bad command lines
for i, l in enumerate(lines):
    if re.search(r'0x2473|0x2471', l) and ('recv' in l or 'attrib' in l or 'dispatch' in l):
        print('L%d: %s' % (i+1, l[:200]))
        if i > 300000: break
print('--- denied / bad command ---')
c2 = 0
for i, l in enumerate(lines):
    if re.search(r'denied cmd|bad command|dispatch\] cmd=0x247', l):
        print('L%d: %s' % (i+1, l[:180]))
        c2 += 1
        if c2 > 10: break
