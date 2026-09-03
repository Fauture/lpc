# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# around L5489433-5489480 show all lines (suspend reason)
for i in range(5489432, min(5489500, len(lines))):
    print('L%d: %s' % (i+1, lines[i][:190]))
