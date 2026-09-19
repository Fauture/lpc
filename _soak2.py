# -*- coding: utf-8 -*-
"""Real-client soak watcher: sample the client window title + the gs log.

usage: python -X utf8 _soak2.py <logdir> [minutes] [charname]

Reports every 10 s: elapsed, window title, how many 0x5108 challenges arrived,
how many 0x21B1 answers were sent, and whether the client sent 0x3e2a (logout)
or the title lost the character name (dropped back to the login screen).
"""
import io
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, '.')
import _shot  # noqa: E402

LOGDIR = sys.argv[1] if len(sys.argv) > 1 else 'acfix4'
MINUTES = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
LOG = os.path.join(LOGDIR, 'live_gs.txt')

t0 = time.time()
deadline = t0 + MINUTES * 60
last_len = 0
ac_recv = 0
ac_sent = 0
logout = 0
first_ack_at = None
last_ack = None

print('soak start: char=%s log=%s for %.1f min' % (CHAR, LOG, MINUTES), flush=True)
while time.time() < deadline:
    el = time.time() - t0
    # --- log tail scan (incremental)
    try:
        data = io.open(LOG, 'rb').read().decode('utf-8', 'replace')
    except Exception:
        data = ''
    if len(data) != last_len:
        chunk = data[last_len:]
        last_len = len(data)
        ac_recv += chunk.count('0x5108')
        for m in re.finditer(r'\[ac\] sent 0x21B1 cno=(\d+) x=(-?\d+) y=(-?\d+) answer=(-?\d+)', chunk):
            ac_sent += 1
            last_ack = m.group(0)
            if first_ack_at is None:
                first_ack_at = el
        logout += len(re.findall(r'\[recv\] t=\d+ cno=\d+ cmd=0x3e2a', chunk))
    # --- client window state
    title = ''
    try:
        import ctypes
        import ctypes.wintypes as wt
        user32 = ctypes.windll.user32
        found = []

        @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
        def cb(h, l):
            n = user32.GetWindowTextLengthW(h)
            b = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(h, b, n + 1)
            if '问道(1.60' in b.value:
                found.append(b.value)
            return True
        user32.EnumWindows(cb, 0)
        title = found[0] if found else '<no client window>'
    except Exception as e:
        title = '<err %s>' % e
    ingame = ('[%s]' % CHAR) in title
    print('[%6.1fs] ingame=%s title=%s | 0x5108=%d 0x21B1=%d logout=%d'
          % (el, 'YES' if ingame else 'NO ', title.strip(), ac_recv, ac_sent, logout), flush=True)
    if ac_sent and el > 30:
        shot = '_soak_%03ds.jpg' % int(el)
        if int(el) % 60 < 12:
            subprocess.run([sys.executable, '-X', 'utf8', '_shot.py', shot, '700', '55', 'client'],
                           capture_output=True)
    time.sleep(10)

print('--- summary ---')
print('challenges 0x5108 : %d' % ac_recv)
print('answers    0x21B1 : %d' % ac_sent)
print('client logout     : %d' % logout)
print('first answer at   : %s' % ('%.1fs' % first_ack_at if first_ack_at else 'never'))
print('last answer       : %s' % (last_ack or 'none'))
