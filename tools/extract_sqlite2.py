# -*- coding: utf-8 -*-
raw = open(r"tools\main_builtins.cpp", "rb").read()
txt = raw.decode("utf-16")
lines = txt.split("\n")

out = []
# sqlite helpers: 2033-2110
out.append("==== helpers 2033-2115 ====")
for i in range(2032, min(2115, len(lines))):
    l = lines[i].rstrip()
    if len(l) > 115:
        l = l[:115]
    out.append("%d: %s" % (i + 1, l))
# sqlite efuns: 3420-3650
out.append("==== efuns 3418-3660 ====")
for i in range(3417, min(3660, len(lines))):
    l = lines[i].rstrip()
    if len(l) > 115:
        l = l[:115]
    out.append("%d: %s" % (i + 1, l))
open(r"tools\main_sqlite_blocks.txt", "w", encoding="utf-8").write("\n".join(out))
print("written", len(out), "lines")
