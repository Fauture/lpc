# -*- coding: utf-8 -*-
"""Login, record EVERY packet (including the login burst), then open the NPC menu
(0x7012) for every announced object id and click menu entries.

usage: python -X utf8 _menu_drive.py [acct] [pwd] [char] [menuItem]
"""
import importlib.util
import io
import struct
import sys
import time

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
ep = ll.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '2'
PWD = sys.argv[2] if len(sys.argv) > 2 else '2'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
MENUITEM = sys.argv[4] if len(sys.argv) > 4 else ''

NAMES = {0x20D1: 'DRAMA', 0x107B: 'TALK-DRAMA', 0x1FE5: 'DIALOG', 0x2603: 'TASKLOG',
         0x115D: 'MOVE-EVENT', 0x2037: 'MENU', 0x3393: 'SYSLINE', 0x0A0F: 'TEMP-NPC-UNREG',
         0xE003: 'EXITS', 0x1155: 'POS', 0xF0AB: 'GUIDE', 0x2173: 'SHOP-GOODS',
         0x1141: 'OBJ-LIST', 0x0A0E: 'TEMP-NPC-REG', 0xF05D: 'UID', 0x20D2: 'ECHO',
         0x115F: 'NPC-IN-SIGHT', 0x1160: 'OBJ-IN-SIGHT', 0xF061: 'CHARLIST'}

seen = []
_orig = ep.read_pkt


def rec(sock, t):
    r = _orig(sock, t)
    if r and r[0] != 'timeout':
        seen.append((r[0], r[1]))
    return r


ep.read_pkt = rec


def dump(tag):
    print('--- %s: %d packets' % (tag, len(seen)), flush=True)
    for c, b in seen:
        nm = NAMES.get(c, '')
        if c == 0x1FE5:
            txt = b[1:].split(b'\x00')[0].decode('gbk', 'replace')
            print('   DIALOG len=%-5d %s' % (len(b), txt[:64]), flush=True)
        elif c == 0x20D2:
            continue
        else:
            print('   %-12s 0x%04X len=%-5d %s' % (nm, c, len(b), b[:56].hex()), flush=True)


def menu_items(b):
    """NUL-separated strings inside a 0x2037 menu buffer."""
    out = []
    for part in b.split(b'\x00'):
        if len(part) >= 2:
            try:
                s = part.decode('gbk')
            except Exception:
                continue
            if s.isprintable() and len(s) >= 2:
                out.append(part)
    return out


def main():
    s = ll.Session(ACCT, PWD, CHAR, tag='MD')
    s.connect(window=12, verbose=False)
    print('uid=%s pos=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos), flush=True)
    if not s.g:
        return
    s.pump(4)
    dump('login')
    ids = []
    for c, b in seen:
        if c in (0x1155, 0x115F, 0x1160) and len(b) >= 25:
            for off in range(13, len(b) - 3, 4):
                v = struct.unpack('>I', b[off:off + 4])[0]
                if 1000 <= v <= 999999:
                    ids.append(v)
        if c == 0x2037 and len(b) >= 4:
            ids.append(struct.unpack('>I', b[:4])[0])
    ids = list(dict.fromkeys(ids))
    print('--- candidate ids: %s' % ids[:24], flush=True)
    for oid in ids[:24]:
        s.g.sendall(ep.frame(0x7012, struct.pack('>I', oid)))
        t0 = time.time()
        while time.time() - t0 < 1.0:
            time.sleep(0.2)
    s.pump(3)
    dump('after open_menu')
    items = []
    for c, b in seen:
        if c == 0x2037:
            items += menu_items(b)
    items = list(dict.fromkeys(items))
    print('--- menu items: %s' % [i.decode('gbk', 'replace')[:24] for i in items[:12]], flush=True)
    pick = [MENUITEM.encode('gbk')] if MENUITEM else items[:6]
    for it in pick:
        for cmd in (0x000A, 0x1042, 0x30CA, 0x215E):
            s.g.sendall(ep.frame(cmd, struct.pack('>I', 0) + bytes([len(it)]) + it + b'\x00'))
        time.sleep(0.6)
    s.pump(4)
    dump('after menu click')
    s.close()


if __name__ == '__main__':
    main()
