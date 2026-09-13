# -*- coding: utf-8 -*-
"""Echo/heartbeat check: the real client sends 0x20D2 every 1-3s and waits for
0xF9C3.  Before the fix the server answered only the FIRST echo.

Also dumps the two client-visible story packets for byte comparison.

  python -X utf8 _echo_test.py [host] [acct] [pwd] [char]
"""
import hashlib, importlib.util, io, socket, struct, sys, time

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

HOST = sys.argv[1] if len(sys.argv) > 1 else '192.168.200.1'
ACCT = sys.argv[2] if len(sys.argv) > 2 else '1'
PWD = sys.argv[3] if len(sys.argv) > 3 else '1'
WORLD = '20544DE38C2160D6EAC9BFE07486242E'

# --- AAA login (same as _login_timing.py) ---------------------------------
s = socket.create_connection((HOST, 8001), timeout=6)
tick = int(time.time() * 1000) & 0xFFFFFFFF
s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
          + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
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
r = ep.recv_until(s, 0x4355, time.time() + 30)
b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2
server = ''
for _ in range(cnt):
    sl = b[off]; off += 1
    server = b[off:off + sl].decode('gbk', 'replace'); off += sl
    sl = b[off]; off += 1
    off += sl + 2
    break
s.sendall(ep.frame(0x3356, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8(server)))
r = ep.recv_until(s, 0x3357, time.time() + 30)
b = r[1]; off = 6
ln = b[off]; off += 1
ip = b[off:off + ln].decode('gbk', 'replace'); off += ln
gport = struct.unpack('>h', b[off:off + 2])[0]; off += 2
seed = struct.unpack('>i', b[off:off + 4])[0]

g = socket.create_connection((ip, gport), timeout=8)
g.settimeout(6)

# The real client's heartbeat shape: echo with its own ms tick, 1-3s apart,
# expecting 0xF9C3 each time.
base = int(time.time() * 1000) & 0xFFFFFFFF
print('=== echo heartbeat test (0x20D2 -> 0xF9C3) ===', flush=True)
ok = 0
for k in range(5):
    t = (base + k * 1500) & 0xFFFFFFFF
    g.sendall(ep.frame(0x20D2, struct.pack('>I', t)))
    t0 = time.time()
    rep = None
    while time.time() - t0 < 4:
        r = ep.read_pkt(g, 1.0)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        if r[0] == 0xF9C3:
            rep = (time.time() - t0) * 1000.0
            break
    if rep is None:
        print('  echo #%d tick=%d -> NO 0xF9C3 within 4s   <== BUG' % (k + 1, t), flush=True)
    else:
        ok += 1
        print('  echo #%d tick=%d -> 0xF9C3 in %.1f ms' % (k + 1, t, rep), flush=True)
    time.sleep(1.2)
print('echo replies: %d/5' % ok, flush=True)
g.close(); s.close()
