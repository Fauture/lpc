# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('total lines:', len(lines))
# find douchong_dahui do_init execution
for i, l in enumerate(lines):
    if 'douchong_dahui' in l and ('do_init' in l or 'register_post_init' in l or 'owner-init' in l or 'postinit' in l or 'cur=create' in l):
        print('L%d: %s' % (i+1, l[:190]))
