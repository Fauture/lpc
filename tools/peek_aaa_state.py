# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa total:', len(lines))
for l in lines[-20:]:
    print(l[:180])
print('--- key markers ---')
import re
for i, l in enumerate(lines):
    if re.search(r'Load: /daemons/configd|init_sql|Done of init|master-phase|post_init|create ID pool|init_id_module', l):
        print('L%d: %s' % (i+1, l[:160]))
