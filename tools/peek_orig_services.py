# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\linux\gs.log', 'rb') as f:
    data = f.read()
text = data.decode('gbk', errors='replace')
lines = text.splitlines()
for i in range(170, 215):
    if i < len(lines):
        print('L%d: %s' % (i+1, lines[i][:190]))
