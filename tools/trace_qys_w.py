# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
hits = []
for i, l in enumerate(lines):
    if 'qianyuanshan' in l and ('loadobj-cop' in l or 'construct-done' in l or 'create error' in l or 'obj-init' in l or '__init__' in l or 'suspend' in l):
        hits.append((i+1, l[:175]))
print('qianyuanshan construction events:', len(hits))
for h in hits[:20]:
    print('L%d: %s' % h)
print('...')
for h in hits[-10:]:
    print('L%d: %s' % h)
