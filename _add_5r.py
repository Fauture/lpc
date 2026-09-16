# -*- coding: utf-8 -*-
"""Append HANDOVER 5.R with the round-1 findings of the task-panel hunt."""
import io

P = 'HANDOVER.md'
raw = io.open(P, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in raw else '\n'
lines = raw.replace('\r', '').split('\n')

SEC = open('_sec5r.txt', encoding='utf-8').read().replace('\r', '').rstrip('\n').split('\n')

i = next(k for k, l in enumerate(lines) if l.startswith('### Q.'))
j = next(k for k in range(i + 1, len(lines)) if lines[k].startswith('## '))
out = lines[:j] + SEC + [''] + lines[j:]
io.open(P, 'w', encoding='utf-8', newline='').write(nl.join(out))
print('inserted 5.R before line %d (%s)' % (j + 1, lines[j][:36]))
