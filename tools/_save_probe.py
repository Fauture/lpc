# -*- coding: utf-8 -*-
"""Clean logout + save check: login, wait, send cmd_logout (0x3E2A), wait, then
report the character's ddb rows (the bag/carry branch must contain the newbie
gift the char was given on entering the world)."""
import importlib.util
import io
import struct
import sys
import time

import pymysql

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
ep = ll.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'gift5'
WAIT = float(sys.argv[4]) if len(sys.argv) > 4 else 25.0


def rows():
    c = pymysql.connect(host='192.168.200.128', user='root', password='123456',
                        database='dl_ddb_1_test', charset='utf8')
    cur = c.cursor()
    cur.execute("SELECT name,branch,LENGTH(content),time FROM data WHERE path='user' ORDER BY name,branch")
    out = cur.fetchall()
    cur.execute("SELECT name,content FROM data WHERE path='user' AND branch='carry'")
    carry = cur.fetchall()
    c.close()
    return out, carry


print('=== before ===', flush=True)
r0, c0 = rows()
for r in r0:
    print('   ', r, flush=True)

s = ll.Session(ACCT, PWD, CHAR, tag='S')
s.connect(window=20)
print('logged in uid=%s pos=%s' % (s.uid, s.pos), flush=True)
s.pump(3)

# cmd_logout 0x3E2A: no body expected by _cmd_logout (client_no only)
s.g.sendall(ep.frame(0x3E2A, b''))
print('sent logout 0x3E2A', flush=True)
t0 = time.time()
while time.time() - t0 < 6:
    r = ep.read_pkt(s.g, 0.5)
    if r is None:
        print('   connection closed by server', flush=True)
        break
    if r[0] != 'timeout':
        print('   recv 0x%04X len=%d' % (r[0], len(r[1])), flush=True)
s.close()

print('=== waiting %.0fs for the save ===' % WAIT, flush=True)
time.sleep(WAIT)
r1, c1 = rows()
for r in r1:
    print('   ', r, flush=True)
print('--- carry after ---', flush=True)
for n, ct in c1:
    print('   ', n, repr(ct[:600] if ct else ct), flush=True)
