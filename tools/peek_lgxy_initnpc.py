# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# trace longgong_xianyi do_init region (L398006 start) - look for init_all_npc / attach_form / read_etc / init_all_btimers / prepare_rank
found = 0
for i, l in enumerate(lines):
    if i+1 < 398006: continue
    if 'longgong_xianyi' in l or 'init_all_npc' in l or 'attach_form_file' in l or 'prepare_rank_player' in l or 'register_task_template' in l:
        print('L%d: %s' % (i+1, l[:170]))
        found += 1
        if found >= 30: break
print('found:', found)
