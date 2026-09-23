#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Per-run dialog gate: dialogs are ASCII codes inside the 0x1FE5 hex body.

    python -X utf8 _flowgate4.py <logdir> <name1> <name2> ...

The dialog numbers (9101 「正在处理中」 / 9112 「数据库维护中」 / 9113 「名字已存在」)
appear as ASCII inside the hex payload of the 0x1FE5 send line, so they must be
counted against the hex text, not against the visible characters.
"""
import io
import os
import re
import sys

logdir = sys.argv[1]
names = sys.argv[2:]
d = io.open(os.path.join(logdir, 'live_gs.txt'), encoding='gbk', errors='replace').read()

creates = [(m.start(), m.group(1)) for m in
           re.finditer(r'args=\[create_new_char \| \(\{"([^"]+)"', d)]
print('GATE %s: created order = %s' % (logdir, [nm for _, nm in creates][:6]))

for want in names:
    hits = [p for p, nm in creates if nm == want]
    if not hits:
        print('  %-8s NOT CREATED' % want)
        continue
    start = hits[0]
    nxt = [p for p, _ in creates if p > start]
    seg = d[start:(nxt[0] if nxt else len(d))]
    hexcodes = re.findall(r'by=/gs/clone/char/login\.o::send_dialog_ok hex=([0-9A-Fa-f]+)', seg)
    counts = {c: sum(1 for h in hexcodes if c.encode() in h.encode()) for c in ('9101', '9112', '9113')}
    loads = re.findall(r'args=\[load_existed_char \| \(\{"([^"]+)"', d)
    print('  %-8s create_cmds=%-3d dialogs[9101=%d 9112=%d 9113=%d] entered=%s'
          % (want, len(hits), counts['9101'], counts['9112'], counts['9113'], want in loads))
