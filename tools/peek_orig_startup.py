# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\linux\gs.log', 'rb') as f:
    data = f.read()
text = data.decode('gbk', errors='replace')
lines = text.splitlines()
# Show the first 60 lines to understand startup order
for i in range(0, min(60, len(lines))):
    print('L%d: %s' % (i+1, lines[i][:180]))
