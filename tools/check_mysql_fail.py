# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# any mysql in the failing window
for i in range(3350600, min(len(lines), 3350745)):
    if 'mysql' in lines[i].lower() or 'query' in lines[i].lower():
        print('L%d: %s' % (i+1, lines[i][:180]))
# what object/zone is being built? find create/import for the monster
for i in range(3350000, min(len(lines), 3350745)):
    if re.search(r'import.*monster|setup_monster|monster.*import|basic\.o#4792.*(create|set)|level.*=.*\d|set args=2.*level', lines[i]):
        print('M L%d: %s' % (i+1, lines[i][:180]))
