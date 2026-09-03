# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# longgong do_init L398006. Look for clone_object_lgxy / misc/longgong_xianyi clone / write_content / filed around
for i, l in enumerate(lines):
    if 398006 <= i+1 <= 400500 and ('longgong' in l.lower() or 'clone_object_lgxy' in l or 'misc/longgong_xianyi' in l or 'challenge' in l):
        print('L%d: %s' % (i+1, l[:170]))
