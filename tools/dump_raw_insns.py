# -*- coding: utf-8 -*-
"""Dump raw instruction stream of a named function from a .o file.
Usage: python dump_raw_insns.py <file.o> <function_name> [offset] [count]
Prints: index, opcode(an), p16, p8 for every instruction, so the caller can
cross-check decompiler labels against the raw bytes.
"""
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lpc_decompiler_v37 import LPCOFile, find_prologue_segments, match_func_names  # noqa


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    path, fname = sys.argv[1], sys.argv[2]
    enc = 'gb2312'
    o = LPCOFile(path, encoding=enc).parse()
    prog = o.prog

    segs = find_prologue_segments(prog)
    dlt_segs = getattr(o, 'dlt_func_segs', {})
    if dlt_segs:
        segs = [s for s in segs if s['ns'] in dlt_segs]
    dlt_names = getattr(o, 'dlt_func_names_by_seg', {})
    for s in segs:
        nm = dlt_names.get(s['ns'])
        if nm:
            s['func_name'] = nm
    match_names = getattr(o, 'all_symbol_names', o.user_funcs)
    match_name_bytes = getattr(o, 'all_name_bytes', None)
    segs_to_match = [s for s in segs if not s.get('func_name')]
    segs_to_match = match_func_names(prog, match_names, segs_to_match, match_name_bytes, encoding=enc)
    matched_dict = {s['ns']: s for s in segs_to_match}
    for s in segs:
        if not s.get('func_name') and s['ns'] in matched_dict:
            s['func_name'] = matched_dict[s['ns']].get('func_name')

    target = None
    for s in segs:
        if s.get('func_name') == fname or (s.get('func_name') and fname in s['func_name']):
            target = s
            break
    if not target:
        print("function %r not found; segments:" % fname)
        for s in sorted(segs, key=lambda x: x['ns']):
            print("  ns=%d nc=%d name=%r" % (s['ns'], s['nc'], s.get('func_name')))
        return 1

    ns, nc = target['ns'], target['nc']
    print("### function %r ns=%d nc=%d" % (target.get('func_name'), ns, nc))
    for ni in range(nc):
        off = ns + ni * 4
        if off + 4 > len(prog):
            break
        b0, b1, b2, b3 = prog[off:off + 4]
        an = b3
        p16 = b0 | (b1 << 8)
        p8 = b2
        print("[%3d] op=%d p16=0x%04X(%d) p8=0x%02X(%d)  bytes=%02X %02X %02X %02X" %
              (ni, an, p16, p16, p8, p8, b3, b0, b1, b2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
