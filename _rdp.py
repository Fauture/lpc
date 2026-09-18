# -*- coding: utf-8 -*-
"""RDP client driver: launch/µ-drive the real 问道 client for in-game testing.

Everything works in WINDOW-relative fractions so it survives the window being
moved/resized, and every step can emit a compressed screenshot.

usage:
  python -X utf8 _rdp.py rect
  python -X utf8 _rdp.py start
  python -X utf8 _rdp.py shot [out.jpg] [maxw]
  python -X utf8 _rdp.py click <fx> <fy>            # fractions 0..1 of the window
  python -X utf8 _rdp.py clickpx <x> <y>            # window pixels
  python -X utf8 _rdp.py paste <text>
  python -X utf8 _rdp.py key <SendKeys>
  python -X utf8 _rdp.py login <acct> <pwd> [tag]   # full login drive + screenshots
  python -X utf8 _rdp.py enter <charname> [tag]     # select/create the character
"""
import ctypes
import ctypes.wintypes as wt
import io
import os
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

CLIENT_DIR = r'E:\bf\BaiduNetdiskDownload\asktao1.60'
CLIENT_TITLE = '问道'
user32 = ctypes.windll.user32

# relative coordinates (fraction of the client window), measured from screenshots
BTN_SERVER_OK = (0.419, 0.812)
BTN_VERIFY_OK = (0.500, 0.478)      # 选服界面的"确定"
LOGIN_ACCOUNT = (0.500, 0.325)      # 登录界面账号输入框
LOGIN_PASSWORD = (0.500, 0.396)     # 密码输入框
LOGIN_BUTTON = (0.500, 0.646)       # 登录按钮
CHAR_FIRST = (0.315, 0.352)         # 角色列表第一个角色
CHAR_ENTER = (0.500, 0.855)         # 进入游戏
CHAR_CREATE_NAME = (0.416, 0.400)   # 建角名字输入框
CHAR_CREATE_OK = (0.500, 0.855)     # 建角确认


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
        out.append((hwnd, buf.value, (r.left, r.top, r.right - r.left, r.bottom - r.top)))
        return True

    user32.EnumWindows(cb, 0)
    return out


def client():
    for hwnd, title, rect in windows():
        if CLIENT_TITLE in title and rect[2] > 200 and rect[3] > 200:
            return hwnd, title, rect
    return None


def focus():
    """Bring the client to the FRONT.

    The client window does not raise itself (the user's point: "游戏界面不会直接
    弹出在顶部"), and ImageGrab captures whatever is visually on top - so a plain
    SetForegroundWindow is not enough: force HWND_TOPMOST, restore, then focus.
    """
    c = client()
    if not c:
        return None
    hwnd = c[0]
    HWND_TOPMOST, HWND_NOTOPMOST = -1, -2
    SWP_NOMOVE, SWP_NOSIZE, SWP_SHOWWINDOW = 0x0002, 0x0001, 0x0040
    user32.ShowWindow(hwnd, 9)                     # SW_RESTORE
    user32.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    user32.SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.45)
    return client()


def shot(out, maxw=800, quality=60):
    c = focus()
    if not c:
        print('no client window')
        return None
    x, y, w, h = c[2]
    subprocess.run([sys.executable, '-X', 'utf8', '_shot.py', out, str(maxw), str(quality),
                    str(x), str(y), str(w), str(h)], capture_output=True)
    print('shot %s (%dx%d window at %d,%d)' % (out, w, h, x, y))
    return out


def click_frac(fx, fy):
    c = focus()
    if not c:
        print('no client window')
        return False
    x, y, w, h = c[2]
    px, py = int(x + fx * w), int(y + fy * h)
    user32.SetCursorPos(px, py)
    time.sleep(0.15)
    user32.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.06)
    user32.mouse_event(0x0004, 0, 0, 0, 0)
    print('click frac(%.3f,%.3f) -> screen(%d,%d)' % (fx, fy, px, py))
    return True


def paste(text):
    focus()
    p = subprocess.run(['powershell', '-NoProfile', '-Command',
                        'Set-Clipboard -Value ([Console]::In.ReadToEnd())'],
                       input=text.encode('utf-8'), capture_output=True)
    time.sleep(0.3)
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Add-Type -AssemblyName System.Windows.Forms; '
                    "[System.Windows.Forms.SendKeys]::SendWait('^v')"], capture_output=True)
    print('pasted %d chars (rc=%d)' % (len(text), p.returncode))


def keys(s):
    focus()
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Add-Type -AssemblyName System.Windows.Forms; '
                    "[System.Windows.Forms.SendKeys]::SendWait('%s')" % s], capture_output=True)
    print('keys %s' % s)


CLIENT_EXE = 'E:\\bf\\BaiduNetdiskDownload\\asktao1.60\\asktao.mod'
CLIENT_ARGS = '试剑内测;192.168.200.1;8001;kbd:0;swictch:0;proxy:0;flag:;uncheck'


def start_client():
    """Launch the client OUTSIDE this shell's job object.

    A plain Popen/cmd start makes the client inherit the DSH shell's memory job,
    and Windows then fails CreateProcess with 内存资源不足.  Win32_Process.Create
    makes it a child of WmiPrvSE instead, which is what reliably works here.
    """
    if client():
        print('client already running')
        return
    ps = ("Invoke-CimMethod -ClassName Win32_Process -MethodName Create "
          "-Arguments @{CommandLine='\"%s\" \"%s\"'; CurrentDirectory='%s'} | Out-Null"
          % (CLIENT_EXE, CLIENT_ARGS, CLIENT_DIR))
    subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True)
    for _ in range(20):
        time.sleep(1)
        if client():
            print('client window up')
            return
    print('client window did not appear')


def wait_gs(timeout=2400, step=4):
    """Block until the gs port accepts - the client cannot enter before that."""
    import socket
    t0 = time.time()
    while time.time() - t0 < timeout:
        s = socket.socket()
        s.settimeout(1.0)
        try:
            s.connect(('192.168.200.1', 8160))
            print('gs ready after %ds' % int(time.time() - t0))
            return True
        except Exception:
            pass
        finally:
            s.close()
        time.sleep(step)
    print('gs NOT ready after %ds' % timeout)
    return False


def quick_login(acct, pwd, tag='q'):
    """One-shot fast login: wait for the gs, then click straight through.

    Every step waits 0.6-2s and the result is judged from the screenshots at the
    end - sleeping 10s per step (the old flow) wasted minutes per attempt.
    """
    wait_gs()
    focus()
    click_frac(*BTN_VERIFY_OK)          # "安全验证" dialog if it is up
    time.sleep(0.7)
    click_frac(*BTN_SERVER_OK)          # 选服 确定
    time.sleep(1.2)
    click_frac(*LOGIN_ACCOUNT)
    paste(acct)
    click_frac(*LOGIN_PASSWORD)
    paste(pwd)
    shot('%s_1_filled.jpg' % tag, 700)
    click_frac(*LOGIN_BUTTON)
    time.sleep(2.0)
    click_frac(*BTN_VERIFY_OK)          # re-verify dialog if it appears
    time.sleep(1.5)
    shot('%s_2_after_login.jpg' % tag, 700)
    click_frac(*CHAR_FIRST)             # first character in the list
    time.sleep(0.9)
    click_frac(*CHAR_ENTER)             # 进入游戏
    time.sleep(2.5)
    shot('%s_3_ingame.jpg' % tag, 700)
    print('quick_login done (%s) - check %s_*.jpg' % (acct, tag))


def login(acct, pwd, tag='l1'):
    start_client()
    time.sleep(2)
    shot('%s_1_server.jpg' % tag)
    click_frac(*BTN_SERVER_OK)          # 选服 -> 确定
    time.sleep(4)
    shot('%s_2_login.jpg' % tag)
    click_frac(*LOGIN_ACCOUNT)
    paste(acct)
    click_frac(*LOGIN_PASSWORD)
    paste(pwd)
    shot('%s_3_filled.jpg' % tag)
    click_frac(*LOGIN_BUTTON)
    time.sleep(6)
    shot('%s_4_after_login.jpg' % tag)
    print('login driven for account %s - inspect the screenshots' % acct)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'rect'
    if cmd == 'rect':
        c = client()
        print('no client' if not c else 'hwnd=0x%08X rect=%s title=%s' % (c[0], c[2], c[1]))
    elif cmd == 'start':
        start_client()
    elif cmd == 'shot':
        shot(sys.argv[2] if len(sys.argv) > 2 else 'rdp_shot.jpg',
             int(sys.argv[3]) if len(sys.argv) > 3 else 800)
    elif cmd == 'click':
        click_frac(float(sys.argv[2]), float(sys.argv[3]))
    elif cmd == 'clickpx':
        c = focus()
        if c:
            x, y, w, h = c[2]
            px, py = int(x + float(sys.argv[2])), int(y + float(sys.argv[3]))
            user32.SetCursorPos(px, py)
            time.sleep(0.15)
            user32.mouse_event(0x0002, 0, 0, 0, 0)
            time.sleep(0.06)
            user32.mouse_event(0x0004, 0, 0, 0, 0)
            print('click px(%s,%s) -> screen(%d,%d)' % (sys.argv[2], sys.argv[3], px, py))
    elif cmd == 'paste':
        paste(sys.argv[2])
    elif cmd == 'key':
        keys(sys.argv[2])
    elif cmd == 'login':
        login(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else 'l1')
    elif cmd == 'qlogin':
        quick_login(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else 'q')
    elif cmd == 'waitgs':
        wait_gs()
    elif cmd == 'enter':
        click_frac(*CHAR_FIRST)
        time.sleep(1)
        click_frac(*CHAR_ENTER)
        time.sleep(5)
        shot('%s_enter.jpg' % (sys.argv[2] if len(sys.argv) > 2 else 'e1'))
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
