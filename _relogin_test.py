# -*- coding: utf-8 -*-
"""Two reported problems, one session:

  1) 过图 (map transition) twice in a row - the second one reportedly does
     nothing.  A transition is the client packet 0x1026/0x234A/0x2212
     -> cmd_enter_room(cmdgroup, cno, new_room) -> player process_command
     -> feature/common/move.c::enter_room(room,x,y,dir).
  2) 下线后再次上线 - after the client disconnects and logs back in the flow is
     wrong (previously: it went straight into the world with a broken avatar
     instead of showing the character list, because the account was still
     considered online).

  python -X utf8 _relogin_test.py [host] [acct] [pwd] [char]

Run it while the four services are already up (boot them with
_run_probe.py <dir> <sec> LPCVM_ROOM=1).
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

sys.stdout = __import__('io').TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

HOST = sys.argv[1] if len(sys.argv) > 1 else '192.168.200.1'
ACCT = sys.argv[2] if len(sys.argv) > 2 else '1'
PWD = sys.argv[3] if len(sys.argv) > 3 else '1'
CHAR = sys.argv[4] if len(sys.argv) > 4 else 'awesrd'

ROOMS = ['/gs/zone/lanxianzhen/lanxianzhen.c',
         '/gs/zone/lanxianzhenwai/lanxianzhenwai.c',
         '/gs/zone/lanxianzhen/lanxianzhen.c',
         '/gs/zone/lanxianzhenwai/lanxianzhenwai.c',
         # rooms whose map_index was seen zeroed by a duplicate build_map in the
         # boot log ("[bm0] name-exists ...") - the regression targets.
         '/gs/zone/misc/zhengdaodian.c',
         '/gs/zone/misc/tongtianta.c',
         '/gs/zone/misc/dongfu.c']

WORLD = '20544DE38C2160D6EAC9BFE07486242E'


def aaa_login():
    """AAA 8001 -> auth_key + the game ip/port/seed."""
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
    r = ep.recv_until(s, 0x4355, time.time() + 15)
    b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2
    server = ''
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
    return s, auth_key, seed, ip, gport


def gs_login(auth_key, seed, ip, gport, want_char, create_ok):
    g = socket.create_connection((ip, gport), timeout=8)
    g.settimeout(2)
    tick = int(time.time() * 1000) & 0xFFFFFFFF
    g.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
              + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
    try:
        ep.read_pkt(g, 2)
    except Exception:
        pass
    ab = ACCT.encode('gbk', 'replace')
    vb = (WORLD + ':D41D8CD98F00B204E9800998ECF8427E').encode('ascii')
    body = (bytes([len(ab)]) + ab + struct.pack('>i', auth_key) + struct.pack('>i', seed)
            + bytes([len(vb)]) + vb + bytes([0, 0, 0]))
    g.sendall(ep.frame(0x4124, bytes(body)))

    seen = []
    charlist = None
    uid = None
    selected = False
    created = False
    t0 = time.time()
    while time.time() - t0 < 45:
        r = ep.read_pkt(g, 2)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        seen.append(r[0])
        if r[0] == 0xF061:
            cnt = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
            if charlist is None:
                charlist = cnt
            if cnt > 0 and not selected:
                g.sendall(ep.frame(0x1060, ep.s8(want_char)))
                selected = True
                t_sel = time.time()
            elif cnt == 0 and create_ok and not created:
                nb = want_char.encode('gbk')
                g.sendall(ep.frame(0x205C, bytes([len(nb)]) + nb + bytes([1, 1, 1])))
                created = True
                time.sleep(10)
        if r[0] in (0x39F1, 0xFA01, 0xF05D) and len(r[1]) >= 4 and uid is None:
            uid = struct.unpack('>I', r[1][:4])[0]
        if selected and time.time() - t_sel > 8:
            break
        if created and not selected and time.time() - t0 > 40:
            break
    return g, seen, charlist, uid, selected, created


def drain(g, seconds):
    """Collect (cmd, len) for `seconds`; returns the list."""
    out = []
    t0 = time.time()
    while time.time() - t0 < seconds:
        r = ep.read_pkt(g, 0.5)
        if r is None:
            break
        if r[0] == 'timeout':
            continue
        out.append((r[0], len(r[1])))
    return out


def show(pkts):
    return ' '.join('%04X(%d)' % (c, l) for c, l in pkts) if pkts else '(no packet)'


print('=== session 1: login / select / enter ===', flush=True)
s1, ak, seed, ip, gport = aaa_login()
g1, seen1, cnt1, uid1, sel1, cre1 = gs_login(ak, seed, ip, gport, CHAR, True)
print('charlist count=%s selected=%s created=%s uid=%s' % (cnt1, sel1, cre1, uid1), flush=True)
drain(g1, 2)

print()
print('=== 过图 x3 (client 0x1026) ===', flush=True)
for i, room in enumerate(ROOMS, 1):
    g1.sendall(ep.frame(0x1026, ep.s8(room)))
    pk = drain(g1, 4)
    print('  #%d enter_room %-46s -> %s' % (i, room, show(pk)), flush=True)

print()
print('=== session 1 ends: proper logout (0x3E2A) then disconnect ===', flush=True)
try:
    g1.sendall(ep.frame(0x3E2A, b''))     # cmd_logout -> logout.o::main -> disconnected(..,1)
    print('  sent logout 0x3E2A', flush=True)
    drain(g1, 3)
except Exception as e:
    print('  logout send failed:', e, flush=True)
g1.close()
try:
    s1.close()
except Exception:
    pass
print('  sockets closed at %s; waiting 30s for time_expired' % time.strftime('%H:%M:%S'), flush=True)
time.sleep(30)

print()
print('=== session 2: login again (再次上线) ===', flush=True)
s2, ak2, seed2, ip2, gp2 = aaa_login()
g2, seen2, cnt2, uid2, sel2, cre2 = gs_login(ak2, seed2, ip2, gp2, CHAR, False)
print('  charlist count=%s selected=%s created=%s uid=%s' % (cnt2, sel2, cre2, uid2), flush=True)
print('  packets: %s' % show([(c, 0) for c in seen2[:24]]), flush=True)
if cnt2 and cnt2 > 0:
    print('  VERDICT: 角色列表下发（选角界面）-> 正常', flush=True)
elif uid2 is not None:
    print('  VERDICT: 未下发角色列表、直接进入游戏 -> 复现「选线直接进入游戏没有选择」', flush=True)
else:
    print('  VERDICT: 两种都没看到（见上面的包序列）', flush=True)

print()
print('=== session 2: is the character usable? (walk + NPC click) ===', flush=True)
if sel2 or uid2 is not None:
    drain(g2, 2)
    # walk (cmd_multi_move_to 0xFD72: u32 uid, u32 map, u16 count, u16 x, u16 y, u16 dir)
    walk = drain(g2, 0)
    for (cx, cy) in ((61, 176), (62, 176), (60, 176)):
        body = (struct.pack('>I', uid2 or 0) + struct.pack('>I', 1000)
                + struct.pack('>H', 1) + struct.pack('>H', cx) + struct.pack('>H', cy) + struct.pack('>H', 6))
        g2.sendall(ep.frame(0xFD72, body))
        walk = drain(g2, 1.5)
        print('  walk to (%d,%d) -> %s' % (cx, cy, show(walk)), flush=True)
        if walk:
            break
    # NPC click (0x7012 u32 npc id): the walk replies carry announced npc ids;
    # just report what session 2 received overall.
    print('  session-2 packet ids: %s' % show([(c, 0) for c in seen2[:40]]), flush=True)

g2.close()
try:
    s2.close()
except Exception:
    pass
print('=== DONE ===', flush=True)
