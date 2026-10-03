# -*- coding: utf-8 -*-
"""Online-mall (在线商城) end-to-end test.

  python -X utf8 _mall_test.py [acct] [pwd] [char]

login (aaa 8001) -> select char -> enter world ->
  1) 0x3E24 (CMD_OPEN_ONLINE_MALL) body = s8(char) s8(client_key) u8(no)
     expect 0xFD67 (MSG_ONLINE_MALL_LIST) + 0x5109 (MSG_OPEN_ONLINE_MALL, key)
  2) 0x1166 (CMD_BUY_FROM_ONLINE_MALL) body = s8(barcode) u16(amount)
     s8(coin_pwd) s8(coin_type) s8(rebate)     expect 0xFD27 (buy result)
  3) 0x3E24 reopen
The dynamic key from step 1 is printed (it must be a 7-char string, NOT the int 1),
the mall list is scraped for legal barcodes, and every 0xFD27 result is decoded.
"""
import hashlib
import re
import socket
import struct
import sys
import time
import importlib.util

spec = importlib.util.spec_from_file_location('ep', '_enter_probe.py')
ep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ep)

HOST = '192.168.200.1'
ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'

NAMES = {0xFD67: 'MALL_LIST', 0x5109: 'MALL_OPEN(key)', 0xFD27: 'BUY_RESULT',
         0xF061: 'CHARLIST', 0xF05D: 'OBJECT', 0x2037: 'MENU', 0x3393: 'DIALOG',
         0x1FE5: 'NOTICE', 0x211B: 'ASK_FIELDS', 0xFD64: 'GENERAL_NOTIFY',
         0x5103: 'COIN_INFO', 0x5105: 'COIN_INFO2'}


def dec(b):
    try:
        return ''.join(c if (c.isprintable() or c == ' ') else '.'
                       for c in b.decode('gbk', 'replace'))
    except Exception:
        return repr(b)


def buy_result(payload):
    """0xFD27 body = u16 result + s8 barcode."""
    if len(payload) >= 2:
        r = struct.unpack('>H', payload[:2])[0]
        bc = ''
        if len(payload) >= 3:
            n = payload[2]
            bc = payload[3:3 + n].decode('gbk', 'replace')
        return r, bc
    return None, ''


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

# --- enter world -----------------------------------------------------------
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
selected = False
t_sel = time.time()
seen = []
mall_bytes = bytearray()
t0 = time.time()
while time.time() - t0 < 40:
    r = ep.read_pkt(g, 2)
    if r is None:
        break
    if r[0] == 'timeout':
        continue
    seen.append(r[0])
    if r[0] in (0x39F1, 0xFA01, 0xF05D) and len(r[1]) >= 4 and uid is None:
        uid = struct.unpack('>I', r[1][:4])[0]
    if r[0] == 0xF061:
        c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
        if c2 > 0 and not selected:
            g.sendall(ep.frame(0x1060, ep.s8(CHAR)))
            selected = True
            t_sel = time.time()
    if selected and time.time() - t_sel > 6:
        break
print('entered: uid=%s pkts=%s' % (uid, ['0x%04X' % c for c in seen[:14]]), flush=True)


def drain(sec, collect=False, show=(), allpkts=False):
    got = []
    shown = 0
    t1 = time.time()
    while time.time() - t1 < sec:
        r = ep.read_pkt(g, 0.6)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        got.append(r)
        if collect:
            mall_bytes.extend(r[1])
        if r[0] in show:
            extra = ''
            if r[0] == 0xFD27:
                res, bc = buy_result(r[1])
                extra = ' result=%s barcode=%s' % (res, bc)
            print('  %-16s 0x%04X len=%d%s' % (NAMES.get(r[0], 'pkt'), r[0], len(r[1]), extra),
                  flush=True)
            if r[0] in (0x3393, 0x1FE5):
                print('      %s' % dec(r[1])[:200], flush=True)
        elif allpkts and shown < 15:
            shown += 1
            print('  %-16s 0x%04X len=%d %s' % (NAMES.get(r[0], 'pkt'), r[0], len(r[1]),
                                                dec(r[1])[:90]), flush=True)
    return got


drain(3.0)

# --- 1) open the online mall ----------------------------------------------
print('--- 0x3E24 open_online_mall (name=%s) ---' % CHAR, flush=True)
g.sendall(ep.frame(0x3E24, ep.s8(CHAR) + ep.s8('') + bytes([0])))
got = drain(6.0, collect=True, show=(0xFD67, 0x5109, 0xFD27, 0x5103, 0x5105, 0x3393))
key = None
for cmd, payload in got:
    if cmd == 0x5109 and len(payload) >= 5:
        n = payload[4]
        key = payload[5:5 + n].decode('gbk', 'replace')
        print('      key=%r (len=%d) raw=%s' % (key, n, payload.hex()), flush=True)

barcodes = []
for m in re.finditer(rb'T[0-9A-Z]{6,10}', bytes(mall_bytes)):
    bc = m.group(0).decode('ascii')
    if bc not in barcodes:
        barcodes.append(bc)
print('barcodes: %s' % barcodes[:8], flush=True)

# --- 2) buy attempts --------------------------------------------------------
# Barcodes are taken from etc/onlinemall.list:
#   T00000003 惊妖铃       328 元宝 coin_type 3 (gold or silver), no time window
#   T00000004 飞行符       216 元宝 coin_type 1 (gold only)
#   T00000006 特级血玲珑  1600 元宝 coin_type 3
#   T00000051 28清凉一夏养育礼包 2800 元宝 coin_type 1, gold window
#             2014-06-26..2014-07-10 -> EXPIRED, fails SILENTLY (no dialog:
#             the config has no `msg`), which is correct upstream behaviour and
#             is kept as a control case.
# A silent failure answers within milliseconds; a precost attempt answers only
# after the aaa round trip or the 30s do_when_precost_timeout, so wait it out
# before the next attempt (otherwise oper_map[gid] is still held and later
# attempts fail early with the "正在处理" path).
attempts = [
    ('T00000003', 'gold_coin', ''),
    ('T00000004', 'gold_coin', ''),
    ('T00000006', 'gold_coin', ''),
    ('T00000003', 'silver_coin', ''),
    ('T00000051', 'gold_coin', ''),
]
if not barcodes:
    print('!! no barcodes scraped from the mall list', flush=True)

for idx, (barcode, coin, pwd) in enumerate(attempts):
    print('--- 0x1166 buy #%d barcode=%s coin=%s pwd=%r ---' % (idx + 1, barcode, coin, pwd),
          flush=True)
    g.sendall(ep.frame(0x1166, ep.s8(barcode) + struct.pack('>H', 1) + ep.s8(pwd)
                       + ep.s8(coin) + ep.s8('')))
    got = drain(4.0, show=(0xFD27, 0x3393, 0x5103, 0x5105), allpkts=True)
    if any(c == 0xFD27 for c, _ in got):
        continue
    print('      (no immediate reply -> precost path? waiting 32s for the timeout)', flush=True)
    got2 = drain(32.0, show=(0xFD27, 0x3393, 0x5103, 0x5105), allpkts=True)
    if not any(c == 0xFD27 for c, _ in got + got2):
        print('  !! NO 0xFD27 for attempt #%d' % (idx + 1), flush=True)

# --- 3) reopen the mall -----------------------------------------------------
print('--- 0x3E24 reopen ---', flush=True)
g.sendall(ep.frame(0x3E24, ep.s8(CHAR) + ep.s8('') + bytes([0])))
drain(6.0, show=(0xFD67, 0x5109, 0x3393))

g.close(); s.close()
