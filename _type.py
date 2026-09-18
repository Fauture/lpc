# -*- coding: utf-8 -*-
"""Type text with SendInput + hardware scan codes.

SendKeys/keybd_event are ignored by this client for text entry (it reads raw
input), so the keystrokes are injected at the driver level with the real scan
codes instead.

usage: python -X utf8 _type.py <text> [--enter]
"""
import ctypes
import io
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

user32 = ctypes.windll.user32
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [('wVk', ctypes.c_ushort), ('wScan', ctypes.c_ushort),
                ('dwFlags', ctypes.c_ulong), ('time', ctypes.c_ulong),
                ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]


class INPUT(ctypes.Structure):
    class _U(ctypes.Union):
        _fields_ = [('ki', KEYBDINPUT)]
    _anonymous_ = ('u',)
    _fields_ = [('type', ctypes.c_ulong), ('u', _U)]


def key_scan(scan, up=False):
    inp = INPUT()
    inp.type = INPUT_KEYBOARD
    inp.ki = KEYBDINPUT(0, scan, KEYEVENTF_SCANCODE | (KEYEVENTF_KEYUP if up else 0), 0, None)
    user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))


def press_scan(scan):
    key_scan(scan)
    time.sleep(0.03)
    key_scan(scan, True)
    time.sleep(0.03)


def main():
    text = sys.argv[1] if len(sys.argv) > 1 else ''
    for ch in text:
        vk = user32.VkKeyScanW(ord(ch)) & 0xFF
        scan = user32.MapVirtualKeyW(vk, 0)      # MAPVK_VK_TO_VSC
        press_scan(scan)
    if '--enter' in sys.argv:
        press_scan(0x1C)                          # Enter scancode
    print('typed %r (%d chars)%s' % (text, len(text), ' + ENTER' if '--enter' in sys.argv else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
