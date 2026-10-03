# -*- coding: utf-8 -*-
"""Save the CURRENT client window as a state reference for _r_common.classify.

usage: python -X utf8 _state_ref.py <name> [shot.jpg]
       python -X utf8 _state_ref.py --list
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
# NOTE: _r_common re-wraps sys.stdout for utf-8; do NOT wrap it here too - the
# second TextIOWrapper closes the shared buffer when collected (documented in
# _ingame_sig.py) and every later print dies with "I/O operation on closed file".
import _r_common as C  # noqa: E402


def main():
    os.makedirs(C.STATE_DIR, exist_ok=True)
    if len(sys.argv) > 1 and sys.argv[1] == '--list':
        st = C.load_states()
        for n in sorted(st):
            print('  %-12s %d bytes of thumb' % (n, len(st[n])))
        return 0
    name = sys.argv[1]
    src = sys.argv[2] if len(sys.argv) > 2 else None
    from PIL import Image
    if src:
        img = Image.open(src).convert('RGB')
    else:
        img = C.grab()
    if img is None:
        print('no client window and no source image')
        return 2
    out = os.path.join(C.STATE_DIR, '%s.png' % name)
    img.save(out, 'PNG')
    print('saved state %s from %s %s (%dx%d)' % (name, src or 'live window', out, *img.size))
    return 0


if __name__ == '__main__':
    sys.exit(main())
