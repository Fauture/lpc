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

try:
    sys.stdout.write('')
except Exception:
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

CLIENT_DIR = r'E:\bf\BaiduNetdiskDownload\asktao1.60'
CLIENT_TITLE = '问道(1.60'   # stable client prefix; the VMware window is '问道1.63 - ...'
user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

# relative coordinates (fraction of the client window), measured from screenshots
BTN_SERVER_OK = (0.419, 0.812)
# 选服/区组界面必须点【取消】（点“确定”会又回到区组列表）
BTN_SERVER_CANCEL = (0.579, 0.903)
BTN_VERIFY_OK = (0.500, 0.478)      # 选服界面的"确定"
LOGIN_ACCOUNT = (0.500, 0.325)      # 登录界面账号输入框
LOGIN_PASSWORD = (0.500, 0.396)     # 密码输入框
LOGIN_BUTTON = (0.500, 0.646)       # 登录按钮
CHAR_FIRST = (0.315, 0.352)         # 角色列表第一个角色
CHAR_ENTER = (0.500, 0.907)         # 进入游戏
CHAR_CREATE_NAME = (0.843, 0.736)  # 建角界面的 名称 输入框
SCHOOL_FIRST = (0.364, 0.815)       # 建角界面第一个门派图标
CREATE_ENTER = (0.500, 0.907)       # 建角界面的 进入游戏（确认建角并进游戏）
CREATE_FORM_OK = (0.857, 0.846)     # 门派/名字表单里的 创建角色（比列表页的高）
CHAR_CREATE_OK = (0.914, 0.885)     # 角色列表页右下角的 创建角色
SKIP_DRAMA = (0.897, 0.836)         # 剧情右下角 跳过
TASK_TRACK = (0.850, 0.290)         # 右上角任务追踪条目：点击后自动寻路到目标NPC


def windows():
    out = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(hwnd, lparam):
        # NOTE: hidden/minimized windows are listed too - the client gets
        # minimized between attempts and a visibility filter made every capture
        # report "no client" while asktao.mod was running.
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
    """The client window, restored if it is minimized.

    A minimized window reports rect (-32000,-32000,160,28), which the old size
    check rejected - so every capture said "no client" while asktao.mod was
    running."""
    cands = [w for w in windows() if CLIENT_TITLE in w[1]]
    for hwnd, title, rect in cands:
        if rect[2] > 200 and rect[3] > 200 and rect[0] > -10000:
            return hwnd, title, rect
    if cands:
        hwnd, title, rect = cands[0]
        user32.ShowWindow(hwnd, 9)      # SW_RESTORE
        time.sleep(0.5)
        for h2, t2, r2 in windows():
            if h2 == hwnd:
                return h2, t2, r2
    return None


def focus():
    """Bring the client to the FRONT *and* make it the foreground window.

    The window must be the foreground one or injected KEYSTROKES are delivered
    elsewhere (mouse clicks still work because they follow the cursor), which is
    exactly how the create-character name box ignored every typed name.
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
    # ALT down/up makes the next SetForegroundWindow call allowed
    VK_MENU, KEYEVENTF_KEYUP = 0x12, 0x0002
    user32.keybd_event(VK_MENU, 0, 0, 0)
    user32.keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0)
    fg = user32.GetForegroundWindow()
    tidFg = user32.GetWindowThreadProcessId(fg, None)
    tidMe = kernel32.GetCurrentThreadId()
    user32.AttachThreadInput(tidMe, tidFg, True)
    user32.BringWindowToTop(hwnd)
    user32.SetForegroundWindow(hwnd)
    user32.SetFocus(hwnd)
    user32.AttachThreadInput(tidMe, tidFg, False)
    time.sleep(0.45)
    if user32.GetForegroundWindow() != hwnd:
        print('WARN: client is not the foreground window (fg=0x%08X)' % user32.GetForegroundWindow())
    return client()


def shot(out, maxw=800, quality=60, retries=3):
    """Capture the client area to `out`.  Returns the path, or None on failure.

    VALIDATED on purpose: the old version printed "shot x.jpg" without checking
    anything, so a failed capture was invisible and the verdict that followed it
    read a STALE file (or reported `unreadable`) - that is exactly how account 35
    of the 2026-09-24 fair run got a FAIL that said nothing about the server.
    """
    c = focus()
    if not c:
        print('no client window')
        return None
    x, y, w, h = c[2]
    for attempt in range(retries):
        r = subprocess.run([sys.executable, '-X', 'utf8', '_shot.py', out, str(maxw),
                            str(quality), str(x), str(y), str(w), str(h)],
                           capture_output=True, text=True, encoding='utf-8', errors='replace')
        if os.path.exists(out) and os.path.getsize(out) > 1000:
            print('shot %s (%dx%d window at %d,%d, %d bytes)' %
                  (out, w, h, x, y, os.path.getsize(out)))
            return out
        print('shot %s FAILED (attempt %d): %s' %
              (out, attempt + 1, (r.stdout or r.stderr or '').strip()[:120]))
        time.sleep(0.6)
    return None


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
    """Launch the client EXACTLY like the desktop shortcut: cmd runs arun.bat.

    Two constraints, both learned the hard way:
      * arun.bat's `start asktao.mod <dist;ip;port;flags>` is what the client
        expects; building the command line myself made it show an extra 区组.
      * the cmd process must NOT be a child of the DSH shell (its job object's
        memory limit makes CreateProcess fail with 内存资源不足), so it is created
        through Win32_Process.Create.
    """
    if client():
        print('client already running')
        return
    bat = os.path.join(CLIENT_DIR, 'arun.bat')
    ps = ("Invoke-CimMethod -ClassName Win32_Process -MethodName Create "
          "-Arguments @{CommandLine='cmd.exe /c \"%s\"'; CurrentDirectory='%s'} | Out-Null"
          % (bat, CLIENT_DIR))
    subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True)
    for _ in range(25):
        time.sleep(1)
        if client():
            print('client window up')
            return
    print('client window did not appear')


def set_field_text(fx, fy, text):
    """Write text into the control under window-fraction (fx, fy).

    WM_SETTEXT/WM_CHAR straight to the control: the client's 密码 box refuses
    Ctrl+V and ignores SendKeys at times, so this is the fallback path.
    """
    c = focus()
    if not c:
        print('no client window')
        return False
    x, y, w, h = c[2]
    px, py = int(x + fx * w), int(y + fy * h)
    user32.SetCursorPos(px, py)
    time.sleep(0.15)
    pt = wt.POINT(px, py)
    hwnd = user32.WindowFromPoint(pt)
    WM_SETTEXT, WM_CHAR = 0x000C, 0x0102
    for _ in range(3):
        if not hwnd:
            break
        user32.SendMessageW(hwnd, WM_SETTEXT, 0, text)
        for ch in text:
            user32.SendMessageW(hwnd, WM_CHAR, ord(ch), 0)
        hwnd = user32.GetParent(hwnd)
    print('set_field_text frac(%.3f,%.3f) <- %r at screen(%d,%d)' % (fx, fy, text, px, py))
    return True


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
    """Kept for the CLI/_rt.py: the fast burst login is the real implementation."""
    return fastlogin(acct, pwd, tag)


def makechar(name, tag='mc'):
    """Create the character on the create-char screen: pick the first school,
    type the name, confirm with 进入游戏."""
    wait_gs()
    focus()
    click_frac(*SCHOOL_FIRST)
    time.sleep(0.5)
    click_frac(*CHAR_CREATE_NAME)
    keys(name)
    time.sleep(0.5)
    shot('%s_1_filled.jpg' % tag, 700)
    click_frac(*CREATE_ENTER)
    time.sleep(3.0)
    shot('%s_2_after.jpg' % tag, 700)
    k, d = screen_kind()
    print('makechar(%s) state=%s(%d)' % (name, k, int(d)))
    return k


def post_chars(text, enter=False):
    """Type into the client with PostMessage WM_CHAR - the ONLY path this client
    accepts (SendInput keystrokes are dropped as injected input)."""
    c = focus()
    if not c:
        return False
    args = [sys.executable, '-X', 'utf8', '_postchar.py', text]
    if enter:
        args.append('--enter')
    r = subprocess.run(args, cwd=os.path.dirname(os.path.abspath(__file__)),
                       capture_output=True, text=True, encoding='utf-8', errors='replace')
    if r.stdout.strip():
        print('  ' + r.stdout.strip())
    return True


def auto(acct, pwd, name, tag='auto'):
    """Full automated session: client -> login -> (create) character -> in game."""
    wait_gs()
    start_client()
    focus()
    k, d = screen_kind()
    print('start state=%s(%d)' % (k, int(d)))
    if k == 'dialog':
        click_frac(*BTN_VERIFY_OK)
        time.sleep(0.8)
        k, d = screen_kind()
        print('  cleared verify dialog -> %s(%d)' % (k, int(d)))
    if k == 'serversel':
        click_frac(*BTN_SERVER_CANCEL)
        time.sleep(1.0)
        k, d = screen_kind()
        print('  cancelled server-select -> %s(%d)' % (k, int(d)))
    if k is None or k == 'login':
        # server-select / login screens: ENTER clears dialogs, then type creds
        for _ in range(3):
            focus()
            keys('%s{TAB}%s{ENTER}' % (acct, pwd))
            time.sleep(2.0)
            k, d = screen_kind()
            print('  after creds: %s(%d)' % (k, int(d)))
            if k in ('charsel', 'createchar', 'ingame'):
                break
            if k == 'dialog':
                # The character list of an account opens with the 「帐号安全级别」
                # notice on top; its signature is nearest to 'dialog' and ENTER
                # does not clear it.  Re-typing the credentials there (the old
                # behaviour) typed into the character screen.  Only a real
                # login screen gets the credentials again.
                keys('{ENTER}')
                time.sleep(1.5)
                k, d = screen_kind()
                print('  after dismiss: %s(%d)' % (k, int(d)))
                if k != 'login':
                    if k == 'dialog':
                        k = 'createchar'   # char list behind the notice
                    break
                continue
            keys('{ENTER}')
            time.sleep(1.0)
    if k == 'createchar':
        print('  creating character %s' % name)
        # The character list must have finished drawing, or this click is
        # swallowed and the name is typed into nothing (the client then
        # rejects the empty name: 人物角色名字长度不合格).
        time.sleep(1.5)
        focus()
        click_frac(*CHAR_CREATE_OK)          # 创建角色 -> the school/name form
        time.sleep(1.5)
        focus()
        click_frac(*SCHOOL_FIRST)            # 门派
        time.sleep(0.4)
        focus()
        click_frac(*CHAR_CREATE_NAME)        # 角色名字 input
        post_chars(name)
        time.sleep(0.5)
        shot('%s_1_named.jpg' % tag, 700)
        focus()
        click_frac(*CREATE_FORM_OK)          # 创建角色 in the form
        time.sleep(2.5)
        k, d = screen_kind()
        print('  after create: %s(%d)' % (k, int(d)))
    if k in ('charsel', 'createchar'):
        focus()
        click_frac(*CHAR_FIRST)              # first slot
        time.sleep(0.6)
        focus()
        click_frac(*CHAR_ENTER)              # 进入游戏
        time.sleep(4.0)
    k, d = screen_kind()
    print('after enter: %s(%d)' % (k, int(d)))
    # skip the opening drama (右下角 跳过) - may need a couple of clicks
    for _ in range(3):
        focus()
        click_frac(*SKIP_DRAMA)
        time.sleep(1.2)
    shot('%s_2_ingame.jpg' % tag, 800)
    # follow the task: the tracking entry (top right) walks the character to the
    # target NPC (寻访王老板); clicking 紫霞真人 is useless - it has no dialog yet
    for i in range(6):
        focus()
        click_frac(*TASK_TRACK)
        time.sleep(2.5)
    shot('%s_3_near_npc.jpg' % tag, 800)
    print('auto done -> %s_1_named.jpg / %s_2_ingame.jpg / %s_3_near_npc.jpg' % (tag, tag, tag))
    return k


def fastlogin(acct, pwd, tag='f'):
    """Log in in ONE burst: focus the account box, type account TAB password
    ENTER, then ENTER through the confirm dialogs.

    Why: (a) the login session only lives ~1 minute, so the old flow (screenshot
    after every step, 2-3s waits) expired mid-way; (b) the client accepts ENTER
    wherever it wants 确定, which removes the coordinate guessing; (c) typing goes
    through the same keyboard path as a real player (clipboard paste is refused
    by the password box).
    """
    wait_gs()
    focus()
    k, d = screen_kind()
    if k == 'dialog':                 # 安全验证弹窗：按用户确认点【确定】
        click_frac(*BTN_VERIFY_OK)
        time.sleep(0.8)
    click_frac(*LOGIN_ACCOUNT)
    keys('%s{TAB}%s{ENTER}' % (acct, pwd))   # type both fields, submit
    time.sleep(1.6)
    keys('{ENTER}')                   # 安全验证 / confirm
    time.sleep(1.2)
    keys('{ENTER}')                   # character screen -> enter game
    time.sleep(1.5)
    shot('%s_fast.jpg' % tag, 700)
    k, d = screen_kind()
    print('fastlogin(%s) final state=%s(%d)' % (acct, k, int(d)))
    return k


def quick_login(acct, pwd, tag='q'):
    """State-driven login: clears the "安全验证" dialog, fills the form (password
    TYPED - that box refuses Ctrl+V), submits, then takes the character in.

    State comes from the window signature, so the whole flow costs a few seconds
    of short polls instead of 10s sleeps per step.
    """
    wait_gs()
    focus()
    for attempt in range(4):
        k, d = screen_kind()
        print('  state=%s(%d)' % (k, int(d)))
        if k == 'dialog':
            click_frac(*BTN_VERIFY_OK)
            time.sleep(1.0)
            continue
        if k in ('login', None):
            click_frac(*LOGIN_ACCOUNT)
            paste(acct)
            click_frac(*LOGIN_PASSWORD)
            set_field_text(LOGIN_PASSWORD[0], LOGIN_PASSWORD[1], pwd)
            shot('%s_1_filled.jpg' % tag, 700)
            click_frac(*LOGIN_BUTTON)
            time.sleep(2.5)
            k2, _ = screen_kind()
            print('  after submit: %s' % k2)
            if k2 == 'dialog':
                continue
            break
        break
    k, d = screen_kind()
    print('  final state=%s(%d)' % (k, int(d)))
    if k == 'charsel':
        click_frac(*CHAR_FIRST)
        time.sleep(0.8)
        click_frac(*CHAR_ENTER)
        time.sleep(2.5)
    shot('%s_3_ingame.jpg' % tag, 700)
    print('quick_login done (%s)' % acct)


def sig_image(img):
    """16x16 grayscale signature - enough to tell the login form from the
    "安全验证" dialog without OCR."""
    small = img.convert('L').resize((16, 16), 1)
    return bytes(small.tobytes())


def sig_of_window():
    import _shot as S   # reuse the PrintWindow capture
    c = client()
    if not c:
        return None
    img = S.capture_window(c[0])
    return sig_image(img) if img else None


def sig_path(name):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), '_sig_%s.bin' % name)


def sig_save(name):
    s = sig_of_window()
    if not s:
        print('no window')
        return
    with open(sig_path(name), 'wb') as fh:
        fh.write(s)
    print('saved signature %s' % name)


def sig_load(name):
    p = sig_path(name)
    if not os.path.exists(p):
        return None
    with open(p, 'rb') as fh:
        return fh.read()


def screen_kind():
    """Nearest known signature: 'login' / 'dialog' / 'charsel' / 'ingame' / None."""
    cur = sig_of_window()
    if not cur:
        return None, 0
    best, bestd = None, 10 ** 9
    for name in ('login', 'dialog', 'charsel', 'ingame', 'createchar'):
        ref = sig_load(name)
        if not ref or len(ref) != len(cur):
            continue
        d = sum(abs(a - b) for a, b in zip(cur, ref)) / len(cur)
        if d < bestd:
            best, bestd = name, d
    return best, bestd


def wait_kind(kinds, timeout=12.0, poll=0.5):
    import time as _t
    t0 = _t.time()
    while _t.time() - t0 < timeout:
        k, d = screen_kind()
        if k in kinds and d < 18:
            return k
        _t.sleep(poll)
    return None


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
    elif cmd == 'makechar':
        makechar(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else 'mc')
    elif cmd == 'auto':
        auto(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] if len(sys.argv) > 5 else 'auto')
    elif cmd == 'fastlogin':
        fastlogin(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else 'f')
    elif cmd == 'qlogin':
        quick_login(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else 'q')
    elif cmd == 'waitgs':
        wait_gs()
    elif cmd == 'sigfile':
        from PIL import Image
        img = Image.open(sys.argv[3])
        with open(sig_path(sys.argv[2]), 'wb') as fh:
            fh.write(sig_image(img))
        print('saved signature %s from %s' % (sys.argv[2], sys.argv[3]))
    elif cmd == 'setfield':
        set_field_text(float(sys.argv[2]), float(sys.argv[3]), sys.argv[4])
    elif cmd == 'fg':
        c = focus()
        fg = user32.GetForegroundWindow()
        print('client=0x%08X foreground=0x%08X match=%s' % ((c[0] if c else 0), fg,
              bool(c and fg == c[0])))
    elif cmd == 'kind':
        k, d = screen_kind()
        print('state=%s dist=%d' % (k, int(d)))
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
