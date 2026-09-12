# -*- coding: utf-8 -*-
"""Click each announced NPC and decode EVERY reply (0x3393 dialog text, 0x2037
menu payload), so "no reaction" can be told apart from "some reply".

  python -X utf8 _npc_verbose.py [acct] [pwd] [char]
"""
import socket, struct, sys, time, importlib.util

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)

HOST = '192.168.200.1'
ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'


def dec(b):
    """Try to decode a dialog body: some frames carry s8-prefixed text."""
    out = []
    for text in (b, b[1:], b[2:], b[3:]):
        try:
            s = text.rstrip(b'\x00').decode('gbk', 'replace')
            if s and any('\u4e00' <= ch <= '\u9fff' for ch in s):
                out.append(s)
        except Exception:
            pass
    return out[0] if out else repr(b[:60])


# --- login (aaa 8001) ------------------------------------------------------
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
import hashlib
b = r[1]; off = 4; ln = b[off]; off += 1
cookie = b[off:off + ln].decode('gbk', 'replace')
pwd_hex = hashlib.md5(PWD.encode('gbk', 'replace')).hexdigest().upper()
enc = ep.des_encrypt(pwd_hex.encode('ascii'), cookie)
body = (ep.s8(ACCT) + ep.s8(enc.hex().upper().encode('ascii')) + ep.s8('0000000000000000')
        + ep.s8('') + ep.s8('') + ep.s8('试剑内测') + bytes([0])
        + ep.s8('20544DE38C2160D6EAC9BFE07486242E') + ep.s8(''))
s.sendall(ep.frame(0x2350, body))
r = ep.read_pkt(s, 12)
auth_key = struct.unpack('>i', r[1][4:8])[0]
s.sendall(ep.frame(0x3354, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8('试剑内测') + bytes([0])))
r = ep.recv_until(s, 0x4355, time.time() + 15)
b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2; server = ''
for k in range(cnt):
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
vb = '20544DE38C2160D6EAC9BFE07486242E:D41D8CD98F00B204E9800998ECF8427E'.encode('ascii')
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
    if r[0] in (0x39F1, 0xFA01, 0xF05D) and len(r[1]) >= 4 and uid is None:
        uid = struct.unpack('>I', r[1][:4])[0]
    if r[0] == 0xF05D and len(r[1]) >= 4:
        nid = struct.unpack('>I', r[1][:4])[0]
        if nid and nid != uid and nid not in npc_ids:
            npc_ids.append(nid)
    if r[0] == 0xF061 and not selected:
        c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
        if c2 > 0:
            g.sendall(ep.frame(0x1060, ep.s8(CHAR)))
            selected = True
            t1 = time.time()
print('uid=%s npc_ids=%s' % (uid, [hex(x) for x in npc_ids]), flush=True)

# settle: drain world packets
t2 = time.time()
while time.time() - t2 < 3:
    ep.read_pkt(g, 0.5)

for nid in npc_ids[:6]:
    print('--- click 0x%X (0x7012) ---' % nid, flush=True)
    g.sendall(ep.frame(0x7012, struct.pack('>I', nid)))
    t3 = time.time()
    got = 0
    while time.time() - t3 < 4:
        r = ep.read_pkt(g, 0.5)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        got += 1
        tag = {0x3393: 'DIALOG', 0x2037: 'MENU', 0xF061: 'CHARLIST'}.get(r[0], 'pkt')
        print('    %-9s 0x%04X len=%d' % (tag, r[0], len(r[1])), flush=True)
        if r[0] in (0x3393, 0x2037):
            print('      text: %s' % dec(r[1])[:220], flush=True)
    if got == 0:
        print('    (NO REPLY AT ALL - the reported symptom)', flush=True)
g.close(); s.close()
