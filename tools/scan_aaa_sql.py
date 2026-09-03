# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_aaaAX_t2.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# find cmd_cfg_read_etc execution: select from config / mysql / agent send_message
for i, l in enumerate(lines):
    if re.search(r'select \* from config|mysql_select_single|cmd_cfg_read_etc\.c.*read_etc|cur=read_etc|连接 ADB|9331|9329|send_message\(9331|send_message\(9329|cmd_cfg_read_etc.*->|dist_name.*section', l):
        print('L%d: %s' % (i+1, l[:200]))
