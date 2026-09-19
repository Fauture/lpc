# -*- coding: utf-8 -*-
"""Type arbitrary Unicode text into the client with SendInput KEYEVENTF_UNICODE.

usage: python -X utf8 _utype.py <text> [--enter]

The legacy scan-code path (_type.py) cannot produce Chinese; the WM_CHAR path
(_postchar.py) is ignored unless the game's text field already owns focus.  A
unicode SendInput packet carries the character itself, which the client's raw
input reader sees as typed text.
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

user32 = ctypes.windll.user32
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_UNICODE = 0x0004


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [('wVk', ctypes.c_ushort), ('wScan', ctypes.c_ushort),
                ('dwFlags', ctypes.c_ulong), ('time', ctypes.c_ulong),
                ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]


class INPUT(ctypes.Structure):
    class _U(ctypes.Union):
        _fields_ = [('ki', KEYBDINPUT)]
    _anonymous_ = ('u',)
    _fields_ = [('type', ctypes.c_ulong), ('u', _U)]


def send_unit(unit, up=False):
    inp = INPUT()
    inp.type = INPUT_KEYBOARD
    inp.ki = KEYBDINPUT(0, unit, KEYEVENTF_UNICODE | (KEYEVENTF_KEYUP if up else 0), 0, None)
    user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))


def clients():
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(h, l):
        n = user32.GetWindowTextLengthW(h)
        b = ctypes.create_unicode_buffer(n + 1)
        user32.GetWindowTextW(h, b, n + 1)
        if '问道(1.60' in b.value:
            found.append(h)
        return True

    user32.EnumWindows(cb, 0)
    return found


def main():
    text = sys.argv[1] if len(sys.argv) > 1 else ''
    cs = clients()
    if cs:
        user32.SetForegroundWindow(cs[0])
        time.sleep(0.2)
    for ch in text:
        send_unit(ord(ch))
        time.sleep(0.04)
        send_unit(ord(ch), True)
        time.sleep(0.05)
    if '--enter' in sys.argv:
        send_unit(0x0D)
        time.sleep(0.05)
        send_unit(0x0D, True)
    print('unicode-typed %r' % text)
    return 0


sys.exit(main())
