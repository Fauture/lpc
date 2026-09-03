# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if re.search(r'msg_cfg_my_info|set_config|my_dist|my_name|还没有从 CONFIG_D|get_config\b', l):
        print('L%d: %s' % (i+1, l[:190]))
        if i > 150000: break
