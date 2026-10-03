# -*- coding: utf-8 -*-
"""Click every announced NPC and DECODE the 0x2037 menu payload, showing
id / portrait / pic_no / content / actor_name (the fields the client uses for
the NPC portrait and name).

  python -X utf8 _npc_menu_check.py [acct] [pwd] [char]
"""
import hashlib
import importlib.util
import io
import socket
import struct
import sys
import time

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

HOST = '192.168.200.1'
ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
WORLD = '20544DE38C2160D6EAC9BFE07486242E'


def dec2037(b):
    """build_menu_buffer: u32 id, u32 portrait, u16 pic_no, u16 strlen(content),
    content, s8 key, u16 allow_close, u8 allow_remote, s8 actor_name, ..."""
    try:
        pid, portrait = struct.unpack('>II', b[:8])
        pic_no = struct.unpack('>H', b[8:10])[0]
        clen = struct.unpack('>H', b[10:12])[0]
        content = b[12:12 + clen].decode('gbk', 'replace')
        o = 12 + clen
        klen = b[o]; key = b[o + 1:o + 1 + klen].decode('gbk', 'replace'); o += 1 + klen
        allow_close = struct.unpack('>H', b[o:o + 2])[0]; o += 2
        o += 1                                   # make8(allow_remote)
        nlen = b[o]; name = b[o + 1:o + 1 + nlen].decode('gbk', 'replace')
        return ('id=0x%X portrait=0x%08X(%d) pic_no=%d content=%dB name="%s" len=%d'
                % (pid, portrait, struct.unpack('>i', b[4:8])[0], pic_no, clen, name, nlen))
    except Exception as e:
        return 'decode-error %s raw=%s' % (e, b[:48].hex())


# --- aaa login -------------------------------------------------------------
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
t0 = time.time()
selected = False
while time.time() - t0 < 30:
    r = ep.read_pkt(g, 2)
    if r is None:
        break
    if r[0] == 'timeout':
        continue
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
print('uid=0x%X npcs=%s' % (uid or 0, [hex(x) for x in npc_ids]), flush=True)

ROOM = sys.argv[4] if len(sys.argv) > 4 else ''
if ROOM:
    g.sendall(ep.frame(0x1026, ep.s8(ROOM)))
    print('sent 0x1026 ->', ROOM, flush=True)
    npc_ids = []
    t = time.time()
    while time.time() - t < 5:
        r = ep.read_pkt(g, 0.5)
        if r is None or r[0] == 'timeout':
            continue
        if r[0] == 0x1155:
            print('  got 0x1155 (room refreshed)', flush=True)
        if r[0] == 0xF05D and len(r[1]) >= 4:
            nid = struct.unpack('>I', r[1][:4])[0]
            if nid not in npc_ids:
                npc_ids.append(nid)
    print('after transition npcs=%s' % [hex(x) for x in npc_ids], flush=True)

t2 = time.time()
while time.time() - t2 < 3:
    ep.read_pkt(g, 0.5)

ok = 0
tot = 0
for nid in npc_ids[:8]:
    g.sendall(ep.frame(0x7012, struct.pack('>I', nid)))
    tot += 1
    t3 = time.time()
    while time.time() - t3 < 3:
        r = ep.read_pkt(g, 0.5)
        if r is None or r[0] == 'timeout':
            continue
        if r[0] == 0x2037:
            info = dec2037(r[1])
            good = ('portrait=0xFFFFFFFF' not in info) and ('name=""' not in info)
            ok += 1 if good else 0
            print('  0x%X MENU  %s   %s' % (nid, info, 'OK' if good else '<== no portrait/name'), flush=True)
        elif r[0] == 0x3393:
            print('  0x%X DIALOG %s' % (nid, r[1][2:].rstrip(b'\x00').decode('gbk', 'replace')[:80]), flush=True)
print('menus with portrait+name: %d/%d' % (ok, tot), flush=True)
g.close(); s.close()
