# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find read_etc calls with lgxy_server and their results around L398006-398500
for i, l in enumerate(lines):
    if 398006 <= i+1 <= 399200 and ('lgxy_server' in l or 'read_etc' in l or 'match_server' in l or '_server_name' in l):
        print('L%d: %s' % (i+1, l[:170]))
