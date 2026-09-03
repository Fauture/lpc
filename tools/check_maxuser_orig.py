# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\linux\gs.log', 'rb') as f:
    data = f.read()
text = data.decode('gbk', errors='replace')
lines = text.splitlines()
# find "Max user permit" context
for i, l in enumerate(lines):
    if 'Max user permit' in l or 'permit to login' in l:
        print('=== around L%d ===' % (i+1))
        for j in range(max(0, i-20), min(len(lines), i+5)):
            print('L%d: %s' % (j+1, lines[j][:160]))
