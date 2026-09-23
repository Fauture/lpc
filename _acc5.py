#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Per-run dialogs, counted correctly (GBK hex inside the 1FE5 payload).

    python -X utf8 _acc5.py <logdir>

The dialog codes are GBK text inside the hex body, so "9101" appears as the hex
`39313031`, not as the ASCII characters - which is why every earlier counter that
searched for the literal text reported 0 (and why the numbers quoted from the old
tool were wrong).

Runs are delimited by the first appearance of each character name; each run is one
login (one account).
"""
import io
import os
import re
import sys

logdir = sys.argv[1]
lines = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk',
                errors='replace').read().split('\n')

order = []
seen = set()
for i, l in enumerate(lines):
    m = re.search(r'args=\[create_new_char \| \(\{"([^"]+)"', l)
    if m and m.group(1) not in seen:
        seen.add(m.group(1))
        order.append((i, m.group(1)))

CODES = {'9101': '39313031', '9113': '39313133', '9112': '39313132'}
print('== %s: %d run(s) ==' % (logdir, len(order)))
for k, (start, nm) in enumerate(order):
    stop = order[k + 1][0] if k + 1 < len(order) else len(lines)
    seg = lines[start:stop]
    counts = {c: sum(1 for l in seg if 'cmd=0x1fe5' in l.lower() and h in l)
              for c, h in CODES.items()}
    creates = sum(1 for l in seg if 'args=[create_new_char' in l)
    loads = sum(1 for l in seg if 'args=[load_existed_char' in l)
    lists = [int(b) for b in re.findall(r'cmd=0xf061 body=(\d+)', '\n'.join(seg))]
    print('%-9s creates=%-3d loads=%-2d dialogs[9101=%d 9113=%d 9112=%d] lists=%s'
          % (nm, creates, loads, counts['9101'], counts['9113'], counts['9112'],
             lists[:6]))
