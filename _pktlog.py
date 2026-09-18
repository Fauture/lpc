# -*- coding: utf-8 -*-
"""Decode the raw client packets captured in a gs log ([recv-accept] ... <hex>).

usage: python -X utf8 _pktlog.py <log> <cmd-hex>[,<cmd-hex>...] [--last N]
"""
import io
import re
import sys

log = sys.argv[1]
cmds = set()
for tok in sys.argv[2].split(','):
    tok = tok.strip()
    if tok:
        cmds.add(int(tok, 16))
last = 40
for i, a in enumerate(sys.argv):
    if a == '--last':
        last = int(sys.argv[i + 1])

pat = re.compile(r'\[recv-accept\] t=(\d+)ms client=(\d+) n=(\d+)\s+([0-9A-Fa-f]+)')
hits = []
for line in io.open(log, encoding='utf-8', errors='replace'):
    m = pat.search(line)
    if not m:
        continue
    raw = bytes.fromhex(m.group(4))
    if len(raw) < 12 or raw[0:2] != b'MZ':
        continue
    ln = int.from_bytes(raw[6:8], 'big')
    cmd = int.from_bytes(raw[8:10], 'big')
    body = raw[10:10 + max(0, ln - 2)]
    if cmd in cmds:
        hits.append((int(m.group(1)), int(m.group(2)), cmd, body))

print('matched %d packets for cmds %s' % (len(hits), [hex(c) for c in sorted(cmds)]))
for t, cn, cmd, body in hits[-last:]:
    hx = body.hex().upper()
    dec = ''
    if len(body) >= 4:
        dec = 'id=%d(0x%X)' % (int.from_bytes(body[0:4], 'big'), int.from_bytes(body[0:4], 'big'))
    print('t=%-9d cno=%-4d cmd=0x%04X len=%-3d %s  body=%s' % (t, cn, cmd, len(body), dec, hx[:70]))
