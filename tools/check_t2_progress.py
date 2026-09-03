# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('gs lines:', len(lines))
# last postinit/do_init markers
last = None
for i, l in enumerate(lines):
    if 'do_init' in l and ('owner-resolve' in l or 'postinit' in l):
        last = (i+1, l[:160])
print('last postinit marker:', last)
# count msg_cfg_read_etc executions (successful read_etc responses)
succ = 0
for i, l in enumerate(lines):
    if 'msg_cfg_read_etc args=2' in l or 'msg_cfg_read_etc first' in l:
        succ += 1
print('msg_cfg_read_etc responses:', succ)
# find read_etc returning -1 or errors near longgong
for i, l in enumerate(lines):
    if re.search(r'Cannot find guardd|longgong|init_all_npc|douchong_dahui.*do_init', l):
        print('L%d: %s' % (i+1, l[:170]))
        if i > 2200000: break
