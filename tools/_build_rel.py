# -*- coding: utf-8 -*-
"""Release build variant: builds Release|Win32 (/O2), fails loudly if no exe."""
import os, subprocess, time

ROOT = os.path.dirname(os.path.abspath(__file__))
if not os.path.exists(os.path.join(ROOT, 'build', 'lpcvm.sln')):
    _up = os.path.dirname(ROOT)
    if os.path.exists(os.path.join(_up, 'build', 'lpcvm.sln')):
        ROOT = _up
EXE = os.path.join(ROOT, 'build', 'Release', 'lpcvm.exe')
MSBUILD = r'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

subprocess.run(['taskkill', '/F', '/IM', 'lpcvm.exe'], capture_output=True, text=True)
time.sleep(3)
if os.path.exists(EXE):
    os.remove(EXE)

r = subprocess.run([MSBUILD, 'build/lpcvm.sln', '-p:Configuration=Release',
                    '-p:Platform=Win32', '-m', '-v:m', '-nologo'],
                   cwd=ROOT, capture_output=True, text=True)
log = (r.stdout or '') + (r.stderr or '')
errs = [l for l in log.splitlines() if ': error' in l or 'LNK' in l]
if errs:
    print('BUILD ERRORS:')
    for l in errs[:12]:
        print('  ', l[:180])
if os.path.exists(EXE):
    print('RELEASE BUILD OK', time.strftime('%H:%M:%S'))
else:
    print('!! RELEASE BUILD FAILED')
