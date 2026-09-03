# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_k.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find the recv of 3365 before the first msg_cfg_my_info parse
first = None
for i, l in enumerate(lines):
    if 'msg_cfg_my_info client' in l and 'settings=19' in l:
        first = i
        break
print('first my_info parse at L', first+1 if first else None)
if first:
    for j in range(max(0, first-5), first):
        print('L%d: %s' % (j+1, lines[j][:200]))
