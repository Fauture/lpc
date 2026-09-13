# -*- coding: utf-8 -*-
"""Brute-force the framing of the 0x20D1 talk-drama body so we can tell whether
our pad_buffer/make8 encoding matches what the client expects.

  python -X utf8 _dec_drama.py <hex>
"""
import io, sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
hx = sys.argv[1] if len(sys.argv) > 1 else ''
if hx.lower().startswith('20d1'):
    hx = hx[4:]
body = bytes.fromhex(hx)
print('body %d bytes' % len(body))
print('head:', body[:48].hex().upper())


def try_frame(start, width):
    """start = index of list_count; width = fragment-length prefix width."""
    p = start
    n = body[p]
    p += 1
    if n == 0 or n > 200:
        return None
    frs = []
    for _ in range(n):
        if p + width > len(body):
            return None
        ln = int.from_bytes(body[p:p + width], 'big')
        p += width
        if ln <= 0 or p + ln > len(body):
            return None
        frs.append((body[p], ln, body[p:p + ln]))
        p += ln
    return n, frs, len(body) - p


for start in range(4, 10):
    for width in (1, 2, 4):
        r = try_frame(start, width)
        if r and r[2] == 0:
            n, frs, left = r
            print('MATCH start=%d width=%d count=%d leftover=%d' % (start, width, n, left))
            for i, (t, ln, b) in enumerate(frs):
                print('   [%2d] type=%-3d len=%-5d %s' % (i, t, ln, b[:28].hex().upper()))
            sys.exit(0)
print('no framing consumed the whole body')
for start in range(4, 10):
    for width in (1, 2, 4):
        r = try_frame(start, width)
        if r:
            print('partial start=%d width=%d count=%d leftover=%d' % (start, width, r[0], r[2]))
