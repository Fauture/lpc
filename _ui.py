# -*- coding: utf-8 -*-
"""Minimal UI automation for RDP game testing (ctypes only, no dependencies).

commands:
  list                          list top-level windows (title + rect + pid)
  find <title substring>        print the matching window's rect
  focus <title substring>       bring the window to the foreground
  click <x> <y>                 move the mouse and click at screen coords
  move <x> <y>                  move the mouse
  key <keys>                    send keys (SendKeys syntax, e.g. {ENTER}, abc)
  paste <text>                  put text on the clipboard and press Ctrl+V (GBK-safe)
  size                          print the virtual screen size

usage: python -X utf8 _ui.py <command> [args]
"""
import ctypes
import ctypes.wintypes as wt
import io
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

SW_RESTORE = 9


def windows():
    out = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(hwnd, lparam):
        if not user32.IsWindowVisible(hwnd):
            return True
        n = user32.GetWindowTextLengthW(hwnd)
        if n <= 0:
            return True
        buf = ctypes.create_unicode_buffer(n + 1)
        user32.GetWindowTextW(hwnd, buf, n + 1)
        r = wt.RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(r))
        pid = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        out.append((hwnd, buf.value, (r.left, r.top, r.right - r.left, r.bottom - r.top), pid.value))
        return True

    user32.EnumWindows(cb, 0)
    return out


def find(sub):
    sub = sub.lower()
    for hwnd, title, rect, pid in windows():
        if sub in title.lower():
            return hwnd, title, rect, pid
    return None


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'list'
    if cmd == 'list':
        for hwnd, title, rect, pid in windows():
            print('0x%08X pid=%-6d rect=%s  %s' % (hwnd, pid, rect, title[:70]))
    elif cmd == 'find':
        r = find(sys.argv[2])
        print('not found' if not r else '0x%08X rect=%s %s' % (r[0], r[2], r[1]))
    elif cmd == 'focus':
        r = find(sys.argv[2])
        if not r:
            print('not found')
            return 1
        hwnd = r[0]
        user32.ShowWindow(hwnd, SW_RESTORE)
        user32.SetForegroundWindow(hwnd)
        time.sleep(0.4)
        print('focused 0x%08X %s rect=%s' % (hwnd, r[1][:40], r[2]))
    elif cmd in ('click', 'move'):
        x, y = int(sys.argv[2]), int(sys.argv[3])
        user32.SetCursorPos(x, y)
        time.sleep(0.15)
        if cmd == 'click':
            user32.mouse_event(0x0002, 0, 0, 0, 0)   # LEFTDOWN
            time.sleep(0.05)
            user32.mouse_event(0x0004, 0, 0, 0, 0)   # LEFTUP
        print('%s at %d,%d' % (cmd, x, y))
    elif cmd == 'key':
        import subprocess
        script = ("Add-Type -AssemblyName System.Windows.Forms; "
                  "[System.Windows.Forms.SendKeys]::SendWait('%s')" % sys.argv[2])
        subprocess.run(['powershell', '-NoProfile', '-Command', script], capture_output=True)
        print('keys sent')
    elif cmd == 'paste':
        text = sys.argv[2]
        # clipboard via powershell (Set-Clipboard handles unicode/GBK text)
        import subprocess
        ps = 'Set-Clipboard -Value ([Console]::In.ReadToEnd())'
        p = subprocess.run(['powershell', '-NoProfile', '-Command', ps], input=text.encode('utf-8'),
                           capture_output=True)
        time.sleep(0.3)
        script = ("Add-Type -AssemblyName System.Windows.Forms; "
                  "[System.Windows.Forms.SendKeys]::SendWait('^v')")
        subprocess.run(['powershell', '-NoProfile', '-Command', script], capture_output=True)
        print('pasted %d chars (rc=%d)' % (len(text), p.returncode))
    elif cmd == 'size':
        print('screen %dx%d' % (user32.GetSystemMetrics(0), user32.GetSystemMetrics(1)))
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
