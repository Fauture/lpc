# -*- coding: utf-8 -*-
import re, sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
data = open(r'src\o_loader.cpp', 'rb').read().decode('utf-8', errors='replace')
names = re.findall(r'"([a-z_0-9]+)"', data)
seen = set()
for n in names:
    if ('listen' in n or 'bind' in n or 'accept' in n or 'socket' in n) and n not in seen:
        seen.add(n)
        print(n)
