# -*- coding: utf-8 -*-
"""Extract the 21 unique challenge scripts and survey their statement shapes."""
import io
import os
import re
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
            scripts.append(' '.join(x.strip() for x in parts))
uniq, seen = [], set()
for s in scripts:
    if s not in seen:
        seen.add(s)
        uniq.append(s)

with io.open(os.path.join(ROOT, '_ac_all.txt'), 'w', encoding='utf-8', newline='\n') as f:
    for k, s in enumerate(uniq):
        f.write('### %d\n%s\n' % (k, s))
print('scripts: %d -> _ac_all.txt' % len(uniq))

# ---- survey: bodies of challenge_server, statement by statement
lhs = {}
loops = {}
for k, s in enumerate(uniq):
    m = re.search(r'challenge_server\(int x, int y\)\s*\{(.*?)\}\s*int get_x', s, re.S)
    body = m.group(1) if m else s
    # split statements crudely at ';'
    for st in body.split(';'):
        st = st.strip()
        if not st:
            continue
        mm = re.match(r'^loop\s*\(([^)]*)\)\s*(.*)$', st, re.S)
        if mm:
            head, inner = mm.group(1).strip(), mm.group(2).strip()
            key = 'loop(%s)' % re.sub(r'[0-9]+', 'N', head)
            loops[key] = loops.get(key, 0) + 1
            # look at the body's LHS
            lm = re.match(r'^([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(\+=|-=|=|&=|\|=|\+\+|--)', inner)
            if lm:
                shape = '%s%s %s' % (lm.group(1), '[]' if lm.group(2) else '', lm.group(3))
                lhs[shape] = lhs.get(shape, 0) + 1
        else:
            lm = re.match(r'^([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(\+=|-=|=|&=|\|=|\+\+|--)', st)
            if lm:
                shape = '%s%s %s' % (lm.group(1), '[]' if lm.group(2) else '', lm.group(3))
                lhs[shape] = lhs.get(shape, 0) + 1

print('--- loop headers ---')
for k in sorted(loops):
    print('  %-58s x%d' % (k, loops[k]))
print('--- assignment shapes ---')
for k in sorted(lhs, key=lambda x: -lhs[x]):
    print('  %-24s x%d' % (k, lhs[k]))
