# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('total lines:', len(lines))
# last postinit marker
pi = [m for m in re.finditer(r'\[postinit-sync\] (\S+?) on (\S+)', text)]
if pi:
    m = pi[-1]
    print('last postinit:', m.group(1), m.group(2))
# douchong?
for kw in ['douchong_dahui', 'guardd', 'longgong']:
    hits = [i+1 for i, l in enumerate(lines) if kw in l and 'do_init' in l]
    print(kw, 'do_init lines:', len(hits), hits[:3], hits[-3:] if len(hits)>3 else '')
