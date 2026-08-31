# -*- coding: utf-8 -*-
"""Dump function argument descriptors (Vm_Fun_Arg_T: no, defaultActionNum)
from a .o bytecode file, mirroring src/o_loader.cpp's Reader layout.
Usage: python dump_arg_meta.py <file.o> [function_name_substring]
"""
import struct
import sys


class Reader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def u16(self):
        v = self.data[self.pos] | (self.data[self.pos + 1] << 8)
        self.pos += 2
        return v

    def u32(self):
        v = struct.unpack_from('<I', self.data, self.pos)[0]
        self.pos += 4
        return v

    def bytes(self, n):
        b = self.data[self.pos:self.pos + n]
        self.pos += n
        return b

    def cstr(self, n):
        b = self.bytes(n)
        z = b.find(b'\0')
        if z >= 0:
            b = b[:z]
        return b.decode('gb2312', 'replace')

    def raw(self, n):
        b = self.bytes(n)
        return b

    def tempString(self):
        n = self.u32()
        return self.cstr(n)


def parse(path):
    data = open(path, 'rb').read()
    r = Reader(data)
    magic = r.u32()
    version = r.u32()
    functionBC = r.u32()
    defineCount = r.u32()
    for _ in range(defineCount):
        r.u32()
        r.tempString()
        r.tempString()
    stringCount = r.u32()
    for _ in range(stringCount):
        n = r.u32()
        r.cstr(n)
    nameCount = r.u32()
    nameSize = r.u32()
    r.raw(nameSize)
    r.u16()  # globalVarCount
    progVarCount = r.u16()
    if functionBC == 0:
        for _ in range(progVarCount):
            r.bytes(16)
            r.tempString()
    r.u16()  # efunCount
    r.u16()  # functionCount
    hiddenCount = r.u32()
    funcs = []
    for _ in range(hiddenCount):
        fn = parse_function(r)
        funcs.append(fn)
    # inherit table (object files)
    if functionBC == 0:
        r.u16()  # maxConstitution
        inheritCount = r.u32()
        for _ in range(inheritCount):
            r.tempString()
            r.bytes(12)
    # DLT table: function descriptors (mirrors o_loader.cpp)
    dltCount = r.u32()
    for _ in range(dltCount):
        dlt = r.bytes(20)
        fn = parse_function(r)
        nameOff = dlt[0] | (dlt[1] << 8) | (dlt[2] << 16) | (dlt[3] << 24)
        fn['nameOff'] = nameOff
        fn['dltIndex'] = dlt[16] | (dlt[17] << 8) | (dlt[18] << 16) | (dlt[19] << 24)
        funcs.append(fn)
    return funcs


def parse_function(r):
    raw64 = r.bytes(64)
    name = r.tempString()
    argCount = r.u16()
    args = []
    for i in range(argCount):
        desc = r.bytes(20)
        # Vm_Fun_Arg_T: name(4) flag(1) type(1) varAttrib(1) no(2)
        #               defaultActionNum(2) pDefaultValue(4) pNext(4)
        no = desc[8] | (desc[9] << 8)
        action = desc[10] | (desc[11] << 8)
        aname = r.tempString()
        args.append((i, no, action, aname))
    localCount = r.u16()
    for _ in range(localCount):
        r.bytes(20)
        r.tempString()
    # code segment (skip)
    symCount = r.u32()
    for _ in range(symCount):
        flag = r.u8()
        if flag & 1:
            r.tempString()
        if flag & 2:
            r.u32()
    nodesCount = r.u32()
    r.bytes(4 * nodesCount)
    metaLen = r.u32()
    r.bytes(metaLen)
    rtCount = r.u32()
    r.bytes(4 * rtCount)
    return {'name': name, 'args': args}


def main():
    path = sys.argv[1]
    sub = sys.argv[2] if len(sys.argv) > 2 else ''
    funcs = parse(path)
    for fn in funcs:
        if sub and sub not in fn['name']:
            continue
        print('### %s' % fn['name'])
        for (i, no, action, aname) in fn['args']:
            print('  arg[%d] no=%d defaultActionNum=%d name=%s' % (i, no, action, aname))
    return 0


if __name__ == '__main__':
    sys.exit(main())
