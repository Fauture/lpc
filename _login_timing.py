# -*- coding: utf-8 -*-
"""Where do the seconds go when the client "selects a line"?

Times every step of the login the real client performs:
  AAA 8001: echo -> antibot -> get cookie -> cmd_l_account (password DES)
            -> msg_l_auth -> get_server_list (0x4355) -> client_connect_agent
            (0x3356 -> 0x3357 gives ip:port/seed)
  GS  <ip>:<port>: echo -> cmd_login 0x4124 -> first 0x211B / 0xF061

  python -X utf8 _login_timing.py [host] [acct] [pwd] [char]

Run against ALREADY RUNNING services.
"""
import hashlib
import importlib.util
import socket
import struct
import sys
import time

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

HOST = sys.argv[1] if len(sys.argv) > 1 else '192.168.200.1'
ACCT = sys.argv[2] if len(sys.argv) > 2 else '1'
PWD = sys.argv[3] if len(sys.argv) > 3 else '1'
CHAR = sys.argv[4] if len(sys.argv) > 4 else 'awesrd'
WORLD = '20544DE38C2160D6EAC9BFE07486242E'

T0 = time.time()


def mark(what, extra=''):
    print('%8.1f ms  %s %s' % ((time.time() - T0) * 1000.0, what, extra), flush=True)


mark('start')
s = socket.create_connection((HOST, 8001), timeout=6)
tick = int(time.time() * 1000) & 0xFFFFFFFF
s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
          + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
ep.read_pkt(s, 3)
mark('aaa echo ok')
s.sendall(bytes.fromhex('4D5A0000010F240C00640B05'
    '2036413430453141313130394535443933363336424630343645373338363144454032314137364445363344373442443437454231323131463338324330384639434633393538353445463146413237363541334532383437394133393346444241'))
ep.read_pkt(s, 3)
mark('aaa antibot ok')
s.sendall(bytes.fromhex('4D5A0000010F246A00061B0300000000'))
r = ep.read_pkt(s, 8)
mark('aaa cookie ok', 'cmd=0x%04X' % r[0])
b = r[1]; off = 4; ln = b[off]; off += 1
cookie = b[off:off + ln].decode('gbk', 'replace')
pwd_hex = hashlib.md5(PWD.encode('gbk', 'replace')).hexdigest().upper()
enc = ep.des_encrypt(pwd_hex.encode('ascii'), cookie)
body = (ep.s8(ACCT) + ep.s8(enc.hex().upper().encode('ascii')) + ep.s8('0000000000000000')
        + ep.s8('') + ep.s8('') + ep.s8('试剑内测') + bytes([0]) + ep.s8(WORLD) + ep.s8(''))
s.sendall(ep.frame(0x2350, body))
r = ep.read_pkt(s, 12)
mark('cmd_l_account ok', 'cmd=0x%04X' % r[0])
auth_key = struct.unpack('>i', r[1][4:8])[0]
s.sendall(ep.frame(0x3354, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8('试剑内测') + bytes([0])))
r = ep.recv_until(s, 0x4355, time.time() + 30)
mark('get_server_list ok', 'cmd=0x%04X' % r[0])
b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2
server = ''
for _ in range(cnt):
    sl = b[off]; off += 1
    server = b[off:off + sl].decode('gbk', 'replace'); off += sl
    sl = b[off]; off += 1
    off += sl + 2
    break
s.sendall(ep.frame(0x3356, ep.s8(ACCT) + struct.pack('>i', auth_key) + ep.s8(server)))
r = ep.recv_until(s, 0x3357, time.time() + 60)
mark('client_connect_agent ok', 'cmd=0x%04X' % r[0])
b = r[1]; off = 6
ln = b[off]; off += 1
ip = b[off:off + ln].decode('gbk', 'replace'); off += ln
gport = struct.unpack('>h', b[off:off + 2])[0]; off += 2
seed = struct.unpack('>i', b[off:off + 4])[0]

g = socket.create_connection((ip, gport), timeout=8)
mark('gs connect', '%s:%d' % (ip, gport))
g.settimeout(2)
tick = int(time.time() * 1000) & 0xFFFFFFFF
g.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
          + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
try:
    ep.read_pkt(g, 3)
except Exception:
    pass
mark('gs echo ok')
ab = ACCT.encode('gbk', 'replace')
vb = (WORLD + ':D41D8CD98F00B204E9800998ECF8427E').encode('ascii')
body = (bytes([len(ab)]) + ab + struct.pack('>i', auth_key) + struct.pack('>i', seed)
        + bytes([len(vb)]) + vb + bytes([0, 0, 0]))
g.sendall(ep.frame(0x4124, bytes(body)))
mark('gs cmd_login sent')

first211b = None
firstf061 = None
t1 = time.time()
while time.time() - t1 < 60:
    r = ep.read_pkt(g, 2)
    if r is None:
        break
    if r[0] == 'timeout':
        continue
    if r[0] == 0x211B and first211b is None:
        first211b = (time.time() - T0) * 1000.0
        mark('GS -> 0x211B (fields form)', 'body=%d' % len(r[1]))
    elif r[0] == 0xF061 and firstf061 is None:
        cnt = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
        firstf061 = (time.time() - T0) * 1000.0
        mark('GS -> 0xF061 (character list)', 'count=%d' % cnt)
        break
if firstf061 is None:
    mark('!! no 0xF061 within 60s')
g.close(); s.close()
mark('done')
