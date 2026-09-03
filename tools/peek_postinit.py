# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find Invoke, print surrounding + a window 500 lines after, and last lines
start = next(i for i, l in enumerate(lines) if 'Invoke registered' in l)
print('=== around Invoke ===')
for l in lines[start-3:start+30]:
    print(l[:160])
print('=== last 15 lines ===')
for l in lines[-15:]:
    print(l[:160])
