# -*- coding: utf-8 -*-
"""Build tools/stackdump.exe via msbuild (same channel as _build.py)."""
import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MSB = 'MSBuild.exe'
MSBUILD = os.path.join('C:', os.sep, 'Program Files', 'Microsoft Visual Studio',
                       '2022', 'Community', 'MSBuild', 'Current', 'Bin', MSB)
r = subprocess.run([MSBUILD, 'build/stackdump.vcxproj',
                    '-p:Configuration=Release', '-p:Platform=Win32',
                    '-m', '-v:m', '-nologo'],
                   cwd=ROOT, capture_output=True, text=True)
log = (r.stdout or '') + (r.stderr or '')
print(log[-1200:])
exe = os.path.join(ROOT, 'tools', 'stackdump.exe')
print('EXE OK' if os.path.exists(exe) else 'EXE MISSING')
