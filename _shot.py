# -*- coding: utf-8 -*-
"""Screen capture for RDP game testing (compressed so it costs few tokens).

usage:
  python -X utf8 _shot.py [out.jpg] [maxwidth] [quality] [x y w h]

  out.jpg   default shot.jpg
  maxwidth  default 1280 (downscale; keeps the JPEG small)
  quality   default 55
  x y w h   optional capture rectangle (default: whole virtual screen)

Prints the saved size so the caller can tell how much was uploaded.
"""
import io
import os
import sys

from PIL import ImageGrab

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

out = sys.argv[1] if len(sys.argv) > 1 else 'shot.jpg'
maxw = int(sys.argv[2]) if len(sys.argv) > 2 else 1280
quality = int(sys.argv[3]) if len(sys.argv) > 3 else 55
box = None
if len(sys.argv) > 7:
    # arguments are x y w h (window rect from _ui.py); PIL wants (l, t, r, b)
    bx, by, bw, bh = (int(v) for v in sys.argv[4:8])
    box = (bx, by, bx + bw, by + bh)

img = ImageGrab.grab(bbox=box, all_screens=True) if box else ImageGrab.grab(all_screens=True)
w, h = img.size
if w > maxw:
    img = img.resize((maxw, max(1, int(h * maxw / w))), 1)   # 1 = LANCZOS
img = img.convert('RGB')
img.save(out, 'JPEG', quality=quality, optimize=True)
print('saved %s %dx%d %d bytes (source %dx%d)' % (out, img.size[0], img.size[1],
                                                  os.path.getsize(out), w, h))
