# -*- coding: utf-8 -*-
"""Login helper shared by the probes: aaa -> ccs -> gs, char select, packet pump.

Kept deliberately close to what the real client sends (see _enter_probe.py).
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

HOST = '192.168.200.1'
WORLD = '20544DE38C2160D6EAC9BFE07486242E'


def parse_pos(b):
    """0x1155 body: map_id(4) source_id(4) name(s8) x(2) y(2) dir(2) ..."""
    mid, sid = struct.unpack('>II', b[:8])
    o = 8
    ln = b[o]; o += 1
    name = b[o:o + ln].decode('gbk', 'replace'); o += ln
    x, y, d = struct.unpack('>HHH', b[o:o + 6])
    return mid, sid, name, x, y, d


class Session(object):
    """aaa session + gs connection with the packet pump."""

    def __init__(self, acct, pwd, char, tag=''):
        self.acct, self.pwd, self.char, self.tag = acct, pwd, char, tag
        self.nameb = char.encode('gbk')
        self.uid = None
        self.pos = None
        self.cmds = []
        self.dialogs = []
        self.selected = False
        self.created = False
        self.t0 = time.time()
        self.s = None
        self.g = None

    def mark(self, msg):
        print('  [%6.2fs] %-4s %s' % (time.time() - self.t0, self.tag, msg), flush=True)

    # ---------------------------------------------------------------- login
    def connect(self, select=True, window=20.0, verbose=False):
        s = socket.create_connection((HOST, 8001), timeout=6)
        self.s = s
        tick = int(time.time() * 1000) & 0xFFFFFFFF
        s.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
                  + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
        ep.read_pkt(s, 3)
        s.sendall(bytes.fromhex('4D5A0000010F240C00640B05'
            '2036413430453141313130394535443933363336424630343645373338363144454032314137364445363344373442443437454231323131463338324330384639434633393538353445463146413237363541334532383437394133393346444241'))
        ep.read_pkt(s, 3)
        s.sendall(bytes.fromhex('4D5A0000010F246A00061B0300000000'))
        r = ep.read_pkt(s, 8)
        if not r:
            self.mark('AAA: no cookie')
            return None
        b = r[1]; off = 4; ln = b[off]; off += 1
        cookie = b[off:off + ln].decode('gbk', 'replace')
        enc = ep.des_encrypt(hashlib.md5(self.pwd.encode('gbk', 'replace')).hexdigest().upper().encode('ascii'), cookie)
        body = (ep.s8(self.acct) + ep.s8(enc.hex().upper().encode('ascii')) + ep.s8('0000000000000000')
                + ep.s8('') + ep.s8('') + ep.s8('试剑内测') + bytes([0]) + ep.s8(WORLD) + ep.s8(''))
        s.sendall(ep.frame(0x2350, body))
        r = ep.read_pkt(s, 12)
        if not r or r[0] != 0x5351:
            self.mark('AAA: no auth reply (%s)' % (hex(r[0]) if r else 'timeout'))
            return None
        auth_key = struct.unpack('>i', r[1][4:8])[0]
        s.sendall(ep.frame(0x3354, ep.s8(self.acct) + struct.pack('>i', auth_key) + ep.s8('试剑内测') + bytes([0])))
        r = ep.recv_until(s, 0x4355, time.time() + 15)
        if not r:
            self.mark('CCS: no server list')
            return None
        b = r[1]; cnt = struct.unpack('>H', b[:2])[0]; off = 2; server = ''
        for _ in range(cnt):
            sl = b[off]; off += 1
            server = b[off:off + sl].decode('gbk', 'replace'); off += sl
            sl = b[off]; off += 1
            off += sl + 2
            break
        s.sendall(ep.frame(0x3356, ep.s8(self.acct) + struct.pack('>i', auth_key) + ep.s8(server)))
        r = ep.recv_until(s, 0x3357, time.time() + 12)
        if not r:
            self.mark('CCS: no agent')
            return None
        b = r[1]; off = 6
        ln = b[off]; off += 1
        ip = b[off:off + ln].decode('gbk', 'replace'); off += ln
        gport = struct.unpack('>h', b[off:off + 2])[0]; off += 2
        seed = struct.unpack('>i', b[off:off + 4])[0]
        g = socket.create_connection((ip, gport), timeout=6)
        g.settimeout(2)
        self.g = g
        tick = int(time.time() * 1000) & 0xFFFFFFFF
        g.sendall(b'MZ\x00\x00' + struct.pack('>I', tick) + struct.pack('>H', 10)
                  + struct.pack('>H', 0x20D2) + struct.pack('>I', tick) + b'\xFF\xFF\xFF\xFF')
        try:
            ep.read_pkt(g, 2)
        except Exception:
            pass
        ab = self.acct.encode('gbk', 'replace')
        vb = (WORLD + ':D41D8CD98F00B204E9800998ECF8427E').encode('ascii')
        body = (bytes([len(ab)]) + ab + struct.pack('>i', auth_key) + struct.pack('>i', seed)
                + bytes([len(vb)]) + vb + bytes([0, 0, 0]))
        g.sendall(ep.frame(0x4124, bytes(body)))
        self.select = select
        self.pump(window, verbose=verbose)
        return g

    # ---------------------------------------------------------------- pump
    def pump(self, secs, verbose=False, select_timeout=None):
        t0 = time.time()
        while time.time() - t0 < secs:
            r = ep.read_pkt(self.g, 0.4)
            if r is None:
                self.mark('GS closed the connection')
                return False
            if r[0] == 'timeout':
                continue
            self.cmds.append(r[0])
            if r[0] == 0x1155 and len(r[1]) >= 21:
                try:
                    self.pos = parse_pos(r[1])
                except Exception:
                    pass
            if r[0] == 0xF05D and len(r[1]) >= 8 and self.nameb in r[1]:
                self.uid = struct.unpack('>I', r[1][:4])[0]
            if r[0] == 0x1FE5:
                txt = r[1][1:].split(b'\x00')[0].decode('gbk', 'replace')
                self.dialogs.append(txt)
                self.mark('DIALOG(0x1FE5): %s' % txt)
            if r[0] == 0x3393:
                txt = r[1][20:].split(b'\x00')[0].decode('gbk', 'replace')
                if verbose:
                    self.mark('0x3393: %s' % txt[:80])
            if r[0] == 0xF061:
                c2 = struct.unpack('>H', r[1][:2])[0] if len(r[1]) >= 2 else 0
                self.mark('char list 0xF061 count=%d' % c2)
                if self.select:
                    if c2 == 0 and not self.created:
                        nb = self.nameb
                        self.g.sendall(ep.frame(0x205C, bytes([len(nb)]) + nb + bytes([1, 1, 1])))
                        self.created = True
                        self.mark('created char %s' % self.char)
                    elif c2 > 0 and not self.selected:
                        self.g.sendall(ep.frame(0x1060, ep.s8(self.char)))
                        self.selected = True
                        self.mark('selected char %s' % self.char)
            elif verbose and r[0] not in (0x20D2,):
                self.mark('recv 0x%04X len=%d %s' % (r[0], len(r[1]), r[1].hex()[:80]))
        return True

    def query_pos(self, timeout=3.0):
        """Server-side position: a room name that cannot be an exit makes
        enter_room fall through to refresh_current_pos() -> 0x1155."""
        self.pos = None
        rb = b'###nope###'
        self.g.sendall(ep.frame(0x1026, bytes([len(rb)]) + rb))
        t0 = time.time()
        while time.time() - t0 < timeout:
            r = ep.read_pkt(self.g, 0.4)
            if r is None or r[0] == 'timeout':
                continue
            if r[0] == 0x1155 and len(r[1]) >= 21:
                self.pos = parse_pos(r[1])
                return self.pos
        return None

    def hard_close(self):
        """RST instead of FIN: the client 'crashed'."""
        try:
            self.g.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        except Exception:
            pass
        try:
            self.g.close()
        except Exception:
            pass
        try:
            self.s.close()
        except Exception:
            pass

    def close(self):
        for sock in (self.g, self.s):
            try:
                sock.close()
            except Exception:
                pass
