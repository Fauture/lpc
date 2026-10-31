# -*- coding: utf-8 -*-
"""Dump the 64-byte Vm_Fun_T file header for functions in a .o file.
Read order mirrors src/o_loader.cpp exactly.
usage: python -X utf8 tools/_fn_header.py <path.o> [name-filter]
"""
import struct
import sys


class R:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def u8(self):
        v = self.d[self.p]; self.p += 1; return v

    def u16(self):
        v = struct.unpack_from('<H', self.d, self.p)[0]; self.p += 2; return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]; self.p += 4; return v

    def take(self, n):
        v = self.d[self.p:self.p + n]; self.p += n; return v

    def cstr(self, n):
        b = self.take(n)
        z = b.find(b'\0')
        return (b[:z] if z >= 0 else b).decode('gb2312', 'replace')

    def tstr(self):
        n = self.u32()
        return self.cstr(n)

    def code_segment(self):
        symc = self.u32()
        for _ in range(symc):
            flag = self.u8()
            if flag & 1: self.tstr()
            if flag & 2: self.u32()
        nodes = self.u32()
        if nodes: self.take(4 * nodes)
        metalen = self.u32()
        if metalen: self.take(metalen)
        rtc = self.u32()
        if rtc: self.take(4 * rtc)

    def function(self):
        hdr = self.take(64)
        name = self.tstr()
        argc = self.u16()
        for _ in range(argc):
            self.take(20); self.tstr()
        locc = self.u16()
        for _ in range(locc):
            self.take(20); self.tstr()
        self.code_segment()
        return name, hdr, argc, locc


def parse(path):
    r = R(open(path, 'rb').read())
    magic = r.u32(); version = r.u32(); fbc = r.u32()
    for _ in range(r.u32()):          # defines
        r.u32(); r.tstr(); r.tstr()
    for _ in range(r.u32()):          # strings
        n = r.u32(); r.cstr(n)
    r.u32()                            # nameCount
    r.take(r.u32())                    # namePool
    r.u16()                            # globalVarCount
    pvc = r.u16()
    if fbc == 0:
        for _ in range(pvc):           # prog vars: 16 bytes + name
            r.take(16); r.tstr()
    r.u16()                            # efunCount
    r.u16()                            # functionCount
    hidden = r.u32()
    fns = []
    for _ in range(hidden):
        fns.append(r.function())
    if fbc == 0:
        r.u16()                        # maxConstitution
        inherits = r.u32()
        for _ in range(inherits):
            r.tstr(); r.take(12)
    dlt = r.u32()
    for _ in range(dlt):
        e = r.take(20)
        attrib = struct.unpack_from('<I', e, 4)[0]
        if attrib & 0x8000000:
            name, hdr, argc, locc = r.function()
            fns.append((name, hdr, argc, locc))
    return fns


def main():
    path = sys.argv[1]
    filt = sys.argv[2] if len(sys.argv) > 2 else None
    fns = parse(path)
    for name, hdr, argc, locc in fns:
        if filt and filt not in name:
            continue
        h = ' '.join('%02x' % b for b in hdr)
        u16s = [struct.unpack_from('<H', hdr, o)[0] for o in range(0, 64, 2)]
        print('%s: args=%d locals=%d' % (name, argc, locc))
        print('  hdr: %s' % h)
        print('  u16@0..: %s' % ' '.join('%d' % v for v in u16s[:8]))
        print('  u32@0: %d  u32@4: %d  u32@16: %d  u32@24: %d u32@36: %d' %
              (struct.unpack_from('<I', hdr, 0)[0],
               struct.unpack_from('<I', hdr, 4)[0],
               struct.unpack_from('<I', hdr, 16)[0],
               struct.unpack_from('<I', hdr, 24)[0],
               struct.unpack_from('<I', hdr, 36)[0]))
        # extra u16 candidates around locals count
        print('  u16@8..15: %s' % ' '.join('%d' % struct.unpack_from('<H', hdr, o)[0]
                                           for o in range(8, 16, 2)))
        print()


if __name__ == '__main__':
    main()
