# -*- coding: utf-8 -*-
import sys, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
STRS = ['Load all global feature', 'Load all global inherits', 'Load all global objects', 'Load all global cmds',
        'Start THREAD_D.all services', 'Start CONFIG_D.all services', 'Invoke registered initialization routines',
        'Driver load ok', 'Driver startup', 'Done of init_platform_module', 'My configuration',
        'load all global', 'post_init', 'load_dist_start_time', 'Master is', 'setup', 'start_service']
f = open(path, 'rb')
# read 0..260MB
f.seek(0)
raw = f.read(260_000_000)
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
txt = raw.decode('utf-16-le', errors='replace')
lines = txt.splitlines()
print('lines in 0..260MB: %d' % len(lines))
pat = re.compile('|'.join(re.escape(s) for s in STRS))
for i, l in enumerate(lines):
    m = pat.search(l)
    if m:
        print('%7d %s' % (i, l[:200]))
        if i > 400000:
            break
