# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
# find full my_info line with keys
idx = text.find('msg_cfg_my_info client=1')
if idx >= 0:
    print(text[idx:idx+1200])
