# -*- coding: utf-8 -*-
"""List command-table entries whose LPC name matches any of the given keys."""
import re
import sys

L = open('src/game_service.cpp', 'rb').read().decode('utf-8', 'replace').split('\n')
pat = re.compile(r'^\s*\{\s*(0x[0-9A-Fa-f]+)\s*,\s*(ATTRIB_[A-Z_]+)\s*,\s*"([^"]+)"')
keys = sys.argv[1:] or ['apply']
for i, l in enumerate(L):
    m = pat.match(l)
    if m and any(k in m.group(3) for k in keys):
        print('%6d %-8s %-14s %s' % (i + 1, m.group(1), m.group(2), m.group(3)))
