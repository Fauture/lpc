# -*- coding: utf-8 -*-
"""Compare the GS->client packet set of a FIRST login with a RE-login.

Motivation: after logging out and back in, the client shows no items and no
quest list until the player moves once.  Something the first entry sends is
missing on the second, so diff the two cmd histograms (and the order) rather
than guessing.

usage: python -X utf8 _relogin_diff.py [acct] [pwd] [char]
"""
import importlib.util
import io
import sys
import time

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
ep = ll.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'diff1'


def hist(cmds):
    h = {}
    for c in cmds:
        h[c] = h.get(c, 0) + 1
    return h


print('=== first login (%s) ===' % CHAR, flush=True)
a = ll.Session(ACCT, PWD, CHAR, tag='A')
a.connect(window=20)
a.pump(4)
n1 = len(a.cmds)
print('   uid=%s pos=%s cmds=%d dialogs=%s' %
      (('0x%X' % a.uid) if a.uid else None, a.pos, n1, a.dialogs), flush=True)
h1 = hist(a.cmds)

print('=== clean logout ===', flush=True)
a.g.sendall(ep.frame(0x3E2A, b''))
t0 = time.time()
while time.time() - t0 < 6:
    r = ep.read_pkt(a.g, 0.5)
    if r is None:
        break
a.close()
time.sleep(2)

print('=== second login ===', flush=True)
b = ll.Session(ACCT, PWD, CHAR, tag='B')
b.connect(window=25)
b.pump(4)
n2 = len(b.cmds)
print('   uid=%s pos=%s cmds=%d dialogs=%s' %
      (('0x%X' % b.uid) if b.uid else None, b.pos, n2, b.dialogs), flush=True)
h2 = hist(b.cmds)

print('=== histogram diff (first -> second) ===', flush=True)
for c in sorted(set(list(h1) + list(h2))):
    x, y = h1.get(c, 0), h2.get(c, 0)
    flag = '' if x == y else ('   <<< LOST %d' % (x - y) if x > y else '   >>> EXTRA +%d' % (y - x))
    print('   0x%04X  %3d -> %3d%s' % (c, x, y, flag), flush=True)

print('=== first-login order ===', flush=True)
print('   ' + ' '.join('0x%04X' % c for c in a.cmds), flush=True)
print('=== second-login order ===', flush=True)
print('   ' + ' '.join('0x%04X' % c for c in b.cmds), flush=True)
b.close()
