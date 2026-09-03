# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa fast lines:', len(lines))
for l in lines[-15:]:
    print(l[:170])
import re
# was aaa able to serve? look for accept / dispatch / master-phase done
for i, l in enumerate(lines):
    if re.search(r'master-phase.*2->3|Done of init_service|Load: /aaa/daemons/configd|recv-accept|dispatch\] cmd=0x4470', l):
        print('L%d: %s' % (i+1, l[:150]))
