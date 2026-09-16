# -*- coding: utf-8 -*-
"""Cell-by-cell blocking probe (reuses _walk_probe's login, which does yield the
player id).

A rejected 0xFD72 move is answered with 0x1155 refresh_current_pos (carrying the
authoritative x/y); an accepted move is silent.  So scanning single cells east /
north maps the blocking geometry around the player.

usage: python -X utf8 _cell_scan2.py [acct] [pwd] [char] [cols]
"""
import importlib.util
import io
import struct
import sys
import time

spec = importlib.util.spec_from_file_location('wp', '_walk_probe.py')
wp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wp)
ep = wp.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'fix1'
COLS = int(sys.argv[4]) if len(sys.argv) > 4 else 24

wp.ACCT = ACCT
wp.PWD = PWD
wp.CHAR = CHAR
wp.NAMEB = CHAR.encode('gbk')

s, g = wp.login()
c = wp.Conn(g)
c.pump(25)
if not c.uid:
    print('!! no player id - abort', flush=True)
    sys.exit(1)
p = c.query_pos()
mid, sid, name, x, y, d = p
print('uid=0x%X map_id=%d room=%s pos=(%d,%d)' % (c.uid, mid, name, x, y), flush=True)


def try_cell(nx, ny):
    c.move(mid, [(nx, ny)])
    t0 = time.time()
    while time.time() - t0 < 0.6:
        r = ep.read_pkt(g, 0.2)
        if r is None:
            return None, None
        if r[0] == 'timeout':
            continue
        if r[0] == 0x1155 and len(r[1]) >= 21:
            return False, wp.parse_pos(r[1])
    return True, None


print('--- east from (%d,%d) ---' % (x, y), flush=True)
out = []
for k in range(COLS):
    nx = x + k + 1
    ok, pos = try_cell(nx, y)
    if ok is None:
        out.append('%d:DISCONNECT' % nx)
        break
    if ok:
        out.append('%d:ok' % nx)
    else:
        out.append('%d:BLOCK%s' % (nx, ('(real %d,%d)' % (pos[3], pos[4])) if pos else ''))
        if pos:
            x, y = pos[3], pos[4]
            break
print('   ' + ' '.join(out), flush=True)
cx, cy = x, y
print('--- north from (%d,%d) ---' % (cx, cy), flush=True)
out = []
for k in range(COLS):
    ny = cy + k + 1
    ok, pos = try_cell(cx, ny)
    if ok is None:
        out.append('%d:DISCONNECT' % ny)
        break
    if ok:
        out.append('%d:ok' % ny)
    else:
        out.append('%d:BLOCK%s' % (ny, ('(real %d,%d)' % (pos[3], pos[4])) if pos else ''))
        break
print('   ' + ' '.join(out), flush=True)
g.close()
s.close()
