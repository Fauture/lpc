# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# longgong do_init at L398006: print context around it
for i, l in enumerate(lines):
    if i+1 >= 398000 and i+1 <= 398080:
        print('L%d: %s' % (i+1, l[:170]))
