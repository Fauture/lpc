# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# Look at the last 2000 lines - what obj/fn are active?
from collections import Counter
cnt = Counter()
last5000 = lines[-5000:]
for l in last5000:
    m = re.search(r'obj=(\S+?\.o)', l)
    if m:
        cnt[m.group(1)] += 1
print('=== last 5000 lines activity ===')
for k, v in cnt.most_common(15):
    print('  %5d %s' % (v, k))
# any post_init cur= markers in last 5000
for l in last5000:
    if 'cur=do_init' in l or 'cur=post_init' in l or 'register_post_init' in l:
        print(l[:160])
        break
