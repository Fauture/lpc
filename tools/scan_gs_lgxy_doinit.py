# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# trace configd read_etc calls: args (section,key) shown at call_other read_etc args=2
# find longgong do_init region and its read_etc
for i, l in enumerate(lines):
    if re.search(r'longgong_xianyi\.o cur=do_init|tasks/longgong_xianyi|lgxy_server', l):
        print('L%d: %s' % (i+1, l[:190]))
