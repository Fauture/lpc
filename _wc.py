# -*- coding: utf-8 -*-
"""Click a point given in CLIENT-WINDOW image coordinates (as _shot.py saves).

usage: python -X utf8 _wc.py <img_x> <img_y> [button]
"""
import ctypes
import ctypes.wintypes as wt
import subprocess
import sys
import time

sys.path.insert(0, '.')
import _shot  # noqa: E402

x = int(sys.argv[1])
y = int(sys.argv[2])
btn = sys.argv[3] if len(sys.argv) > 3 else 'left'

hwnd = _shot.find_client_hwnd()
rect = wt.RECT()
ctypes.windll.user32.GetWindowRect(hwnd, ctypes.byref(rect))
print('hwnd=%s rect=(%d,%d)-(%d,%d)' % (hwnd, rect.left, rect.top, rect.right, rect.bottom))
sx, sy = rect.left + x, rect.top + y
print('click screen (%d,%d)' % (sx, sy))
subprocess.run([sys.executable, '-X', 'utf8', '_click.py', str(sx), str(sy), btn])
time.sleep(0.4)
