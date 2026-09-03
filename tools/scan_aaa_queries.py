# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
# all config queries with name=
for i, l in enumerate(lines):
    m = re.search(r"mysql_query.*name='([^']*)'", l)
    if m:
        print('L%d: %s' % (i+1, l[:190]))
