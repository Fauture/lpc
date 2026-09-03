# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('gs lines:', len(lines))
# find read_etc request send and responses
for i, l in enumerate(lines):
    if re.search(r'17520|0x4470|send_to_client.*aaa|read_etc\b(?!_file)|ev_readetcd|wait_event.*readetc|recv.*2473|recv.*2471', l):
        print('L%d: %s' % (i+1, l[:190]))
        if i > 400000: break
