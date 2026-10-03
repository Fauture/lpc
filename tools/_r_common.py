# -*- coding: utf-8 -*-
"""Shared helpers for the account-loop harness: window control, screenshots and
OCR-free screen STATE classification (template match against reference shots).

Why not `_rdp.screen_kind()`: it uses 16x16 grayscale signatures and has been
documented to call the character-creation form "ingame".  For a loop that must
tell 1000 accounts apart we need something we can eyeball and re-label.
"""
import ctypes
import ctypes.wintypes as wt
import io
import os
import subprocess
import sys
import time

from PIL import Image

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ROOT = os.path.dirname(os.path.abspath(__file__))
STATE_DIR = os.path.join(ROOT, '_states')
user32 = ctypes.windll.user32

CLIENT_TITLE = '问道(1.60'
CLIENT_DIR = r'E:\bf\BaiduNetdiskDownload\asktao1.60'
CLIENT_EXE = 'asktao.mod'


# ---------------------------------------------------------------- window ----
def windows():
    out = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(hwnd, lparam):
        n = user32.GetWindowTextLengthW(hwnd)
        if n > 0:
            buf = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(hwnd, buf, n + 1)
            r = wt.RECT()
            user32.GetWindowRect(hwnd, ctypes.byref(r))
            out.append((hwnd, buf.value,
                        (r.left, r.top, r.right - r.left, r.bottom - r.top)))
        return True

    user32.EnumWindows(cb, 0)
    return out


def client_hwnd():
    """The asktao window handle, restored if minimized."""
    cands = [w for w in windows() if CLIENT_TITLE in w[1]]
    if not cands:
        return None
    for hwnd, title, rect in cands:
        if rect[2] > 200 and rect[3] > 200 and rect[0] > -10000:
            return hwnd
    hwnd = cands[0][0]
    user32.ShowWindow(hwnd, 9)          # SW_RESTORE
    time.sleep(0.6)
    return hwnd


def client_rect():
    hwnd = client_hwnd()
    if not hwnd:
        return None
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return (r.left, r.top, r.right - r.left, r.bottom - r.top)


def focus():
    hwnd = client_hwnd()
    if not hwnd:
        return False
    HWND_TOPMOST, HWND_NOTOPMOST = -1, -2
    SWP_NOMOVE, SWP_NOSIZE, SWP_SHOWWINDOW = 0x0002, 0x0001, 0x0040
    user32.ShowWindow(hwnd, 9)
    user32.SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    user32.SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)
    user32.keybd_event(0x12, 0, 0, 0)               # ALT tap unlocks foreground
    user32.keybd_event(0x12, 0, 0x0002, 0)
    fg = user32.GetForegroundWindow()
    tid_fg = user32.GetWindowThreadProcessId(fg, None)
    tid_me = ctypes.windll.kernel32.GetCurrentThreadId()
    user32.AttachThreadInput(tid_me, tid_fg, True)
    user32.BringWindowToTop(hwnd)
    user32.SetForegroundWindow(hwnd)
    user32.SetFocus(hwnd)
    user32.AttachThreadInput(tid_me, tid_fg, False)
    time.sleep(0.35)
    return user32.GetForegroundWindow() == hwnd


def click_frac(fx, fy):
    rect = client_rect()
    if not rect:
        return False
    x, y, w, h = rect
    px, py = int(x + fx * w), int(y + fy * h)
    user32.SetCursorPos(px, py)
    time.sleep(0.12)
    user32.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.05)
    user32.mouse_event(0x0004, 0, 0, 0, 0)
    return True


def key(sendkeys):
    focus()
    subprocess.run(['powershell', '-NoProfile', '-Command',
                    'Add-Type -AssemblyName System.Windows.Forms; '
                    "[System.Windows.Forms.SendKeys]::SendWait('%s')" % sendkeys],
                   capture_output=True)
    return True


def post_text(text):
    """WM_CHAR straight to the client window (name box drops SendInput)."""
    r = subprocess.run([sys.executable, '-X', 'utf8', '_postchar.py', text],
                       cwd=ROOT, capture_output=True, text=True,
                       encoding='utf-8', errors='replace')
    return r.stdout.strip()


WM_CHAR, WM_KEYDOWN, WM_KEYUP = 0x0102, 0x0100, 0x0101


def click_native(fx, fy):
    """Click by posting WM_LBUTTONDOWN/UP to the window.

    Coordinates are CLIENT-area pixels; used for dialogs whose buttons are drawn
    by the game (no child HWND), where a real SetCursorPos click can land on an
    occluding window on this RDP desktop.
    """
    hwnd = client_hwnd()
    if not hwnd:
        return False
    r = wt.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    w, h = r.right, r.bottom
    lparam = ((int(fy * h) & 0xFFFF) << 16) | (int(fx * w) & 0xFFFF)
    user32.PostMessageW(hwnd, 0x0201, 1, lparam)      # WM_LBUTTONDOWN
    time.sleep(0.08)
    user32.PostMessageW(hwnd, 0x0202, 0, lparam)      # WM_LBUTTONUP
    return True


# ----------------------------------------------------------- screenshots ----
def grab():
    """Capture the client window (occlusion-safe) as an RGB image."""
    hwnd = client_hwnd()
    if not hwnd:
        return None
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    if w < 100 or h < 100:
        return None
    hdc = user32.GetWindowDC(hwnd)
    mem = ctypes.windll.gdi32.CreateCompatibleDC(hdc)
    bmp = ctypes.windll.gdi32.CreateCompatibleBitmap(hdc, w, h)
    ctypes.windll.gdi32.SelectObject(mem, bmp)
    ok = user32.PrintWindow(hwnd, mem, 2)             # PW_RENDERFULLCONTENT

    class BMIH(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                    ('biPlanes', wt.WORD), ('biBitCount', wt.WORD),
                    ('biCompression', wt.DWORD), ('biSizeImage', wt.DWORD),
                    ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                    ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]

    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight = w, -h
    bi.biPlanes, bi.biBitCount = 1, 32
    buf = ctypes.create_string_buffer(w * h * 4)
    ctypes.windll.gdi32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)
    ctypes.windll.gdi32.DeleteObject(bmp)
    ctypes.windll.gdi32.DeleteDC(mem)
    user32.ReleaseDC(hwnd, hdc)
    if not ok:
        return None
    return Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB')


def save(img, path, maxw=700, quality=55):
    if img is None:
        return None
    w, h = img.size
    if w > maxw:
        img = img.resize((maxw, max(1, int(h * maxw / w))), 1)
    img.convert('RGB').save(path, 'JPEG', quality=quality, optimize=True)
    return path


def shot(path, maxw=700, quality=55):
    img = grab()
    return save(img, path, maxw, quality)


# --------------------------------------------------- state classification ---
# A "state" is a labelled reference image in _states/<name>.png.  Comparison uses
# a 32x32 grayscale thumbnail of the CLIENT AREA (title bar and borders cropped),
# so it is invariant to the window being moved, resized, or captured from a JPEG
# whose aspect ratio differs slightly.  Comparing whole-window thumbs broke this
# once: a 700x546 reference scored 12.0 against the live 806x629 window, which
# made an empty character page (「你还没有任何角色」) look like "has a character".
STATE_NAMES = ['login', 'dialog', 'serversel', 'charsel', 'charsel_empty',
               'createchar', 'ingame', 'ingamedlg', 'loading', 'error']
BORDER_TOP = 0.040      # title bar
BORDER_SIDE = 0.008
BORDER_BOTTOM = 0.010


def content_box(img):
    """The drawn client area inside the window frame."""
    w, h = img.size
    return (int(w * BORDER_SIDE), int(h * BORDER_TOP),
            int(w * (1 - BORDER_SIDE)), int(h * (1 - BORDER_BOTTOM)))


def thumb(img, n=32):
    if img is None:
        return None
    g = img.crop(content_box(img)).convert('L').resize((n, n), 1)
    return list(g.tobytes())


_STATE_CACHE = None


def load_states():
    """Reference thumbnails from _states/*.png (cached: the loop classifies
    hundreds of times and re-decoding every PNG each time is pure waste)."""
    global _STATE_CACHE
    if _STATE_CACHE is None:
        import glob
        out = {}
        for p in glob.glob(os.path.join(STATE_DIR, '*.png')):
            name = os.path.splitext(os.path.basename(p))[0]
            try:
                out[name] = thumb(Image.open(p))
            except Exception:
                pass
        _STATE_CACHE = out
    return _STATE_CACHE


def classify(img=None, states=None):
    """Return (best_state_name, distance, ranked list) or (None, inf, [])."""
    if img is None:
        img = grab()
    if img is None:
        return None, 1e9, []
    if states is None:
        states = load_states()
    cur = thumb(img)
    if not states:
        return None, 1e9, []
    scored = []
    for name, ref in states.items():
        if not ref or len(ref) != len(cur):
            continue
        d = sum(abs(a - b) for a, b in zip(cur, ref)) / len(cur)
        scored.append((d, name))
    scored.sort()
    if not scored:
        return None, 1e9, []
    return scored[0][1], scored[0][0], scored


def wait_state(names, timeout=20.0, poll=0.6, maxdist=16.0):
    """Poll until the screen matches `names` within `maxdist`."""
    t0 = time.time()
    last = (None, 1e9)
    while time.time() - t0 < timeout:
        img = grab()
        k, d, _ = classify(img)
        last = (k, d)
        if k in names and d <= maxdist:
            return k, d
        time.sleep(poll)
    return last[0], last[1]
