# -*- coding: utf-8 -*-
"""Last-resort text injection: PostMessage WM_CHAR straight to the CLIENT WINDOW.

SendInput keystrokes never showed up in the create-character name box (the
anti-cheat drops injected input), so this posts the characters as window messages
instead - the path the IME itself uses.

usage: python -X utf8 _postchar.py <text>
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
user32 = ctypes.windll.user32
WM_CHAR, WM_KEYDOWN, WM_KEYUP = 0x0102, 0x0100, 0x0101


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
    if not cs:
        print('no client window')
        return 2
    hwnd = cs[0]
    for ch in text:
        user32.PostMessageW(hwnd, WM_CHAR, ord(ch), 1)
        time.sleep(0.05)
    if '--enter' in sys.argv:
        user32.PostMessageW(hwnd, WM_KEYDOWN, 0x0D, 0)
        user32.PostMessageW(hwnd, WM_CHAR, 0x0D, 0)
        user32.PostMessageW(hwnd, WM_KEYUP, 0x0D, 0)
    print('posted %r to hwnd=0x%08X' % (text, hwnd))
    return 0


if __name__ == '__main__':
    sys.exit(main())
