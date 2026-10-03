# -*- coding: utf-8 -*-
# Login, enter world, then try client walking (cmd 0xFD72 cmd_multi_move_to)
# from several candidate start cells around the fresh-char position and print
# the server's responses per attempt.
import socket, struct, sys, time, hashlib, importlib.util

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec); spec.loader.exec_module(ep)

host, port, acct, pwd, charname = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5]
map_id = int(sys.argv[6]) if len(sys.argv) > 6 else 1000

# ---- AAA login ----
s = socket.create_connection((host, port), timeout=6)
tick = int(time.time() * 1000) & 0xFFFFFFFF
s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
ep.read_pkt(s, 3)
s.sendall(bytes.fromhex('4D5A0000010F240C00640B05'
    '2036413430453141313130394535443933363336424630343645373338363144454032314137364445363344373442443437454231323131463338324330384639434633393538353445463146413237363541334532383437394133393346444241'))
ep.read_pkt(s, 3)
s.sendall(bytes.fromhex('4D5A0000010F246A00061B0300000000'))
r = ep.read_pkt(s, 8)
cookie = None
if r and r[0] == 0x2B04:
    b = r[1]; off = 4; ln = b[off]; off += 1
    cookie = b[off:off+ln].decode('gbk', 'replace')
pwd_hex = hashlib.md5(pwd.encode('gbk', 'replace')).hexdigest().upper()
enc = ep.des_encrypt(pwd_hex.encode('ascii'), cookie)
body = ep.s8(acct) + ep.s8(enc.hex().upper().encode('ascii')) + ep.s8('0000000000000000') + ep.s8('') + ep.s8('') + ep.s8('试剑内测') + bytes([0]) + ep.s8('20544DE38C2160D6EAC9BFE07486242E') + ep.s8('')
s.sendall(ep.frame(0x2350, body))
r = ep.read_pkt(s, 12)
bb = r[1]; auth_key = struct.unpack('>i', bb[4:8])[0]
s.sendall(ep.frame(0x3354, ep.s8(acct) + struct.pack('>i', auth_key) + ep.s8('试剑内测') + bytes([0])))
r = ep.recv_until(s, 0x4355, time.time() + 15)
b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2; server = ''; ip = ''
for k in range(cnt):
    sl = b[off]; off += 1
    server = b[off:off+sl].decode('gbk', 'replace'); off += sl
    sl = b[off]; off += 1
    ip = b[off:off+sl].decode('gbk', 'replace'); off += sl
    off += 2
    break
s.sendall(ep.frame(0x3356, ep.s8(acct) + struct.pack('>i', auth_key) + ep.s8(server)))
r = ep.recv_until(s, 0x3357, time.time() + 12)
b = r[1]; off = 6
ip, off = (lambda o: (b[o+1:o+1+b[o]].decode('gbk', 'replace'), o+1+b[o]))(off)
gport = struct.unpack('>h', b[off:off+2])[0]; off += 2
seed = struct.unpack('>i', b[off:off+4])[0]

g = socket.create_connection((ip, gport), timeout=6)
g.settimeout(2)
tick = int(time.time() * 1000) & 0xFFFFFFFF
g.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10) + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
try: ep.read_pkt(g, 2)
except Exception: pass
ab = acct.encode('gbk', 'replace')
vb = '20544DE38C2160D6EAC9BFE07486242E:D41D8CD98F00B204E9800998ECF8427E'.encode('ascii')
body = bytes([len(ab)]) + ab + struct.pack('>i', auth_key) + struct.pack('>i', seed) + bytes([len(vb)]) + vb + bytes([0, 0, 0])
g.sendall(ep.frame(0x4124, bytes(body)))

uid = None
selected = False
t0 = time.time()
while time.time() - t0 < 40:
    r = ep.read_pkt(g, 2)
    if r is None: break
    if r[0] == 'timeout': continue
    if r[0] in (0x39F1, 0xFA01, 0xF05D) and len(r[1]) >= 4 and uid is None:
        uid = struct.unpack('>I', r[1][:4])[0]
    if r[0] == 0xF061:
        c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
        if c2 > 0 and not selected:
            g.sendall(ep.frame(0x1060, ep.s8(charname)))
            selected = True
            t_sel = time.time()
    if selected and time.time() - t_sel > 6:
        break
print('uid=%s selected=%s' % (uid, selected), flush=True)

cands = [(61, 175), (60, 175), (62, 175), (61, 174), (61, 176), (62, 176), (60, 176), (63, 175)]
for (cx, cy) in cands:
    for dirv in (6, 0, 2):
        body = struct.pack('>I', uid or 0) + struct.pack('>I', map_id)
        body += struct.pack('>H', 1) + struct.pack('>H', cx) + struct.pack('>H', cy) + struct.pack('>H', dirv)
        g.sendall(ep.frame(0xFD72, body))
        got = []
        t1 = time.time()
        while time.time() - t1 < 1.2:
            r = ep.read_pkt(g, 0.6)
            if r is None: break
            if r[0] == 'timeout': continue
            got.append(hex(r[0]))
        print('move start=(%d,%d) dir=%d -> %s' % (cx, cy, dirv, got), flush=True)
        if got:
            break
g.close(); s.close()
