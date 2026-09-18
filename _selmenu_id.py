# -*- coding: utf-8 -*-
"""Send cmd_select_menu_item with an EXPLICIT id and report what the server does.

Reproduces the reported "距离NPC过远" without a client: after login the character
sits in 揽仙镇外; sending a menu selection for the character's OWN id (and for a
few other ids) shows whether the server takes the reserved-object path or falls
into the distance check.

usage: python -X utf8 _selmenu_id.py [acct] [pwd] [char] [id ...]
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
IDS = [int(x) for x in sys.argv[4:]] or [0, 1, 2, 3, 5]

s = ll.Session(ACCT, PWD, CHAR, tag='SM')
s.connect(window=12)
print('uid=%s (own id?) pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)
if not s.g:
    sys.exit(1)
s.pump(2)

if s.uid:
    IDS = list(dict.fromkeys(IDS + [s.uid]))

DIALOGS = []


def pump(sec):
    t0 = time.time()
    while time.time() - t0 < sec:
        r = ep.read_pkt(s.g, 0.3)
        if r is None:
            print('   !! closed', flush=True)
            return
        if r[0] == 'timeout':
            continue
        if r[0] == 0x1FE5:
            txt = r[1][1:].split(b'\x00')[0].decode('gbk', 'replace')
            DIALOGS.append(txt)
            print('   DIALOG: %s' % txt[:80], flush=True)
        elif r[0] == 0x2037:
            print('   MENU len=%d' % len(r[1]), flush=True)
        elif r[0] == 0xF0AB:
            print('   GUIDE len=%d %s' % (len(r[1]), r[1].hex()[:40]), flush=True)


for mid in IDS:
    item = '测试项'.encode('gbk')
    body = struct.pack('>I', mid) + bytes([len(item)]) + item + b'\x00'
    print('--- select_menu_item id=%d (0x%X)' % (mid, mid), flush=True)
    for cmd in (0x000A, 0x1042, 0x30CA, 0x215E):
        s.g.sendall(ep.frame(cmd, body))
    pump(2.5)
s.close()
print('dialogs: %s' % [d[:40] for d in DIALOGS], flush=True)
