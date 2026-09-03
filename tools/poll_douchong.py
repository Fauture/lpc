# -*- coding: utf-8 -*-
import sys, time, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = r'C:\Users\Administrator\Desktop\lpc\logs_gsAX_w.txt'
for _ in range(240):
    time.sleep(6)
    try:
        with open(path, 'rb') as f:
            data = f.read()
        text = data.decode('utf-8', errors='replace')
    except Exception:
        continue
    # douchong do_init executed and its init_all_npc?
    d = re.search(r'do_init on /gs/daemons/tasks/douchong_dahui\.o', text)
    if d:
        print('DOUCHONG DO_INIT STARTED')
    if re.search(r'douchong_dahui\.o.*owner-init DONE|DONE do_init on /gs/daemons/tasks/douchong_dahui', text):
        print('DOUCHONG DO_INIT DONE')
        break
print('done at size', len(text))
