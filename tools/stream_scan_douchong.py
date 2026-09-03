# -*- coding: utf-8 -*-
# STREAMING scan: no full-file load into memory
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_err.txt'
target_start = 5489430
target_end = 5492000
with open(path, 'rb') as f:
    # seek near target: count lines quickly by scanning for the line marker is
    # expensive; instead read all but only keep target window (bounded).
    # We cannot seek by line; do one pass, print only the window + matched.
    line_no = 0
    buf = []
    for raw in f:
        line_no += 1
        if line_no < target_start: continue
        if line_no > target_end: break
        l = raw.decode('utf-8', errors='replace')
        buf.append(l)
        if len(buf) > 400:
            buf.pop(0)
print('window lines:', len(buf))
for l in buf:
    if any(k in l for k in ['recv', '2473', '2471', 'msg_cfg', 'raise_event', 'wake', 'ev_readetcd', 'read_etc', 'douchong', '1EB3D338', 'sched-park', 'timeout', 'suspend']):
        print(l.rstrip()[:200])
