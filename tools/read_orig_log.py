# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\linux\gs.log', 'rb') as f:
    data = f.read()
text = data.decode('gbk', errors='replace')
lines = text.splitlines()
print('orig total lines:', len(lines))
# find post_init markers
for i, l in enumerate(lines):
    if any(k in l for k in ['post_init', 'init_service', 'server started', 'all service', 'start ok', 'bind port', 'listening', 'Begin init', 'end init']):
        print('L%d: %s' % (i+1, l[:150]))
# print last 25 lines
print('--- last 25 ---')
for l in lines[-25:]:
    print(l[:150])
