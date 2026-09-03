# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i in range(3350550, min(len(lines), 3350745)):
    l = lines[i]
    if any(k in l for k in ['calculate', 'random_assign', 'efun idx=28', 'formula', 'qianyuanshan', 'jinguang', 'express', 'sscanf', 'atoi', 'int', 'error']):
        print('L%d: %s' % (i+1, l[:190]))
