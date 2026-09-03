# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_k.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
for i, l in enumerate(lines):
    if re.search(r'get_my_info|cmd_cfg_get_my_info|my_info|13157|0x3365|type=', l):
        print('L%d: %s' % (i+1, l[:190]))
        if i > 6000: break
