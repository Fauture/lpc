# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
for i, l in enumerate(lines):
    if re.search(r'adb|ADB|assure_connection|mysql|Database|dl_adb|connection.*adb|dbd|config', l, re.I):
        if 'interface' in l and 'add' not in l: continue
        print('L%d: %s' % (i+1, l[:190]))
        if i > 20000: break
