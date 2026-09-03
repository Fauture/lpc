# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('total:', len(lines))
# last 8 lines
for l in lines[-8:]:
    print(l[:170])
# find my_info parse - max_user value now
for i, l in enumerate(lines):
    if 'msg_cfg_my_info client' in l and 'settings=' in l:
        print('L%d: %s' % (i+1, l[:400]))
        break
