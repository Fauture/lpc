# -*- coding: utf-8 -*-
"""Find duplicate cmd numbers in the C++ command tables."""
import re
import sys

path = sys.argv[1] if len(sys.argv) > 1 else 'src/game_service.cpp'
L = open(path, 'rb').read().decode('utf-8', 'replace').split('\n')
pat = re.compile(r'^\s*\{\s*(0x[0-9A-Fa-f]+)\s*,\s*(ATTRIB_[A-Z_]+)\s*,\s*"([^"]+)"')
seen = {}
for i, l in enumerate(L):
    m = pat.match(l)
    if m:
        k = int(m.group(1), 16)
        seen.setdefault(k, []).append((i + 1, m.group(2), m.group(3)))
print('entries=%d unique=%d' % (sum(len(v) for v in seen.values()), len(seen)))
dups = {k: v for k, v in seen.items() if len(v) > 1}
print('duplicates=%d' % len(dups))
for k, v in sorted(dups.items()):
    print('  0x%04X %s' % (k, v))
for probe in (0x2403, 0x2407, 0x4401, 0xFD72, 0x1026):
    print('  0x%04X -> %s' % (probe, seen.get(probe)))
