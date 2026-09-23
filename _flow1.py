# -*- coding: utf-8 -*-
"""Print the server-side wire+LPC timeline of ONE account's login/enter-game run.

usage: python -X utf8 _flow1.py <logdir> <acct|-> [<name>]

Shows, in time order: inbound packets (with their command code), the dialogs the
server sent (0x1FE5 decoded from GBK), the character-list replies (0xF061 body
length) and the LPC entry points (create_new_char / load_existed_char / ...).
That is what tells "the client retried because the server was slow" apart from
"the server answered and the client ignored it".
"""
import io
import os
import re
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

logdir = sys.argv[1] if len(sys.argv) > 1 else 'vF13'
acct = sys.argv[2] if len(sys.argv) > 2 else '-'
name = sys.argv[3] if len(sys.argv) > 3 else None

lines = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk',
                errors='replace').read().split('\n')

# clientNo of the account: from cmdLogin ("account=\"9\"") -> then find the cno
cno = None
if acct != '-':
    for i, l in enumerate(lines):
        m = re.search(r'\[cmdLogin\] cno=(\d+) account="([^"]*)"', l)
        if m and m.group(2) == acct:
            cno = m.group(1)
    if cno is None:
        print('account %s not found in %s' % (acct, logdir))
        raise SystemExit(1)
    print('account %s -> cno %s' % (acct, cno))


def dec(hexs):
    try:
        return bytes.fromhex(hexs).decode('gbk', errors='replace')
    except Exception:
        return '?'


ev = []
for l in lines:
    m = re.search(r'\[recv-accept\] t=(\d+)ms client=(\d+) n=(-?\d+)(?: ([0-9A-F]+))?', l)
    if m:
        t = int(m.group(1))
        c = m.group(2)
        h = m.group(3) or ''
        cmd = h[24:28].lower() if len(h) >= 28 else ''
        if cno is None or c == cno:
            ev.append((t, 'RECV cno=%-4s cmd=0x%s n=%s' % (c, cmd or '????', m.group(3))))
        continue
    m = re.search(r'\[send\] t=(\d+)ms cno=(\d+) cmd=0x1fe5 body=(\d+) hex=([0-9A-F]+)', l)
    if m and (cno is None or m.group(2) == cno):
        ev.append((int(m.group(1)), 'DIALOG(%s) %s' % (m.group(3), dec(m.group(4)))))
        continue
    m = re.search(r'\[send\] t=(\d+)ms cno=(\d+) cmd=0x(f061) body=(\d+)', l)
    if m and (cno is None or m.group(2) == cno):
        ev.append((int(m.group(1)), 'CHARTLIST body=%s' % m.group(4)))
        continue
    m = re.search(r't=(\d+)ms ev=BEGIN cno=(\d+) key=-?\d+ vm=\S+ entry=(\w+)', l)
    if m and (cno is None or m.group(2) == cno):
        ev.append((int(m.group(1)), 'LPC %s' % m.group(3)))
        continue
    if name and ('args=[create_new_char' in l or 'args=[load_existed_char' in l):
        m = re.search(r'args=\[(\w+) \| \(\{"([^"]*)"', l)
        if m and m.group(2) == name:
            ev.append((0, 'LPC-CALL %s(%s)' % (m.group(1), m.group(2))))

ev.sort(key=lambda x: x[0])
for t, what in ev:
    print('%9d  %s' % (t, what))
