# -*- coding: utf-8 -*-
"""Grep C++ sources in src/ for a regex.

usage: python -X utf8 _srcgrep.py <regex> [limit]
"""
import io
import os
import re
import sys

pat = re.compile(sys.argv[1])
limit = int(sys.argv[2]) if len(sys.argv) > 2 else 40
n = 0
for f in sorted(os.listdir('src')):
    if not f.endswith(('.cpp', '.h')):
        continue
    for i, l in enumerate(io.open('src/' + f, encoding='utf-8', errors='replace').read().splitlines(), 1):
        if pat.search(l):
            print('%s:%d: %s' % (f, i, l.strip()[:150]))
            n += 1
            if n >= limit:
                sys.exit(0)
