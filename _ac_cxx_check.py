# -*- coding: utf-8 -*-
"""Compile the C++ AntiCheatScript standalone and compare it with _ac_eval.py."""

import io
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(ROOT, 'src', 'original_vm.cpp')
lines = io.open(SRC, encoding='utf-8').read().split('\n')
a = next(i for i, l in enumerate(lines) if 'class AntiCheatScript {' in l)
b = next(i for i, l in enumerate(lines) if i > a and l.rstrip() == '};')
body = '\n'.join(lines[a:b + 1])

TEST = '''#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
namespace lpc {
''' + body + '''
} // namespace lpc

int main() {
    std::string src, line;
    while (std::getline(std::cin, line)) src += line + "\\n";
    lpc::AntiCheatScript acs;
    if (!acs.parse(src)) { std::cout << "PARSE_FAIL\\n"; return 3; }
    int x = 0, y = 0, ans = 0;
    if (!acs.eval("get_x", {}, x)) { std::cout << "GETX_FAIL\\n"; return 4; }
    if (!acs.eval("get_y", {}, y)) { std::cout << "GETY_FAIL\\n"; return 5; }
    if (!acs.eval("challenge_server", {x, y}, ans)) { std::cout << "CS_FAIL\\n"; return 6; }
    std::cout << x << " " << y << " " << ans << "\\n";
    return 0;
}
'''
io.open(os.path.join(ROOT, '_ac_cxx.cpp'), 'w', encoding='utf-8', newline='\n').write(TEST)

VCVARS = r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'
cmd = '"%s" >nul 2>&1 && cl /nologo /EHsc /std:c++17 /Fe:_ac_cxx.exe _ac_cxx.cpp /Fo:_ac_cxx.obj' % VCVARS
r = subprocess.run(['cmd', '/c', cmd], cwd=ROOT, capture_output=True, text=True)
if not os.path.exists(os.path.join(ROOT, '_ac_cxx.exe')):
    print('COMPILE FAILED rc=%s' % r.returncode)
    print((r.stdout or '')[-3000:])
    print((r.stderr or '')[-2000:])
    sys.exit(2)

sys.path.insert(0, ROOT)
import _ac_eval as E  # noqa: E402

DIRS = ['acx', 'acw', 'acy', 'acz', 'acgo', 'acfinal', 'acfix', 'acrun',
        'final3', 'final2', 'audit2', 'acv']
scripts = []
for d in DIRS:
    p = os.path.join(ROOT, d, 'live_gs.txt')
    if not os.path.exists(p):
        continue
    all_lines = io.open(p, 'rb').read().decode('utf-8', 'replace').splitlines()
    for i, ln in enumerate(all_lines):
        if ln.strip() == '[ac-src] <<<':
            parts, j = [], i + 1
            while j < len(all_lines) and all_lines[j].strip() != '[ac-src] >>>':
                parts.append(all_lines[j])
                j += 1
            scripts.append(' '.join(x.strip() for x in parts))
uniq, seen = [], set()
for s in scripts:
    if s not in seen:
        seen.add(s)
        uniq.append(s)

print('scripts: %d' % len(uniq))
mismatch = 0
for k, src in enumerate(uniq):
    py = subprocess.run([sys.executable, '-X', 'utf8', '_ac_eval.py'], input=src,
                        capture_output=True, text=True, cwd=ROOT)
    cxx = subprocess.run([os.path.join(ROOT, '_ac_cxx.exe')], input=src,
                         capture_output=True, text=True, cwd=ROOT)
    p_out = (py.stdout or '').strip()
    c_out = (cxx.stdout or '').strip()
    same = (p_out == c_out)
    if not same:
        mismatch += 1
    print('#%2d %s  py=%-22s cxx=%s' % (k, 'OK  ' if same else 'DIFF', p_out, c_out))
print('mismatches: %d / %d' % (mismatch, len(uniq)))
sys.exit(1 if mismatch else 0)
