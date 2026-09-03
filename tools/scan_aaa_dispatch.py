# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# after interface registration L5965, look for dispatch / handler / recv of 17520
for i, l in enumerate(lines):
    if re.search(r'dispatch\] cmd=0x4470|dispatch\] cmd=0x|analyse cmd=0x4470|recv.*4470|cmd_cfg_read_etc\(|cur=read_etc|assure_connection\("adb"|连接 ADB|adb.*fail|cmd_cfg_read_etc\.c', l):
        print('L%d: %s' % (i+1, l[:190]))
