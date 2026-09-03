# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# around last zone activity, show master parks
for i in range(3865850, min(len(lines), 3865950)):
    l = lines[i]
    if any(k in l for k in ['sched-park', 'suspend', 'loadobj-cop', 'obj-init', 'construct-done', 'safe_load_object', 'Load: /gs/zone']):
        print('L%d: %s' % (i+1, l[:175]))
