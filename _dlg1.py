# -*- coding: utf-8 -*-
"""Decode the 0x1FE5 dialogs sent to one connection, in time order.

usage: python -X utf8 _dlg1.py <logdir> <cno> [<t_from> <t_to>]

The line layout is
  [send] t=<ms> cno=<n> cmd=0x1fe5 body=<len> by=<obj>::<fn> hex=<HEX>
so hex= comes LAST - a regex that expects hex= before cmd= silently matches
nothing (which is exactly how a first version of this tool reported "no
dialogs" for a connection that had six).
"""
import io
import os
import re
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
logdir = sys.argv[1]
cno = sys.argv[2]
t_from = int(sys.argv[3]) if len(sys.argv) > 3 else 0
t_to = int(sys.argv[4]) if len(sys.argv) > 4 else 1 << 62

lines = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk',
                errors='replace').read().split('\n')
pat = re.compile(r'\[send\] t=(\d+)ms cno=(\d+) cmd=0x1fe5 body=(\d+) by=(\S+) hex=([0-9A-F]+)')
n = 0
for l in lines:
    m = pat.search(l)
    if not m or m.group(2) != cno:
        continue
    t = int(m.group(1))
    if not (t_from <= t <= t_to):
        continue
    try:
        txt = bytes.fromhex(m.group(5)).decode('gbk', errors='replace')
    except Exception:
        txt = '?'
    print('%9d body=%-4s %-46s %s' % (t, m.group(3), m.group(4).split('::')[-1], txt.strip()))
    n += 1
print('-- %d dialog(s) for cno=%s --' % (n, cno))
