# -*- coding: utf-8 -*-
"""Compare two boot_smoke JSON snapshots (baseline vs current step).

Prints per-service: final Load counts, port-up status, storm lines, DB rows,
and flags a regression when loads at the end of the window are materially
lower (>= 20%) than the baseline or a port that was up is now down.
Usage: python tools/compare_boot.py regress/baseline_boot.json regress/step1_boot.json
"""
import json, sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

base = json.load(open(sys.argv[1], encoding='utf-8'))
cur = json.load(open(sys.argv[2], encoding='utf-8'))
bs, cs = base['summary'], cur['summary']
print('%-4s %-9s %-9s %-8s %-8s  %s' % ('svc', 'loadsB', 'loadsC', 'portB', 'portC', 'note'))
bad = 0
for name in ['aaa', 'dba', 'ccs', 'gs']:
    b, c = bs.get(name, {}), cs.get(name, {})
    lb, lc = b.get('loads', -1), c.get('loads', -1)
    pb, pc = b.get('port'), c.get('port')
    note = ''
    if lc < lb * 0.8 and lb > 0:
        note += ' SLOWER/STUCK'; bad += 1
    if pb and not pc:
        note += ' PORT-DOWN'; bad += 1
    if not c.get('alive', True):
        note += ' EXITED'; bad += 1
    sb = sorted(b.get('storms', []))
    sc = sorted(c.get('storms', []))
    if sb != sc:
        note += ' STORMS-DIFF'
    print('%-4s %-9s %-9s %-8s %-8s %s' % (name, lb, lc, pb, pc, note))
if 'db_dc_lg' in bs and 'db_dc_lg' in cs:
    print('DB dc/lg: base=%s cur=%s' % (bs['db_dc_lg'], cs['db_dc_lg']))
if bad:
    print('REGRESSION FLAGS: %d' % bad)
    sys.exit(1)
print('OK: boot regression parity')
