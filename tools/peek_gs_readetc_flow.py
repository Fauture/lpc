# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# Show the read_etc flow at L13415-13460 with detail: what section/key was requested
for i in range(13400, 13470):
    if i < len(lines):
        print('L%d: %s' % (i+1, lines[i][:190]))
