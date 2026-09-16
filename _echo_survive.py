# -*- coding: utf-8 -*-
"""Heartbeat SURVIVAL test with the REAL client's cadence.

The 问道 client sends CMD_ECHO (0x20D2, body = its own monotonic ms tick) every
~10 s (measured on a live session: 10000/10010/10016 ms deltas) and hangs up a
few hundred ms after an echo it never got a reply to.  gs closes a client whose
time_receive_echo has not been refreshed for 300 s (scansocketd::daemon).

This replays that cadence for N seconds and reports every MSG_REPLY_ECHO
(0xF9C3) plus any close, so "角色过几分钟掉线" is measurable instead of guessed.

usage: python -X utf8 _echo_survive.py [acct] [pwd] [char] [seconds]
"""
import importlib.util
import io
import struct
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
SECS = int(sys.argv[4]) if len(sys.argv) > 4 else 400
PERIOD = 10.0

s = None
for attempt in range(1, 8):
    cand = ll.Session(ACCT, PWD, CHAR, tag='E%d' % attempt)
    try:
        cand.connect(window=25)
    except Exception as e:
        print('connect attempt %d failed: %s' % (attempt, e), flush=True)
        time.sleep(5)
        continue
    if cand.uid is not None or cand.pos is not None:
        s = cand
        break
    cand.close()
    time.sleep(5)
if s is None:
    print('could not enter the world', flush=True)
    sys.exit(1)

print('in world: uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)

t0 = time.time()
echoes = replies = 0
dead = None
while time.time() - t0 < SECS:
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    s.g.sendall(ep.frame(0x20D2, struct.pack('>I', tick)))
    echoes += 1
    t1 = time.time()
    got = False
    while time.time() - t1 < PERIOD:
        r = ep.read_pkt(s.g, 0.5)
        if r is None:
            dead = time.time() - t0
            break
        if r[0] == 'timeout':
            continue
        if r[0] == 0xF9C3:
            replies += 1
            got = True
            break
    if dead is not None:
        break
    if echoes <= 3 or echoes % 6 == 0:
        print('[%6.1fs] echo#%-3d tick=%d -> %s' %
              (time.time() - t0, echoes, tick, '0xF9C3' if got else 'NO REPLY'), flush=True)

print('---- result ----', flush=True)
print('echoes sent = %d, 0xF9C3 received = %d' % (echoes, replies), flush=True)
if dead is None:
    print('SURVIVED %.0fs (never disconnected)' % (time.time() - t0), flush=True)
else:
    print('DISCONNECTED after %.1fs (echo#%d)' % (dead, echoes), flush=True)
s.close()
