# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# around L3350743 - find the error
for i in range(3350500, min(len(lines), 3350755)):
    l = lines[i]
    if any(k in l for k in ['error', 'Error', 'exception', '__init__ failed', 'process', 'create error', 'waking', 'suspend', 'sched-park', 'catch']):
        print('L%d: %s' % (i+1, l[:200]))
