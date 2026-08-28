# -*- coding: utf-8 -*-
import re
t = open(r'linux\aaa-c\aaa\start_aaa_v37.c', encoding='gbk', errors='replace').read()

def extract_def(name):
    pat = re.compile(r'\n((?:private|static|override|nomask|varargs)\s+)*(?:mixed|int|void|string|array|mapping|object|float|buffer)\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', re.S)
    m = pat.search(t)
    if not m:
        print("no def for", name)
        return
    start = m.start()
    i = t.index('{', start)
    depth = 0; j = i
    while j < len(t):
        if t[j] == '{': depth += 1
        elif t[j] == '}':
            depth -= 1
            if depth == 0: break
        j += 1
    print("=====", name, "=====")
    print(t[start:j+1][:3000])

for n in ['cmd_new_connection', '_cmd_new_connection', 'post_new_connection']:
    extract_def(n)
