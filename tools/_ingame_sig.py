# -*- coding: utf-8 -*-
"""Ground truth for "is the client actually IN the game?".

The old harness answered this with `screen_kind()` (a text/colour heuristic)
and got it WRONG - it reported `ingame` for three runs whose screenshot shows
the character-creation form with 「请求处理中，请稍候！（原因：9101）」 dialogs
(2026-09-24, vF8/vF9).  This module decides by comparing the frame against
reference shots of a real in-game screen, over regions that only exist in game:
the status/exp bar under the character (bottom left), the action bar (bottom
middle) and the minimap ring (top right).

usage:  python -X utf8 _ingame_sig.py <shot.jpg> [<shot.jpg> ...]
        python -X utf8 _ingame_sig.py --refs            # print reference sigs
"""
import io
import os
import sys

# NOTE: do NOT re-wrap sys.stdout here.  This module is imported by _rapid2.py,
# which already wrapped it; a second TextIOWrapper closes the shared underlying
# buffer when it is collected, and the importing script then dies with
# "ValueError: I/O operation on closed file" AFTER its burst (measured
# 2026-09-24, account 11 of the vF10 run: the verdict line was never printed).
from PIL import Image  # noqa: E402

# References captured from runs whose in-game state was confirmed by eye.
# NOTE: a freshly created character lands in the OPENING NEWBIE STORY
# (`跳过` button, black cinematic bands, name tag on the ground) - NOT on the
# town screen.  Without that reference a perfectly successful run was scored
# "not-ingame (distance 82.6)" for all three accounts (vF11, 2026-09-24), so it
# is part of the in-game reference set, not an exception.
REFS = ['_ref_story_open.jpg', 'c7_after.jpg', '_v6_relogin_ingame.jpg']

REGIONS = (
    (0.02, 0.90, 0.35, 0.99),   # bottom-left status / exp band
    (0.30, 0.88, 0.75, 0.99),   # bottom-middle action bar
    (0.70, 0.02, 0.86, 0.20),   # top-right minimap ring
)


def region_means(img, box):
    w, h = img.size
    x0, y0, x1, y1 = box
    px = img.crop((int(w * x0), int(h * y0), int(w * x1), int(h * y1))).resize((16, 16))
    data = list(px.getdata())
    n = len(data)
    return [sum(c[i] for c in data) / n for i in range(3)]


def signature(path):
    img = Image.open(path).convert('RGB')
    return [region_means(img, r) for r in REGIONS]


def dist(a, b):
    return max(abs(a[i][j] - b[i][j]) for i in range(len(a)) for j in range(3))


def refs():
    out = []
    for f in REFS:
        if os.path.exists(f):
            out.append((f, signature(f)))
    return out


def classify(path, thresh=45.0):
    try:
        s = signature(path)
    except Exception as e:
        return 'unreadable(%s)' % e, 1e9
    best = None
    for name, rs in refs():
        d = dist(s, rs)
        if best is None or d < best[0]:
            best = (d, name)
    if best is None:
        return 'no-reference', 1e9
    return ('INGAME' if best[0] <= thresh else 'not-ingame'), best[0]


def main():
    if '--refs' in sys.argv:
        for name, s in refs():
            print(name, [[round(v) for v in r] for r in s])
        return 0
    for f in sys.argv[1:]:
        verdict, d = classify(f)
        print('%-28s %-11s (min distance %.1f)' % (f, verdict, d))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
