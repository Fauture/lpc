# -*- coding: utf-8 -*-
"""Look up the function at a given merged index in a .o, and its inherit table."""
import re
import subprocess
import sys

EXE = r'build\Release\lpcvm.exe'
path = sys.argv[1]
want = [int(x) for x in sys.argv[2:]] or []
out = subprocess.run([EXE, 'disasm', path], capture_output=True, text=True,
                     encoding='utf-8', errors='replace').stdout
lines = out.split('\n')
mode = None
own = []
inh = []
for l in lines:
    s = l.rstrip()
    if s.startswith('inherits:'):
        mode = 'inh'
        continue
    if s.startswith('functions:'):
        mode = 'fn'
        continue
    if s.startswith('names:'):
        mode = 'nm'
        continue
    if mode == 'inh' and re.match(r'^\s+\d+: ', s):
        inh.append(s.strip())
    elif mode == 'fn':
        m = re.match(r'^\s+(\d+): (\S+) dlt=(\d+) args=(\d+)', s)
        if m:
            own.append((int(m.group(3)), m.group(2), int(m.group(4))))
for h in lines[:4]:
    print(h.rstrip())
print('inherits:')
for i in inh:
    print('   ', i)
print('own functions (dlt -> name):')
for idx, name, argc in sorted(own):
    mark = '   <== wanted' if idx in want else ''
    print('   %5d %-36s args=%d%s' % (idx, name, argc, mark))
