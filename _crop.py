# -*- coding: utf-8 -*-
"""Capture the client window at native size and crop a region for precise
coordinate measurement (window pixels, title bar included).

usage: python -X utf8 _crop.py <out.jpg> <x> <y> <w> <h> [zoom]
"""
import sys

sys.path.insert(0, '.')
import _shot  # noqa: E402

out = sys.argv[1] if len(sys.argv) > 1 else '_crop.jpg'
x = int(sys.argv[2]) if len(sys.argv) > 2 else 0
y = int(sys.argv[3]) if len(sys.argv) > 3 else 0
w = int(sys.argv[4]) if len(sys.argv) > 4 else 300
h = int(sys.argv[5]) if len(sys.argv) > 5 else 200
zoom = float(sys.argv[6]) if len(sys.argv) > 6 else 2.0

hwnd = _shot.find_client_hwnd()
img = _shot.capture_window(hwnd)
print('window image %s' % (img.size,))
crop = img.crop((x, y, x + w, y + h))
crop = crop.resize((int(crop.width * zoom), int(crop.height * zoom)))
crop = crop.convert('RGB')
crop.save(out, quality=80)
print('saved %s %s (crop %d,%d %dx%d zoom %.1f)' % (out, crop.size, x, y, w, h, zoom))
