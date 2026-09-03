# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# 0x2471/0x2473 arrivals and msg_cfg handlers
for i, l in enumerate(lines):
    if re.search(r'0x2471|0x2473|msg_cfg_read_etc|msg_cfg_etc_page_data|ev_readetcd|raise_event|oper_map', l):
        print('L%d: %s' % (i+1, l[:170]))
        if i > 500000: break
