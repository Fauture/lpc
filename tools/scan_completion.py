# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_fast.txt', 'rb') as f:
    data = f.read()
text = data.decode('utf-8', errors='replace')
lines = text.splitlines()
# scan entire log for completion markers
for key in ['Driver load GS ok', 'init_service_module', 'allows user to login', 'Start service on port', 'post_init', 'init_after_loaded on /gs/daemons/taskd', 'LONGONG', 'sched-park', 'Cannot find']:
    cnt = 0
    for i, l in enumerate(lines):
        if key in l:
            cnt += 1
            if cnt <= 3:
                print('L%d: %s' % (i+1, l[:150]))
    print('==', key, 'count:', cnt)
