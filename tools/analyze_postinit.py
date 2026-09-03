# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
# Find post_init invocation and what runs after
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('total lines:', len(lines))
# find Invoke line
start = None
for i, l in enumerate(lines):
    if 'Invoke registered' in l:
        start = i
        break
print('Invoke at line index', start)
# count post_init-related daemon do_init runs after start: sample do_init owners
from collections import Counter
cnt = Counter()
for l in lines[start:start+80000]:
    m = None
    import re
    m = re.search(r'do_init on (\S+)', l)
    if m:
        cnt[m.group(1)] += 1
print('distinct do_init owners in first 80k lines after Invoke:', len(cnt))
for k, v in cnt.most_common(15):
    print('  %5d %s' % (v, k))
