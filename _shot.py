# -*- coding: utf-8 -*-
"""Screen capture for RDP game testing (compressed so it costs few tokens).

Importable: capture_window(hwnd), find_client_hwnd(), grab_full(box), save_jpeg().
CLI:
  python -X utf8 _shot.py [out.jpg] [maxwidth] [quality] [x y w h | client]
  python -X utf8 _shot.py out.jpg 800 60 client      # the 问道 window, occluded-safe
"""
import ctypes
import ctypes.wintypes as wt
import io
import os
import sys
import time

from PIL import Image, ImageGrab

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32


def find_client_hwnd(title='问道(1.60', minw=200):
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(h, l):
        n = user32.GetWindowTextLengthW(h)
        b = ctypes.create_unicode_buffer(n + 1)
        user32.GetWindowTextW(h, b, n + 1)
        if title in b.value:
            r = wt.RECT()
            user32.GetWindowRect(h, ctypes.byref(r))
            # a MINIMIZED window reports 160x28: keep it as a candidate (the
            # restore below fixes it up) instead of dropping it on size.
            if r.right - r.left > minw or r.left < -10000:
                found.append(h)
        return True

    user32.EnumWindows(cb, 0)
    if not found:
        return None
    hwnd = found[0]
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    if r.left < -10000 or r.right - r.left <= minw:      # minimized/offscreen
        user32.ShowWindow(hwnd, 9)                        # SW_RESTORE
        time.sleep(0.6)
    return hwnd


def capture_window(hwnd):
    """PrintWindow capture: works even when the window is occluded (the DSH GUI
    and other windows sit on top of the game on this RDP desktop, so a plain
    screen grab returns THEIR pixels)."""
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    hdc = user32.GetWindowDC(hwnd)
    mem = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    gdi32.SelectObject(mem, bmp)
    ok = user32.PrintWindow(hwnd, mem, 2)   # 2 = PW_RENDERFULLCONTENT

    class BMIH(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                    ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                    ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG),
                    ('biYPelsPerMeter', wt.LONG), ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]

    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight = w, -h
    bi.biPlanes, bi.biBitCount = 1, 32
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(mem)
    user32.ReleaseDC(hwnd, hdc)
    if not ok:
        return None
    return Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB')


def grab_full(box=None):
    return ImageGrab.grab(bbox=box, all_screens=True) if box else ImageGrab.grab(all_screens=True)


def save_jpeg(img, out, maxw=1280, quality=55):
    w, h = img.size
    if w > maxw:
        img = img.resize((maxw, max(1, int(h * maxw / w))), 1)
    img.convert('RGB').save(out, 'JPEG', quality=quality, optimize=True)
    return img.size


def main():
    try:
        sys.stdout.write('')
    except Exception:
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    out = sys.argv[1] if len(sys.argv) > 1 else 'shot.jpg'
    maxw = int(sys.argv[2]) if len(sys.argv) > 2 else 1280
    quality = int(sys.argv[3]) if len(sys.argv) > 3 else 55
    img = None
    src = 'screen'
    if len(sys.argv) > 4 and sys.argv[4] == 'client':
        hwnd = find_client_hwnd()
        if not hwnd:
            print('no client window')
            return 2
        img = capture_window(hwnd)
        if img is None:
            print('PrintWindow failed')
            return 3
        src = 'client'
    elif len(sys.argv) > 7:
        bx, by, bw, bh = (int(v) for v in sys.argv[4:8])
        img = grab_full((bx, by, bx + bw, by + bh))
    else:
        img = grab_full()
    size = save_jpeg(img, out, maxw, quality)
    print('saved %s %dx%d %d bytes (%s)' % (out, size[0], size[1], os.path.getsize(out), src))
    return 0


if __name__ == '__main__':
    sys.exit(main())
