# -*- coding: utf-8 -*-
"""x64 Release build: same as _build_rel.py but platform=x64, byte-safe output."""
import os, subprocess, time

ROOT = r'C:\Users\Administrator\Desktop\lpc'
EXE = os.path.join(ROOT, 'build64', 'Release', 'lpcvm.exe')
MSBUILD = r'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

subprocess.run(['taskkill', '/F', '/IM', 'lpcvm.exe'], capture_output=True)
time.sleep(3)
if os.path.exists(EXE):
    os.remove(EXE)

r = subprocess.run([MSBUILD, 'build64/lpcvm.sln', '-p:Configuration=Release',
                    '-p:Platform=x64', '-m', '-v:m', '-nologo'],
                   cwd=ROOT, capture_output=True)
log = ((r.stdout or b'') + (r.stderr or b'')).decode('utf-8', errors='replace')
with open(os.path.join(ROOT, 'tmp', 'build64_last.log'), 'w', encoding='utf-8') as f:
    f.write(log)
errs = [l for l in log.splitlines() if ': error' in l or 'LNK1' in l]
if errs:
    print('BUILD ERRORS:')
    for l in errs[:12]:
        print('  ', l[:180])
if os.path.exists(EXE):
    print('X64 BUILD OK', time.strftime('%H:%M:%S'))
else:
    print('!! X64 BUILD FAILED')
