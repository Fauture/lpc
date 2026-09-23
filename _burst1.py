# -*- coding: utf-8 -*-
"""Did the 9101/9113 dialogs in the 20-ENTER run come from the server being slow?

Answers it from the SERVER's own timeline for one connection: it prints the
inbound command codes, the dialogs that were sent back (decoded), and the LPC
entry points, so "the server answered the first submit and then rejected the
duplicates" can be told apart from "the server never answered".

usage: python -X utf8 _burst1.py <logdir> <name>
"""
import io
import os
import re
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
logdir, name = sys.argv[1], sys.argv[2]

lines = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk',
                errors='replace').read().split('\n')

# the connection that created this character, then the window of its first run
cno = None
for l in lines:
    m = re.search(r'args=\[create_new_char \| \(\{"' + re.escape(name) + r'"', l)
    if m:
        m2 = re.search(r'vm=(\S+)', l)
        vm = m2.group(1) if m2 else None
        if vm:
            # find the [lg] line of the same clone, which carries no cno; the
            # cno comes from the nearest preceding cmdLogin for this account
            pass
        break

# simplest robust route: cmdLogin for the account that owns this character
for l in lines:
    m = re.search(r'\[cmdLogin\] cno=(\d+) account="([^"]*)"', l)
    if m:
        pass

ev = []
send_pat = re.compile(r'\[send\] t=(\d+)ms cno=(\d+) cmd=0x1fe5 body=(\d+) '
                      r'by=\S+ hex=([0-9A-F]+)')
recv_pat = re.compile(r'\[recv-accept\] t=(\d+)ms client=(\d+) n=(-?\d+)(?: ([0-9A-F]+))?')
for l in lines:
    m = send_pat.search(l)
    if m:
        try:
            txt = bytes.fromhex(m.group(4)).decode('gbk', errors='replace').strip()
        except Exception:
            txt = '?'
        ev.append((int(m.group(1)), m.group(2), 'DIALOG  ' + txt))
        continue
    m = recv_pat.search(l)
    if m:
        h = m.group(4) or ''
        cmd = h[24:28].lower() if len(h) >= 28 else '????'
        ev.append((int(m.group(1)), m.group(2), 'recv 0x%s n=%s' % (cmd, m.group(3))))
        continue
    if name and 'create_new_char' in l and name in l:
        t = re.search(r't=(\d+)ms', l)
        ev.append((int(t.group(1)) if t else 0, '-', 'LPC   create_new_char(%s)' % name))

# keep only the window around this character's creation
idx = [i for i, e in enumerate(ev) if 'create_new_char' in e[2]]
if idx:
    lo, hi = max(0, idx[0] - 12), min(len(ev), idx[-1] + 14)
    ev = ev[lo:hi]
cno_seen = [e[1] for e in ev if e[1] != '-']
print('connection(s) in window: %s' % sorted(set(cno_seen)))
for t, c, what in ev:
    print('%9d  cno=%-4s %s' % (t, c, what))
