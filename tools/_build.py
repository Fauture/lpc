# -*- coding: utf-8 -*-
"""Rebuild helper that CANNOT silently do nothing.

Two traps cost hours in this session:
  1. the running services hold build/Release/lpcvm.exe open, so the link fails
     with LNK1104 while the old exe stays in place - and if the output is piped
     through Select-String the error can be missed entirely, so the "new" build
     is really the previous one;
  2. MSBuild skips a project whose sources look older than the object files
     (restoring a source from a backup resets its timestamp), so a "successful"
     build may not relink at all.

This script kills the services, touches the sources, deletes the exe, builds,
and FAILS LOUDLY unless a fresh exe appears.
"""
import os, subprocess, sys, time

ROOT = os.path.dirname(os.path.abspath(__file__))
# tools/ 内的脚本：工程根是上一级（build/lpcvm.sln 所在处）。在根目录放脚本时保持旧行为。
if not os.path.exists(os.path.join(ROOT, 'build', 'lpcvm.sln')):
    _up = os.path.dirname(ROOT)
    if os.path.exists(os.path.join(_up, 'build', 'lpcvm.sln')):
        ROOT = _up
EXE = os.path.join(ROOT, 'build', 'Release', 'lpcvm.exe')
MSBUILD = r'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

subprocess.run(['taskkill', '/F', '/IM', 'lpcvm.exe'],
               capture_output=True, text=True)
time.sleep(3)
for f in ('src/original_vm.cpp', 'src/builtins.cpp', 'src/value.cpp', 'src/value.h',
          'src/pak_loader.cpp', 'src/main.cpp'):
    p = os.path.join(ROOT, f)
    if os.path.exists(p):
        os.utime(p, None)
if os.path.exists(EXE):
    os.remove(EXE)
    print('removed stale exe')

r = subprocess.run([MSBUILD, 'build/lpcvm.sln', '-p:Configuration=Release', '-p:Platform=Win32',
                    '-m', '-v:m', '-nologo'], cwd=ROOT, capture_output=True, text=True)
log = (r.stdout or '') + (r.stderr or '')
errs = [l for l in log.splitlines() if ': error' in l or 'LNK' in l]
if errs:
    print('BUILD ERRORS:')
    for l in errs[:10]:
        print('  ', l[:180])
if not os.path.exists(EXE):
    print('!! BUILD FAILED: no exe produced (services holding it? compile error?)')
    sys.exit(1)
print('BUILD OK: %s  %d bytes  %s' % (EXE, os.path.getsize(EXE),
                                      time.strftime('%H:%M:%S', time.localtime(os.path.getmtime(EXE)))))
