# -*- coding: utf-8 -*-
"""Logout -> re-login cycle that reproduces the 「暂时无法登录（9026）」stun.

usage: python -X utf8 _relogin_cycle.py [acct] [pwd] [char]
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
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'cyc1'

print('=== session A: enter world ===', flush=True)
a = ll.Session(ACCT, PWD, CHAR, tag='A')
a.connect(window=20)
print('   A uid=%s pos=%s' % (('0x%X' % a.uid) if a.uid else None, a.pos), flush=True)
a.pump(2)

print('=== clean logout (0x3E2A) ===', flush=True)
a.g.sendall(ep.frame(0x3E2A, b''))
t0 = time.time()
while time.time() - t0 < 8:
    r = ep.read_pkt(a.g, 0.5)
    if r is None:
        print('   closed after %.1fs' % (time.time() - t0), flush=True)
        break
    if r[0] != 'timeout':
        print('   recv 0x%04X' % r[0], flush=True)
a.close()

print('=== immediate re-login (tries every 5s) ===', flush=True)
t1 = time.time()
tryno = 0
while time.time() - t1 < 90:
    tryno += 1
    b = ll.Session(ACCT, PWD, CHAR, tag='B%d' % tryno)
    try:
        b.connect(window=10)
        entered = b.uid is not None or b.pos is not None
        print('[+%5.1fs] try#%d uid=%s pos=%s dialogs=%s' %
              (time.time() - t1, tryno, ('0x%X' % b.uid) if b.uid else None, b.pos, b.dialogs), flush=True)
        if entered:
            print('RE-LOGIN OK after %.1fs (%d tries)' % (time.time() - t1, tryno), flush=True)
            b.close()
            break
    except Exception as e:
        print('[+%5.1fs] try#%d error %s' % (time.time() - t1, tryno, e), flush=True)
    finally:
        try:
            b.close()
        except Exception:
            pass
    time.sleep(5)
else:
    print('still stunned after 90s', flush=True)
