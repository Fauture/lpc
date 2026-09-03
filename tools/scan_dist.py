# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if re.search(r'my_dist|dist_name|msg_cfg_my_dist|17520|17524|_dist_name|msg_cfg_dist_list', l):
        print('L%d: %s' % (i+1, l[:180]))
        if i > 200000: break
