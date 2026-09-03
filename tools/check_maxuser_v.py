# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
for m in re.finditer(r'max_user=([^,]+)', text):
    print('max_user setting =', repr(m.group(1)))
    break
mu = re.search(r'Max user permit to login: (\d+)', text)
print('MAX_USER_PERMIT:', mu.group(1) if mu else 'not found')
# check if it got past logind
for pat in ['Load: /gs/daemons/logind', 'Load: /gs/daemons/sysinfod', 'post_init funcs']:
    print(pat, '->', 'FOUND' if pat in text else 'no')
