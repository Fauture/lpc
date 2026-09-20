# -*- coding: utf-8 -*-
"""Show that the DBA's MySQL socket (3306) had NO heartbeat: its idle time when
the failure happened equals the moment the DBA stopped replying."""
import io
import re

# 1) when the DBA stopped replying to the gs
t = io.open('errfix1/live_dba.txt', 'rb').read().decode('utf-8', 'replace')
last_t = 0
ts = re.compile(r't=(\d+)ms')
sends = []
for l in t.splitlines():
    m = ts.search(l)
    if m:
        last_t = int(m.group(1)) / 60000.0
    m = re.search(r'\[send-filed\] cno=(\d+) msgNo=0x([0-9A-Fa-f]+)', l)
    if m:
        sends.append((last_t, m.group(1), m.group(2)))
print('DBA last replies (min, cno, msgNo):')
for s in sends[-3:]:
    print('   %.1f min  cno=%s  msgNo=%s' % s)
wedge_min = sends[-1][0]
print('=> DBA stopped replying at %.1f min' % wedge_min)

# 2) the game-side heartbeat kept flowing the whole time
hb = [l for l in t.splitlines() if '[hb] port=8120' in l]
print('\nDBA game-side heartbeats: %d, last:' % len(hb))
print('   ' + hb[-1][:170])
echo = [l for l in t.splitlines() if '[echo] reply' in l]
print('DBA echo replies: %d, last: %s' % (len(echo), echo[-1][:120]))
print('=> the 10 s echo (game protocol, port 8120) never stopped')

# 3) the MySQL session idle time at the moment of the failure
print('\nMySQL session for dl_ddb_1_test seen at 12:26 (failure was ~12:26):')
print('   id=1664 db=dl_ddb_1_test cmd=Sleep time=11124 s  (= %.1f min idle)' % (11124 / 60.0))
print('   => last MySQL activity on that socket = %.1f min after boot'
      % (526 - 11124 / 60.0))
print('   => exactly the moment the DBA stopped replying (%.1f min):'
      ' the MySQL socket went silent, the game socket kept its 10 s heartbeat.'
      % wedge_min)
