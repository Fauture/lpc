# -*- coding: utf-8 -*-
"""Walk the player to a target cell with 0xFD72 and then try 过图.

Feedback channel: `move_v37::multi_move_to` sends NOTHING when it accepts the
path, and `refresh_current_pos()` (0x1155, the authoritative my_x/my_y) when
`check_steps` rejects it.  So:
    move packet -> silence   = accepted
    move packet -> 0x1155    = rejected (and tells us where we really are)

usage: python -X utf8 _walk_probe.py [acct] [pwd] [char] [target_x] [target_y] [room]
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
TX = int(sys.argv[4]) if len(sys.argv) > 4 else 1
TY = int(sys.argv[5]) if len(sys.argv) > 5 else 131
ROOM = sys.argv[6] if len(sys.argv) > 6 else '揽仙镇'
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
    enc = ep.des_encrypt(hashlib.md5(PWD.encode('gbk', 'replace')).hexdigest().upper().encode('ascii'), cookie)
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


class Conn(object):
    def __init__(self, g):
        self.g = g
        self.uid = None
        self.pos = None
        self.cmds = set()
        self.selected = False

    def pump(self, secs):
        t0 = time.time()
        while time.time() - t0 < secs:
            r = ep.read_pkt(self.g, 0.3)
            if r is None:
                return False
            if r[0] == 'timeout':
                continue
            self.cmds.add(r[0])
            if r[0] == 0x1155 and len(r[1]) >= 21:
                try:
                    self.pos = parse_pos(r[1])
                except Exception:
                    pass
            if r[0] == 0xF05D and len(r[1]) >= 8:
                if NAMEB in r[1]:
                    self.uid = struct.unpack('>I', r[1][:4])[0]
            if r[0] == 0xF061 and not self.selected:
                c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
                if c2 > 0:
                    self.g.sendall(ep.frame(0x1060, ep.s8(CHAR)))
                    self.selected = True
        return True

    def query_pos(self, timeout=3.0):
        """Server-side position via a failing enter_room -> refresh_current_pos."""
        self.pos = None
        self.g.sendall(ep.frame(0x1026, bytes([len(b'###x###')]) + b'###x###'))
        t0 = time.time()
        while time.time() - t0 < timeout:
            r = ep.read_pkt(self.g, 0.4)
            if r is None or r[0] == 'timeout':
                continue
            if r[0] == 0x1155 and len(r[1]) >= 21:
                self.pos = parse_pos(r[1])
                return self.pos
        return None

    def move(self, map_id, steps, dirv=1):
        body = struct.pack('>I', self.uid) + struct.pack('>I', map_id) + struct.pack('>H', len(steps))
        for sx, sy in steps:
            body += struct.pack('>H', sx) + struct.pack('>H', sy)
        body += struct.pack('>H', dirv) + struct.pack('>I', int(time.time() * 1000) & 0xFFFFFFFF)
        self.cmds = set()
        self.g.sendall(ep.frame(0xFD72, body))

    def step(self, map_id, nx, ny, wait=0.25):
        """One cell.  Returns True when the server accepted it."""
        self.move(map_id, [(nx, ny)])
        t0 = time.time()
        rejected = False
        while time.time() - t0 < wait:
            r = ep.read_pkt(self.g, 0.15)
            if r is None:
                break
            if r[0] == 'timeout':
                continue
            self.cmds.add(r[0])
            if r[0] == 0x1155 and len(r[1]) >= 21:
                try:
                    self.pos = parse_pos(r[1])
                except Exception:
                    pass
                rejected = True
        return not rejected

    def enter_room(self, name):
        rb = name.encode('gbk')
        self.cmds = set()
        self.g.sendall(ep.frame(0x1026, bytes([len(rb)]) + rb))


def main():
    s, g = login()
    c = Conn(g)
    c.pump(25)
    if not c.uid:
        print('!! no player id - abort', flush=True)
        return
    p = c.query_pos()
    if not p:
        print('!! no position - abort', flush=True)
        return
    mid, sid, name, x, y, d = p
    print('uid=0x%X start map_id=%d room=%s pos=(%d,%d)' % (c.uid, mid, name, x, y), flush=True)
    print('target (%d,%d) in %s, 过图 to %s' % (TX, TY, name, ROOM), flush=True)

    blocked = set()
    for i in range(400):
        if max(abs(x - TX), abs(y - TY)) <= 8:
            break
        cand = []
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if dx == 0 and dy == 0:
                    continue
                nx, ny = x + dx, y + dy
                if nx < 0 or ny < 0 or (nx, ny) in blocked:
                    continue
                dist = max(abs(nx - TX), abs(ny - TY))
                cand.append((dist, abs(dx) + abs(dy), dx, dy, nx, ny))
        if not cand:
            print('!! boxed in at (%d,%d)' % (x, y), flush=True)
            break
        cand.sort()
        moved = False
        for _dist, _c, dx, dy, nx, ny in cand[:4]:
            if c.step(mid, nx, ny):
                x, y = nx, ny
                moved = True
                break
            blocked.add((nx, ny))
            if c.pos:
                x, y = c.pos[3], c.pos[4]
        if not moved:
            # re-sync: maybe our idea of the position is stale
            q = c.query_pos()
            if q:
                x, y = q[3], q[4]
            else:
                print('!! lost the connection at (%d,%d)' % (x, y), flush=True)
                break
        if i % 20 == 19:
            print('   step %3d -> (%d,%d) blocked=%d' % (i + 1, x, y, len(blocked)), flush=True)
    print('arrived (%d,%d) after walking; blocked cells=%d' % (x, y, len(blocked)), flush=True)
    q = c.query_pos()
    if q:
        print('server confirms pos=(%d,%d) map_id=%d room=%s' % (q[3], q[4], q[0], q[2]), flush=True)
        mid = q[0]

    dist = max(abs((q[3] if q else x) - TX), abs((q[4] if q else y) - TY))
    print('--- 过图: 0x1026 "%s" (distance to the exit cell = %d, the gate is <=10) ---' % (ROOM, dist), flush=True)
    c.pos = None
    c.enter_room(ROOM)
    t0 = time.time()
    while time.time() - t0 < 6:
        r = ep.read_pkt(g, 0.4)
        if r is None or r[0] == 'timeout':
            continue
        print('    recv 0x%04X len=%d %s' % (r[0], len(r[1]), r[1].hex()[:100]), flush=True)
        if r[0] == 0x1155 and len(r[1]) >= 21:
            c.pos = parse_pos(r[1])
    if c.pos:
        print('   now map_id=%d room=%s pos=(%d,%d)  -> %s'
              % (c.pos[0], c.pos[2], c.pos[3], c.pos[4],
                 '过图 OK' if (c.pos[0], c.pos[2]) != (mid, name) else '*** STILL %s ***' % name), flush=True)
    else:
        print('   no position reply', flush=True)
    g.close(); s.close()


if __name__ == '__main__':
    main()
