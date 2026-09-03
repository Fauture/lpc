# -*- coding: utf-8 -*-
"""Resolve EIP sample RVAs (tools/eip_sampler.py output) against lpcvm.map.
Usage: python tools/rva_resolve.py regress/eip_tally.txt [topN]
Prints the hottest functions by sample count with approximate names.
"""
import io, re, sys
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

lines = open(sys.argv[1], encoding='utf-8', errors='replace').read().splitlines()
base = 0
samples = []
stacks = []
in_stacks = False
for ln in lines:
    if ln.startswith('base '):
        base = int(ln.split()[1], 16)
        continue
    if ln.startswith('--stacks--'):
        in_stacks = True
        continue
    p = ln.split()
    if in_stacks:
        if len(p) >= 2:
            stacks.append((int(p[0]), [int(x, 16) for x in p[1:]]))
    elif len(p) == 2:
        samples.append((int(p[0], 16), int(p[1])))
if not base:
    print('no module base in file; guessing 0x400000'); base = 0x400000
print('module base=%x samples=%d stacks=%d' % (base, len(samples), len(stacks)))

# parse publics: "0001:00rva  symbol  size  .text"
funcs = []
pat = re.compile(r'^\s*0001:([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]+)\s')
for ln in open('build/Debug/lpcvm.map', encoding='utf-8', errors='replace'):
    m = pat.match(ln)
    if not m:
        continue
    rva = int(m.group(1), 16)
    name = m.group(2)
    funcs.append((rva, name))
funcs.sort()
print('map code symbols: %d' % len(funcs))

def undecorate(name):
    if name.startswith('?'):
        n = name[1:]
        parts = n.split('@')
        # parts[0] is the function name; parts[1] the first scope
        base_ = parts[0]
        scope = parts[1] if len(parts) > 1 and parts[1] and '?' not in parts[1] else ''
        return base_ + (('@' + scope.split('@@')[0]) if scope else '')
    if name.startswith('__catch$') or name.startswith('__unwind') or name.startswith('__ehhandler'):
        return name.split('$')[0]
    return name.split('?')[0]

import bisect
rv = [f[0] for f in funcs]
from collections import Counter
hot = Counter()
for eip, cnt in samples:
    rva = eip - base
    if rva < 0x1000:
        continue
    i = bisect.bisect_right(rv, rva) - 1
    if i < 0:
        hot['<before-first>'] += cnt
        continue
    frva, fname = funcs[i]
    # avoid attributing to the previous function when far below its end (~6k)
    nxt = rv[i + 1] if i + 1 < len(rv) else frva + 0x100000
    if rva - frva > nxt - frva:
        hot['<data>'] += cnt
        continue
    hot[undecorate(fname)] += cnt
total = sum(hot.values())
topn = int(sys.argv[2]) if len(sys.argv) > 2 else 40
for name, cnt in hot.most_common(topn):
    print('%6.2f%%  %6d  %s' % (100.0 * cnt / total, cnt, name))

if stacks:
    print('\n===== TOP CALL STACKS (leaf -> caller) =====')
    seen = {}
    for cnt, chain in stacks:
        names = tuple(undecorate(funcs[max(0, bisect.bisect_right(rv, r) - 1)][1]) if r >= 0x1000 else '<sys>' for r in chain)
        seen[names] = seen.get(names, 0) + cnt
    for names, cnt in sorted(seen.items(), key=lambda kv: -kv[1])[:25]:
        print('%6d  ' % cnt + ' <- '.join(names[:8]))
