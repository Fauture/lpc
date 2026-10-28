# -*- coding: utf-8 -*-
"""Drive the character into a fight and dump the combat packets.

Login is headless (see _login_lib.py).  The fight itself is started by the
server exactly like it starts for the real client: the character walks around a
wild map (揽仙镇外 carries a monster list) until a monster engages, which the
server announces with 0x1DE5 + 0xF0A9.

usage (cwd must be tools/):
  python -X utf8 _combat_drive.py [acct] [pwd] [char] <stage> [args]
stages:
  dump                 log in, list every packet with names / exits / sights
  enter <room>         log in and 过图 to <room>
  hunt <room> <steps>  log in, 过图 to <room>, then walk <steps> cells
  watch <secs>         log in and pump <secs> seconds standing still
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
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'soak1'
STAGE = sys.argv[4] if len(sys.argv) > 4 else 'dump'
ARG1 = sys.argv[5] if len(sys.argv) > 5 else ''
ARG2 = int(sys.argv[6]) if len(sys.argv) > 6 else 60

NAMES = {0x20D1: 'DRAMA', 0x107B: 'TALK-DRAMA', 0x1FE5: 'DIALOG', 0x2603: 'TASKLOG',
         0x115D: 'MOVE-EVENT', 0x2037: 'MENU', 0x3393: 'SYSLINE', 0x0A0F: 'TEMP-NPC-UNREG',
         0xE003: 'EXITS', 0x1155: 'POS', 0xF0AB: 'GUIDE', 0x2173: 'SHOP-GOODS',
         0x1141: 'OBJ-LIST', 0x0A0E: 'TEMP-NPC-REG', 0xF05D: 'UID', 0x20D2: 'ECHO',
         0x115F: 'NPC-IN-SIGHT', 0x1160: 'OBJ-IN-SIGHT', 0xF061: 'CHARLIST',
         0x1DE5: 'COMBAT-START', 0xF0A9: 'COMBAT-MEMBERS', 0x1DE6: 'COMBAT-START2',
         0xF0AA: 'COMBAT-MEMBERS2', 0x1DE7: 'COMBAT-ROUND'}

COMBAT_CMDS = (0x1DE5, 0x1DE6, 0xF0A9, 0xF0AA, 0x1DE7)
state = {'mid': None, 'x': None, 'y': None, 'uid': None, 'combat': False}
seen = []
_orig = ep.read_pkt


def rec(sock, t):
    r = _orig(sock, t)
    if r and r[0] != 'timeout':
        seen.append((r[0], r[1]))
        if r[0] == 0x1155 and len(r[1]) >= 21:
            try:
                mid, sid, name, x, y, d = ll.parse_pos(r[1])
                state['mid'], state['x'], state['y'] = mid, x, y
            except Exception:
                pass
        elif r[0] == 0xF05D and len(r[1]) >= 4:
            state['uid'] = struct.unpack('>I', r[1][:4])[0]
        elif r[0] in COMBAT_CMDS:
            state['combat'] = True
            print('  >>> %s 0x%04X len=%d %s'
                  % (NAMES.get(r[0], '?'), r[0], len(r[1]), r[1].hex()), flush=True)
    return r


ep.read_pkt = rec


def gbk(b):
    return b.decode('gbk', 'replace')


def strings(b, minlen=2):
    return [gbk(p) for p in b.split(b'\x00') if len(p) >= minlen]


def dump(tag, full=False):
    print('--- %s: %d packets' % (tag, len(seen)), flush=True)
    for c, b in seen:
        nm = NAMES.get(c, '')
        if c in COMBAT_CMDS:
            print('   %-15s 0x%04X len=%-5d %s' % (nm, c, len(b), b.hex()), flush=True)
        elif c == 0x1FE5:
            print('   DIALOG    len=%-5d %s' % (len(b), gbk(b[1:].split(b'\x00')[0])[:70]), flush=True)
        elif c == 0x3393:
            print('   SYSLINE   len=%-5d %s' % (len(b), gbk(b[20:].split(b'\x00')[0])[:70]), flush=True)
        elif c == 0x20D2:
            continue
        elif c in (0xE003, 0x1141, 0x115F, 0x1160):
            print('   %-15s 0x%04X len=%-5d strings=%s' % (nm, c, len(b), strings(b)[:12]), flush=True)
        elif full:
            print('   %-15s 0x%04X len=%-5d %s' % (nm, c, len(b), b[:48].hex()), flush=True)


def pump(secs):
    """Pump for secs; returns True as soon as a combat packet was seen."""
    t0 = time.time()
    while time.time() - t0 < secs:
        r = ep.read_pkt(s.g, 0.25)
        if r is None:
            print('!! connection closed', flush=True)
            return False
        if state['combat']:
            return True
    return state['combat']


def step(mid, uid, nx, ny):
    body = (struct.pack('>I', uid) + struct.pack('>I', mid) + struct.pack('>H', 1)
            + struct.pack('>H', nx) + struct.pack('>H', ny)
            + struct.pack('>H', 1) + struct.pack('>I', int(time.time() * 1000) & 0xFFFFFFFF))
    s.g.sendall(ep.frame(0xFD72, body))


def main():
    global s
    s = ll.Session(ACCT, PWD, CHAR, tag='CD')
    s.connect(window=25)
    if not s.g:
        print('!! no game socket', flush=True)
        return 1
    print('uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)
    if s.uid:
        state['uid'] = s.uid

    if STAGE == 'dump':
        pump(4)
        dump('login', full=True)
        return 0

    if STAGE == 'watch':
        got = pump(ARG2)
        dump('watch')
        print('COMBAT=%s' % got, flush=True)
        return 0

    if STAGE in ('enter', 'hunt'):
        seen.clear()
        rb = ARG1.encode('gbk')
        s.g.sendall(ep.frame(0x1026, bytes([len(rb)]) + rb))
        print('-> 过图 %s' % ARG1, flush=True)
        got = pump(6)
        print('pos after 过图: mid=%s (%s,%s)' % (state['mid'], state['x'], state['y']), flush=True)
        if got:
            dump('combat')
            return 0
        if STAGE == 'enter':
            dump('enter')
            return 0
        if state['mid'] is None:
            print('!! no position', flush=True)
            return 1
        ax, ay = state['x'], state['y']
        print('hunting in map_id=%s around (%d,%d)' % (state['mid'], ax, ay), flush=True)
        ring = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1),
                (2, 0), (2, 2), (0, 2), (-2, 2), (-2, 0), (-2, -2), (0, -2), (2, -2)]
        for i in range(ARG2):
            dx, dy = ring[i % len(ring)]
            nx = max(0, ax + dx)
            ny = max(0, ay + dy)
            step(state['mid'], state['uid'], nx, ny)
            if pump(0.45):
                print('!! COMBAT after %d steps' % (i + 1), flush=True)
                pump(3)
                dump('combat')
                return 0
            if (i + 1) % 16 == 0:
                print('   %d steps, server pos=(%s,%s)'
                      % (i + 1, state['x'], state['y']), flush=True)
        dump('hunt-end')
        print('no combat after %d steps' % ARG2, flush=True)
        return 0

    print('unknown stage %s' % STAGE, flush=True)
    return 2


if __name__ == '__main__':
    rc = main()
    print('exit=%d' % rc, flush=True)
    sys.exit(rc)
