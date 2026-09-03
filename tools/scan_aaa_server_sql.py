# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_k.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
for i, l in enumerate(lines):
    if re.search(r'mysql_query.*(from server\b|server\b)|fetch_config_list.*server|select \* from server', l):
        print('L%d: %s' % (i+1, l[:190]))
