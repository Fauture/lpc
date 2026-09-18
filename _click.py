# -*- coding: utf-8 -*-
"""Click with SendInput (driver-level), not the legacy mouse_event.

mouse_event is ignored by DirectInput-based game UIs, which is why clicks on the
create-character screen did nothing.  SendInput with MOUSEEVENTF_ABSOLUTE injects
a real pointer event.

usage: python -X utf8 _click.py <x> <y> [button] [count]
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
user32 = ctypes.windll.user32
INPUT_MOUSE = 0
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_ABSOLUTE = 0x8000
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [('dx', ctypes.c_long), ('dy', ctypes.c_long), ('mouseData', ctypes.c_ulong),
                ('dwFlags', ctypes.c_ulong), ('time', ctypes.c_ulong),
                ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]


class INPUT(ctypes.Structure):
    class _U(ctypes.Union):
        _fields_ = [('mi', MOUSEINPUT)]
    _anonymous_ = ('u',)
    _fields_ = [('type', ctypes.c_ulong), ('u', _U)]


def send(flags, dx=0, dy=0, data=0):
    inp = INPUT()
    inp.type = INPUT_MOUSE
    inp.mi = MOUSEINPUT(dx, dy, data, flags, 0, None)
    user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))


def move_abs(x, y):
    sw, sh = user32.GetSystemMetrics(0), user32.GetSystemMetrics(1)
    send(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
         int(x * 65535 / max(1, sw - 1)), int(y * 65535 / max(1, sh - 1)))


def click(x, y, right=False, count=1):
    move_abs(x, y)
    time.sleep(0.25)
    down = MOUSEEVENTF_RIGHTDOWN if right else MOUSEEVENTF_LEFTDOWN
    up = MOUSEEVENTF_RIGHTUP if right else MOUSEEVENTF_LEFTUP
    for _ in range(count):
        send(down)
        time.sleep(0.06)
        send(up)
        time.sleep(0.12)
    print('SendInput click %d,%d%s x%d' % (x, y, ' right' if right else '', count))


if __name__ == '__main__':
    x, y = int(sys.argv[1]), int(sys.argv[2])
    right = len(sys.argv) > 3 and sys.argv[3] == 'right'
    count = int(sys.argv[4]) if len(sys.argv) > 4 else 1
    click(x, y, right, count)
