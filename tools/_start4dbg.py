# -*- coding: utf-8 -*-
"""One-shot boot of all 4 services with the DEBUG exe (stack-capture runs).

Same contract as _start4.py (wipe processes+db, boot, exit immediately) but:
  * runs build/Debug/lpcvm.exe (full symbols for tools/_gs_stack2.py)
  * passes LPCVM_CTRACE / LPCVM_TAIL / LPCVM_TRACE through from the caller

usage: python -X utf8 tools/_start4dbg.py <tmp_logdir>
"""
import os
import subprocess, sys, io, shutil, time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
LOG_DIR = sys.argv[1]
if not LOG_DIR.replace('\\', '/').startswith('tmp/'):
    print('!! log dir must live under tmp/'); sys.exit(2)
shutil.rmtree(LOG_DIR, ignore_errors=True)
os.makedirs(LOG_DIR, exist_ok=True)

subprocess.run(['powershell', '-NoProfile', '-Command',
                'Get-Process lpcvm -ErrorAction SilentlyContinue | Stop-Process -Force'],
               capture_output=True, text=True)
time.sleep(2)
subprocess.run([sys.executable,
                os.path.join(os.path.dirname(os.path.abspath(__file__)), '_dbclear.py')],
               check=True, capture_output=True, timeout=180)

env = dict(os.environ)
for k in [k for k in list(env) if k.startswith('LPCVM_')]:
    env.pop(k, None)
env['LPCVM_TRACE'] = os.environ.get('LPCVM_TRACE', '0')
for passthru in ('LPCVM_CTRACE', 'LPCVM_TAIL', 'LPCVM_PCDUMP', 'LPCVM_FILEDDBG'):
    if passthru in os.environ:
        env[passthru] = os.environ[passthru]

_here = os.path.dirname(os.path.abspath(__file__))
_exe = os.path.abspath(os.path.join(_here, '..', 'build', 'Debug', 'lpcvm.exe'))
procs = []
for name in ['aaa', 'dba', 'ccs', 'gs']:
    fn = os.path.join(LOG_DIR, 'live_%s.txt' % name)
    f = open(fn, 'wb', buffering=0)
    p = subprocess.Popen([_exe, 'run', 'linux/%s/pack_data/lib_%s32.pak' % (name, name)],
                         stdout=f, stderr=subprocess.STDOUT, env=env, cwd=os.getcwd())
    procs.append((name, p.pid, fn))
    time.sleep(3)

for name, pid, fn in procs:
    print('[booted-DBG] %-4s pid=%-6d log=%s' % (name, pid, os.path.abspath(fn)))
print('[done] debug services launched; ports 8001/8110/8120/8160 when booted')
