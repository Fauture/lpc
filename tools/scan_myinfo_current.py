# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('total:', len(lines))
for i, l in enumerate(lines):
    if 'msg_cfg_my_info client=' in l or 'Max user permit' in l:
        print('L%d: %s' % (i+1, l[:220]))
