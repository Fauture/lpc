# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
for i, l in enumerate(lines):
    if re.search(r'cmd_cfg_get_my_info|0x3365|13157|get_info_by_type.*server|send_message.*13157|max_user', l):
        print('L%d: %s' % (i+1, l[:190]))
