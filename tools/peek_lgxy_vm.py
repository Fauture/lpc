# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find longgong do_init VM (1FD71030) activity after L398006
vm = '1FD71030'
seen = 0
for i, l in enumerate(lines):
    if i+1 >= 398006 and vm in l:
        print('L%d: %s' % (i+1, l[:170]))
        seen += 1
        if seen >= 15: break
print('seen vm activity:', seen)
