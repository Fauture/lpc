# -*- coding: utf-8 -*-
"""Cell-by-cell blocking map: try single-cell 0xFD72 moves and report which are
REJECTED (a rejection is answered with 0x1155 refresh_current_pos, an acceptance
is silent).

usage: python -X utf8 _cell_scan.py [acct] [pwd] [char] [dx] [dy] [cols]
        dx/dy = scan direction (default east then north), cols = how many cells
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
COLS = int(sys.argv[4]) if len(sys.argv) > 4 else 20

s = None
for attempt in range(1, 8):
    cand = ll.Session(ACCT, PWD, CHAR, tag='C%d' % attempt)
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
# The player id arrives in the 0xF05D that carries the character's own name;
# a slow world load can push it past connect()'s window.
for _ in range(12):
    if s.uid is not None:
        break
    s.pump(5)
if s.uid is None and s.pos is None:
    print('no uid and no pos - abort', flush=True)
    sys.exit(1)
print('uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)
if s.pos is None:
    s.query_pos()
else:
    pass
print('uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)
mid, sid, name, x, y, d = s.pos


def try_cell(nx, ny):
    """Send one single-cell move; True = accepted (silence), False = rejected."""
    body = (struct.pack('>I', s.uid) + struct.pack('>I', mid) + struct.pack('>H', 1)
            + struct.pack('>H', nx) + struct.pack('>H', ny)
            + struct.pack('>H', 1) + struct.pack('>I', int(time.time() * 1000) & 0xFFFFFFFF))
    s.pos = None
    s.g.sendall(ep.frame(0xFD72, body))
    t0 = time.time()
    while time.time() - t0 < 0.6:
        r = ep.read_pkt(s.g, 0.2)
        if r is None:
            return False, None
        if r[0] == 'timeout':
            continue
        if r[0] == 0x1155 and len(r[1]) >= 21:
            return False, ll.parse_pos(r[1])
    return True, None


print('--- scan east from (%d,%d) in %s (map_id=%d) ---' % (x, y, name, mid), flush=True)
cx, cy = x, y
line = []
for k in range(COLS):
    nx = x + k + 1
    ok, pos = try_cell(nx, y)
    if ok:
        line.append('%d:OK' % nx)
        cx = nx
    else:
        line.append('%d:BLOCK' % nx)
        if pos:
            line[-1] += '(real %d,%d)' % (pos[3], pos[4])
            x, y = pos[3], pos[4]
print('   ' + ' '.join(line), flush=True)
print('   ended at (%d,%d)' % (cx, cy), flush=True)
print('--- scan north from (%d,%d) ---' % (cx, cy), flush=True)
line = []
for k in range(COLS):
    ny = cy + k + 1
    ok, pos = try_cell(cx, ny)
    if ok:
        line.append('%d:OK' % ny)
    else:
        line.append('%d:BLOCK' % ny)
        if pos:
            line[-1] += '(real %d,%d)' % (pos[3], pos[4])
            break
print('   ' + ' '.join(line), flush=True)
s.close()
