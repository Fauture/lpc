# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if re.search(r"name='lgxy_server'|name='lgxy|lgxy_server|name='douchong|send_message.*9331|send_message.*9329", l):
        print('L%d: %s' % (i+1, l[:200]))
