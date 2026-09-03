# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
# stream gs log: find douchong do_init read_etc result (was it woken?)
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt'
with open(path, 'rb') as f:
    line_no = 0
    in_window = False
    wake_seen = 0
    for raw in f:
        line_no += 1
        if line_no == 5489438:
            in_window = True
        if not in_window:
            if line_no > 5489438 + 30000: break
            continue
        if line_no > 5489438 + 30000: break
        l = raw.decode('utf-8', errors='replace')
        if any(k in l for k in ['recv]', 'msg_cfg_read_etc', 'raise_event', 'wake]', 'ev_readetcd', 'map_delete', 'return -1', 'douchong']):
            print('L%d: %s' % (line_no, l.rstrip()[:200]))
            wake_seen += 1
            if wake_seen > 40: break
print('done at line', line_no)
