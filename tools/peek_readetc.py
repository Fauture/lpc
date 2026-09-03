# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# look for 17520 send, 9329/9331 recv, ev_readetcd waits
import re
for i, l in enumerate(lines):
    if re.search(r'17520|9329|9331|ev_readetcd|read_etc|msg_cfg_read_etc|lzss_compress', l):
        print('L%d: %s' % (i+1, l[:170]))
        if i > 300000: break
