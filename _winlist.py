# -*- coding: utf-8 -*-
"""List top-level windows (including hidden/minimized) matching a title.

usage: python -X utf8 _winlist.py [substring]
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sub = sys.argv[1] if len(sys.argv) > 1 else ''
user32 = ctypes.windll.user32


@ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
def cb(h, l):
    n = user32.GetWindowTextLengthW(h)
    if n <= 0:
        return True
    b = ctypes.create_unicode_buffer(n + 1)
    user32.GetWindowTextW(h, b, n + 1)
    if sub and sub not in b.value:
        return True
    r = wt.RECT()
    user32.GetWindowRect(h, ctypes.byref(r))
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(h, ctypes.byref(pid))
    print('0x%08X pid=%-6d vis=%d min=%d rect=(%d,%d,%dx%d) %s' % (
        h, pid.value, 1 if user32.IsWindowVisible(h) else 0,
        1 if user32.IsIconic(h) else 0, r.left, r.top, r.right - r.left, r.bottom - r.top,
        b.value[:60]))
    return True


user32.EnumWindows(cb, 0)
