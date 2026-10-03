# -*- coding: utf-8 -*-
"""Object-variable slot map for a .o (mirrors src/o_loader.cpp).

Prints, for one object file and its whole inherit chain, the absolute slot of
every instance variable - slot = baseVarOffset(constitution) + valueNo, exactly
what the original's vm_getObjVar (0x80BC5C0) and vm_pushObjVar compute against
pObject->pObjectValues, which vm_allocateObject sizes with pProgram->maxVariableNo
(the u16 the engine calls globalVarCount).

usage: python -X utf8 _ovars.py linux/gs/pack_data/lib_gs32/gs/clone/npc/basic.o [varname ...]
"""
import io
import os
import struct
import sys


class R:
    def __init__(self, d):
        self.d = d
        self.p = 0

    def u8(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def u16(self):
        v = self.d[self.p] | (self.d[self.p + 1] << 8)
        self.p += 2
        return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

    def raw(self, n):
        v = self.d[self.p:self.p + n]
        self.p += n
        return v

    def cstr(self, n):
        return self.raw(n).split(b'\0')[0]

    def tstr(self):
        return self.cstr(self.u32()).decode('gbk', 'replace')


def code_segment(r):
    n = r.u32()
    for _ in range(n):
        f = r.u8()
        if f & 1:
            r.tstr()
        if f & 2:
            r.u32()
    nodes = r.u32()
    if nodes:
        r.raw(4 * nodes)
    meta = r.u32()
    if meta:
        r.raw(meta)
    rt = r.u32()
    if rt:
        r.raw(4 * rt)


def function(r):
    r.raw(64)
    name = r.tstr()
    argc = r.u16()
    for _ in range(argc):
        r.raw(20)
        r.tstr()
    loc = r.u16()
    for _ in range(loc):
        r.raw(20)
        r.tstr()
    code_segment(r)
    return name


class O:
    def __init__(self, path):
        r = R(io.open(path, 'rb').read())
        self.path = path
        self.magic = r.u32()
        self.version = r.u32()
        self.function_bc = r.u32()
        ndef = r.u32()
        for _ in range(ndef):
            r.u32()
            r.tstr()
            r.tstr()
        nstr = r.u32()
        for _ in range(nstr):
            r.cstr(r.u32())
        nname = r.u32()
        r.raw(r.u32())
        self.global_var_count = r.u16()
        nprog = r.u16()
        self.vars = []          # (name, raw16)
        if self.function_bc == 0:
            for _ in range(nprog):
                raw = r.raw(16)
                self.vars.append((r.tstr(), raw))
        r.u16()                 # efunCount
        r.u16()                 # functionCount
        nhid = r.u32()
        for _ in range(nhid):
            function(r)
        self.inherits = []      # (path, baseFunOffset, baseVarOffset, raw)
        if self.function_bc == 0:
            r.u16()             # maxConstitution
            ninh = r.u32()
            for _ in range(ninh):
                path = r.tstr()
                raw = r.raw(12)
                bfo = struct.unpack_from('<h', raw, 2)[0]
                bvo = struct.unpack_from('<h', raw, 4)[0]
                self.inherits.append((path, bfo, bvo, raw))


def find_o(root, vpath):
    rel = vpath.lstrip('/')
    if rel.endswith('.c'):
        rel = rel[:-2] + '.o'
    for cand in (os.path.join(root, rel),
                 os.path.join(root, rel.replace('/', os.sep))):
        if os.path.isfile(cand):
            return cand
    # the pak tree keeps a copy under <lib>/<first component>/...
    for base, _dirs, files in os.walk(root):
        for f in files:
            if f == os.path.basename(rel) and base.replace('\\', '/').endswith(os.path.dirname(rel).replace('\\', '/')):
                return os.path.join(base, f)
    return None


def walk(root, path, base, seen, out, depth=0):
    o = O(path)
    for nm, raw in o.vars:
        # Vm_Prog_Var_T.valueNo is at file offset 0xa (IDA: struct offs 0xa, i16);
        # the original's slot is valueNo + <constitution baseVarOffset>.
        vno = struct.unpack_from('<h', raw, 0xa)[0]
        out.append((base + vno, nm, o.path, raw))
    if depth > 12:
        return o
    for vpath, _bfo, bvo, _raw in o.inherits:
        p2 = find_o(root, vpath)
        if not p2 or p2 in seen:
            continue
        seen.add(p2)
        walk(root, p2, base + bvo, seen, out, depth + 1)
    return o



def chain(root, path, base, seen, depth=0):
    o = O(path)
    print('%s%s base=%d gvc=%d ownVars=%d' % ('  ' * depth, path.replace(root, ''), base,
                                              o.global_var_count, len(o.vars)))
    for vpath, _f, bvo, _r in o.inherits:
        p2 = find_o(root, vpath)
        if not p2 or p2 in seen:
            print('%s  skip %s -> %s' % ('  ' * depth, vpath, p2))
            continue
        seen.add(p2)
        chain(root, p2, base + bvo, seen, depth + 1)


def main():
    top = sys.argv[1]
    root = os.path.dirname(os.path.dirname(os.path.dirname(top)))  # .../pack_data/lib_xxx
    names = sys.argv[2:]
    out = []
    topobj = walk(root, top, 0, {top}, out)
    mx = max([s for s, _n, _p, _r in out], default=-1)
    print('file            : %s' % top)
    print('globalVarCount  : %d   (original: pProgram->maxVariableNo)' % topobj.global_var_count)
    print('progVars (own)  : %d' % len(topobj.vars))
    print('inherit entries : %s' % [(p, bvo) for p, _f, bvo, _r in topobj.inherits])
    print('vars in chain   : %d   max slot used = %d' % (len(out), mx))
    dup = {}
    for s, nm, pp, _r in out:
        dup.setdefault(s, []).append('%s(%s)' % (nm, os.path.basename(pp)))
    bad = {s: v for s, v in dup.items() if len(v) > 1}
    if bad:
        print('SLOT COLLISIONS : %s' % bad)
    fit = 'OK' if topobj.global_var_count > mx else '*** TOO SMALL - inherited slots are out of range ***'
    print('array size check: %s' % fit)
    if '-chain' in names:
        chain(root, top, 0, {top}, 0)
        return
    for s, nm, p, raw in sorted(out):
        flag = ' <== OOB' if s >= topobj.global_var_count else ''
        if not names or any(n in nm for n in names):
            print('  slot %-4d %-34s from=%s%s' % (s, nm, os.path.basename(p), flag))


main()
