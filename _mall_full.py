# -*- coding: utf-8 -*-
"""Print the complete ordered online-mall buy chain (NO keyword filtering).

  python -X utf8 _mall_full.py <log>

Only the repeated unrelated polling (valid_grant / get_server_status /
query_dynamic_key / set_dynamic_key / get_module_status) is dropped, because it
runs every few seconds and would bury the buy.  Everything that happens between
a `buy_from_mall` ENTER and the following `send_result` is printed verbatim.
"""
import io
import re
import sys

path = sys.argv[1] if len(sys.argv) > 1 else 'mall1/live_gs.txt'
data = io.open(path, 'rb').read().decode('utf-8', 'replace').splitlines()

NOISE = re.compile(r'fn=(valid_grant|get_server_status|get_module_status|query_dynamic_key|'
                   r'set_dynamic_key|gen_dynamic_key|is_interactive)\b')
SEND = re.compile(r'\[mall-send\]')

# locate buy attempts
idxs = [i for i, l in enumerate(data) if 'fn=buy_from_mall passed' in l]
print('buy_from_mall attempts found: %d' % len(idxs))
if not idxs:
    print('!! none - dumping every [mall] line that is not pure noise')
    for l in data:
        if '[mall]' in l and not NOISE.search(l):
            print('  ' + l[:240])
        elif SEND.search(l):
            print('  ' + l[:180])
    sys.exit(0)

for k, st in enumerate(idxs):
    en = idxs[k + 1] if k + 1 < len(idxs) else len(data)
    print('================= attempt %d =================' % (k + 1))
    for l in data[st:en]:
        if '[mall-op]' in l:
            continue
        if SEND.search(l):
            print('  SEND ' + l[l.index('[mall-send]'):][:170])
        elif '[mall]' in l and not NOISE.search(l):
            print('  ' + l[:250])
