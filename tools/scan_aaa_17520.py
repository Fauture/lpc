# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if re.search(r'17520|0x4470|cmd_cfg_read_etc|read_etc|assure_connection|config.*agent|agent.*config|recv.*4470', l):
        print('L%d: %s' % (i+1, l[:190]))
