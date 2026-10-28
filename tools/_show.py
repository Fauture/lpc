# -*- coding: utf-8 -*-
"""Print a text file with line numbers, auto-detecting GBK vs UTF-8.

Usage:
  python -X utf8 tools\\_show.py <file> [start] [count]
  python -X utf8 tools\\_show.py <file> --grep <regex> [-C n]

Decompiled LPC trees under linux/*/pack_data are GBK-encoded, so the harness
`read` tool rejects them; this helper decodes them for inspection only.
"""
import io
import re
import sys


def decode(raw: bytes) -> str:
    for enc in ("utf-8", "gb18030", "latin-1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1", "replace")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    path = argv[1]
    with open(path, "rb") as fh:
        text = decode(fh.read())
    lines = text.splitlines()

    if "--grep" in argv:
        i = argv.index("--grep")
        pat = re.compile(argv[i + 1])
        ctx = 0
        if "-C" in argv:
            ctx = int(argv[argv.index("-C") + 1])
        hits = [n for n, ln in enumerate(lines) if pat.search(ln)]
        shown = set()
        for n in hits:
            for k in range(max(0, n - ctx), min(len(lines), n + ctx + 1)):
                shown.add(k)
        prev = -2
        for k in sorted(shown):
            if k != prev + 1:
                print("     ...")
            print("%6d| %s" % (k + 1, lines[k]))
            prev = k
        print("== %d matching lines ==" % len(hits))
        return 0

    start = int(argv[2]) if len(argv) > 2 else 1
    count = int(argv[3]) if len(argv) > 3 else 200
    for n in range(start - 1, min(len(lines), start - 1 + count)):
        print("%6d| %s" % (n + 1, lines[n]))
    print("== %s: %d lines total ==" % (path, len(lines)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
