# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('fast total:', len(lines))
# around L398006-398200 show all gfn/cfu/efun/call_other activity
for i in range(398005, min(398300, len(lines))):
    l = lines[i]
    if any(k in l for k in ['[gfn]','[cfu]','[vm] efun','call_other','read_etc','send_to_client','wait_event','raise_event','clone_object','save','init_all_npc','register_task','attach_form']):
        print('L%d: %s' % (i+1, l[:170]))
