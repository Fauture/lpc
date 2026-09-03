# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_v.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
for i, l in enumerate(lines):
    if 'fetch_config_list' in l and 'server' in l and ('max_user' in l or '""' in l or '1200' in l):
        print('L%d: %s' % (i+1, l[:600]))
        break
# find max_user in any server row dump
for i, l in enumerate(lines):
    if 'max_user' in l and ('1200' in l or '1' in l):
        print('MAX L%d: %s' % (i+1, l[:400]))
        break
