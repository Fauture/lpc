# -*- coding: utf-8 -*-
"""Movement / 过图 probe (v2).

The v1 probe took the char id from the first 0xF05D of the window, which on a
re-login is an NPC - so every move packet was rejected by
`start_gs::cmd_multi_move_to`'s `me->get_id() != id` guard and the test proved
nothing.  Here the player's id is the 0xF05D packet that carries the char name.

Server position is read back with a *failed* `cmd_enter_room` (a room name that
is not in the room's `exits`): enter_room's every `break` path ends in
`me->refresh_current_pos()`, which sends 0x1155 with the authoritative
my_x/my_y.

usage: python -X utf8 _move_probe.py [acct] [pwd] [char] [room]
"""
import hashlib, importlib.util, io, socket, struct, sys, time

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

HOST = '192.168.200.1'
ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
ROOM = sys.argv[4] if len(sys.argv) > 4 else '/gs/zone/lanxianzhen/lanxianzhen.c'
WORLD = '20544DE38C2160D6EAC9BFE07486242E'
NAMEB = CHAR.encode('gbk')


def login():
    s = socket.create_connection((HOST, 8001), timeout=6)
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2)
              + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
    ep.read_pkt(s, 3)
    s.sendall(bytes.fromhex('4D5A0000010F240C00640B05'
        '2036413430453141313130394535443933363336424630343645373338363144454032314137364445363344373442443437454231323131463338324330384639434633393538353445463146413237363541334532383437394133393346444241'))
    ep.read_pkt(s, 3)
    s.sendall(bytes.fromhex('4D5A0000010F246A00061B0300000000'))
    r = ep.read_pkt(s, 8)
    b = r[1]; off = 4; ln = b[off]; off += 1
    cookie = b[off:off + ln].decode('gbk', 'replace')
    pwd_hex = hashlib.md5(PWD.encode('gbk', 'replace')).hexdigest().upper()
    enc = ep.des_encrypt(pwd_hex.encode('ascii'), cookie)
    body = (ep.s8(ACCT) + ep.s8(enc.hex().upper().encode('ascii')) + ep.s8('0000000000000000')
            + ep.s8('') + ep.s8('') + ep.s8('试剑内测') + bytes([0]) + ep.s8(WORLD) + ep.s8(''))
    s.sendall(ep.frame(0x2350, body))
    r = ep.read_pkt(s, 12)
    auth_key = struct.unpack('>i', r[1][4:8])[0]
    s.sendall(ep.frame(0x3354, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8('试剑内测') + bytes([0])))
    r = ep.recv_until(s, 0x4355, time.time() + 15)
    b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2; server = ''
    for _ in range(cnt):
        sl = b[off]; off += 1
        server = b[off:off + sl].decode('gbk', 'replace'); off += sl
        sl = b[off]; off += 1
        off += sl + 2
        break
    s.sendall(ep.frame(0x3356, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8(server)))
    r = ep.recv_until(s, 0x3357, time.time() + 12)
    b = r[1]; off = 6
    ln = b[off]; off += 1
    ip = b[off:off + ln].decode('gbk', 'replace'); off += ln
    gport = struct.unpack('>h', b[off:off + 2])[0]; off += 2
    seed = struct.unpack('>i', b[off:off + 4])[0]
    g = socket.create_connection((ip, gport), timeout=6)
    g.settimeout(2)
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    g.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2)
              + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
    try:
        ep.read_pkt(g, 2)
    except Exception:
        pass
    ab = ACCT.encode('gbk', 'replace')
    vb = (WORLD + ':D41D8CD98F00B204E9800998ECF8427E').encode('ascii')
    body = (bytes([len(ab)]) + ab + struct.pack('>i', auth_key) + struct.pack('>i', seed)
            + bytes([len(vb)]) + vb + bytes([0, 0, 0]))
    g.sendall(ep.frame(0x4124, bytes(body)))
    return s, g


def parse_pos(b):
    mid, sid = struct.unpack('>II', b[:8])
    o = 8
    ln = b[o]; o += 1
    name = b[o:o + ln].decode('gbk', 'replace'); o += ln
    x, y, d = struct.unpack('>HHH', b[o:o + 6])
    return mid, sid, name, x, y, d


def collect(g, secs, st, verbose=False):
    """st = dict(uid=?, npcs=[], pos=None, cmds=set())"""
    t0 = time.time()
    while time.time() - t0 < secs:
        r = ep.read_pkt(g, 1)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        st['cmds'].add(r[0])
        if r[0] == 0x1155 and len(r[1]) >= 21:
            try:
                st['pos'] = parse_pos(r[1])
            except Exception:
                pass
        if r[0] == 0xF05D and len(r[1]) >= 8:
            nid = struct.unpack('>I', r[1][:4])[0]
            if NAMEB in r[1]:
                st['uid'] = nid
                st['self_f05d'] += 1
            elif nid != st.get('uid') and nid not in st['npcs']:
                st['npcs'].append(nid)
        if r[0] == 0xF061 and not st['selected']:
            c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
            if c2 > 0:
                g.sendall(ep.frame(0x1060, ep.s8(CHAR)))
                st['selected'] = True
        if verbose and r[0] not in (0x20D2,):
            print('    recv 0x%04X len=%d %s' % (r[0], len(r[1]), r[1].hex()[:90]), flush=True)
    return st


def newstate():
    return {'uid': None, 'npcs': [], 'pos': None, 'cmds': set(), 'selected': False, 'self_f05d': 0}


def query_pos(g, st):
    """Ask the server where we are: a room name that cannot be in `exits`
    makes enter_room fall through to me->refresh_current_pos() -> 0x1155."""
    st['pos'] = None
    st['cmds'] = set()
    rb = b'###no_such_room###'
    g.sendall(ep.frame(0x1026, bytes([len(rb)]) + rb))
    collect(g, 2.5, st)
    return st['pos']


def move(g, uid, map_id, steps, dirv=1):
    body = struct.pack('>I', uid) + struct.pack('>I', map_id) + struct.pack('>H', len(steps))
    for sx, sy in steps:
        body += struct.pack('>H', sx) + struct.pack('>H', sy)
    body += struct.pack('>H', dirv) + struct.pack('>I', int(time.time() * 1000) & 0xFFFFFFFF)
    g.sendall(ep.frame(0xFD72, body))


def main():
    s, g = login()
    st = newstate()
    print('--- logging in / selecting %s ---' % CHAR, flush=True)
    collect(g, 25, st)
    print('uid=0x%X (from %d 0xF05D packets carrying the name) npcs=%d'
          % (st['uid'] or 0, st['self_f05d'], len(st['npcs'])), flush=True)
    if not st['uid']:
        print('!! could not identify the player id - abort', flush=True)
        return
    collect(g, 2, st)
    p = query_pos(g, st)
    if not p:
        print('!! no position reply to the enter_room probe - abort', flush=True)
        return
    mid, sid, name, x, y, d = p
    print('POS map_id=%d source_id=%d room=%s x=%d y=%d dir=%d' % (mid, sid, name, x, y, d), flush=True)

    print('--- 1) 0xFD72 move east %d steps with map_id=%d ---' % (4, mid), flush=True)
    steps = [(x + i, y) for i in range(1, 5)]
    move(g, st['uid'], mid, steps)
    collect(g, 1.5, st)
    p2 = query_pos(g, st)
    if p2:
        print('   server now: x=%d y=%d (wanted %d,%d)  -> %s'
              % (p2[3], p2[4], steps[-1][0], steps[-1][1],
                 'MOVED' if (p2[3], p2[4]) != (x, y) else '*** SERVER POS UNCHANGED ***'), flush=True)
    else:
        print('   no 0x1155 after the move; cmds=%s' % [hex(c) for c in st['cmds']], flush=True)
    if p2:
        x, y = p2[3], p2[4]

    print('--- 2) control: same move with a WRONG map_id=0x7FFFFFFF ---', flush=True)
    move(g, st['uid'], 0x7FFFFFFF, [(x + i, y) for i in range(1, 5)])
    collect(g, 1.5, st)
    p3 = query_pos(g, st)
    print('   server now: %s' % ('x=%d y=%d' % (p3[3], p3[4]) if p3 else 'no reply'), flush=True)

    print('--- 3) 0x1026 cmd_enter_room -> %s ---' % ROOM, flush=True)
    st['pos'] = None; st['cmds'] = set()
    rb = ROOM.encode('gbk')
    g.sendall(ep.frame(0x1026, bytes([len(rb)]) + rb))
    collect(g, 6, st, verbose=True)
    if st['pos']:
        q = st['pos']
        print('   now map_id=%d room=%s x=%d y=%d (was map_id=%d room=%s) -> %s'
              % (q[0], q[2], q[3], q[4], mid, name,
                 'MAP CHANGED (过图 OK)' if q[0] != mid or (q[3], q[4]) != (x, y) else
                 'SAME CELL (enter_room rejected: distance > 10 or not an exit)'), flush=True)
    else:
        print('   no 0x1155; cmds=%s' % [hex(c) for c in st['cmds']], flush=True)

    print('--- 4) idle 20s: watch for drops/heartbeats ---', flush=True)
    st['cmds'] = set()
    collect(g, 20, st)
    print('   cmds=%s' % [hex(c) for c in st['cmds']], flush=True)
    g.close(); s.close()


if __name__ == '__main__':
    main()
