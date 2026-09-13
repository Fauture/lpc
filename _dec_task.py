# -*- coding: utf-8 -*-
"""Decode the 0x2603 (task log) and 0x20D1 (talk drama) bodies captured by the
[send] probe, following the LPC byte layout that produced them.

  python -X utf8 _dec_task.py <hexfile-or-inline-hex>
"""
import io, sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')


def gb(b):
    return b.decode('gbk', 'replace')


class R:
    def __init__(self, d):
        self.d = d
        self.o = 0

    def u8(self):
        v = self.d[self.o]
        self.o += 1
        return v

    def u16(self):
        v = (self.d[self.o] << 8) | self.d[self.o + 1]
        self.o += 2
        return v

    def u32(self):
        v = int.from_bytes(self.d[self.o:self.o + 4], 'big')
        self.o += 4
        return v

    def raw(self, n):
        v = self.d[self.o:self.o + n]
        self.o += n
        return v

    def s8(self):
        n = self.u8()
        return self.raw(n)


def dec2603(body):
    r = R(body)
    print('--- 0x2603 task log ---')
    print('  int          =', r.u16())
    print('  task_name    =', gb(r.s8()))
    n = r.u16()
    print('  desc(%d)      = %s' % (n, gb(r.raw(n))))
    n = r.u16()
    print('  log(%d)       = %s' % (n, gb(r.raw(n))))
    n = r.u16()
    print('  tip(%d)       = %s' % (n, gb(r.raw(n))))
    print('  refresh(u16) =', r.u16())
    print('  make8 common =', r.u8())
    print('  make8 type   =', r.u8())
    print('  make8 tip    =', r.u8())
    print('  make8 diff   =', r.u8())
    n = r.u8()
    print('  person(%d)    = %s' % (n, gb(r.raw(n))))
    n = r.u8()
    print('  level(%d)     = %s' % (n, gb(r.raw(n))))
    n = r.u16()
    print('  bonus(%d)     = %s' % (n, gb(r.raw(n))))
    print('  task_type    =', gb(r.s8()))
    print('  att32        =', r.u32())
    print('  left bytes   =', len(body) - r.o, body[r.o:].hex().upper())
    assert r.o == len(body), 'MISALIGNED: consumed %d of %d' % (r.o, len(body))
    print('  OK: consumed exactly %d bytes' % r.o)


def dec20d1(body):
    r = R(body)
    print('--- 0x20D1 talk drama ---')
    print('  talk_npc id  =', r.u32())
    n = r.u8()
    print('  para(%d)      = %r' % (n, r.raw(n)))
    print('  remaining    = %d bytes' % (len(body) - r.o))
    # fragments: looping over `content`; each fragment starts with make8(type)
    off = r.o
    cnt = 0
    while off < len(body) and cnt < 40:
        t = body[off]
        print('   frag@%4d type=%-3d %s' % (off, t, body[off:off + 24].hex().upper()))
        off += 1
        cnt += 1
        # crude walk: skip the most common payload sizes so the dump stays readable
        break
    print('  first 200 bytes:', body[:200].hex().upper())


if __name__ == '__main__':
    src = sys.argv[1]
    try:
        d = io.open(src, 'rb').read()
        txt = d.decode('ascii', 'replace')
    except Exception:
        txt = src
    for line in txt.splitlines():
        line = line.strip()
        if 'cmd=0x2603' in line:
            hx = line.split('hex=')[1]
            body = bytes.fromhex(hx)[2:]   # drop the cmd
            dec2603(body)
        if 'cmd=0x20d1' in line:
            hx = line.split('hex=')[1]
            body = bytes.fromhex(hx)[2:]
            dec20d1(body)
