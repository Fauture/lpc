#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Canonical per-run acceptance — the ONLY dialog counter to trust.

    python -X utf8 _acc6.py <logdir> [<name> ...]

WHY A NEW COUNTER
-----------------
The dialog codes (9101 「正在处理中」 / 9112 「数据库维护中」 / 9113 「名字已存在」) are GBK
text inside the hex payload of the 0x1FE5 send line, so in the log they appear as
the hex sequence `39313031` - not as the ASCII characters `9101`.  Every earlier
counter (including `_acc.py`) searched for the literal text and therefore reported
numbers that did not correspond to the dialogs actually sent; version comparisons
built on those numbers are void.

This tool counts per run (a run = one login = one character name), reports the
dialogs, the create/load requests and the 0xF061 list bodies, and prints the total
so two variants can be compared directly.
"""
import io
import os
import re
import sys

CODES = {'9101': '39313031', '9113': '39313133', '9112': '39313132'}


def main() -> int:
    logdir = sys.argv[1]
    only = set(sys.argv[2:])
    lines = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk',
                    errors='replace').read().split('\n')

    order = []
    seen = set()
    for i, l in enumerate(lines):
        m = re.search(r'args=\[create_new_char \| \(\{"([^"]+)"', l)
        if m and m.group(1) not in seen:
            seen.add(m.group(1))
            order.append((i, m.group(1)))

    totals = {c: 0 for c in CODES}
    print('== %s: %d run(s) ==' % (logdir, len(order)))
    for k, (start, nm) in enumerate(order):
        if only and nm not in only:
            continue
        stop = order[k + 1][0] if k + 1 < len(order) else len(lines)
        seg = lines[start:stop]
        counts = {c: sum(1 for l in seg if 'cmd=0x1fe5' in l.lower() and h in l)
                  for c, h in CODES.items()}
        for c in CODES:
            totals[c] += counts[c]
        creates = sum(1 for l in seg if 'args=[create_new_char' in l)
        loads = sum(1 for l in seg if 'args=[load_existed_char' in l)
        lists = [int(b) for b in re.findall(r'cmd=0xf061 body=(\d+)', '\n'.join(seg))]
        print('%-9s creates=%-3d loads=%-2d dialogs[9101=%d 9113=%d 9112=%d] lists=%s'
              % (nm, creates, loads, counts['9101'], counts['9113'], counts['9112'],
                 lists[:6]))
    print('TOTAL 9101=%d 9113=%d 9112=%d   (9111/9113 are close to original behaviour; '
          '9112 must be 0)' % (totals['9101'], totals['9113'], totals['9112']))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
