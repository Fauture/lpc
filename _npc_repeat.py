# -*- coding: utf-8 -*-
"""Walk a bit, then click every announced NPC several times, reporting which
clicks get a reply (0x2037 menu / 0x3393 dialog / 0x1FE5 popup) and which get
NOTHING - and print the client-side position we walked to.

  python -X utf8 _npc_repeat.py [acct] [pwd] [char] [room]
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
WORLD = '20544DE38C2160D6EAC9BFE07486242E'

# --- aaa login ---
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

uid = None
npc_ids = []
selected = False
pos = None            # (x, y) from the last 0x1155
t0 = time.time()
while time.time() - t0 < 30:
    r = ep.read_pkt(g, 2)
    if r is None:
        break
    if r[0] == 'timeout':
        continue
    if r[0] == 0x1155 and len(r[1]) >= 21:
        # map_id(4) source_id(4) name(s8) x(2) y(2)
        try:
            o = 8; ln2 = r[1][o]; o += 1 + ln2
            pos = (struct.unpack('>H', r[1][o:o + 2])[0], struct.unpack('>H', r[1][o + 2:o + 4])[0])
        except Exception:
            pass
    if r[0] == 0xF05D and len(r[1]) >= 4:
        nid = struct.unpack('>I', r[1][:4])[0]
        if uid is None:
            uid = nid
        elif nid != uid and nid not in npc_ids:
            npc_ids.append(nid)
    if r[0] == 0xF061 and not selected:
        c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
        if c2 > 0:
            g.sendall(ep.frame(0x1060, ep.s8(CHAR)))
            selected = True
print('uid=0x%X npcs=%s pos=%s' % (uid or 0, [hex(x) for x in npc_ids], pos), flush=True)

t2 = time.time()
while time.time() - t2 < 3:
    ep.read_pkt(g, 0.5)

# walk: multi_move_to (0xFD72) v, mapId, count, x/y pairs, dir
if pos:
    x0, y0 = pos
    steps = [(x0 + i, y0 + i) for i in range(1, 5)]
    body = struct.pack('>I', 0x1234) + struct.pack('>I', 2000) + struct.pack('>H', len(steps))
    for sx, sy in steps:
        body += struct.pack('>H', sx) + struct.pack('>H', sy)
    body += struct.pack('>H', 1)
    g.sendall(ep.frame(0xFD72, body))
    print('sent move to %s' % (steps[-1],), flush=True)
    t3 = time.time()
    while time.time() - t3 < 2:
        r = ep.read_pkt(g, 0.5)
        if r and r[0] == 0x1155 and len(r[1]) >= 21:
            try:
                o = 8; ln2 = r[1][o]; o += 1 + ln2
                print('  server pos now:', struct.unpack('>H', r[1][o:o + 2])[0],
                      struct.unpack('>H', r[1][o + 2:o + 4])[0], flush=True)
            except Exception:
                pass

print('--- repeat clicks (3x each, same ids) ---', flush=True)
for nid in npc_ids[:5]:
    row = []
    for k in range(3):
        g.sendall(ep.frame(0x7012, struct.pack('>I', nid)))
        t4 = time.time()
        got = []
        while time.time() - t4 < 1.5:
            r = ep.read_pkt(g, 0.4)
            if r is None or r[0] == 'timeout':
                continue
            got.append('0x%04X' % r[0])
            if r[0] == 0x2037 and len(r[1]) >= 12:
                pid, portrait = struct.unpack('>II', r[1][:8])
                clen = struct.unpack('>H', r[1][10:12])[0]
                got.append('portrait=0x%08X' % portrait)
            if r[0] == 0x2238 or r[0] == 0x219A:
                pass
        row.append(','.join(got) if got else '*** NO REPLY ***')
    print('  0x%X: %s' % (nid, ' | '.join(row)), flush=True)
g.close(); s.close()
