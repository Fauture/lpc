# -*- coding: utf-8 -*-
"""Correlate [recv-accept] packets with the LPC dispatch line that follows them.

usage: python -X utf8 _corr.py <log> <dispatch-substring> [--last N] [--back K]
"""
import io
import re
import sys

log = sys.argv[1]
needle = sys.argv[2]
last = 10
back = 12
for i, a in enumerate(sys.argv):
    if a == '--last':
        last = int(sys.argv[i + 1])
    if a == '--back':
        back = int(sys.argv[i + 1])

recv = re.compile(r'\[recv-accept\] t=(\d+)ms client=(\d+) n=(\d+)\s+([0-9A-Fa-f]+)')
lines = io.open(log, encoding='utf-8', errors='replace').read().splitlines()
idx = [i for i, l in enumerate(lines) if needle in l]
print('dispatch hits: %d' % len(idx))
for i in idx[-last:]:
    pkt = None
    for j in range(i - 1, max(0, i - back) - 1, -1):
        m = recv.search(lines[j])
        if m:
            pkt = (j, m)
            break
    print('--- dispatch at line %d: %s' % (i + 1, lines[i].strip()[:120]))
    if pkt:
        j, m = pkt
        raw = bytes.fromhex(m.group(4))
        cmd = int.from_bytes(raw[8:10], 'big') if len(raw) >= 10 and raw[0:2] == b'MZ' else -1
        ln = int.from_bytes(raw[6:8], 'big') if len(raw) >= 8 and raw[0:2] == b'MZ' else -1
        body = raw[10:10 + max(0, ln - 2)] if ln > 0 else b''
        print('    pkt line %d t=%s cno=%s len=%s cmd=0x%04X body=%s' % (
            j + 1, m.group(1), m.group(2), m.group(3), cmd, body.hex().upper()[:80]))
    else:
        print('    (no recv-accept packet within %d lines)' % back)
