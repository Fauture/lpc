# -*- coding: utf-8 -*-
raw = open(r"tools\main_builtins.cpp", "rb").read()
txt = raw.decode("utf-16")
lines = txt.split("\n")

def dump(a, b, label):
    print("==== %s (%d-%d) ====" % (label, a + 1, b))
    for i in range(a, min(b, len(lines))):
        l = lines[i].rstrip()
        if len(l) > 110:
            l = l[:110]
        print("%d: %s" % (i + 1, l))

# struct region start: find 'struct SqliteConn' line - 5 for context
s = None
for i, l in enumerate(lines):
    if l.strip() == "struct SqliteConn {":
        s = i
        break
# end: find next '}' after sqliteRealPath
e = None
for i in range(s, min(s + 200, len(lines))):
    if i > s and lines[i].strip() == "}":
        e = i
        break
# include sqlite3.h presence
for i, l in enumerate(lines[:40]):
    if "sqlite3.h" in l:
        print("include sqlite3.h at line", i + 1, "->", l.strip())
dump(s - 6, e + 2, "sqlite helpers")

# efun region: sql_open_db to end of sqlite block
s2 = None
e2 = None
for i, l in enumerate(lines):
    if 'name == "sql_open_db"' in l:
        s2 = i - 1
    if 'name == "sql_get_error"' in l:
        e2 = i
# find end: after sql_get_db_info block, look for next if(name==
e3 = None
for i in range(s2, len(lines)):
    if i > e2 and lines[i].strip().startswith("if (name =="):
        e3 = i
        break
dump(s2, e3, "sqlite efuns")
