# -*- coding: utf-8 -*-
"""Warehouse (仓库) repro: teleport into 天下钱庄, open 钱老板's store, put a bag
item in (cmd_store 0xF376), take it back out (cmd_take 0x3504).

usage: python -X utf8 _store_probe.py <acct> <pwd> <char> [bag_pos] [store_pos] [back_pos]

All packets after each step are printed (cmd / len / hex head) so the store
and bag positions and any refusal dialog are visible.  READ-ONLY for the DB:
everything goes through normal game commands.
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

ACCT, PWD, CHAR = sys.argv[1], sys.argv[2], sys.argv[3]
BAG_POS = int(sys.argv[4]) if len(sys.argv) > 4 else 41
STORE_POS = int(sys.argv[5]) if len(sys.argv) > 5 else 0
BACK_POS = int(sys.argv[6]) if len(sys.argv) > 6 else 0
ROOM = '/gs/zone/lanxianzhen/tianxia-qianzhuang.c'


def gbk(b):
    return b.decode('gbk', 'replace')


def drain(s, secs, label, want_names=None):
    got = []
    t0 = time.time()
    while time.time() - t0 < secs:
        r = ep.read_pkt(s.g, 0.4)
        if r is None:
            print('  !! connection closed')
            break
        if r[0] == 'timeout':
            continue
        cmd, b = r
        got.append((cmd, b))
        extra = ''
        if cmd == 0x1FE5:
            extra = ' DIALOG: ' + gbk(b[1:].split(b'\x00')[0])
        elif cmd in (0x2037, 0x3393):
            extra = ' TEXT: ' + gbk(b)[:160]
        elif cmd == 0x0B0D:
            extra = ' MSG: ' + gbk(b[10:])[:120]
        elif cmd == 0x1155:
            try:
                extra = ' POS: %s' % (ll.parse_pos(b),)
            except Exception:
                pass
        print('  [%s] 0x%04X len=%-5d %s%s' % (label, cmd, len(b), b[:24].hex(), extra))
    return got


s = ll.Session(ACCT, PWD, CHAR, 'st')
if not s.connect(window=15):
    sys.exit('login failed')
print('logged in; pos=%s uid=%s' % (s.pos, s.uid))
drain(s, 2, 'settle')

print('--- teleport ->', ROOM)
rb = ROOM.encode('gbk')
s.g.sendall(ep.frame(0x5A, bytes([len(rb)]) + rb))
pk = drain(s, 4, 'tele')

npc = None
for cmd, b in pk:
    if cmd == 0xF05D and len(b) > 8 and '钱老板'.encode('gbk') in b:
        npc = struct.unpack('>I', b[:4])[0]
if not npc:
    ids = [struct.unpack('>I', b[:4])[0] for c, b in pk if c == 0xF05D and len(b) >= 4]
    print('F05D ids:', [hex(i) for i in ids])
    sys.exit('钱老板 not found')
print('钱老板 id=0x%X' % npc)

print('--- click NPC')
s.g.sendall(ep.frame(0x7012, struct.pack('>I', npc)))
pk = drain(s, 3, 'menu')
items = []
for cmd, b in pk:
    if cmd == 0x2037:
        import re
        items = re.findall(r'\[([^\]]+)\]', gbk(b))
print('menu items:', items)
pick = [x for x in items if '存' in x or '取' in x or '仓库' in x]
if pick:
    it = pick[0].split('/')[0].encode('gbk')
    print('--- select', pick[0])
    s.g.sendall(ep.frame(0x1042, struct.pack('>I', npc) + bytes([len(it)]) + it + bytes([0])))
    drain(s, 4, 'open')

if STORE_POS:
    if BAG_POS:
        print('--- cmd_store bag %d -> store %d' % (BAG_POS, STORE_POS))
        s.g.sendall(ep.frame(0xF376, struct.pack('>IHHH', npc, BAG_POS, STORE_POS, 1)))
        drain(s, 4, 'store')
    print('--- cmd_take store %d -> bag %d' % (STORE_POS, BACK_POS))
    s.g.sendall(ep.frame(0x3504, struct.pack('>IHHH', npc, STORE_POS, BACK_POS, 1)))
    drain(s, 4, 'take')
s.close()
