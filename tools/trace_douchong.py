# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# douchong do_init at L5489430 - trace vm=1EB3D338 activity after
vm = '1EB3D338'
seen = 0
for i, l in enumerate(lines):
    if i+1 >= 5489430 and vm in l:
        print('L%d: %s' % (i+1, l[:190]))
        seen += 1
        if seen >= 30: break
print('seen:', seen)
# find if douchong data written (douchong_dahui path in DB or write_content)
for i, l in enumerate(lines):
    if re.search(r'write_content.*douchong|"douchong_dahui"|douchong_dahui.*save\(|init_all_npc|challenge.*douchong', l):
        print('DBWRITE L%d: %s' % (i+1, l[:180]))
