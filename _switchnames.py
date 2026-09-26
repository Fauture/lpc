# -*- coding: utf-8 -*-
"""列出所有运行时开关名（去重）。"""
import io
import os
import re
import collections

pat = re.compile(r'(dynProbeFlag|LPCVM_GETENV|std::getenv|::getenv)\s*\(\s*"([A-Za-z0-9_]+)"')
names = collections.Counter()
where = {}
for fn in sorted(os.listdir('src')):
    if not fn.endswith(('.cpp', '.h', '.inc')):
        continue
    t = io.open('src/' + fn, encoding='utf-8', errors='surrogateescape').read()
    for i, l in enumerate(t.split('\n')):
        for m in pat.finditer(l):
            names[m.group(2)] += 1
            where.setdefault(m.group(2), '%s:%d' % (fn, i + 1))
print('开关名共 %d 个：' % len(names))
lst = sorted(names)
for i in range(0, len(lst), 3):
    print('   ' + '   '.join('%-24s' % x for x in lst[i:i + 3]))
print('')
print('=== 名字里带 OLD/LEGACY/NEW/OFF/NO_/USE/STRICT/FORCE/BYTE/CHAR/VALUENO 的 ===')
for n in lst:
    if any(k in n for k in ('OLD', 'LEGACY', 'NEW', 'OFF', 'NO_', 'USE', 'STRICT', 'FORCE',
                            'BYTE', 'CHAR', 'VALUENO', 'RULE', 'MODE')):
        print('   %-26s %s' % (n, where[n]))
