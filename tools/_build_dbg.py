# -*- coding: utf-8 -*-
"""Debug build variant of _build.py: kills services, builds Debug|Win32 with
full symbols (PDB), fails loudly if no exe appears."""
import os, subprocess, sys, time

ROOT = os.path.dirname(os.path.abspath(__file__))
if not os.path.exists(os.path.join(ROOT, 'build', 'lpcvm.sln')):
    _up = os.path.dirname(ROOT)
    if os.path.exists(os.path.join(_up, 'build', 'lpcvm.sln')):
        ROOT = _up
EXE = os.path.join(ROOT, 'build', 'Debug', 'lpcvm.exe')
MSBUILD = r'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

subprocess.run(['taskkill', '/F', '/IM', 'lpcvm.exe'], capture_output=True, text=True)
time.sleep(3)
if os.path.exists(EXE):
    os.remove(EXE)
    print('removed stale debug exe')

r = subprocess.run([MSBUILD, 'build/lpcvm.sln', '-p:Configuration=Debug',
                    '-p:Platform=Win32', '-m', '-v:m', '-nologo'],
                   cwd=ROOT, capture_output=True, text=True)
log = (r.stdout or '') + (r.stderr or '')
errs = [l for l in log.splitlines() if ': error' in l or 'LNK' in l]
if errs:
    print('BUILD ERRORS:')
    for l in errs[:12]:
        print('  ', l[:180])
if not os.path.exists(EXE):
    print('!! DEBUG BUILD FAILED')
    sys.exit(1)
print('DEBUG BUILD OK: %s  %d bytes  %s' % (EXE, os.path.getsize(EXE),
      time.strftime('%H:%M:%S', time.localtime(os.path.getmtime(EXE)))))
