# -*- coding: utf-8 -*-
import sys, time, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_v.txt'
for _ in range(150):
    time.sleep(5)
    try:
        with open(path, 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
    except Exception:
        continue
    # my_info max_user value
    for m in re.finditer(r'max_user=([^,]+)', text):
        print('max_user setting =', repr(m.group(1)))
        break
    mu = re.search(r'Max user permit to login: (\d+)', text)
    if mu:
        print('MAX_USER_PERMIT:', mu.group(1))
        break
print('done')
