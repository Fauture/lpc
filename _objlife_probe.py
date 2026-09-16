# -*- coding: utf-8 -*-
"""Is the character object actually destroyed at logout?

If the char object survives, every later login takes the light "reconnected"
path instead of the full start_game() path - which is exactly the reported
symptom set (task log/tip empty, shops empty, bag not re-synced).  The
observable difference on the server side is whether the login re-LOADS
/gs/clone/char/user.o (a fresh object) or reuses the live one.

usage: python -X utf8 _objlife_probe.py [acct] [pwd] [char] [wait]
"""
import importlib.util
import io
import os
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
LOG = sys.argv[5] if len(sys.argv) > 5 else 'mv23/live_gs.txt'


def mark():
    try:
        return sum(1 for _ in open(LOG, 'rb'))
    except Exception:
        return 0


def tail_since(n):
    out = []
    with open(LOG, 'rb') as f:
        for i, line in enumerate(f):
            if i < n:
                continue
            out.append(line.decode('gbk', 'replace').rstrip())
    return out


def interesting(lines):
    keys = ('clone/char/user.o', 'clone:gs/clone/char/user.o', 'ENTER reconnected',
            'reconnected', 'destroy', 'destruct', 'start_game', 'new_user',
            'callDestructors', '[life]', 'Load: /gs/clone/char/login.o')
    return [l for l in lines if any(k in l for k in keys)]


def login(tag):
    for attempt in range(1, 8):
        s = ll.Session(ACCT, PWD, CHAR, tag=tag)
        try:
            s.connect(window=20)
        except Exception as e:
            print('  %s try%d connect failed: %s' % (tag, attempt, e), flush=True)
            time.sleep(5)
            continue
        if s.uid is not None or s.pos is not None:
            return s
        s.close()
        time.sleep(5)
    return None


m0 = mark()
print('--- login #1 ---', flush=True)
a = login('A')
print('   uid=%s pos=%s' % (('0x%X' % a.uid) if a and a.uid else None, a.pos if a else None), flush=True)
la = interesting(tail_since(m0))
print('   server lines: %d interesting' % len(la), flush=True)
for l in la[:14]:
    print('     %s' % l[:150], flush=True)

print('--- clean logout (0x3E2A) ---', flush=True)
m1 = mark()
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
time.sleep(WAIT)
ld = interesting(tail_since(m1))
print('   logout window: %d interesting lines' % len(ld), flush=True)
for l in ld[:20]:
    print('     %s' % l[:150], flush=True)

m2 = mark()
print('--- login #2 (after %ds) ---' % WAIT, flush=True)
b = login('B')
print('   uid=%s pos=%s' % (('0x%X' % b.uid) if b and b.uid else None, b.pos if b else None), flush=True)
lb = interesting(tail_since(m2))
print('   server lines: %d interesting' % len(lb), flush=True)
for l in lb[:14]:
    print('     %s' % l[:150], flush=True)
if b:
    b.close()
