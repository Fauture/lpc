# -*- coding: utf-8 -*-
"""Normalize a source file's line endings to CRLF (the tree's convention)."""
import sys

path = sys.argv[1]
d = open(path, 'rb').read()
before_crlf = d.count(b'\r\n')
d2 = d.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
open(path, 'wb').write(d2)
print('%s: crlf lines before=%d after=%d  bytes %d -> %d'
      % (path, before_crlf, d2.count(b'\r\n'), len(d), len(d2)))
