# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
print('aaa lines:', len(lines))
for i, l in enumerate(lines):
    if re.search(r'dispatch\] cmd=0x4470|dispatch\] cmd=0x247[0-9a-f]|analyse cmd=0x4470|recv.*4470', l):
        print('L%d: %s' % (i+1, l[:190]))
# count dispatch lines
cnt = 0
for l in lines:
    if 'dispatch] cmd=' in l: cnt += 1
print('total dispatch:', cnt)
# all dispatch cmd values
cmds = {}
for l in lines:
    m = re.search(r'dispatch\] cmd=0x([0-9a-f]+)', l)
    if m: cmds[m.group(1)] = cmds.get(m.group(1), 0) + 1
print('dispatch cmds:', dict(sorted(cmds.items())))
