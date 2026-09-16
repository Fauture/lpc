# -*- coding: utf-8 -*-
"""Which LPC function sends each packet, on a first login vs a re-login?

Arms nothing itself: `LPCVM_SEND` must be on (it is runtime-toggleable via
_probe_on\\LPCVM_SEND) so the 8000-line cap starts counting at the login under
test instead of being eaten by the boot traffic.

For each login it reads the gs log tail and builds cmd -> {sender: count},
then prints the diff.  The senders are the LPC objects/functions to read next.

usage: python -X utf8 _sender_probe.py [acct] [pwd] [char] [wait] [log]
"""
import importlib.util
import io
import re
import sys
import time

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
ep = ll.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'fix1'
WAIT = int(sys.argv[4]) if len(sys.argv) > 4 else 30
LOG = sys.argv[5] if len(sys.argv) > 5 else 'mv24/live_gs.txt'

RX = re.compile(r'\[send\] t=(\d+)ms cno=(\d+) cmd=0x([0-9A-Fa-f]+) body=(\d+) by=(\S+)')


def mark():
    try:
        return sum(1 for _ in open(LOG, 'rb'))
    except Exception:
        return 0


def senders_since(n):
    out = {}
    order = []
    with open(LOG, 'rb') as f:
        for i, line in enumerate(f):
            if i < n:
                continue
            m = RX.search(line.decode('gbk', 'replace'))
            if not m:
                continue
            cmd = int(m.group(3), 16)
            by = m.group(5)
            out.setdefault(cmd, {})
            out[cmd][by] = out[cmd].get(by, 0) + 1
            order.append(cmd)
    return out, order


def login(tag):
    for attempt in range(1, 8):
        s = ll.Session(ACCT, PWD, CHAR, tag=tag)
        try:
            s.connect(window=20)
        except Exception as e:
            print('  %s try%d failed: %s' % (tag, attempt, e), flush=True)
            time.sleep(5)
            continue
        if s.uid is not None or s.pos is not None:
            return s
        s.close()
        time.sleep(5)
    return None


m = mark()
print('--- login #1 ---', flush=True)
a = login('A')
print('   uid=%s pos=%s' % (('0x%X' % a.uid) if a and a.uid else None, a.pos if a else None), flush=True)
time.sleep(2)
sa, oa = senders_since(m)
print('   [send] lines: %d, distinct cmds: %d' % (len(oa), len(sa)), flush=True)

print('--- clean logout ---', flush=True)
try:
    a.g.sendall(ep.frame(0x3E2A, b''))
    t0 = time.time()
    while time.time() - t0 < 8:
        r = ep.read_pkt(a.g, 0.5)
        if r is None:
            break
except Exception:
    pass
a.close()
print('--- wait %ds ---' % WAIT, flush=True)
time.sleep(WAIT)

m2 = mark()
print('--- login #2 ---', flush=True)
b = login('B')
print('   uid=%s pos=%s' % (('0x%X' % b.uid) if b and b.uid else None, b.pos if b else None), flush=True)
time.sleep(2)
sb, ob = senders_since(m2)
print('   [send] lines: %d, distinct cmds: %d' % (len(ob), len(sb)), flush=True)

print('=== cmd: login#1 -> login#2   (sender: count) ===', flush=True)
for cmd in sorted(set(list(sa) + list(sb))):
    x = sa.get(cmd, {})
    y = sb.get(cmd, {})
    nx = sum(x.values())
    ny = sum(y.values())
    mark_ = ''
    if nx != ny:
        mark_ = '   <<< LOST %d' % (nx - ny) if nx > ny else '   >>> EXTRA %d' % (ny - nx)
    if not mark_ and x == y:
        continue
    print('  0x%04X %2d -> %2d%s' % (cmd, nx, ny, mark_), flush=True)
    if mark_ or x != y:
        for k in sorted(set(list(x) + list(y))):
            print('        %-52s %3d -> %3d' % (k[:52], x.get(k, 0), y.get(k, 0)), flush=True)

print('=== full map for the interesting cmds ===', flush=True)
for cmd in (0x5103, 0x09FF, 0x2603, 0x39F1, 0x2421, 0x2821, 0xFD1D, 0x0092,
            0x1A03, 0x1FB9, 0x2351, 0x2909, 0x3DF3, 0xFA1D, 0xF05D):
    x = sa.get(cmd, {})
    y = sb.get(cmd, {})
    if not x and not y:
        continue
    print('  0x%04X  A=%s' % (cmd, x), flush=True)
    print('           B=%s' % (y,), flush=True)
if b:
    b.close()
