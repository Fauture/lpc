# -*- coding: utf-8 -*-
"""Search in-project decompiled LPC sources for a regex, printing path:line: text.

usage: python -X utf8 _lpcgrep.py <regex> [--base gs] [--files substr]
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(ROOT, 'linux')

args = [a for a in sys.argv[1:] if not a.startswith('--')]
opts = [a for a in sys.argv[1:] if a.startswith('--')]
pat = re.compile(args[0] if args else r'get_id')
sub = None
base_filter = None
for o in opts:
    if o.startswith('--files'):
        sub = o.split('=', 1)[1] if '=' in o else None
    if o.startswith('--base'):
        base_filter = o.split('=', 1)[1] if '=' in o else None

count = 0
for root, dirs, files in os.walk(BASE):
    for f in files:
        if not f.endswith('.c'):
            continue
        p = os.path.join(root, f)
        pn = p.replace('\\', '/')
        if sub and sub not in pn:
            continue
        if base_filter and ('lib_%s32-c' % base_filter) not in pn:
            continue
        try:
            t = io.open(p, encoding='gbk', errors='replace').read()
        except Exception:
            continue
        for i, line in enumerate(t.splitlines(), 1):
            if pat.search(line):
                rel = p.replace(ROOT + os.sep, '').replace('\\', '/')
                sys.stdout.write('%s:%d: %s\n' % (rel, i, line.strip()[:150]))
                count += 1
                if count >= 200:
                    sys.exit(0)
