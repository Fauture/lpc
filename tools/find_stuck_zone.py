# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find the LAST zone loadobj-cop or construct activity
last_zone = 0
last_line = ''
for i, l in enumerate(lines):
    if re.search(r'loadobj-cop.*zone|Load: /gs/zone/', l):
        last_zone = i
        last_line = l[:150]
print('last zone activity L%d: %s' % (last_zone+1, last_line))
# last sched-park (what master waits on)
for i in range(len(lines)-1, max(0, len(lines)-5000), -1):
    if 'sched-park] __master__' in lines[i] or 'sched-park] __init__' in lines[i]:
        print('L%d: %s' % (i+1, lines[i][:170]))
        break
