# -*- coding: utf-8 -*-
import sys, time, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt'
for _ in range(180):
    time.sleep(5)
    try:
        with open(path, 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
    except Exception:
        continue
    d = re.search(r'Driver: (\S+)', text)
    if d: print('DRIVER:', d.group(1))
    mu = re.search(r'Max user permit to login: (\d+)', text)
    if mu:
        print('MAX_USER:', mu.group(1))
        break
    m2 = re.search(r'max_user=([^,\]]+)', text)
    if m2: print('setting max_user=', repr(m2.group(1)))
print('done')
