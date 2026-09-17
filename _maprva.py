# -*- coding: utf-8 -*-
"""Map lpcvm.exe RVAs to symbol names using the linker map file.

The LPCVM_STACKDBG probe prints raw stack words that point into lpcvm.exe as
RVAs (`[nstack] ... rvas: 1a2b3c ...`).  build/Release/lpcvm.map (enabled with
<GenerateMapFile> in build/lpcvm.vcxproj) lists every public symbol with its
preferred address (base 0x400000), so an RVA is `addr - 0x400000`.

usage:
  python -X utf8 _maprva.py 1a2b3c 4d5e6f ...      # explicit RVAs (hex)
  python -X utf8 _maprva.py --log sp5/live_gs.txt  # every [nstack] line
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
MAP = os.path.join(ROOT, 'build', 'Release', 'lpcvm.map')
PREFERRED_BASE = 0x400000

# " 0001:00001234       ?foo@@YAXXZ   00401234     foo.obj"
SYM = re.compile(r'^\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s')


def load_symbols(path=MAP):
    syms = []
    with io.open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = SYM.match(line)
            if not m:
                continue
            addr = int(m.group(4), 16)
            syms.append((addr - PREFERRED_BASE, m.group(3)))
    syms.sort()
    return syms


def nearest(syms, rva):
    lo, hi = 0, len(syms) - 1
    best = None
    while lo <= hi:
        mid = (lo + hi) // 2
        if syms[mid][0] <= rva:
            best = syms[mid]
            lo = mid + 1
        else:
            hi = mid - 1
    if best is None:
        return '?', 0
    return best[1], rva - best[0]


def main(argv):
    syms = load_symbols()
    print('symbols: %d (map %s)' % (len(syms), MAP))
    rvas = []
    logs = []
    for a in argv:
        if a == '--log':
            logs.append(True)
        elif logs:
            logs[-1] = a
        else:
            rvas.append(a)
    if logs:
        for lg in logs:
            for line in io.open(lg, encoding='utf-8', errors='replace'):
                if '[nstack]' not in line:
                    continue
                head, _, tail = line.partition('rvas:')
                print('== %s' % head.strip())
                for i, tok in enumerate(tail.split()):
                    try:
                        rva = int(tok, 16)
                    except ValueError:
                        continue
                    name, off = nearest(syms, rva)
                    print('   #%-3d rva=0x%-8x %s +0x%x' % (i, rva, name, off))
    for tok in rvas:
        rva = int(tok, 16)
        name, off = nearest(syms, rva)
        print('rva=0x%-8x %s +0x%x' % (rva, name, off))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
