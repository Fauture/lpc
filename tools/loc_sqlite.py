# -*- coding: utf-8 -*-
import io

raw = open(r"tools\main_builtins.cpp", "rb").read()
txt = raw.decode("utf-16")
lines = txt.split("\n")
pats = [
    "struct SqliteConn",
    "struct SqliteQuery",
    "sqliteConns()",
    "sqliteQueries()",
    "sqliteConnFor(",
    "sqliteRealPath(",
    "sqliteMutex()",
    "sqliteNextConn",
    "sqliteNextQuery",
    'name == "sql_open_db"',
    'name == "sql_query"',
    'name == "sql_step"',
    'name == "sql_finalize"',
    'name == "sql_get_db_info"',
    'name == "sql_get_error"',
]
for pat in pats:
    for i, l in enumerate(lines):
        if pat in l:
            print(i + 1, pat, "->", l.strip()[:90])
            break
