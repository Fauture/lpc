# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find all register_post_init for longgong/douchong + their do_init execution markers
targets = ['longgong_xianyi', 'douchong_dahui', 'douchong_tiaozhansai']
reg = []
for i, l in enumerate(lines):
    if 'register_post_init' in l and any(t in l for t in targets):
        reg.append((i+1, l[:150]))
print('register_post_init hits:', len(reg))
for r in reg[:10]:
    print('L%d: %s' % r)
# search do_init invocation of these (post_init funcs loop calls closures; look for owner-init or fn call)
for t in targets:
    cnt = 0
    for i, l in enumerate(lines):
        if 'do_init' in l and t in l:
            cnt += 1
            if cnt <= 5:
                print('L%d: %s' % (i+1, l[:160]))
    print(t, 'do_init lines:', cnt)
