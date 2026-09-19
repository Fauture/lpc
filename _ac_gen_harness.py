# -*- coding: utf-8 -*-
"""Regenerate the standalone harness from src/original_vm.cpp."""
import io
import os

ROOT = os.path.dirname(os.path.abspath(__file__))
src = io.open(os.path.join(ROOT, 'src', 'original_vm.cpp'), encoding='utf-8').read()
lines = src.split('\n')
a = next(i for i, l in enumerate(lines) if 'class AntiCheatScript {' in l)
b = next(i for i, l in enumerate(lines) if i > a and l.rstrip() == '};')
body = '\n'.join(lines[a:b + 1])

HDR = '''#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
namespace lpc {
'''
MAIN = '''
} // namespace lpc

int main() {
    std::string src, line;
    while (std::getline(std::cin, line)) { src += line; src += "\\n"; }
    lpc::AntiCheatScript acs;
    if (!acs.parse(src)) { std::cout << "PARSE_FAIL\\n"; return 3; }
    int x = 0, y = 0, ans = 0;
    acs.eval("get_x", {}, x);
    acs.eval("get_y", {}, y);
    acs.eval("challenge_server", {x, y}, ans);
    std::cout << x << " " << y << " " << ans << "\\n";
    return 0;
}
'''
out = os.path.join(ROOT, '_clean.cpp')
io.open(out, 'w', encoding='utf-8', newline='\n').write(HDR + body + MAIN)
print('wrote %s (%d bytes)' % (out, os.path.getsize(out)))
