# -*- coding: utf-8 -*-
import sys, time, os, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_k.txt'
found_keys = False
for _ in range(120):
    time.sleep(5)
    try:
        with open(path, 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
    except Exception:
        continue
    for m in re.finditer(r'\[game\] msg_cfg_my_info.*?keys=\[(.*?)\]', text):
        keys = m.group(1)
        print('MY_INFO KEYS:', keys[:600])
        found_keys = True
        break
    mu = re.search(r'Max user permit to login: (\d+)', text)
    if mu:
        print('MAX_USER:', mu.group(1))
    if found_keys:
        break
print('done')
