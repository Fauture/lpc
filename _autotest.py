# -*- coding: utf-8 -*-
"""Automated in-game tests (protocol level) for the reported bugs.

Each test logs in, drives the server through the same packets the client sends,
and prints PASS/FAIL with the raw evidence (the reply text is GBK-decoded, so the
"距离NPC过远" family is detected exactly as the player sees it).

usage:
  python -X utf8 _autotest.py [acct] [pwd] [char] <test> [args]
tests:
  dump                 log in and print every packet the server sends
  selfid               select a menu entry for the CHARACTER'S OWN id (bug 3/6)
  selid <id>           select a menu entry for an explicit object id
  openid <id>          open the NPC menu (0x7012) for an explicit id
  bag                  print the inventory packet summary (iid restore / bug 2)
  task                 print the task log packet summary
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

ACCT = sys.argv[1] if len(sys.argv) > 1 else '2'
PWD = sys.argv[2] if len(sys.argv) > 2 else '2'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
TEST = sys.argv[4] if len(sys.argv) > 4 else 'selfid'
ARG = sys.argv[5] if len(sys.argv) > 5 else None

NAMES = {0x1FE5: 'DIALOG', 0x2037: 'MENU', 0x2603: 'TASKLOG', 0x115D: 'MOVE-EVENT',
         0xF0AB: 'GUIDE', 0x2131: 'BAG?', 0x5103: 'BAG', 0x1155: 'POS', 0x20D1: 'DRAMA'}

seen = []
_orig = ep.read_pkt


def rec(sock, t):
    r = _orig(sock, t)
    if r and r[0] != 'timeout':
        seen.append((r[0], r[1]))
    return r


ep.read_pkt = rec
FAILS = []


def dialogs():
    out = []
    for c, b in seen:
        if c == 0x1FE5:
            out.append(b[1:].split(b'\x00')[0].decode('gbk', 'replace'))
    return out


def pump(sec):
    t0 = time.time()
    while time.time() - t0 < sec:
        r = ep.read_pkt(s.g, 0.3)
        if r is None:
            print('   !! connection closed')
            return
        if r[0] == 'timeout':
            continue


def login():
    s = ll.Session(ACCT, PWD, CHAR, tag='AT')
    s.connect(window=12)
    print('login: uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos))
    return s


def main():
    global s
    s = login()
    if not s.g:
        print('FAIL: no game socket')
        return 1
    pump(3)

    if TEST == 'dump':
        for c, b in seen:
            nm = NAMES.get(c, '')
            extra = ''
            if c == 0x1FE5:
                extra = b[1:].split(b'\x00')[0].decode('gbk', 'replace')[:60]
            print('0x%04X %-10s len=%-6d %s' % (c, nm, len(b), extra or b[:40].hex()))
        return 0

    if TEST in ('selfid', 'selid'):
        mid = s.uid if TEST == 'selfid' else int(ARG)
        item = '测试'.encode('gbk')
        body = struct.pack('>I', mid) + bytes([len(item)]) + item + b'\x00'
        seen.clear()
        for cmd in (0x000A, 0x1042, 0x30CA, 0x215E):
            s.g.sendall(ep.frame(cmd, body))
        pump(3)
        ds = dialogs()
        bad = [d for d in ds if '距离' in d]
        print('replies: %s' % [d[:50] for d in ds])
        if bad:
            print('FAIL: distance error for id=%d -> %s' % (mid, bad[0][:60]))
            FAILS.append(TEST)
        else:
            print('PASS: no distance error for id=%d' % mid)
        return 0

    if TEST == 'openid':
        seen.clear()
        s.g.sendall(ep.frame(0x7012, struct.pack('>I', int(ARG))))
        pump(3)
        menus = [(c, b) for c, b in seen if c == 0x2037]
        print('menu packets: %d' % len(menus))
        for c, b in menus:
            print('  len=%d head=%s' % (len(b), b[:40].hex()))
            txt = [p.decode('gbk', 'replace') for p in b.split(b'\x00') if len(p) >= 2]
            print('  strings: %s' % txt[:8])
        print('PASS' if menus else 'FAIL: no menu for id=%s' % ARG)
        return 0

    if TEST in ('bag', 'task'):
        want = 0x2131 if TEST == 'bag' else 0x2603
        hits = [(c, b) for c, b in seen if c == want]
        for c, b in hits:
            print('0x%04X len=%d head=%s' % (c, len(b), b[:48].hex()))
        print('%s: %d packet(s)' % (TEST, len(hits)))
        return 0

    print('unknown test %s' % TEST)
    return 2


if __name__ == '__main__':
    rc = main()
    print('FAILS=%s' % FAILS)
    sys.exit(rc)
