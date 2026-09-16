# -*- coding: utf-8 -*-
"""Append HANDOVER section 5.Q just before the next '## ' heading after 5.P."""
import io

P = 'HANDOVER.md'
raw = io.open(P, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in raw else '\n'
d = raw.replace('\r', '')
lines = d.split('\n')

SEC = open('_sec5q.txt', encoding='utf-8').read().replace('\r', '').rstrip('\n').split('\n')

i = next(k for k, l in enumerate(lines) if l.startswith('### P.'))
j = next(k for k in range(i + 1, len(lines)) if lines[k].startswith('## '))
out = lines[:j] + SEC + [''] + lines[j:]
io.open(P, 'w', encoding='utf-8', newline='').write(nl.join(out))
print('inserted %d lines before line %d (%s)' % (len(SEC), j + 1, lines[j][:40]))
