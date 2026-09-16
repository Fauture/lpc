# -*- coding: utf-8 -*-
"""Generate _cell_scan3.py from _walk_probe.py: same login, but instead of the
greedy walk it probes single cells and prints accept/REJECT for each, so the
blocking geometry around the player is visible without a rebuild."""
import io

src = io.open('_walk_probe.py', encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in src else '\n'

old_start = src.index('    blocked = set()')
old_end = src.index('    print(\'--- 过图:')
assert old_start < old_end
new_block = '''    print('--- single-cell scan from (%d,%d) ---' % (x, y), flush=True)

    def try_cell(nx, ny):
        c.move(mid, [(nx, ny)])
        t0 = time.time()
        while time.time() - t0 < 0.6:
            r = ep.read_pkt(g, 0.2)
            if r is None:
                return None
            if r[0] == 'timeout':
                continue
            if r[0] == 0x1155 and len(r[1]) >= 21:
                return False
        return True

    dirs = []
    for dx, dy, label in ((1, 0, 'E'), (-1, 0, 'W'), (0, 1, 'N'), (0, -1, 'S'),
                          (1, 1, 'NE'), (-1, -1, 'SW')):
        out = []
        cx, cy = x, y
        for k in range(24):
            nx, ny = x + dx * (k + 1), y + dy * (k + 1)
            if nx < 0 or ny < 0:
                break
            ok = try_cell(nx, ny)
            if ok is None:
                out.append('%d,%d:DISCONNECT' % (nx, ny))
                break
            out.append('%d,%d:%s' % (nx, ny, 'ok' if ok else 'BLOCK'))
            if not ok:
                break
        print('   %-3s %s' % (label, ' '.join(out)), flush=True)

'''.replace('\n', nl)

out = src[:old_start] + new_block + src[old_end:]
io.open('_cell_scan3.py', 'w', encoding='utf-8', newline='').write(out)
print('wrote _cell_scan3.py (%d bytes)' % len(out))
