# -*- coding: utf-8 -*-
"""Dump one corpus script (by index) so it can be inspected / hand-traced."""
import io
import os
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
DIRS = ['acx', 'acw', 'acy', 'acz', 'acgo', 'acfinal', 'acfix', 'acrun',
        'final3', 'final2', 'audit2', 'acv']
scripts = []
for d in DIRS:
    p = os.path.join(ROOT, d, 'live_gs.txt')
    if not os.path.exists(p):
        continue
    all_lines = io.open(p, 'rb').read().decode('utf-8', 'replace').splitlines()
    for i, ln in enumerate(all_lines):
        if ln.strip() == '[ac-src] <<<':
            parts, j = [], i + 1
            while j < len(all_lines) and all_lines[j].strip() != '[ac-src] >>>':
                parts.append(all_lines[j])
                j += 1
            scripts.append((d, ' '.join(x.strip() for x in parts)))
uniq, seen = [], set()
for d, s in scripts:
    if s not in seen:
        seen.add(s)
        uniq.append((d, s))

idx = int(sys.argv[1]) if len(sys.argv) > 1 else 12
d, src = uniq[idx]
print('### index %d  from %s  len=%d' % (idx, d, len(src)))
io.open(os.path.join(ROOT, '_ac_one.txt'), 'w', encoding='utf-8', newline='\n').write(src)
print(src)
