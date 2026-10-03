# -*- coding: utf-8 -*-
"""Bag / quest packet probe: what does the server actually send at login, and
what changes on a RE-login?

User report: after re-entering the world the bag shows nothing (the items DO
exist - swapping one makes the whole bag appear) and the quest list is empty.
Capture the raw bodies of the packets that build those two panels on a first
login and on a re-login, then diff them.

Note: Session.connect() pumps internally, so the recording must happen inside
pump() - a separate pump afterwards sees only the idle stream (total pkts=0).

usage: python -X utf8 _bagtask_probe.py [acct] [pwd] [char]
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
WAIT = int(sys.argv[4]) if len(sys.argv) > 4 else 3

WATCH = [0x5103, 0x5105, 0x5109, 0x39F1, 0xF05D, 0xFA1D, 0x2603, 0x20D1,
         0xFD1D, 0x2421, 0x2821, 0xF9D3, 0x1155, 0xE003, 0x600A, 0x7007,
         0x211B, 0xF0B1, 0x3DF3, 0x09FF, 0xFA01, 0xF0FF]


class RecSession(ll.Session):
    """Session that records every packet received while pumping."""

    def __init__(self, *a, **k):
        ll.Session.__init__(self, *a, **k)
        self.seen = []

    def pump(self, secs, verbose=False, select_timeout=None):
        t0 = time.time()
        while time.time() - t0 < secs:
            r = ep.read_pkt(self.g, 0.4)
            if r is None:
                self.mark('GS closed the connection')
                return False
            if r[0] == 'timeout':
                continue
            self.seen.append((r[0], r[1]))
            self.cmds.append(r[0])
            if r[0] == 0x1155 and len(r[1]) >= 21:
                try:
                    self.pos = ll.parse_pos(r[1])
                except Exception:
                    pass
            if r[0] == 0xF05D and len(r[1]) >= 8 and self.nameb in r[1]:
                self.uid = struct.unpack('>I', r[1][:4])[0]
            if r[0] == 0x1FE5:
                txt = r[1][1:].split(b'\x00')[0].decode('gbk', 'replace')
                self.dialogs.append(txt)
                self.mark('DIALOG(0x1FE5): %s' % txt)
            if r[0] == 0xF061:
                c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
                self.mark('char list 0xF061 count=%d' % c2)
                if self.select:
                    if c2 == 0 and not self.created:
                        nb = self.nameb
                        self.g.sendall(ep.frame(0x205C, bytes([len(nb)]) + nb + bytes([1, 1, 1])))
                        self.created = True
                        self.mark('created char %s' % self.char)
                    elif c2 > 0 and not self.selected:
                        self.g.sendall(ep.frame(0x1060, ep.s8(self.char)))
                        self.selected = True
                        self.mark('selected char %s' % self.char)
        return True


def capture(tag, window=18):
    s = None
    for attempt in range(1, 8):
        cand = RecSession(ACCT, PWD, CHAR, tag='%s%d' % (tag, attempt))
        try:
            cand.connect(window=window)
        except Exception as e:
            print('  %s connect %d failed: %s' % (tag, attempt, e), flush=True)
            time.sleep(5)
            continue
        if cand.uid is not None or cand.pos is not None:
            s = cand
            break
        cand.close()
        time.sleep(5)
    if s is None:
        print('  %s: could not enter the world' % tag, flush=True)
        return None, None
    out = {}
    for c, b in s.seen:
        if c in WATCH:
            out.setdefault(c, []).append(b)
    print('=== %s: uid=%s pos=%s total pkts=%d ===' %
          (tag, ('0x%X' % s.uid) if s.uid else None, s.pos, len(s.seen)), flush=True)
    for c in WATCH:
        v = out.get(c)
        if not v:
            continue
        print('  0x%04X x%-2d lens=%s' % (c, len(v), [len(x) for x in v][:8]), flush=True)
        if c in (0x2603, 0x5103, 0xFD1D, 0x2821, 0x2421):
            for k, body in enumerate(v[:2]):
                print('        [%d] full(%d): %s' % (k, len(body), body.hex()), flush=True)
        else:
            print('        first: %s' % v[0][:72].hex(), flush=True)
    return s, out


print('--- first login ---', flush=True)
a, oa = capture('A')
if a:
    print('--- clean logout (0x3E2A) ---', flush=True)
    try:
        a.g.sendall(ep.frame(0x3E2A, b''))
        t0 = time.time()
        while time.time() - t0 < 6:
            r = ep.read_pkt(a.g, 0.5)
            if r is None:
                break
    except Exception:
        pass
    a.close()
    print('--- waiting %ds before the re-login ---' % WAIT, flush=True)
    time.sleep(WAIT)
    print('--- re-login ---', flush=True)
    b, ob = capture('B', window=18)
    if b and oa and ob:
        print('=== diff (first -> second) ===', flush=True)
        for c in WATCH:
            x = oa.get(c, [])
            y = ob.get(c, [])
            if not x and not y:
                continue
            mark = '' if len(x) == len(y) else ('   <<< LOST %d' % (len(x) - len(y)) if len(x) > len(y)
                                                else '   >>> EXTRA %d' % (len(y) - len(x)))
            print('  0x%04X %2d -> %2d  lens %s -> %s%s' %
                  (c, len(x), len(y), [len(t) for t in x][:6], [len(t) for t in y][:6], mark), flush=True)
            if x and y and x[0] != y[0]:
                print('        differs: A=%s' % x[0][:56].hex(), flush=True)
                print('                 B=%s' % y[0][:56].hex(), flush=True)
    if b:
        b.close()
