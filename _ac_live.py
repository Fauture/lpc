# -*- coding: utf-8 -*-
"""Evaluate every [ac-src] script found in a live gs log with _ac_eval.py and
compare with the [ac] sent 0x21B1 lines (answers as the server computed them).

usage: python -X utf8 _ac_live.py <logdir>
"""
import io
import os
import re
import subprocess
import sys

D = sys.argv[1] if len(sys.argv) > 1 else 'acfix4'
LOG = os.path.join(D, 'live_gs.txt')
lines = io.open(LOG, 'rb').read().decode('utf-8', 'replace').splitlines()

scripts = []
i = 0
while i < len(lines):
    if lines[i].strip() == '[ac-src] <<<':
        parts, j = [], i + 1
        while j < len(lines) and lines[j].strip() != '[ac-src] >>>':
            parts.append(lines[j])
            j += 1
        scripts.append(' '.join(x.strip() for x in parts))
        i = j
    i += 1

sent = []
for ln in lines:
    m = re.search(r'\[ac\] sent 0x21B1 cno=(\d+) x=(-?\d+) y=(-?\d+) answer=(-?\d+)', ln)
    if m:
        sent.append(tuple(int(g) for g in m.groups()))

print('scripts=%d  answers=%d' % (len(scripts), len(sent)))
ok = 0
for k, src in enumerate(scripts):
    r = subprocess.run([sys.executable, '-X', 'utf8', '_ac_eval.py'], input=src,
                       capture_output=True, text=True, cwd='.')
    py = (r.stdout or '').strip()
    srv = sent[k] if k < len(sent) else (0, 0, 0, 0)
    pyv = [int(x) for x in py.split()] if py else [0, 0, 0]
    match = (pyv == [srv[1], srv[2], srv[3]])
    ok += 1 if match else 0
    print('#%d %s server=(x=%d y=%d answer=%d) python=%s'
          % (k, 'MATCH' if match else 'DIFF ', srv[1], srv[2], srv[3], py))
print('matched %d/%d' % (ok, len(scripts)))
