# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
# our gs early markers
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('=== our gs first 12000 lines: init module markers ===')
for i, l in enumerate(lines[:12000]):
    if re.search(r'Done of init_|Load all global|Load sub dir|Start .*\.all services|Driver:|PDB is|Connect to DAS|init_startup|compile module', l):
        print('L%d: %s' % (i+1, l[:160]))
