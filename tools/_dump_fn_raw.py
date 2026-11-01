# -*- coding: utf-8 -*-
"""Dump raw operands + arg vm_idx table for a given function in a .o file.
usage: python -X utf8 tools/_dump_fn_raw.py <path.o> <funcname> [maxn]
"""
import sys
import importlib.util

spec = importlib.util.spec_from_file_location(
    "dec37", "tools/lpc_decompiler_v37.py")
dec = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dec)

path, fname = sys.argv[1], sys.argv[2]
maxn = int(sys.argv[3]) if len(sys.argv) > 3 else 40

o = dec.LPCOFile(path, encoding='gb2312').parse()
prog = o.prog
segs = dec.find_prologue_segments(prog)
dlt_segs = getattr(o, 'dlt_func_segs', {})
if dlt_segs:
    segs = [s for s in segs if s['ns'] in dlt_segs]
    for s in segs:
        nc_dlt = dlt_segs.get(s['ns'])
        if nc_dlt and nc_dlt != s['nc']:
            s['nc'] = nc_dlt
    existing_ns = {s['ns'] for s in segs}
    for ns_dlt, nc_dlt in dlt_segs.items():
        if ns_dlt not in existing_ns and nc_dlt > 1:
            segs.append({'ns': ns_dlt, 'nc': nc_dlt, 'ne': ns_dlt + nc_dlt*4,
                         'func_name': None, 'name_pos': 0})
dlt_names = getattr(o, 'dlt_func_names_by_seg', {})
for s in segs:
    nm = dlt_names.get(s['ns'])
    if nm:
        s['func_name'] = nm

target = None
for s in segs:
    if s.get('func_name') == fname:
        target = s
        break
if not target:
    print("function %s not found; have: %s" %
          (fname, sorted({s.get('func_name') for s in segs})[:30]))
    sys.exit(1)

ns, nc = target['ns'], target['nc']
# locate args by segment via func_args_by_seg
args_by_seg = getattr(o, 'func_args_by_seg', {})
locals_by_seg = getattr(o, 'func_local_vars_by_seg', {})
print("seg ns=%d nc=%d" % (ns, nc))
args_list = args_by_seg.get(ns, [])
locals_list = locals_by_seg.get(ns, {})
print("ARGS  (vm_idx -> name):", sorted(args_list))
print("LOCALS(vm_idx -> name):", sorted(locals_list))
print("---- instructions ----")
for ni in range(min(nc, maxn)):
    b0, b1, b2, b3 = prog[ns + ni*4: ns + ni*4 + 4]
    an = b3
    p16 = b0 | (b1 << 8)
    if p16 >= 0x8000:
        p16s = p16 - 0x10000
    else:
        p16s = p16
    print("[%3d] op=%3d p16=%5d (signed %5d) p8=%3d" % (ni, an, p16, p16s, b2))
