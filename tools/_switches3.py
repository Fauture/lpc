# -*- coding: utf-8 -*-
"""精确分类：对每个开关出现处，做花括号配对取出被守卫的代码块，
   若块内存在"非输出语句" => 功能开关；否则 => 日志开关。
   无法配对（例如 static const bool X = getenv(...) 形式）时标记 ?，便于人工核对。"""
import io
import os
import re
import collections

pat = re.compile(r'(dynProbeFlag|LPCVM_GETENV|std::getenv|::getenv)\s*\(\s*"([A-Za-z0-9_]+)"')
OUT = re.compile(r'(std::cerr|std::cout|fprintf|fflush|<<\s*"|std::endl)')

verdicts = collections.defaultdict(list)   # name -> [(verdict, file:line, first_other)]


def block_after(lines, i):
    """从第 i 行（0基）起找第一个 '{'，做配对，返回块内行号区间；找不到返回 None。"""
    depth = 0
    started = False
    for j in range(i, min(i + 60, len(lines))):
        s = lines[j]
        for ch in s:
            if ch == '{':
                depth += 1
                started = True
            elif ch == '}':
                depth -= 1
                if started and depth == 0:
                    return (i, j)
    return None


for fn in sorted(os.listdir('src')):
    if not fn.endswith(('.cpp', '.h', '.inc')):
        continue
    lines = io.open('src/' + fn, encoding='utf-8', errors='surrogateescape').read().split('\n')
    for i, l in enumerate(lines):
        for m in pat.finditer(l):
            name = m.group(2)
            span = block_after(lines, i)
            if not span:
                verdicts[name].append(('?', '%s:%d' % (fn, i + 1), l.strip()[:100]))
                continue
            a, b = span
            other = None
            for j in range(a, b + 1):
                s = lines[j].strip()
                if not s or s.startswith('//') or OUT.search(s):
                    continue
                if pat.search(s) or s in ('{', '}', '});'):
                    continue
                other = s[:110]
                break
            verdicts[name].append(('功能' if other else '日志', '%s:%d' % (fn, i + 1),
                                   other or ''))

func = {n: v for n, v in verdicts.items() if any(x[0] == '功能' for x in v)}
unk = {n: v for n, v in verdicts.items() if n not in func and any(x[0] == '?' for x in v)}
log = {n: v for n, v in verdicts.items() if n not in func and n not in unk}

print('=== 功能开关 %d 个 ===' % len(func))
for n in sorted(func):
    for v, w, o in func[n]:
        if v == '功能':
            print('  %-24s %-22s %s' % (n, w, o))
            break
print('\n=== 需人工核对 %d 个 ===' % len(unk))
for n in sorted(unk):
    print('  %-24s %-22s %s' % (n, unk[n][0][1], unk[n][0][2]))
print('\n=== 日志/探针类 %d 个 ===' % len(log))
print('  ' + ', '.join(sorted(log)))
