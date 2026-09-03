# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
# stream from a tail window of 500MB
nbytes = 500 * 1024 * 1024
f = open(path, 'rb')
f.seek(0, 2)
size = f.tell()
start = max(0, size - nbytes)
f.seek(start)
raw = f.read(size - start)
f.close()
if raw[:2] == b'\xff\xfe':
    raw = raw[2:]
lines = raw.decode('utf-16-le', errors='replace').splitlines()
print('window lines', len(lines))
# last two prints
idx = [i for i, l in enumerate(lines) if 'create dist start time failed' in l]
print('prints in window:', len(idx))
a = idx[-2]
b = idx[-1]
print('between print at %d and %d' % (a, b))
# print lines between, filtered to interesting (master or filed or read/create/queue/send/recv)
import re
keep = re.compile(r'__master__|filed|read_content|create_content|do_operate|queue|send_to_client|\[send\]|\[recv\]|readcontent|dba|load_dist|sleep|wait_event|raise_event|suspend|sched-park|sched-run|sched-call|mysql|connect|dist_start_time|printf.*dist|efun idx=431|efun idx=95|delete_temp|express_|call_other')
out = [l for i, l in enumerate(lines[a:b+1]) if keep.search(l)]
print('filtered lines between:', len(out))
for l in out[:220]:
    print(l[:200])
