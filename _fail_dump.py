# -*- coding: utf-8 -*-
"""Post-mortem dump for the "gs long uptime -> 通讯错误" watch.

usage: python -X utf8 _fail_dump.py <logdir> [tail_lines]

Prints, for every service log in <logdir>:
  * the last tail_lines lines (the moment of failure)
  * gs health: [hb] trend (clients/pktsIn/sendFail/schedIters), [accept] wait
    times (the new-connection registration latency), [send-fail] counts
  * every line mentioning a timeout / failure / disconnect keyword
so the failing moment can be read without grepping by hand.
"""
import io
import os
import re
import sys

D = sys.argv[1] if len(sys.argv) > 1 else 'errfix1'
TAIL = int(sys.argv[2]) if len(sys.argv) > 2 else 25
KEYS = ('timeout', 'fail', 'error', 'err=', 'refused', 'reset', 'closed', 'disconnect',
        'no interface', 'crash', 'gone away')

for name in ('gs', 'ccs', 'aaa', 'dba'):
    p = os.path.join(D, 'live_%s.txt' % name)
    if not os.path.exists(p):
        continue
    txt = io.open(p, 'rb').read().decode('utf-8', 'replace')
    lines = txt.splitlines()
    print('=' * 78)
    print('%s : %d lines, %d bytes' % (p, len(lines), len(txt)))
    hb = [l for l in lines if '[hb]' in l]
    if hb:
        print('--- [hb] trend (every %d) ---' % max(1, len(hb) // 8))
        for l in hb[::max(1, len(hb) // 8)]:
            print('   ' + l[:190])
        print('   LAST: ' + hb[-1][:190])
    acc = [l for l in lines if '[accept]' in l]
    if acc:
        waits = [int(m.group(1)) for m in (re.search(r'wait=(\d+)ms', l) for l in acc) if m]
        print('--- [accept] count=%d  max_wait=%dms  last_wait=%dms ---'
              % (len(acc), max(waits) if waits else -1, waits[-1] if waits else -1))
        for l in acc[-4:]:
            print('   ' + l[:170])
    sf = [l for l in lines if 'send-fail' in l or 'sendFail' in l]
    if sf:
        print('--- send-fail lines: %d (last 5) ---' % len(sf))
        for l in sf[-5:]:
            print('   ' + l[:170])
    hits = [l for l in lines if any(k in l.lower() for k in KEYS)]
    print('--- keyword hits: %d (last 12) ---' % len(hits))
    for l in hits[-12:]:
        print('   ' + l[:190])
    print('--- tail %d ---' % TAIL)
    for l in lines[-TAIL:]:
        print('   ' + l[:190])
