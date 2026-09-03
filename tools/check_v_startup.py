# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('gs lines:', len(lines))
markers = ['Start THREAD_D', 'Start CONFIG_D', 'Driver:', 'Load: /daemons/configd', 'My configuration', 'Name: ',
           'threadd', 'module_maintain', 'all services', 'CONFIG_D.daemon start', 'Load all inherited',
           'Max user permit', 'init_service_module', 'msg_cfg_my_info client', 'max_user=']
for i, l in enumerate(lines):
    for m in markers:
        if m in l:
            print('L%d: %s' % (i+1, l[:180]))
            break
    if i > 400000: break
