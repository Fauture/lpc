# -*- coding: utf-8 -*-
"""Switch the focused window to the en-US keyboard layout.

The client's name/password boxes get their text through the IME, so with a
Chinese IME active the injected ASCII keystrokes never become characters.  This
posts WM_INPUTLANGCHANGEREQUEST (+ activates the layout) so the window receives
plain en-US keys.

usage: python -X utf8 _ime.py [--list]
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
user32 = ctypes.windll.user32

WM_INPUTLANGCHANGEREQUEST = 0x0050


def en_layout():
    hkl = user32.LoadKeyboardLayoutW('00000409', 0x00000001)   # KLF_ACTIVATE
    if not hkl:
        hkl = user32.LoadKeyboardLayoutW('00000409', 0)
    return hkl


def main():
    if '--list' in sys.argv:
        n = user32.GetKeyboardLayoutList(0, None)
        buf = (ctypes.c_void_p * max(1, n))()
        user32.GetKeyboardLayoutList(n, buf)
        for h in buf:
            print('hkl=0x%08X' % (h or 0))
        return 0
    hwnd = user32.GetForegroundWindow()
    hkl = en_layout()
    user32.ActivateKeyboardLayout(hkl, 0)
    user32.PostMessageW(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, hkl)
    print('switched hwnd=0x%08X to en-US hkl=0x%08X' % (hwnd or 0, hkl or 0))
    return 0


if __name__ == '__main__':
    sys.exit(main())
