# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if i >= 7209 and any(m in l for m in ['CONFIG_D', 'Driver:', 'configd.c', 'FILE_D', 'My configuration', 'CONFIG_D.daemon', 'all services']):
        print('L%d: %s' % (i+1, l[:170]))
    if i > 30000: break
